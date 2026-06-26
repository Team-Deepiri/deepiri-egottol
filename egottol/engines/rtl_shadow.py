"""RTL shadow simulation bridge: VHDL inventory, Verilog export, Verilator or Python fallback."""

from __future__ import annotations

import logging
import re
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

from egottol.engines.simulator import EventDrivenSimulator
from egottol.models.base import Circuit, Component, ComponentType, Port, Wire

logger = logging.getLogger(__name__)

_REPO_ROOT = Path(__file__).resolve().parents[2]
_DEFAULT_VHDL_DIRS = (
    _REPO_ROOT / "vhdl",
    _REPO_ROOT / "egottol" / "vhdl",
    _REPO_ROOT / "egottol" / "cpu",
)

# AXI4-Lite register map (byte offsets) matching pim_peripheral.vhd
REG_WEIGHT_ADDR = 0x00
REG_WEIGHT_DATA = 0x04
REG_INPUT_VEC = 0x08
REG_OUTPUT_VEC = 0x0C

_ENTITY_RE = re.compile(r"^\s*entity\s+(\w+)\s+is", re.IGNORECASE | re.MULTILINE)


@dataclass
class AXI4LitePIM:
    """Python stub of the PIM AXI4-Lite peripheral register map."""

    weight_addr: int = 0
    weight_data: int = 0
    input_vec: int = 0
    output_vec: int = 0
    crossbar_weights: Dict[int, float] = field(default_factory=dict)

    def write(self, addr: int, data: int) -> None:
        addr &= 0xFF
        data &= 0xFFFFFFFF
        if addr == REG_WEIGHT_ADDR:
            self.weight_addr = data
        elif addr == REG_WEIGHT_DATA:
            self.weight_data = data
            self._commit_weight_write()
        elif addr == REG_INPUT_VEC:
            self.input_vec = data
            self._update_output()

    def read(self, addr: int) -> int:
        addr &= 0xFF
        if addr == REG_WEIGHT_ADDR:
            return self.weight_addr
        if addr == REG_WEIGHT_DATA:
            return self.weight_data
        if addr == REG_INPUT_VEC:
            return self.input_vec
        if addr == REG_OUTPUT_VEC:
            return self.output_vec
        return 0

    def _commit_weight_write(self) -> None:
        """Program crossbar cell at weight_addr with weight_data (fixed-point)."""
        idx = self.weight_addr & 0xFFFF
        self.crossbar_weights[idx] = float(self.weight_data) / 65536.0

    def _update_output(self) -> None:
        """Placeholder MAC: XOR-mix input with programmed weight nibble."""
        self.output_vec = (self.input_vec ^ self.weight_data) & 0xFFFFFFFF

    def as_dict(self) -> Dict[str, Any]:
        return {
            "weight_addr": self.weight_addr,
            "weight_data": self.weight_data,
            "input_vec": self.input_vec,
            "output_vec": self.output_vec,
            "programmed_weights": len(self.crossbar_weights),
        }


