import logging
from typing import Dict, List, Tuple

import numpy as np

from egottol.engines.eii.types import EIIState, ImpulseEvent
from egottol.models.eii import ActuatorMode, FeedbackActuatorConfig

logger = logging.getLogger(__name__)


class FeedbackActuator:
    """Feedback operator Gamma: inference outputs actuate the circuit."""

    def __init__(self, config: FeedbackActuatorConfig):
        self.config = config
        self.mode = config.mode
        self._last_pre_spike_time: Dict[Tuple[int, int], float] = {}

    def actuate(
        self,
        state: EIIState,
        prediction: np.ndarray,
        confidence: float,
        events: List[ImpulseEvent],
        target_class: int = 0,
    ) -> np.ndarray:
        y = np.asarray(prediction, dtype=float).reshape(-1)
        if self.mode == ActuatorMode.DAC:
            control = self._actuate_dac(y)
        elif self.mode == ActuatorMode.STDP:
            control = self._actuate_stdp(state, y, confidence, events, target_class)
        elif self.mode == ActuatorMode.DIGITAL:
            control = self._actuate_digital(state, y)
        elif self.mode == ActuatorMode.OPTICAL:
            control = self._actuate_optical(state, y)
        else:
            logger.warning("Unknown actuator mode %s; falling back to DAC", self.mode)
            control = self._actuate_dac(y)

        state.control = control
        return control

    def _actuate_dac(self, prediction: np.ndarray) -> np.ndarray:
        return self.config.v_dd * np.clip(prediction, 0.0, 1.0)

    def _actuate_stdp(
        self,
        state: EIIState,
        prediction: np.ndarray,
        confidence: float,
        events: List[ImpulseEvent],
        target_class: int,
    ) -> np.ndarray:
        if state.conductances is None:
            return self._actuate_dac(prediction)

        p_correct = float(prediction[target_class % len(prediction)])
        e_inference = 1.0 - p_correct
        control = self._actuate_dac(prediction)

        for event in events:
            key = (event.channel, event.channel)
            dt_spike = state.t - self._last_pre_spike_time.get(key, state.t)
            self._last_pre_spike_time[key] = state.t
            delta_t = dt_spike if event.event_type != "memristor_switch" else 0.0
            delta_g = self._stdp_delta(delta_t) * e_inference * self.config.stdp_eta
            idx = event.channel % state.conductances.size
            state.conductances.flat[idx] = np.clip(
                state.conductances.flat[idx] + delta_g,
                1e-9,
                1.0,
            )
        return control

    def _stdp_delta(self, delta_t: float) -> float:
        if delta_t > 0:
            return self.config.stdp_a_plus * np.exp(-delta_t / max(self.config.stdp_tau_plus, 1e-9))
        if delta_t < 0:
            return -self.config.stdp_a_minus * np.exp(delta_t / max(self.config.stdp_tau_minus, 1e-9))
        return 0.0

    def _actuate_digital(self, state: EIIState, prediction: np.ndarray) -> np.ndarray:
        control = np.zeros_like(prediction)
        for idx, value in enumerate(prediction):
            reg_name = f"OUT_{idx}"
            bit = 1.0 if value >= self.config.digital_threshold else 0.0
            state.logic_registers[reg_name] = bit
            control[idx] = bit
        return control

    def _actuate_optical(self, state: EIIState, prediction: np.ndarray) -> np.ndarray:
        if state.optical_phases is None:
            state.optical_phases = np.zeros(len(prediction), dtype=float)
        delta_phi = self.config.optical_phase_scale * prediction
        state.optical_phases = state.optical_phases + delta_phi
        return delta_phi

    def source_modulation(self, prediction: np.ndarray, t: float) -> float:
        cls = int(np.argmax(prediction))
        amplitude = self.config.source_amplitude_scale * float(prediction[cls])
        omega = 2.0 * np.pi * self.config.source_frequency
        return amplitude * np.sin(omega * t)
