"""Encoding manifold Φ: impulses → embedding z."""

from __future__ import annotations

from typing import List

import numpy as np

from egottol.engines.eii.types import EIIConfig, EIIState, ImpulseEvent


class EncodingManifold:
    """Maps impulse events and probe voltages to fixed-dimension embeddings."""

    MODES = ("rate", "latency", "filter", "population", "continuous")

    def __init__(self, config: EIIConfig):
        self.config = config
        self.alpha = 0.9
        self.tau_lat = 5e-3
        self.tau_m = 2e-3

    def encode(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        mode = self.config.encoder_mode
        if mode not in self.MODES:
            raise ValueError(f"Unknown encoder mode: {mode}")

        dispatch = {
            "rate": self._rate,
            "latency": self._latency,
            "filter": self._filter,
            "population": self._population,
            "continuous": self._continuous,
        }
        z = dispatch[mode](state, events, window_start=window_start)
        if self.config.read_noise_std > 0:
            z = z + np.random.normal(0, self.config.read_noise_std, size=z.shape)
        return z.astype(float)

    def _rate(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        d = self.config.embedding_dim
        counts = np.zeros(d)
        window_events = [e for e in events if e.t >= window_start]
        for e in window_events:
            ch = e.channel % d
            counts[ch] += 1.0
        return counts / max(self.config.window_T, 1e-12)

    def _latency(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        d = self.config.embedding_dim
        z = np.zeros(d)
        window_events = [e for e in events if e.t >= window_start]
        for ch in range(d):
            ch_events = [e for e in window_events if e.channel % d == ch]
            if ch_events:
                t_first = min(e.t for e in ch_events)
                z[ch] = np.exp(-(t_first - self.config.stimulus_time) / self.tau_lat)
        return z

    def _filter(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        d = self.config.embedding_dim
        z = state.filter_state.copy()
        if z.shape[0] != d:
            z = np.zeros(d)
        dt = self.config.dt
        decay = np.exp(-dt / max(self.tau_m, dt))
        z *= decay
        for e in events:
            if e.t >= window_start:
                ch = e.channel % d
                z[ch] += e.amplitude / max(self.tau_m, dt)
        state.filter_state = z.copy()
        return z

    def _population(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        per_channel = self._rate(state, events, window_start=window_start)
        d = self.config.embedding_dim
        if d == 1:
            return per_channel
        pooled = np.zeros(d)
        chunk = max(1, len(per_channel) // d)
        for i in range(d):
            start = i * chunk
            end = start + chunk if i < d - 1 else len(per_channel)
            pooled[i] = np.mean(per_channel[start:end]) if end > start else 0.0
        return pooled[:d]

    def _continuous(
        self,
        state: EIIState,
        events: List[ImpulseEvent],
        *,
        window_start: float,
    ) -> np.ndarray:
        d = self.config.embedding_dim
        probes = state.voltages[:d]
        if len(probes) < d:
            probes = np.pad(probes, (0, d - len(probes)))
        return probes.astype(float)