class VerilatorRunner:
    """Compile and run Verilog via Verilator, or cycle-accurate Python fallback."""

    SUPPORTED_GATES = frozenset({"AND", "OR", "DFF", "D_FF"})

    def __init__(self, verilog: str, top_module: str = "rtl_shadow_top"):
        self.verilog = verilog
        self.top_module = top_module

    @staticmethod
    def available() -> bool:
        return shutil.which("verilator") is not None

    def run(self, cycles: int = 1, clk_period_ns: int = 10) -> Dict[str, Any]:
        if self.available():
            return self._run_verilator(cycles)
        return self._run_python_fallback(cycles, clk_period_ns)

    def _run_verilator(self, cycles: int) -> Dict[str, Any]:
        with tempfile.TemporaryDirectory(prefix="egottol_rtl_") as tmp:
            tmp_path = Path(tmp)
            v_path = tmp_path / f"{self.top_module}.v"
            v_path.write_text(self.verilog, encoding="utf-8")

            tb_path = tmp_path / "sim_main.cpp"
            tb_path.write_text(
                _verilator_tb_cpp(self.top_module, cycles),
                encoding="utf-8",
            )

            obj_dir = tmp_path / "obj_dir"
            cmd = [
                "verilator",
                "--cc",
                "--exe",
                "--build",
                "-j",
                "1",
                str(v_path),
                str(tb_path),
                "-o",
                "Vsim",
            ]
            try:
                subprocess.run(cmd, check=True, capture_output=True, text=True, cwd=tmp)
                sim_bin = obj_dir / "Vsim"
                result = subprocess.run(
                    [str(sim_bin)],
                    check=True,
                    capture_output=True,
                    text=True,
                    cwd=obj_dir,
                )
                return {
                    "backend": "verilator",
                    "cycles": cycles,
                    "stdout": result.stdout.strip(),
                    "ok": True,
                }
            except (subprocess.CalledProcessError, FileNotFoundError) as exc:
                logger.warning("Verilator run failed, falling back to Python sim: %s", exc)
                return self._run_python_fallback(cycles)

    def _run_python_fallback(self, cycles: int, clk_period_ns: int = 10) -> Dict[str, Any]:
        sim = CycleAccurateFallback.from_verilog(self.verilog)
        trace: List[Dict[str, int]] = []
        for cycle in range(cycles):
            sim.tick()
            trace.append({"cycle": cycle, **sim.outputs()})
        return {
            "backend": "python_fallback",
            "cycles": cycles,
            "clk_period_ns": clk_period_ns,
            "trace": trace,
            "final": trace[-1] if trace else {},
            "ok": True,
        }


class CycleAccurateFallback:
    """Cycle-accurate AND/OR via EventDrivenSimulator; DFF on posedge."""

    def __init__(self) -> None:
        self._dffs: List[Tuple[str, str]] = []
        self._output_keys: List[str] = []
        self._net_values: Dict[str, float] = {}
        self._logic_sim: Optional[EventDrivenSimulator] = None
        self._clk = 0

    @classmethod
    def from_verilog(cls, verilog: str) -> "CycleAccurateFallback":
        sim = cls()
        components: List[Component] = []
        wires: List[Wire] = []
        net_to_comp: Dict[str, Tuple[str, str]] = {}
        comp_idx = 0

        for line in verilog.splitlines():
            line = line.strip()
            if line.startswith("output"):
                m = re.search(r"output\s+(?:wire\s+)?(\w+)", line)
                if m:
                    sim._output_keys.append(m.group(1))

            m = re.match(r"assign\s+(\w+)\s*=\s*(.+);", line)
            if m:
                out_net, expr = m.group(1), m.group(2).strip()
                gate = "AND" if "&" in expr else "OR"
                parts = [p.strip() for p in re.split(r"[&|]", expr)]
                cid = f"G{comp_idx}"
                comp_idx += 1
                components.append(
                    Component(
                        id=cid,
                        name=gate,
                        type=ComponentType.LOGIC,
                        ports=[
                            Port(name="A", direction="in", value=0.0),
                            Port(name="B", direction="in", value=0.0),
                            Port(name="Q", direction="out", value=0.0),
                        ],
                    )
                )
                for pin, net in zip(("A", "B"), parts[:2]):
                    if net not in net_to_comp:
                        net_to_comp[net] = (f"IN_{net}", pin)
                        components.append(
                            Component(
                                id=f"IN_{net}",
                                name="INPUT",
                                type=ComponentType.LOGIC,
                                ports=[Port(name=pin, direction="in", value=float(sim._net_values.get(net, 0)))],
                            )
                        )
                    wires.append(
                        Wire(
                            id=f"w_{cid}_{pin}",
                            from_component=net_to_comp[net][0],
                            from_port=net_to_comp[net][1],
                            to_component=cid,
                            to_port=pin,
                        )
                    )
                wires.append(
                    Wire(
                        id=f"w_{cid}_Q",
                        from_component=cid,
                        from_port="Q",
                        to_component=f"OUT_{out_net}",
                        to_port="Q",
                    )
                )
                components.append(
                    Component(
                        id=f"OUT_{out_net}",
                        name="OUTPUT",
                        type=ComponentType.LOGIC,
                        ports=[Port(name="Q", direction="out", value=0.0)],
                    )
                )
                net_to_comp[out_net] = (f"OUT_{out_net}", "Q")
                continue

            m = re.match(
                r"always\s+@\(\s*posedge\s+\w+\s*\)\s+(\w+)\s*<=\s*(\w+)\s*;",
                line,
            )
            if m:
                sim._dffs.append((m.group(1), m.group(2)))

        if components:
            circuit = Circuit(id="rtl_fb", name="rtl_fallback", components=components, wires=wires)
            sim._logic_sim = EventDrivenSimulator(circuit)
            for comp in components:
                if comp.name == "INPUT":
                    for port in comp.ports:
                        sim._logic_sim.state[comp.id][port.name] = 0.0
        return sim

    def tick(self) -> None:
        self._clk ^= 1
        if self._clk == 1:
            for q_net, d_net in self._dffs:
                self._net_values[q_net] = float(self._net_values.get(d_net, 0))
        elif self._logic_sim is not None:
            for comp in self._logic_sim.circuit.components:
                if comp.name == "INPUT":
                    net = comp.id.removeprefix("IN_")
                    for port in comp.ports:
                        self._logic_sim.state[comp.id][port.name] = float(
                            self._net_values.get(net, 0)
                        )
            self._logic_sim.step()
            for comp in self._logic_sim.circuit.components:
                if comp.name == "OUTPUT":
                    net = comp.id.removeprefix("OUT_")
                    self._net_values[net] = self._logic_sim.state[comp.id].get("Q", 0.0)

    def outputs(self) -> Dict[str, int]:
        keys = self._output_keys or list(self._net_values.keys())
        return {k: int(self._net_values.get(k, 0)) & 1 for k in keys}


