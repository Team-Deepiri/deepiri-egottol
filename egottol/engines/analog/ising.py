"""Ising machine for combinatorial optimization via simulated annealing."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Tuple

import numpy as np


@dataclass
class IsingResult:
    spins: np.ndarray
    energy: float
    n_accepted: int


class IsingMachine:
    """
    Simulated annealing Ising solver for QUBO-style problems.

    Hamiltonian H(s) = -sum_{i,j} J_ij s_i s_j - sum_i h_i s_i,  s_i in {-1, +1}.
    QUBO x in {0,1}: map x -> s = 2x - 1.
    """

    def __init__(
        self,
        n_spins: int,
        coupling: Optional[np.ndarray] = None,
        field: Optional[np.ndarray] = None,
        rng: Optional[np.random.Generator] = None,
    ):
        self.n = n_spins
        self.rng = rng or np.random.default_rng(0)
        if coupling is not None:
            self.J = np.asarray(coupling, dtype=float).reshape(n_spins, n_spins)
        else:
            self.J = np.zeros((n_spins, n_spins))
        if field is not None:
            self.h = np.asarray(field, dtype=float).reshape(n_spins)
        else:
            self.h = np.zeros(n_spins)

    @classmethod
    def from_qubo(cls, Q: np.ndarray, rng: Optional[np.random.Generator] = None) -> "IsingMachine":
        """
        Build Ising coupling from QUBO min_x x^T Q x with x in {0,1}.

        Using s = 2x - 1 substitution.
        """
        q = np.asarray(Q, dtype=float)
        n = q.shape[0]
        j = q / 4.0
        h = np.sum(q, axis=1) / 2.0 - np.sum(q, axis=0) / 2.0
        np.fill_diagonal(j, 0.0)
        return cls(n, coupling=j, field=h, rng=rng)

    def energy(self, spins: np.ndarray) -> float:
        s = np.asarray(spins, dtype=float).reshape(self.n)
        s = np.where(s >= 0.0, 1.0, -1.0)
        pair = -np.sum(self.J * np.outer(s, s))
        field = -self.h @ s
        return float(pair + field)

    def _delta_energy(self, spins: np.ndarray, idx: int) -> float:
        s = np.where(spins >= 0.0, 1.0, -1.0)
        local = self.h[idx] + np.sum(self.J[idx, :] * s) + np.sum(self.J[:, idx] * s)
        return float(2.0 * s[idx] * local)

    def solve(
        self,
        n_steps: int = 10000,
        t_start: float = 10.0,
        t_end: float = 0.01,
        initial_spins: Optional[np.ndarray] = None,
    ) -> IsingResult:
        """Simulated annealing over spin flips."""
        if initial_spins is not None:
            spins = np.where(np.asarray(initial_spins).reshape(self.n) >= 0.0, 1.0, -1.0)
        else:
            spins = self.rng.choice([-1.0, 1.0], size=self.n)

        accepted = 0
        for step in range(n_steps):
            frac = step / max(n_steps - 1, 1)
            t = t_start * (t_end / t_start) ** frac
            idx = int(self.rng.integers(0, self.n))
            de = self._delta_energy(spins, idx)
            if de <= 0.0 or self.rng.random() < np.exp(-de / max(t, 1e-18)):
                spins[idx] *= -1.0
                accepted += 1

        return IsingResult(spins=spins, energy=self.energy(spins), n_accepted=accepted)

    def qubo_value(self, spins: np.ndarray) -> float:
        """Evaluate original QUBO objective for binary x = (s + 1) / 2."""
        s = np.where(np.asarray(spins).reshape(self.n) >= 0.0, 1.0, -1.0)
        x = (s + 1.0) / 2.0
        q = self.J * 4.0
        h_eff = self.h / 2.0
        return float(x @ q @ x + 2.0 * h_eff @ x)
