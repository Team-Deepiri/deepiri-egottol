"""Multi-domain simulation orchestrator: analog, digital, analog-compute, and EII."""

from __future__ import annotations

import logging
from typing import Any, Dict, List, Optional

import numpy as np

from egottol.engines.analog_compute.orchestrator import AnalogComputeOrchestrator
from egottol.engines.simulator import EventDrivenSimulator
from egottol.engines.solver import AdvancedMNASolver
from egottol.models.base import Circuit, ComponentType

logger = logging.getLogger(__name__)

try:
    from egottol.engines.eii.pipeline import EIIPipeline
    from egottol.models.eii import EIIPipelineConfig

    _EII_AVAILABLE = True
except ImportError:
    EIIPipeline = None  # type: ignore[misc, assignment]
    EIIPipelineConfig = None  # type: ignore[misc, assignment]
    _EII_AVAILABLE = False


class MultiDomainOrchestrator:
    """Coordinates MNA, event-driven logic, analog compute, and optional EII pipeline."""

    def __init__(
        self,
        circuit: Circuit,
        eii_config: Optional[Any] = None,
        analog_compute: Optional[AnalogComputeOrchestrator] = None,
    ):
        self.circuit = circuit
        self.mna = AdvancedMNASolver(circuit)
        self.logic = EventDrivenSimulator(circuit)
        self.analog_compute = analog_compute or self._build_analog_compute()
        self.eii = self._init_eii(eii_config)
        self.t = 0.0
        self._dc_result: Dict[str, float] = {}
        self._transient_history: List[Dict[str, Any]] = []

    def _build_analog_compute(self) -> AnalogComputeOrchestrator:
        n = 4
        for comp in self.circuit.components:
            if comp.type == ComponentType.ANALOG_COMPUTE:
                n = max(n, int(comp.parameters.get("rows", comp.parameters.get("n_modes", n))))
        return AnalogComputeOrchestrator(n_modes=n)

    def _init_eii(self, eii_config: Optional[Any]):
        if not _EII_AVAILABLE or EIIPipeline is None:
            logger.debug("EII pipeline not available; continuing without EII.")
            return None
        try:
            cfg = eii_config if eii_config is not None else EIIPipelineConfig()
            return EIIPipeline(cfg)
        except Exception as exc:
            logger.warning("Failed to initialize EII pipeline: %s", exc)
            return None

    @property
    def eii_available(self) -> bool:
        return self.eii is not None

    def solve_dc(self) -> Dict[str, float]:
        """Run DC MNA solve and merge analog-compute steady-state probe values."""
        self._dc_result = self.mna.solve_dc()
        ac = self.analog_compute.last_state
        for i, v in enumerate(ac.membrane):
            self._dc_result[f"analog_compute:membrane:{i}"] = float(v)
        for i, c in enumerate(ac.crossbar_currents):
            self._dc_result[f"analog_compute:I_col:{i}"] = float(c)
        return dict(self._dc_result)

    def step_transient(self, dt: float) -> Dict[str, Any]:
        """Advance one coupled timestep across all domains."""
        self.logic.step()
        ac_state = self.analog_compute.step(dt)
        self.t += dt

        snapshot: Dict[str, Any] = {
            "t": self.t,
            "logic_state": {cid: dict(ports) for cid, ports in self.logic.state.items()},
            "analog_compute": {
                "membrane": ac_state.membrane.tolist(),
                "crossbar_currents": ac_state.crossbar_currents.tolist(),
                "n_spikes": len(ac_state.spikes),
            },
        }

        if self.eii is not None:
            try:
                v_probe = ac_state.membrane
                i_probe = ac_state.crossbar_currents
                eii_out = self.eii.step(v_probe, i_probe, dt=dt)
                snapshot["eii"] = eii_out
            except Exception as exc:
                logger.warning("EII step failed, disabling pipeline: %s", exc)
                self.eii = None

        self._transient_history.append(snapshot)
        return snapshot

    def run_eii_window(self, T: float, target_class: int = 0) -> Optional[List[Dict[str, Any]]]:
        """Run EII pipeline over window T using analog-compute probe traces."""
        if self.eii is None:
            logger.info("EII pipeline unavailable; run_eii_window skipped.")
            return None

        dt = self.eii.config.dt
        steps = max(int(round(T / dt)), 1)
        outputs: List[Dict[str, Any]] = []

        self.eii.reset()
        try:
            for _ in range(steps):
                ac_state = self.analog_compute.step(dt)
                self.t += dt
                out = self.eii.step(
                    ac_state.membrane,
                    ac_state.crossbar_currents,
                    dt=dt,
                    target_class=target_class,
                )
                outputs.append(out)
        except Exception as exc:
            logger.warning("EII window run failed: %s", exc)
            self.eii = None
            return outputs or None

        return outputs

    def reset(self) -> None:
        self.t = 0.0
        self._dc_result.clear()
        self._transient_history.clear()
        self.analog_compute.reset()
        if self.eii is not None:
            self.eii.reset()