def _verilator_tb_cpp(top_module: str, cycles: int) -> str:
    return f"""#include "V{top_module}.h"
#include "verilated.h"

int main(int argc, char** argv) {{
    VerilatedContext context;
    V{top_module} top{{&context}};
    for (int i = 0; i < {cycles}; ++i) {{
        top.clk = !top.clk;
        top.eval();
    }}
    top.final();
    return 0;
}}
"""


class RTLShadowEngine:
    """Shadow RTL layer: VHDL inventory, Verilog netlist export, and bus bridge."""

    GATE_PORT_MAP = {
        "AND": {"A": "a", "B": "b", "Q": "y"},
        "OR": {"A": "a", "B": "b", "Q": "y"},
        "DFF": {"D": "d", "CLK": "clk", "Q": "q"},
        "D_FF": {"D": "d", "CLK": "clk", "Q": "q"},
    }

    def __init__(
        self,
        circuit: Circuit,
        vhdl_dirs: Optional[Tuple[Path, ...]] = None,
    ):
        self.circuit = circuit
        self.vhdl_dirs = vhdl_dirs or _DEFAULT_VHDL_DIRS
        self.vhdl_index: Dict[str, Path] = {}
        self.pim = AXI4LitePIM()
        self._cycle = 0
        self._load_vhdl_library()

    def _load_vhdl_library(self) -> None:
        for directory in self.vhdl_dirs:
            if not directory.is_dir():
                continue
            for path in sorted(directory.glob("*.vhd")) + sorted(directory.glob("*.vhdl")):
                text = path.read_text(encoding="utf-8", errors="replace")
                for match in _ENTITY_RE.finditer(text):
                    entity = match.group(1).lower()
                    self.vhdl_index.setdefault(entity, path)
        logger.debug("RTL shadow indexed %d VHDL entities", len(self.vhdl_index))

    def list_vhdl_entities(self) -> List[str]:
        return sorted(self.vhdl_index.keys())

    def load_vhdl(self, entity_name: str) -> str:
        key = entity_name.lower()
        path = self.vhdl_index.get(key)
        if path is None:
            raise FileNotFoundError(f"VHDL entity '{entity_name}' not found in {self.vhdl_dirs}")
        return path.read_text(encoding="utf-8")

    def export_verilog_netlist(self, top_name: str = "rtl_shadow_top") -> str:
        """Export schematic LOGIC components as a simple structural Verilog netlist."""
        lines: List[str] = [
            f"module {top_name} (",
            "    input  wire clk,",
            "    input  wire rst,",
            "    output wire shadow_ok",
            ");",
            "",
        ]

        net_for: Dict[Tuple[str, str], str] = {}
        internal_nets: set[str] = set()
        output_nets: List[str] = []

        def net_name(comp_id: str, port: str) -> str:
            return f"n_{comp_id}_{port}".lower()

        for comp in self.circuit.components:
            if comp.type != ComponentType.LOGIC:
                continue
            for port in comp.ports:
                net_for[(comp.id, port.name)] = net_name(comp.id, port.name)

        for wire in self.circuit.wires:
            src = net_for.get((wire.from_component, wire.from_port))
            dst_key = (wire.to_component, wire.to_port)
            if src and dst_key in net_for:
                net_for[dst_key] = src

        for comp in self.circuit.components:
            if comp.type != ComponentType.LOGIC:
                continue
            gate = comp.name.upper()
            port_map = self.GATE_PORT_MAP.get(gate, {})
            if gate in ("AND", "OR"):
                a = net_for.get((comp.id, "A"), "1'b0")
                b = net_for.get((comp.id, "B"), "1'b0")
                y = net_for.get((comp.id, "Q"), net_name(comp.id, "Q"))
                op = "&" if gate == "AND" else "|"
                lines.append(f"    wire {y};")
                lines.append(f"    assign {y} = {a} {op} {b};")
                internal_nets.add(y)
            elif gate in ("DFF", "D_FF"):
                d = net_for.get((comp.id, "D"), "1'b0")
                q = net_for.get((comp.id, "Q"), net_name(comp.id, "Q"))
                lines.append(f"    reg {q};")
                lines.append(f"    always @(posedge clk) {q} <= {d};")
                internal_nets.add(q)
            else:
                continue
            if gate not in ("DFF", "D_FF"):
                output_nets.append(net_for.get((comp.id, "Q"), net_name(comp.id, "Q")))

        shadow_src = output_nets[0] if output_nets else "1'b1"
        lines.extend(
            [
                "",
                "    assign shadow_ok = " + shadow_src + ";",
                "",
                "endmodule",
                "",
            ]
        )
        return "\n".join(lines)

    def bridge_write_weight(self, addr: int, data: int) -> None:
        """AXI4-Lite write sequence for crossbar weight programming."""
        self.pim.write(REG_WEIGHT_ADDR, addr)
        self.pim.write(REG_WEIGHT_DATA, data)

    def bridge_set_input(self, vec: int) -> None:
        self.pim.write(REG_INPUT_VEC, vec)

    def bridge_read_output(self) -> int:
        return self.pim.read(REG_OUTPUT_VEC)

    def step(
        self,
        cycles: int = 1,
        crossbar_currents: Optional[List[float]] = None,
    ) -> Dict[str, Any]:
        """Run one shadow simulation step (optional sync from analog crossbar)."""
        self._cycle += cycles
        netlist = self.export_verilog_netlist()
        runner = VerilatorRunner(netlist)
        sim_result = runner.run(cycles=cycles)

        if crossbar_currents:
            for i, current in enumerate(crossbar_currents[:16]):
                encoded = int(max(min(current * 65536, 0xFFFFFFFF), 0)) & 0xFFFFFFFF
                self.bridge_write_weight(i, encoded)

        return {
            "cycle": self._cycle,
            "vhdl_entities": len(self.vhdl_index),
            "netlist_bytes": len(netlist),
            "pim": self.pim.as_dict(),
            "simulation": sim_result,
        }
