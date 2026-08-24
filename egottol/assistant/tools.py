"""Copilot tool definitions and execution."""

from __future__ import annotations

import uuid
from typing import Any, Callable, Dict, List, Optional

import numpy as np

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
    {
        "type": "function",
        "function": {
            "name": "lookup_ee_design",
            "description": (
                "Look up electrical design guidance from Egottol's EE knowledge base "
                "(symptom → combination → applied use). Use for flyback diodes, LED "
                "current limits, RC filters, buck/boost, crystals/MUX, PCB floorplan, motors."
            ),
            "parameters": {
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "Symptom or topic, e.g. 'relay kills MOSFET', 'LED burned', 'buck'",
                    },
                    "limit": {
                        "type": "integer",
                        "description": "Max hits (default 5)",
                    },
                },
                "required": ["query"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "run_eii_sim",
            "description": "Run the EII (detector/encoder/engine/actuator) pipeline on the current circuit.",
            "parameters": {
                "type": "object",
                "properties": {
                    "t_stop": {"type": "number", "description": "Simulation window in seconds"},
                    "target_class": {"type": "integer", "description": "Target class for actuator feedback"},
                },
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "tune_analog_ai",
            "description": "Set parameters on an EII or analog-compute component.",
            "parameters": {
                "type": "object",
                "properties": {
                    "component_id": {"type": "string", "description": "Schematic component id"},
                    "parameters": {
                        "type": "object",
                        "description": "Parameter key/value overrides",
                    },
                },
                "required": ["component_id", "parameters"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "analyze_spikes",
            "description": "Analyze spike raster statistics from the last EII or analog-compute simulation.",
            "parameters": {
                "type": "object",
                "properties": {
                    "component_id": {
                        "type": "string",
                        "description": "Optional LIF/spiking component id to filter",
                    },
                },
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "optimize_crossbar",
            "description": "Suggest or program memristor crossbar conductances for XOR or a linear transform.",
            "parameters": {
                "type": "object",
                "properties": {
                    "objective": {
                        "type": "string",
                        "enum": ["xor", "linear_transform"],
                        "description": "Weight programming objective",
                    },
                    "component_id": {
                        "type": "string",
                        "description": "CROSSBAR component id to program",
                    },
                    "program": {
                        "type": "boolean",
                        "description": "If true, write conductances onto the component",
                    },
                    "matrix": {
                        "type": "array",
                        "items": {"type": "array", "items": {"type": "number"}},
                        "description": "Custom weight matrix for linear_transform",
                    },
                },
                "required": ["objective"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "suggest_analog_ai_stack",
            "description": "Place a full EII pipeline (detector, encoder, engine, actuator) on the canvas.",
            "parameters": {
                "type": "object",
                "properties": {
                    "x": {"type": "number", "description": "Origin X coordinate"},
                    "y": {"type": "number", "description": "Origin Y coordinate"},
                    "spacing": {"type": "number", "description": "Horizontal spacing between blocks"},
                },
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "run_nsp",
            "description": "Run NeuralSignalProcessor on a signal trace or NSP_AI component input.",
            "parameters": {
                "type": "object",
                "properties": {
                    "component_id": {
                        "type": "string",
                        "description": "NSP_AI component id (uses last sim trace if omitted)",
                    },
                    "mode": {
                        "type": "string",
                        "enum": ["denoise", "classify", "anomaly"],
                        "description": "NSP processing mode",
                    },
                    "window": {"type": "integer", "description": "Moving-average / FFT window"},
                    "threshold": {"type": "number", "description": "Gate or anomaly threshold"},
                    "signal": {
                        "type": "array",
                        "items": {"type": "number"},
                        "description": "Optional raw signal samples",
                    },
                },
                "required": ["mode"],
            },
        },
    },
    {
        "type": "function",
        "function": {
            "name": "auto_tune_circuit",
            "description": "Invoke AutoTuner to optimize component parameters toward target metrics.",
            "parameters": {
                "type": "object",
                "properties": {
                    "component_id": {"type": "string", "description": "Component to tune"},
                    "targets": {
                        "type": "object",
                        "description": "Target parameter values or metrics",
                    },
                    "max_iterations": {
                        "type": "integer",
                        "description": "Maximum tuning iterations",
                    },
                },
                "required": ["component_id"],
            },
        },
    },
]

_EII_PIPELINE_MAP = {
    "adsb_decode": ("ADSB_TX", "ADS-B Transponder", ComponentType.RF),
    "sdr_rx": ("SDR_RX", "SDR Decoder", ComponentType.RF),
    "gdl90_encode": ("GDL90_TX", "GDL-90 Encoder", ComponentType.RF),
    "neural_signal": ("NSP_AI", "NSP_AI", ComponentType.EXPERIMENTAL),
}

_ANALOG_AI_KEYS = frozenset({
    "IMPULSE_DETECTOR", "INFERENCE_ENCODER", "INFERENCE_ENGINE", "EII_PIPELINE",
    "MEMRISTOR", "CROSSBAR", "LIF_NEURON", "MZI_MESH",
    "OTA", "GILBERT_MULT", "OPAMP_NEURON", "HOPFIELD_NET", "ISING_CELL",
    "ANALOG_SAMPLE_HOLD", "WINNER_TAKE_ALL", "NSP_AI",
})

_EII_STACK_LAYOUT = (
    ("IMPULSE_DETECTOR", "detector"),
    ("INFERENCE_ENCODER", "encoder"),
    ("INFERENCE_ENGINE", "engine"),
    ("EII_PIPELINE", "actuator"),
)


def _find_component(circuit: Circuit, comp_id: str) -> Optional[Component]:
    for comp in circuit.components:
        if comp.id == comp_id:
            return comp
    return None


def _component_registry_key(comp: Component) -> str:
    return str(comp.metadata.get("key") or comp.metadata.get("registry_key") or "")


def _is_analog_ai_component(comp: Component) -> bool:
    key = _component_registry_key(comp)
    return comp.type == ComponentType.ANALOG_COMPUTE or key in _ANALOG_AI_KEYS


def _xor_crossbar_conductances(rows: int = 4, cols: int = 4) -> List[List[float]]:
    """Differential XOR weight pattern for a small crossbar."""
    g_min, g_max = 1e-6, 1e-3
    template = np.array([
        [g_max, g_min, g_max, g_min],
        [g_min, g_max, g_max, g_min],
        [g_max, g_max, g_min, g_min],
        [g_min, g_min, g_min, g_max],
    ], dtype=float)
    g = np.full((rows, cols), g_min)
    r, c = min(rows, 4), min(cols, 4)
    g[:r, :c] = template[:r, :c]
    return g.tolist()


def _linear_transform_conductances(
    rows: int,
    cols: int,
    matrix: Optional[List[List[float]]] = None,
) -> List[List[float]]:
    if matrix:
        g = np.asarray(matrix, dtype=float)
        if g.shape != (rows, cols):
            g = np.resize(g, (rows, cols))
        return np.clip(np.abs(g), 1e-6, 1e-3).tolist()
    g = np.eye(min(rows, cols), dtype=float) * 5e-4
    out = np.full((rows, cols), 1e-6)
    out[: g.shape[0], : g.shape[1]] = g
    return out.tolist()


class _FallbackAutoTuner:
    """Simple parameter nudge toward targets when egottol.engines.ai.auto_tune is unavailable."""

    def tune(
        self,
        parameters: Dict[str, Any],
        targets: Dict[str, Any],
        max_iterations: int = 20,
    ) -> Dict[str, Any]:
        updated = dict(parameters)
        history: List[Dict[str, Any]] = []
        for step in range(max(1, max_iterations)):
            delta = 0.0
            for key, target in targets.items():
                if key not in updated or not isinstance(updated[key], (int, float)):
                    continue
                current = float(updated[key])
                tgt = float(target)
                step_size = (tgt - current) * 0.25
                updated[key] = current + step_size
                delta += abs(step_size)
            history.append({"step": step, "residual": delta})
            if delta < 1e-9:
                break
        return {"parameters": updated, "history": history, "converged": delta < 1e-9}


def _get_auto_tuner():
    try:
        from egottol.engines.ai.auto_tune import AutoTuner
        return AutoTuner()
    except ImportError:
        return _FallbackAutoTuner()


def _extract_signal_from_sim(sim_results: Dict[str, Any]) -> Optional[List[float]]:
    mode = sim_results.get("mode")
    if mode == "transient":
        waveform = sim_results.get("waveform") or []
        if not waveform:
            return None
        first_node = next(iter(waveform[0].get("v", {}).keys()), None)
        if first_node:
            return [float(step["v"].get(first_node, 0.0)) for step in waveform]
    if mode == "eii":
        trace = sim_results.get("voltage_trace")
        if trace:
            return list(trace)
    return None


def _serialize_spikes(spikes: Any) -> List[Dict[str, Any]]:
    out: List[Dict[str, Any]] = []
    for s in spikes or []:
        if isinstance(s, dict):
            out.append(s)
        else:
            out.append({
                "t": float(getattr(s, "t", 0.0)),
                "neuron": int(getattr(s, "neuron", 0)),
                "amplitude": float(getattr(s, "amplitude", 1.0)),
            })
    return out

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

    async def _tool_lookup_ee_design(self, args: Dict[str, Any]) -> Dict[str, Any]:
        from egottol.knowledge import format_lookup, lookup_ee_design

        query = str(args.get("query") or "")
        limit = int(args.get("limit") or 5)
        hits = lookup_ee_design(query, limit=limit)
        return {
            "ok": True,
            "query": query,
            "hits": hits,
            "formatted": format_lookup(query, limit=limit),
            "docs_root": "docs/ee/",
        }

    async def _tool_run_eii_sim(self, args: Dict[str, Any]) -> Dict[str, Any]:
        t_stop = float(args.get("t_stop") or 1e-3)
        target_class = int(args.get("target_class") or 0)
        outputs: List[Dict[str, Any]] = []
        spike_raster: List[Dict[str, Any]] = []
        analog_summary: Dict[str, Any] = {}

        try:
            from egottol.engines.orchestrator import MultiDomainOrchestrator

            orch = MultiDomainOrchestrator(self.circuit)
            if orch.eii_available and hasattr(orch, "run_eii_window"):
                raw = orch.run_eii_window(t_stop, target_class=target_class)
                outputs = raw or []
                ac = orch.analog_compute
                spike_raster = _serialize_spikes(ac.spiking.state.spikes)
                analog_summary = ac.summary()
            else:
                from egottol.engines.eii.pipeline import EIIPipeline

                pipeline = EIIPipeline.from_circuit(self.circuit)
                outputs = pipeline.run_duration(t_stop)
        except ImportError as exc:
            return {"ok": False, "error": f"EII engine unavailable: {exc}"}
        except Exception as exc:
            return {"ok": False, "error": str(exc)}

        last = outputs[-1] if outputs else {}
        prediction = last.get("prediction")
        confidence = last.get("confidence", 0.0)
        if hasattr(prediction, "tolist"):
            prediction = prediction.tolist()

        self.sim_results = {
            "mode": "eii",
            "steps": len(outputs),
            "outputs": outputs[-5:] if len(outputs) > 5 else outputs,
            "last_prediction": prediction,
            "last_confidence": confidence,
            "n_spikes": len(spike_raster),
            "spike_raster": spike_raster,
            "analog_compute": analog_summary,
            "eii": {"steps": len(outputs), "t_stop": t_stop},
        }
        return {
            "ok": True,
            "steps": len(outputs),
            "last_prediction": prediction,
            "last_confidence": confidence,
            "n_spikes": len(spike_raster),
        }

    async def _tool_tune_analog_ai(self, args: Dict[str, Any]) -> Dict[str, Any]:
        comp_id = args.get("component_id", "")
        comp = _find_component(self.circuit, comp_id)
        if comp is None:
            return {"ok": False, "error": f"Component not found: {comp_id}"}
        if not _is_analog_ai_component(comp):
            return {"ok": False, "error": f"Component {comp_id} is not an EII/analog-AI block"}

        overrides = args.get("parameters") or {}
        if not overrides:
            return {"ok": False, "error": "No parameters provided"}

        comp.parameters.update(overrides)
        self._notify_changed()
        return {"ok": True, "component_id": comp_id, "parameters": comp.parameters}

    async def _tool_analyze_spikes(self, args: Dict[str, Any]) -> Dict[str, Any]:
        raster = self.sim_results.get("spike_raster")
        if not raster:
            ac = self.sim_results.get("analog_compute") or {}
            raster = ac.get("spike_raster")

        if not raster:
            return {
                "ok": False,
                "error": "No spike data. Run run_eii_sim or a transient sim with spiking first.",
            }

        comp_id = args.get("component_id")
        if comp_id:
            neuron_filter = None
            comp = _find_component(self.circuit, comp_id)
            if comp and "channel" in comp.parameters:
                neuron_filter = int(comp.parameters["channel"])
            if neuron_filter is not None:
                raster = [s for s in raster if s.get("neuron") == neuron_filter]

        if not raster:
            return {"ok": False, "error": "No spikes matched the filter."}

        times = [float(s["t"]) for s in raster]
        neurons = [int(s["neuron"]) for s in raster]
        unique_neurons = sorted(set(neurons))
        rates: Dict[str, float] = {}
        duration = max(times) - min(times) if len(times) > 1 else 1.0
        duration = max(duration, 1e-9)
        for n in unique_neurons:
            count = sum(1 for x in neurons if x == n)
            rates[str(n)] = count / duration

        analysis = {
            "spike_count": len(raster),
            "unique_neurons": unique_neurons,
            "mean_rate_hz": sum(rates.values()) / max(len(rates), 1),
            "per_neuron_rate_hz": rates,
            "time_span": {"start": min(times), "end": max(times)},
            "burstiness": len(raster) / max(len(unique_neurons), 1),
        }
        self.sim_results["spike_analysis"] = analysis
        return {"ok": True, "analysis": analysis}

    async def _tool_optimize_crossbar(self, args: Dict[str, Any]) -> Dict[str, Any]:
        objective = (args.get("objective") or "xor").lower()
        program = bool(args.get("program", False))
        matrix = args.get("matrix")

        rows, cols = 4, 4
        comp = None
        comp_id = args.get("component_id")
        if comp_id:
            comp = _find_component(self.circuit, comp_id)
            if comp is None:
                return {"ok": False, "error": f"Component not found: {comp_id}"}
            rows = int(comp.parameters.get("rows", rows))
            cols = int(comp.parameters.get("cols", cols))

        if objective == "xor":
            conductances = _xor_crossbar_conductances(rows, cols)
        elif objective == "linear_transform":
            conductances = _linear_transform_conductances(rows, cols, matrix)
        else:
            return {"ok": False, "error": f"Unknown objective: {objective}"}

        result: Dict[str, Any] = {
            "ok": True,
            "objective": objective,
            "conductances": conductances,
            "shape": [rows, cols],
        }

        if program and comp is not None:
            comp.parameters["conductances"] = conductances
            comp.parameters["G"] = conductances
            self._notify_changed()
            result["programmed"] = True
            result["component_id"] = comp_id
        else:
            result["programmed"] = False

        return result

    async def _tool_suggest_analog_ai_stack(self, args: Dict[str, Any]) -> Dict[str, Any]:
        x0 = float(args.get("x") or 0.0)
        y0 = float(args.get("y") or 0.0)
        spacing = float(args.get("spacing") or 180.0)
        placed: List[Dict[str, Any]] = []

        for idx, (key, role) in enumerate(_EII_STACK_LAYOUT):
            defn = COMPONENT_LIBRARY.get(key)
            if defn is None:
                continue
            comp_id = f"{key}_{uuid.uuid4().hex[:6]}"
            self.circuit.components.append(
                Component(
                    id=comp_id,
                    name=defn.name,
                    type=defn.category,
                    ports=list(defn.ports),
                    parameters=dict(defn.parameters),
                    metadata={
                        "key": key,
                        "registry_key": key,
                        "eii": True,
                        "eii_role": role,
                        "x": x0 + idx * spacing,
                        "y": y0,
                    },
                )
            )
            placed.append({"component_id": comp_id, "key": key, "role": role})

        if not placed:
            return {"ok": False, "error": "Could not place EII stack — registry entries missing."}

        self._notify_changed()
        return {"ok": True, "components": placed}

    async def _tool_run_nsp(self, args: Dict[str, Any]) -> Dict[str, Any]:
        mode = (args.get("mode") or "denoise").lower()
        if mode not in ("denoise", "classify", "anomaly"):
            return {"ok": False, "error": f"Invalid NSP mode: {mode}"}

        try:
            from egottol.engines.ai.nsp import NSPConfig, NeuralSignalProcessor
        except ImportError:
            return {"ok": False, "error": "NeuralSignalProcessor engine not available"}

        window = int(args.get("window") or 32)
        threshold = float(args.get("threshold") or 0.15)
        nsp = NeuralSignalProcessor(
            NSPConfig(
                moving_avg_window=max(window, 1),
                spectral_gate_threshold=threshold,
                fft_bins=max(window, 8),
                anomaly_z_threshold=threshold if threshold > 1 else 3.0,
            )
        )

        signal = args.get("signal")
        comp_id = args.get("component_id")
        if comp_id:
            comp = _find_component(self.circuit, comp_id)
            if comp is None:
                return {"ok": False, "error": f"Component not found: {comp_id}"}
            comp.parameters["mode"] = mode
            comp.parameters["window"] = window
            comp.parameters["threshold"] = threshold

        if signal is None:
            extracted = _extract_signal_from_sim(self.sim_results)
            if extracted is None:
                t = np.linspace(0, 1, 256)
                signal = (0.5 * np.sin(2 * np.pi * 10 * t) + 0.05 * np.random.default_rng(0).normal(size=t.size)).tolist()
            else:
                signal = extracted

        x = np.asarray(signal, dtype=float)
        output_summary: Dict[str, Any] = {"mode": mode, "samples": int(x.size)}

        if mode == "denoise":
            out = nsp.denoise(x)
            output_summary["output_rms"] = float(np.sqrt(np.mean(out ** 2)))
            output_summary["snr_improvement_db"] = float(
                10 * np.log10(np.var(out) / max(np.var(x - out), 1e-12))
            )
            self.sim_results = {
                "mode": "nsp",
                "nsp_mode": mode,
                "output": out.tolist(),
                "output_summary": output_summary,
            }
            return {"ok": True, **output_summary}

        if mode == "classify":
            label, probs = nsp.classify(x)
            output_summary["label"] = label
            output_summary["probabilities"] = probs.tolist()
            self.sim_results = {
                "mode": "nsp",
                "nsp_mode": mode,
                "output_summary": output_summary,
            }
            return {"ok": True, **output_summary}

        is_anomaly, z = nsp.anomaly_detect(x)
        output_summary["is_anomaly"] = is_anomaly
        output_summary["z_score"] = z
        self.sim_results = {
            "mode": "nsp",
            "nsp_mode": mode,
            "output_summary": output_summary,
        }
        return {"ok": True, **output_summary}

    async def _tool_auto_tune_circuit(self, args: Dict[str, Any]) -> Dict[str, Any]:
        comp_id = args.get("component_id", "")
        comp = _find_component(self.circuit, comp_id)
        if comp is None:
            return {"ok": False, "error": f"Component not found: {comp_id}"}

        targets = args.get("targets") or {}
        if not targets:
            return {"ok": False, "error": "No targets provided for auto-tuning"}

        max_iterations = int(args.get("max_iterations") or 20)
        tuner = _get_auto_tuner()

        try:
            tune_result = tuner.tune(comp, targets, max_iterations=max_iterations)
        except TypeError:
            tune_result = tuner.tune(comp.parameters, targets, max_iterations=max_iterations)

        if isinstance(tune_result, dict) and "parameters" in tune_result:
            comp.parameters.update(tune_result["parameters"])
        elif isinstance(tune_result, dict):
            comp.parameters.update({k: v for k, v in tune_result.items() if k not in ("history", "converged")})

        self._notify_changed()
        return {
            "ok": True,
            "component_id": comp_id,
            "parameters": comp.parameters,
            "tune_result": tune_result if isinstance(tune_result, dict) else {"parameters": comp.parameters},
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
