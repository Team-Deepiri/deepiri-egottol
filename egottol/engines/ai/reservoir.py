"""Echo-state reservoir with ridge-regression readout."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Tuple, Union

import numpy as np


@dataclass
class ReservoirConfig:
    n_reservoir: int = 100
    spectral_radius: float = 0.9
    leak_rate: float = 0.3
    input_scaling: float = 1.0
    sparsity: float = 0.1
    ridge_alpha: float = 1e-6
    seed: int = 42


class EchoStateReservoir:
    """Random sparse reservoir; readout trained via ridge regression."""

    def __init__(self, config: Optional[ReservoirConfig] = None):
        self.config = config or ReservoirConfig()
        self._rng = np.random.default_rng(self.config.seed)
        self._init_reservoir()
        self._readout: Optional[np.ndarray] = None
        self._state = np.zeros(self.config.n_reservoir, dtype=float)

    def _init_reservoir(self) -> None:
        n = self.config.n_reservoir
        mask = self._rng.random((n, n)) < self.config.sparsity
        w = self._rng.standard_normal((n, n)) * mask
        radius = max(np.max(np.abs(np.linalg.eigvals(w))), 1e-9)
        self.w_res = w * (self.config.spectral_radius / radius)
        self.w_in: Optional[np.ndarray] = None

    def reset(self) -> None:
        self._state.fill(0.0)

    def _ensure_input_weights(self, input_dim: int) -> None:
        if self.w_in is None or self.w_in.shape[1] != input_dim:
            self.w_in = (
                self._rng.standard_normal((self.config.n_reservoir, input_dim))
                * self.config.input_scaling
            )

    def _collect_states(self, inputs: np.ndarray) -> np.ndarray:
        """Collect reservoir states for one trace [T, D] or batch [N, T, D]."""
        inputs = np.asarray(inputs, dtype=float)
        if inputs.ndim == 3:
            finals = []
            for trace in inputs:
                finals.append(self._collect_states(trace)[-1])
            return np.array(finals)
        if inputs.ndim == 1:
            inputs = inputs.reshape(1, -1)
        n_steps, input_dim = inputs.shape
        self._ensure_input_weights(input_dim)
        states = np.zeros((n_steps, self.config.n_reservoir), dtype=float)
        h = self._state.copy()
        leak = self.config.leak_rate
        for t in range(n_steps):
            pre = self.w_in @ inputs[t] + self.w_res @ h
            h = (1.0 - leak) * h + leak * np.tanh(pre)
            states[t] = h
        self._state = h
        return states

    def fit(self, traces: np.ndarray, targets: np.ndarray) -> None:
        """Train readout weights with ridge regression on state trajectories."""
        traces = np.asarray(traces, dtype=float)
        targets = np.asarray(targets, dtype=float)
        if targets.ndim == 1:
            classes = np.unique(targets.astype(int))
            y = np.zeros((targets.size, classes.size), dtype=float)
            for i, c in enumerate(classes):
                y[targets.astype(int) == c, i] = 1.0
            targets = y

        if traces.ndim == 3:
            states = self._collect_states(traces)
        else:
            states = self._collect_states(traces)[-1:]

        xtx = states.T @ states + self.config.ridge_alpha * np.eye(states.shape[1])
        xty = states.T @ targets
        self._readout = np.linalg.solve(xtx, xty)

    def predict(self, trace: np.ndarray) -> np.ndarray:
        """Predict class probabilities for a single trace."""
        if self._readout is None:
            raise RuntimeError("Reservoir readout not trained; call fit() first.")
        states = self._collect_states(np.asarray(trace, dtype=float))
        if states.ndim == 2:
            feature = states[-1]
        else:
            feature = states
        logits = feature @ self._readout
        exp = np.exp(logits - np.max(logits))
        return exp / np.sum(exp)

    def infer(self, trace: Union[np.ndarray, list]) -> Tuple[np.ndarray, float]:
        """Infer on spike or voltage trace; returns probabilities and confidence."""
        probs = self.predict(np.asarray(trace, dtype=float))
        return probs, float(np.max(probs))

    def infer_embedding(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        """Single-step inference when input is an embedding vector."""
        z = np.asarray(z, dtype=float).reshape(1, -1)
        return self.infer(z)
