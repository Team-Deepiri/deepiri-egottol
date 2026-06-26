"""Build JSON context from circuit state and simulation results."""

from __future__ import annotations

from typing import Any, Dict, List, Optional

from egottol.models.base import Circuit


class ContextBuilder:
    """Assembles structured context for LLM prompts."""

    SYSTEM_PROMPT = (
        "You are Egottol Copilot, an assistant for the Deepiri Egottol circuit lab. "
        "Help users design schematics, run simulations, interpret waveforms, and "
        "configure avionics/RF pipelines. Use the provided tools when actions are needed."
    )

    def __init__(self, circuit: Circuit, sim_results: Dict[str, Any] | None = None):
        self.circuit = circuit
        self.sim_results = sim_results or {}

    def build(self) -> Dict[str, Any]:
        return {
            "circuit": self._circuit_payload(),
            "simulation": self._sim_payload(),
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

        return payload


def _sample_dict(data: Dict[str, float], limit: int = 20) -> Dict[str, float]:
    items = list(data.items())[:limit]
    return dict(items)


def _transient_span(waveform: List[Dict[str, Any]]) -> Optional[Dict[str, float]]:
    if not waveform:
        return None
    return {"t_start": waveform[0].get("t"), "t_end": waveform[-1].get("t")}
