"""Copilot tool definitions and execution."""

from __future__ import annotations

import uuid
from typing import Any, Callable, Dict, List, Optional

from egottol.engines.simulator import EventDrivenSimulator
from egottol.engines.solver import AdvancedMNASolver
from egottol.models.base import Circuit, Component, ComponentType
from egottol.models.registry import COMPONENT_LIBRARY


COPILOT_TOOLS: List[Dict[str, Any]] = [
    {
        "type": "function",
        "function": {
            "name": "place_component",
            "description": "Place a component from the library onto the schematic canvas.",
            "parameters": {
                "type": "object",
                "properties": {
                    "component_key": {
                        "type": "string",
                        "description": "Registry key, e.g. RES, CAP, AND, VCC",
                    },
                    "x": {"type": "number", "description": "Canvas X coordinate"},
                    "y": {"type": "number", "description": "Canvas Y coordinate"},
                    "parameters": {
                        "type": "object",
                        "description": "Optional parameter overrides",
                    },
                },
                "required": ["component_key", "x", "y"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "run_sim",
            "description": "Run circuit simulation (DC MNA for analog, event-driven for logic).",
            "parameters": {
                "type": "object",
                "properties": {
                    "mode": {
                        "type": "string",
                        "enum": ["dc", "transient", "logic"],
                        "description": "Simulation mode",
                    },
                    "t_stop": {
                        "type": "number",
                        "description": "Stop time for transient (seconds)",
                    },
                    "dt": {
                        "type": "number",
                        "description": "Time step for transient (seconds)",
                    },
                    "steps": {
                        "type": "integer",
                        "description": "Logic simulation steps",
                    },
                },
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "explain_waveform",
            "description": "Explain a simulated waveform or node voltage trace.",
            "parameters": {
                "type": "object",
                "properties": {
                    "node": {
                        "type": "string",
                        "description": "Node key comp_id:port, e.g. R_abc123:1",
                    },
                    "signal": {
                        "type": "string",
                        "description": "Named signal or generic label",
                    },
                },
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "insert_eii_pipeline",
            "description": "Insert an Egottol Integration Interface (EII) avionics/RF pipeline block.",
            "parameters": {
                "type": "object",
                "properties": {
                    "pipeline_type": {
                        "type": "string",
                        "enum": ["adsb_decode", "sdr_rx", "gdl90_encode", "neural_signal"],
                        "description": "Pipeline template",
                    },
                    "target_component": {
                        "type": "string",
                        "description": "Optional existing component id to attach to",
                    },
                    "x": {"type": "number"},
                    "y": {"type": "number"},
                },
                "required": ["pipeline_type"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "exec_spice_cmd",
            "description": "Execute a SPICE-style dot command (.tran, .ac, .dc, .step).",
            "parameters": {
                "type": "object",
                "properties": {
                    "command": {
                        "type": "string",
                        "description": "Full dot command, e.g. .tran 1m 10m",
                    },
                },
                "required": ["command"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "get_circuit_summary",
            "description": "Return a structured summary of the current schematic.",
            "parameters": {"type": "object", "properties": {}},
        },
    },
]

_EII_PIPELINE_MAP = {
    "adsb_decode": ("ADSB_TX", "ADS-B Transponder", ComponentType.RF),
    "sdr_rx": ("SDR_RX", "SDR Decoder", ComponentType.RF),
    "gdl90_encode": ("GDL90_TX", "GDL-90 Encoder", ComponentType.RF),
    "neural_signal": ("NSP_AI", "NSP_AI", ComponentType.EXPERIMENTAL),
}


class ToolExecutor:
    """Executes copilot tools against a mutable circuit model."""

    def __init__(
        self,
        circuit: Circuit,
        sim_results: Dict[str, Any] | None = None,
        on_circuit_changed: Callable[[], None] | None = None,
    ):
        self.circuit = circuit
        self.sim_results = sim_results if sim_results is not None else {}
        self.on_circuit_changed = on_circuit_changed

    async def execute(self, name: str, arguments: Dict[str, Any]) -> Dict[str, Any]:
        handler = getattr(self, f"_tool_{name}", None)
        if handler is None:
            return {"ok": False, "error": f"Unknown tool: {name}"}
        return await handler(arguments)

    async def _tool_place_component(self, args: Dict[str, Any]) -> Dict[str, Any]:
        key = args.get("component_key", "")
        defn = COMPONENT_LIBRARY.get(key)
        if defn is None:
            return {"ok": False, "error": f"Unknown component key: {key}"}

        comp_id = f"{key}_{uuid.uuid4().hex[:6]}"
        params = dict(defn.parameters)
        params.update(args.get("parameters") or {})

        self.circuit.components.append(
            Component(
                id=comp_id,
                name=defn.name,
                type=defn.category,
                ports=list(defn.ports),
                parameters=params,
                metadata={"x": args.get("x", 0.0), "y": args.get("y", 0.0), "key": key},
            )
        )
        self._notify_changed()
        return {
            "ok": True,
            "component_id": comp_id,
            "name": defn.name,
            "key": key,
        }

    async def _tool_run_sim(self, args: Dict[str, Any]) -> Dict[str, Any]:
        mode = (args.get("mode") or "dc").lower()

        if mode == "logic":
            sim = EventDrivenSimulator(self.circuit)
            steps = int(args.get("steps") or 10)
            for _ in range(steps):
                sim.step()
            self.sim_results = {"mode": "logic", "state": sim.state, "steps": steps}
            return {"ok": True, "mode": "logic", "state": sim.state}

        solver = AdvancedMNASolver(self.circuit)
        if mode == "transient":
            t_stop = float(args.get("t_stop") or 0.01)
            dt = float(args.get("dt") or 1e-4)
            transient = solver.solve_transient(t_stop, dt)
            self.sim_results = {"mode": "transient", "waveform": transient}
            return {"ok": True, "mode": "transient", "samples": len(transient)}

        dc = solver.solve_dc()
        self.sim_results = {"mode": "dc", "nodes": dc}
        return {"ok": True, "mode": "dc", "nodes": dc}

    async def _tool_explain_waveform(self, args: Dict[str, Any]) -> Dict[str, Any]:
        node = args.get("node")
        signal = args.get("signal") or "output"
        mode = self.sim_results.get("mode")

        if mode == "dc" and node:
            nodes = self.sim_results.get("nodes", {})
            voltage = nodes.get(node)
            if voltage is not None:
                return {
                    "ok": True,
                    "explanation": f"Node {node} is at {voltage:.4f} V (DC operating point).",
                    "value": voltage,
                }

        if mode == "transient":
            waveform = self.sim_results.get("waveform") or []
            if not waveform:
                return {"ok": False, "error": "No transient data. Run run_sim with mode=transient first."}
            first = waveform[0]["v"]
            last = waveform[-1]["v"]
            if node and node in last:
                v0, v1 = first.get(node, 0.0), last.get(node, 0.0)
                trend = "rising" if v1 > v0 else "falling" if v1 < v0 else "flat"
                return {
                    "ok": True,
                    "explanation": (
                        f"Signal {signal} at {node}: {v0:.4f}V → {v1:.4f}V ({trend}) "
                        f"over {waveform[-1]['t']:.4f}s."
                    ),
                }

        if mode == "logic":
            return {
                "ok": True,
                "explanation": f"Logic state for signal '{signal}': {self.sim_results.get('state', {})}",
            }

        return {
            "ok": False,
            "error": "No simulation results. Call run_sim first, or specify a valid node.",
        }

    async def _tool_insert_eii_pipeline(self, args: Dict[str, Any]) -> Dict[str, Any]:
        pipeline_type = args.get("pipeline_type", "adsb_decode")
        mapping = _EII_PIPELINE_MAP.get(pipeline_type)
        if mapping is None:
            return {"ok": False, "error": f"Unknown pipeline: {pipeline_type}"}

        key, name, category = mapping
        defn = COMPONENT_LIBRARY.get(key)
        if defn:
            ports = list(defn.ports)
            params = dict(defn.parameters)
        else:
            from egottol.models.base import Port

            ports = [
                Port(name="IN", direction="in"),
                Port(name="OUT", direction="out"),
            ]
            params = {"pipeline": pipeline_type}

        comp_id = f"EII_{uuid.uuid4().hex[:6]}"
        self.circuit.components.append(
            Component(
                id=comp_id,
                name=name,
                type=category,
                ports=ports,
                parameters=params,
                metadata={
                    "eii_pipeline": pipeline_type,
                    "target": args.get("target_component"),
                    "x": args.get("x", 0.0),
                    "y": args.get("y", 0.0),
                },
            )
        )
        self._notify_changed()
        return {"ok": True, "component_id": comp_id, "pipeline_type": pipeline_type}

    async def _tool_exec_spice_cmd(self, args: Dict[str, Any]) -> Dict[str, Any]:
        command = (args.get("command") or "").strip()
        if not command.startswith("."):
            return {"ok": False, "error": "SPICE commands must start with a dot, e.g. .tran 1m 10m"}

        parts = command.split()
        directive = parts[0].lower()

        if directive == ".dc":
            result = await self._tool_run_sim({"mode": "dc"})
            return {"ok": True, "directive": directive, "result": result}

        if directive == ".tran":
            t_stop = _parse_spice_time(parts[2]) if len(parts) > 2 else 0.01
            dt = _parse_spice_time(parts[1]) if len(parts) > 1 else 1e-4
            result = await self._tool_run_sim({"mode": "transient", "t_stop": t_stop, "dt": dt})
            return {"ok": True, "directive": directive, "result": result}

        if directive in (".ac", ".step"):
            return {
                "ok": True,
                "directive": directive,
                "message": f"{directive} accepted; full sweep not yet implemented in Python solver.",
                "parsed": parts[1:],
            }

        return {"ok": False, "error": f"Unsupported SPICE directive: {directive}"}

    async def _tool_get_circuit_summary(self, args: Dict[str, Any]) -> Dict[str, Any]:
        by_type: Dict[str, int] = {}
        for comp in self.circuit.components:
            by_type[comp.type.value] = by_type.get(comp.type.value, 0) + 1

        return {
            "ok": True,
            "circuit_id": self.circuit.id,
            "name": self.circuit.name,
            "component_count": len(self.circuit.components),
            "wire_count": len(self.circuit.wires),
            "components_by_type": by_type,
            "components": [
                {
                    "id": c.id,
                    "name": c.name,
                    "type": c.type.value,
                    "parameters": c.parameters,
                }
                for c in self.circuit.components
            ],
            "wires": [w.model_dump() for w in self.circuit.wires],
            "last_sim_mode": self.sim_results.get("mode"),
        }

    def _notify_changed(self) -> None:
        if self.on_circuit_changed:
            self.on_circuit_changed()


def _parse_spice_time(token: str) -> float:
    suffixes = {"f": 1e-15, "p": 1e-12, "n": 1e-9, "u": 1e-6, "m": 1e-3, "k": 1e3}
    token = token.lower()
    if token and token[-1] in suffixes:
        return float(token[:-1]) * suffixes[token[-1]]
    return float(token)
