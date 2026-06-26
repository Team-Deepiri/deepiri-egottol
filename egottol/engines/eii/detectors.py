"""Impulse extraction: continuous waveforms → discrete events."""

from __future__ import annotations

from typing import List

import numpy as np

from egottol.engines.eii.types import EIIConfig, EIIState, ImpulseEvent


class ImpulseDetector:
    """Configurable detector for threshold, differentiator, comparator, RF, memristor modes."""

    MODES = ("threshold", "differentiator", "comparator", "rf_envelope", "memristor_switch")

    def __init__(self, config: EIIConfig):
        self.config = config
        self.threshold = 0.5
        self.tau_d = 1e-3
        self.theta_rf = 0.1
        self.i_set = 1e-6

    def detect(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        mode = self.config.detector_mode
        if mode not in self.MODES:
            raise ValueError(f"Unknown detector mode: {mode}")

        dispatch = {
            "threshold": self._threshold,
            "differentiator": self._differentiator,
            "comparator": self._comparator,
            "rf_envelope": self._rf_envelope,
            "memristor_switch": self._memristor_switch,
        }
        return dispatch[mode](state, dt)

    def _threshold(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        events: List[ImpulseEvent] = []
        n = min(len(state.voltages), self.config.num_channels)
        for ch in range(n):
            if state.refractory[ch] > 0:
                state.refractory[ch] = max(0.0, state.refractory[ch] - dt)
                continue
            v = state.voltages[ch]
            if v >= self.threshold:
                events.append(
                    ImpulseEvent(
                        t=state.t,
                        channel=ch,
                        event_type="threshold_cross",
                        amplitude=v,
                    )
                )
                state.refractory[ch] = self.config.window_T * 0.1
        return events

    def _differentiator(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        events: List[ImpulseEvent] = []
        n = min(len(state.voltages), self.config.num_channels)
        for ch in range(n):
            dv = (state.voltages[ch] - state.prev_voltages[ch]) / max(dt, 1e-12)
            a = self.tau_d * dv + state.voltages[ch]
            prev_a = state.prev_diff_signal[ch]
            if a > self.threshold and a < prev_a:
                events.append(
                    ImpulseEvent(
                        t=state.t,
                        channel=ch,
                        event_type="spike",
                        amplitude=a,
                    )
                )
            state.prev_diff_signal[ch] = a
        state.prev_voltages = state.voltages.copy()
        return events

    def _comparator(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        events: List[ImpulseEvent] = []
        n = min(len(state.voltages), self.config.num_channels)
        for ch in range(n):
            high = state.voltages[ch] >= self.threshold
            was_high = state.prev_comparator[ch] >= 0.5
            if high and not was_high:
                events.append(
                    ImpulseEvent(
                        t=state.t,
                        channel=ch,
                        event_type="spike",
                        amplitude=state.voltages[ch],
                    )
                )
            state.prev_comparator[ch] = 1.0 if high else 0.0
        return events

    def _rf_envelope(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        events: List[ImpulseEvent] = []
        alpha = dt / max(self.tau_d, dt)
        n = min(len(state.voltages), self.config.num_channels)
        for ch in range(n):
            target = state.voltages[ch] ** 2
            env = (1 - alpha) * state.prev_envelope[ch] + alpha * target
            if env > self.theta_rf and env < state.prev_envelope[ch]:
                events.append(
                    ImpulseEvent(
                        t=state.t,
                        channel=ch,
                        event_type="rf_burst_peak",
                        amplitude=np.sqrt(env),
                    )
                )
            state.prev_envelope[ch] = env
        return events

    def _memristor_switch(self, state: EIIState, dt: float) -> List[ImpulseEvent]:
        events: List[ImpulseEvent] = []
        n = min(len(state.currents), self.config.num_channels)
        for ch in range(n):
            i_now = state.currents[ch]
            i_prev = state.prev_memristor_current[ch]
            if abs(i_now) > self.i_set and np.sign(i_now) != np.sign(i_prev):
                events.append(
                    ImpulseEvent(
                        t=state.t,
                        channel=ch,
                        event_type="memristor_switch",
                        amplitude=i_now,
                    )
                )
            state.prev_memristor_current[ch] = i_now
        return events
