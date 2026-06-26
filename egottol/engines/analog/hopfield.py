"""Hopfield associative memory network."""

from __future__ import annotations

from typing import List, Optional

import numpy as np


class HopfieldNetwork:
    """
    Binary Hopfield network with asynchronous update and energy function.

    Energy: E(s) = -0.5 * s^T W s - b^T s  for s in {-1, +1}^n.
    """

    def __init__(
        self,
        n_neurons: int,
        weights: Optional[np.ndarray] = None,
        bias: Optional[np.ndarray] = None,
        rng: Optional[np.random.Generator] = None,
    ):
        self.n = n_neurons
        self.rng = rng or np.random.default_rng(0)
        if weights is not None:
            self.W = np.asarray(weights, dtype=float).reshape(n_neurons, n_neurons)
        else:
            self.W = np.zeros((n_neurons, n_neurons))
        if bias is not None:
            self.bias = np.asarray(bias, dtype=float).reshape(n_neurons)
        else:
            self.bias = np.zeros(n_neurons)
        self.state = np.ones(n_neurons)

    def energy(self, state: Optional[np.ndarray] = None) -> float:
        s = self._as_bipolar(state if state is not None else self.state)
        return float(-0.5 * s @ self.W @ s - self.bias @ s)

    @staticmethod
    def _as_bipolar(state: np.ndarray) -> np.ndarray:
        s = np.asarray(state, dtype=float).reshape(-1)
        return np.where(s >= 0.0, 1.0, -1.0)

    @staticmethod
    def _as_binary(state: np.ndarray) -> np.ndarray:
        s = np.asarray(state, dtype=float).reshape(-1)
        return np.where(s >= 0.0, 1.0, 0.0)

    def store_pattern(self, pattern: np.ndarray) -> None:
        """Hebbian outer-product storage: W += (2p - 1)(2p - 1)^T, zero diagonal."""
        p = self._as_binary(pattern)
        bp = 2.0 * p - 1.0
        self.W += np.outer(bp, bp)
        np.fill_diagonal(self.W, 0.0)

    def store_patterns(self, patterns: List[np.ndarray]) -> None:
        for pat in patterns:
            self.store_pattern(pat)

    def update_async(self, max_steps: int = 100) -> np.ndarray:
        """Asynchronous random-order updates until convergence or max_steps."""
        for _ in range(max_steps):
            order = self.rng.permutation(self.n)
            changed = False
            for i in order:
                s = self._as_bipolar(self.state)
                h = self.W[i] @ s + self.bias[i]
                new_val = 1.0 if h >= 0.0 else -1.0
                if new_val != s[i]:
                    self.state[i] = new_val
                    changed = True
            if not changed:
                break
        return self.state.copy()

    def recall(
        self,
        cue: np.ndarray,
        max_steps: int = 100,
        noise: float = 0.0,
    ) -> np.ndarray:
        """Recall stored pattern from noisy or partial cue."""
        c = np.asarray(cue, dtype=float).reshape(self.n)
        if noise > 0:
            flip = self.rng.random(self.n) < noise
            c = np.where(flip, -c, c)
        self.state = self._as_bipolar(c)
        return self.update_async(max_steps=max_steps)
