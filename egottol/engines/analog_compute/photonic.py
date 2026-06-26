"""Photonic MZI mesh engine stub: complex unitary transforms on optical amplitudes."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional

import numpy as np


def mzi_2x2_unitary(theta: float, phi: float) -> np.ndarray:
    """2×2 MZI unitary with phase shifters θ (internal) and φ (external)."""
    c, s = np.cos(theta / 2), np.sin(theta / 2)
    phase = np.exp(1j * phi)
    return np.array(
        [
            [c, 1j * s * phase],
            [1j * s * np.conj(phase), c],
        ],
        dtype=complex,
    )


@dataclass
class PhotonicConfig:
    n_modes: int = 4
    responsivity: float = 0.5


class PhotonicEngine:
    """Clements-style MZI mesh: U @ x on complex optical amplitudes."""

    def __init__(
        self,
        n_modes: int = 4,
        phases: Optional[np.ndarray] = None,
        config: Optional[PhotonicConfig] = None,
    ):
        self.config = config or PhotonicConfig(n_modes=n_modes)
        self.n = self.config.n_modes
        if phases is not None:
            self.phases = np.asarray(phases, dtype=float).reshape(-1)
        else:
            self.phases = np.zeros(self.n * (self.n - 1) // 2)
        self._amplitudes = np.zeros(self.n, dtype=complex)

    def build_mesh_unitary(self) -> np.ndarray:
        """Assemble n×n unitary from pairwise MZI phases (Clements decomposition stub)."""
        u = np.eye(self.n, dtype=complex)
        idx = 0
        for layer in range(self.n - 1):
            for i in range(0, self.n - 1 - layer, 2):
                if idx >= len(self.phases):
                    break
                theta = self.phases[idx]
                phi = self.phases[idx + 1] if idx + 1 < len(self.phases) else 0.0
                idx += 2
                mzi = mzi_2x2_unitary(theta, phi)
                embed = np.eye(self.n, dtype=complex)
                embed[i : i + 2, i : i + 2] = mzi
                u = embed @ u
        return u

    def mesh_multiply(self, x: np.ndarray, unitary: Optional[np.ndarray] = None) -> np.ndarray:
        """Apply mesh unitary U to input amplitude vector x."""
        xin = np.asarray(x, dtype=complex).reshape(-1)
        if xin.size < self.n:
            xin = np.pad(xin, (0, self.n - xin.size))
        xin = xin[: self.n]
        u = unitary if unitary is not None else self.build_mesh_unitary()
        return u @ xin

    def step(self, dt: float, optical_in: Optional[np.ndarray] = None) -> np.ndarray:
        """Advance one photonic timestep; propagate optical_in through mesh."""
        if optical_in is not None:
            xin = np.asarray(optical_in, dtype=complex).reshape(-1)
        else:
            xin = self._amplitudes
        self._amplitudes = self.mesh_multiply(xin)
        return self._amplitudes.copy()

    def to_electrical(self, amplitudes: Optional[np.ndarray] = None) -> np.ndarray:
        """Square-law photodetection: I ∝ |E|²."""
        a = self._amplitudes if amplitudes is None else np.asarray(amplitudes, dtype=complex)
        return self.config.responsivity * np.abs(a) ** 2
