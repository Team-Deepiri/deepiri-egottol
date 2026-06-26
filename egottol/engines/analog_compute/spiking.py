"""Neuromorphic spiking engine: LIF dynamics, spike detection, basic STDP."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import List, Optional, Tuple

import numpy as np


@dataclass
class SpikingConfig:
    n_neurons: int = 4
    tau_m: float = 20e-3
    v_rest: float = 0.0
    v_reset: float = 0.0
    v_thresh: float = 1.0
    r_m: float = 1e6
    c_m: float = 1e-9
    refractory: float = 2e-3
    stdp_eta: float = 0.01
    stdp_a_plus: float = 0.001
    stdp_a_minus: float = 0.0012
    stdp_tau_plus: float = 20e-3
    stdp_tau_minus: float = 20e-3


@dataclass
class SpikeEvent:
    t: float
    neuron: int
    amplitude: float = 1.0


@dataclass
class SpikingState:
    v: np.ndarray = field(default_factory=lambda: np.zeros(4))
    refractory: np.ndarray = field(default_factory=lambda: np.zeros(4))
    last_spike_t: np.ndarray = field(default_factory=lambda: np.full(4, -np.inf))
    spikes: List[SpikeEvent] = field(default_factory=list)


class SpikingEngine:
    """Leaky integrate-and-fire population with spike detection and STDP."""

    def __init__(
        self,
        n_neurons: int = 4,
        config: Optional[SpikingConfig] = None,
    ):
        self.config = config or SpikingConfig(n_neurons=n_neurons)
        self.n = self.config.n_neurons
        self.state = SpikingState(
            v=np.full(self.n, self.config.v_rest),
            refractory=np.zeros(self.n),
            last_spike_t=np.full(self.n, -np.inf),
        )
        self.t = 0.0
        self._pre_trace = np.zeros(self.n)
        self._post_trace = np.zeros(self.n)

    def reset(self) -> None:
        self.state = SpikingState(
            v=np.full(self.n, self.config.v_rest),
            refractory=np.zeros(self.n),
            last_spike_t=np.full(self.n, -np.inf),
        )
        self.t = 0.0
        self._pre_trace.fill(0.0)
        self._post_trace.fill(0.0)

    def step(self, i_in: np.ndarray, dt: float) -> Tuple[np.ndarray, List[SpikeEvent]]:
        """Advance LIF dynamics one timestep; return membrane voltages and new spikes."""
        i = np.asarray(i_in, dtype=float).reshape(-1)
        if i.size < self.n:
            i = np.pad(i, (0, self.n - i.size))
        i = i[: self.n]

        self.t += dt
        tau = self.config.tau_m
        v = self.state.v
        spikes: List[SpikeEvent] = []

        for k in range(self.n):
            if self.state.refractory[k] > 0:
                self.state.refractory[k] = max(0.0, self.state.refractory[k] - dt)
                v[k] = self.config.v_reset
                continue

            dv = (-(v[k] - self.config.v_rest) + self.config.r_m * i[k]) / tau
            v[k] += dv * dt

            if v[k] >= self.config.v_thresh:
                spikes.append(SpikeEvent(t=self.t, neuron=k, amplitude=v[k]))
                v[k] = self.config.v_reset
                self.state.refractory[k] = self.config.refractory
                self.state.last_spike_t[k] = self.t

        self.state.v = v
        self.state.spikes.extend(spikes)
        self._decay_traces(dt)
        return v.copy(), spikes

    def _decay_traces(self, dt: float) -> None:
        self._pre_trace *= np.exp(-dt / self.config.stdp_tau_plus)
        self._post_trace *= np.exp(-dt / self.config.stdp_tau_minus)

    def apply_stdp(
        self,
        pre_spikes: List[int],
        post_spikes: List[int],
        dt: float,
    ) -> np.ndarray:
        """Basic pairwise STDP: returns conductance delta matrix (post × pre)."""
        n_pre = len(pre_spikes) if pre_spikes else self.n
        n_post = len(post_spikes) if post_spikes else self.n
        delta = np.zeros((n_post, n_pre))

        for p in pre_spikes:
            if 0 <= p < self.n:
                self._pre_trace[p] += 1.0
        for q in post_spikes:
            if 0 <= q < self.n:
                self._post_trace[q] += 1.0

        cfg = self.config
        for q in post_spikes:
            for p in pre_spikes:
                if 0 <= q < n_post and 0 <= p < n_pre:
                    delta[q, p] += cfg.stdp_eta * cfg.stdp_a_plus * self._pre_trace[p]
        for p in pre_spikes:
            for q in post_spikes:
                if 0 <= q < n_post and 0 <= p < n_pre:
                    delta[q, p] -= cfg.stdp_eta * cfg.stdp_a_minus * self._post_trace[q]

        return delta

    def spikes_from_events(self, spikes: List[SpikeEvent]) -> Tuple[List[int], List[int]]:
        """Split spike list into pre/post indices for STDP (same pool, self-connection)."""
        indices = [s.neuron for s in spikes]
        return indices, indices
