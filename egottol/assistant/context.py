"""Build JSON context from circuit state and simulation results."""

from __future__ import annotations

from typing import Any, Dict, List, Optional

from egottol.models.base import Circuit, ComponentType
from egottol.models.registry import COMPONENT_LIBRARY


_EII_REGISTRY_KEYS = frozenset({
    "IMPULSE_DETECTOR", "INFERENCE_ENCODER", "INFERENCE_ENGINE",
    "EII_PIPELINE", "MEMRISTOR", "CROSSBAR", "LIF_NEURON", "MZI_MESH",
})

_ANALOG_AI_KEYS = frozenset({
    *_EII_REGISTRY_KEYS,
    "OTA", "GILBERT_MULT", "OPAMP_NEURON", "HOPFIELD_NET", "ISING_CELL",
    "ANALOG_SAMPLE_HOLD", "WINNER_TAKE_ALL", "NSP_AI",
})

_AI_COPILOT_TOOLS = [
    "run_eii_sim", "tune_analog_ai", "analyze_spikes", "optimize_crossbar",
    "suggest_analog_ai_stack", "run_nsp", "auto_tune_circuit",
]


class ContextBuilder:
    """Assembles structured context for LLM prompts."""

    SYSTEM_PROMPT = (
        "You are Egottol Copilot, an assistant for the Deepiri Egottol circuit lab. "
        "Help users design schematics, run simulations, interpret waveforms, and "
        "configure avionics/RF and analog-AI pipelines (EII detector/encoder/engine/actuator, "
        "memristor crossbars, spiking neurons, NSP signal processing). "
        "For electrical design questions (series/parallel, flyback diodes, RC/RL/LC, "
        "BJT/MOSFET with C/L, motors, crystals/MUX, PCB floorplanning, symptom→fix), "
        "call lookup_ee_design and cite docs/ee/. "
        "Use run_eii_sim, tune_analog_ai, analyze_spikes, optimize_crossbar, "
        "suggest_analog_ai_stack, run_nsp, and auto_tune_circuit when analog-AI actions are needed. "
        "Consult eii_state, analog_compute_state, and ai_capabilities in the context packet. "
        "Non-negotiable: inductive loads need a freewheeling diode; LEDs need series R; "
        "never parallel crystals — use a MUX."
    )

    def __init__(self, circuit: Circuit, sim_results: Dict[str, Any] | None = None):
        self.circuit = circuit
        self.sim_results = sim_results or {}

    def build(self) -> Dict[str, Any]:
        return {
            "circuit": self._circuit_payload(),
            "simulation": self._sim_payload(),
            "eii_state": self._eii_state_payload(),
            "analog_compute_state": self._analog_compute_state_payload(),
            "ai_capabilities": self._ai_capabilities_payload(),
        }

    def build_system_message(self) -> str:
        import json

        ctx = self.build()
        return (
            f"{self.SYSTEM_PROMPT}\n\n"
            f"Current circuit context (JSON):\n```json\n{json.dumps(ctx, indent=2)}\n```"
        )

    def _circuit_payload(self) -> Dict[str, Any]:
        return {
            "id": self.circuit.id,
            "name": self.circuit.name,
            "component_count": len(self.circuit.components),
            "wire_count": len(self.circuit.wires),
            "components": [
                {
                    "id": c.id,
                    "name": c.name,
                    "type": c.type.value,
                    "ports": [p.name for p in c.ports],
                    "parameters": c.parameters,
                    "metadata": c.metadata,
                }
                for c in self.circuit.components
            ],
            "wires": [
                {
                    "id": w.id,
                    "from": f"{w.from_component}:{w.from_port}",
                    "to": f"{w.to_component}:{w.to_port}",
                }
                for w in self.circuit.wires
            ],
        }

    def _sim_payload(self) -> Dict[str, Any]:
        if not self.sim_results:
            return {"available": False}

        mode = self.sim_results.get("mode")
        payload: Dict[str, Any] = {"available": True, "mode": mode}

        if mode == "dc":
            nodes = self.sim_results.get("nodes", {})
            payload["node_voltages"] = nodes
            payload["sample_nodes"] = _sample_dict(nodes, limit=20)
        elif mode == "transient":
            waveform = self.sim_results.get("waveform") or []
            payload["sample_count"] = len(waveform)
            payload["time_span"] = _transient_span(waveform)
            payload["preview"] = waveform[:5] + waveform[-3:] if len(waveform) > 8 else waveform
        elif mode == "logic":
            payload["state"] = self.sim_results.get("state", {})
            payload["steps"] = self.sim_results.get("steps")
        elif mode == "eii":
            payload["steps"] = self.sim_results.get("steps")
            payload["last_prediction"] = self.sim_results.get("last_prediction")
            payload["last_confidence"] = self.sim_results.get("last_confidence")
            payload["n_spikes"] = self.sim_results.get("n_spikes")
        elif mode == "nsp":
            payload["nsp_mode"] = self.sim_results.get("nsp_mode")
            payload["output_summary"] = self.sim_results.get("output_summary")

        return payload

    def _eii_state_payload(self) -> Dict[str, Any]:
        components: List[Dict[str, Any]] = []
        for comp in self.circuit.components:
            key = comp.metadata.get("key") or comp.metadata.get("registry_key", "")
            if not (
                key in _EII_REGISTRY_KEYS
                or comp.metadata.get("eii_pipeline")
                or comp.metadata.get("eii")
                or comp.metadata.get("eii_role")
            ):
                continue
            components.append({
                "id": comp.id,
                "key": key,
                "name": comp.name,
                "eii_role": comp.metadata.get("eii_role"),
                "parameters": comp.parameters,
            })

        sim_eii = self.sim_results.get("eii") if self.sim_results.get("mode") == "eii" else {}
        return {
            "components_on_canvas": components,
            "pipeline_ready": len(components) > 0,
            "last_run": {
                "steps": sim_eii.get("steps") if isinstance(sim_eii, dict) else None,
                "last_prediction": self.sim_results.get("last_prediction"),
                "last_confidence": self.sim_results.get("last_confidence"),
            } if self.sim_results.get("mode") == "eii" else None,
        }

    def _analog_compute_state_payload(self) -> Dict[str, Any]:
        on_canvas = [
            {
                "id": c.id,
                "key": c.metadata.get("key") or c.metadata.get("registry_key", ""),
                "parameters": c.parameters,
            }
            for c in self.circuit.components
            if c.type == ComponentType.ANALOG_COMPUTE
            or (c.metadata.get("key") or c.metadata.get("registry_key", "")) in _ANALOG_AI_KEYS
        ]

        ac = self.sim_results.get("analog_compute") or {}
        spikes = self.sim_results.get("spike_raster") or ac.get("spike_raster")
        return {
            "components_on_canvas": on_canvas,
            "last_membrane": ac.get("membrane"),
            "last_crossbar_currents": ac.get("crossbar_currents"),
            "n_spikes": ac.get("n_spikes") or self.sim_results.get("n_spikes"),
            "spike_raster_preview": _spike_raster_preview(spikes),
        }

    def _ai_capabilities_payload(self) -> Dict[str, Any]:
        analog_keys = sorted(
            k for k, defn in COMPONENT_LIBRARY.items()
            if defn.category == ComponentType.ANALOG_COMPUTE or k in _ANALOG_AI_KEYS
        )
        return {
            "copilot_tools": _AI_COPILOT_TOOLS,
            "nsp_modes": ["denoise", "classify", "anomaly"],
            "crossbar_objectives": ["xor", "linear_transform"],
            "analog_compute_registry_keys": analog_keys,
            "eii_stack_keys": [
                "IMPULSE_DETECTOR", "INFERENCE_ENCODER", "INFERENCE_ENGINE", "EII_PIPELINE",
            ],
        }


def _sample_dict(data: Dict[str, float], limit: int = 20) -> Dict[str, float]:
    items = list(data.items())[:limit]
    return dict(items)


def _transient_span(waveform: List[Dict[str, Any]]) -> Optional[Dict[str, float]]:
    if not waveform:
        return None
    return {"t_start": waveform[0].get("t"), "t_end": waveform[-1].get("t")}


def _spike_raster_preview(spikes: Any, limit: int = 50) -> Optional[List[Dict[str, Any]]]:
    if not spikes:
        return None
    preview = []
    for s in spikes[:limit]:
        if isinstance(s, dict):
            preview.append(s)
        else:
            preview.append({"t": getattr(s, "t", None), "neuron": getattr(s, "neuron", None)})
    return preview
