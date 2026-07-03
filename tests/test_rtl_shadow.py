"""Tests for RTL shadow simulation bridge."""

from pathlib import Path

import pytest

from egottol.engines.orchestrator import MultiDomainOrchestrator
from egottol.engines.rtl_shadow import (
    AXI4LitePIM,
    REG_INPUT_VEC,
    REG_OUTPUT_VEC,
    REG_WEIGHT_ADDR,
    REG_WEIGHT_DATA,
    RTLShadowEngine,
    VerilatorRunner,
    _REPO_ROOT,
)
from egottol.models.base import Circuit, Component, ComponentType, Port, Wire


def _logic_and_circuit() -> Circuit:
    return Circuit(
        id="rtl",
        name="rtl_test",
        components=[
            Component(
                id="G1",
                name="AND",
                type=ComponentType.LOGIC,
                ports=[
                    Port(name="A", direction="in", value=1.0),
                    Port(name="B", direction="in", value=1.0),
                    Port(name="Q", direction="out", value=0.0),
                ],
            ),
            Component(
                id="G2",
                name="OR",
                type=ComponentType.LOGIC,
                ports=[
                    Port(name="A", direction="in", value=0.0),
                    Port(name="B", direction="in", value=1.0),
                    Port(name="Q", direction="out", value=0.0),
                ],
            ),
        ],
        wires=[],
    )


def test_vhdl_library_loads_repo_entities():
    engine = RTLShadowEngine(_logic_and_circuit())
    entities = engine.list_vhdl_entities()
    assert "d_ff" in entities
    assert "and_gate" in entities
    assert "pim_peripheral" in entities
    assert "cpu_top" in entities


def test_load_vhdl_source():
    engine = RTLShadowEngine(_logic_and_circuit())
    src = engine.load_vhdl("pim_peripheral")
    assert "entity pim_peripheral" in src.lower()
    assert "S_AXI_AWADDR" in src


def test_export_verilog_netlist():
    engine = RTLShadowEngine(_logic_and_circuit())
    netlist = engine.export_verilog_netlist()
    assert "module rtl_shadow_top" in netlist
    assert "assign" in netlist
    assert "endmodule" in netlist


def test_verilator_runner_python_fallback():
    engine = RTLShadowEngine(_logic_and_circuit())
    netlist = engine.export_verilog_netlist()
    runner = VerilatorRunner(netlist)
    result = runner.run(cycles=2)
    assert result["ok"] is True
    assert result["backend"] in ("python_fallback", "verilator")
    if result["backend"] == "python_fallback":
        assert len(result["trace"]) == 2


def test_axi4lite_pim_register_map():
    pim = AXI4LitePIM()
    pim.write(REG_WEIGHT_ADDR, 3)
    pim.write(REG_WEIGHT_DATA, 0x0000FFFF)
    assert pim.read(REG_WEIGHT_ADDR) == 3
    assert pim.read(REG_WEIGHT_DATA) == 0x0000FFFF
    assert 3 in pim.crossbar_weights
    pim.write(REG_INPUT_VEC, 0xAA)
    assert pim.read(REG_OUTPUT_VEC) == (0xAA ^ 0xFFFF) & 0xFFFFFFFF


def test_rtl_shadow_bridge_weight_write():
    engine = RTLShadowEngine(_logic_and_circuit())
    engine.bridge_write_weight(1, 0x1000)
    assert engine.pim.crossbar_weights[1] == pytest.approx(0x1000 / 65536.0)


def test_rtl_shadow_step():
    engine = RTLShadowEngine(_logic_and_circuit())
    out = engine.step(cycles=1, crossbar_currents=[0.01, 0.02])
    assert "simulation" in out
    assert out["pim"]["programmed_weights"] >= 1
    assert out["vhdl_entities"] > 10


def test_orchestrator_rtl_shadow_step():
    circuit = Circuit(
        id="orch",
        name="orch_rtl",
        components=[
            Component(
                id="CB1",
                name="Crossbar",
                type=ComponentType.ANALOG_COMPUTE,
                metadata={"registry_key": "CROSSBAR"},
                parameters={"rows": 4},
            ),
            Component(
                id="G1",
                name="AND",
                type=ComponentType.LOGIC,
                ports=[
                    Port(name="A", direction="in", value=1.0),
                    Port(name="B", direction="in", value=1.0),
                    Port(name="Q", direction="out"),
                ],
            ),
        ],
        wires=[],
    )
    orch = MultiDomainOrchestrator(circuit, use_nonlinear=False)
    assert orch.rtl_shadow is not None
    snap = orch.step_transient(1e-4)
    assert "rtl_shadow" in snap
    assert snap["rtl_shadow"]["simulation"]["ok"] is True


def test_pim_vhdl_file_exists():
    path = _REPO_ROOT / "egottol" / "vhdl" / "pim_peripheral.vhd"
    assert path.is_file()
    text = path.read_text(encoding="utf-8")
    assert "REG_WEIGHT_ADDR" in text or "x\"00\"" in text
