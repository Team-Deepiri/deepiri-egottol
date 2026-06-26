"""Hopfield network recall as an EII inference backend helper."""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional, Tuple

import numpy as np


@dataclass
class HopfieldConfig:
    max_iters: int = 20
    beta: float = 1.0


class HopfieldNetwork:
    """Binary Hopfield network with asynchronous updates and energy tracking."""

    def __init__(self, n_units: int, config: Optional[HopfieldConfig] = None):
        self.n = n_units
        self.config = config or HopfieldConfig()
        self.w = np.zeros((n_units, n_units), dtype=float)

    def store(self, patterns: np.ndarray) -> None:
        """Hebbian outer-product storage; zero diagonal."""
        patterns = np.atleast_2d(patterns)
        p = np.sign(np.clip(patterns, -1.0, 1.0))
        self.w = p.T @ p / max(p.shape[0], 1)
        np.fill_diagonal(self.w, 0.0)

    def energy(self, state: np.ndarray) -> float:
        s = np.sign(np.clip(state, -1.0, 1.0))
        return float(-0.5 * s @ self.w @ s)

    def recall(self, probe: np.ndarray, async_order: Optional[List[int]] = None) -> np.ndarray:
        """Asynchronous recall until convergence or max_iters."""
        s = np.sign(np.clip(np.asarray(probe, dtype=float).reshape(-1), -1e-9, 1e-9))
        s[s == 0] = 1.0
        order = async_order or list(range(self.n))
        for _ in range(self.config.max_iters):
            changed = False
            for i in order:
                h = self.w[i] @ s
                new = 1.0 if h >= 0 else -1.0
                if new != s[i]:
                    s[i] = new
                    changed = True
            if not changed:
                break
        return s


class HopfieldInference:
    """Maps Hopfield recall to class probabilities for InferenceEngine."""

    def __init__(
        self,
        patterns: np.ndarray,
        config: Optional[HopfieldConfig] = None,
    ):
        patterns = np.atleast_2d(patterns)
        self.patterns = patterns
        self.config = config or HopfieldConfig()
        self.network = HopfieldNetwork(patterns.shape[1], self.config)
        self.network.store(patterns)

    def infer(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        """Recall nearest stored pattern; return softmax over Hamming similarity."""
        z = np.asarray(z, dtype=float).reshape(-1)
        recalled = self.network.recall(z)
        recalled_bin = np.sign(np.clip(recalled, -1e-9, 1e-9))
        recalled_bin[recalled_bin == 0] = 1.0

        sims = np.array(
            [np.sum(recalled_bin == np.sign(p)) for p in self.patterns],
            dtype=float,
        )
        logits = self.config.beta * sims
        exp = np.exp(logits - np.max(logits))
        probs = exp / np.sum(exp)
        return probs, float(np.max(probs))
