"""Memristor crossbar engine: conductance-matrix analog multiply with MNA stamping."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

import numpy as np


@dataclass
class CrossbarConfig:
    rows: int = 4
    cols: int = 4
    ir_drop: float = 0.01
    read_noise_std: float = 0.0
    g_min: float = 1e-6
    g_max: float = 1e-3


class CrossbarEngine:
    """Programmable conductance crossbar: I = G @ V with non-idealities."""

    def __init__(
        self,
        rows: int = 4,
        cols: int = 4,
        conductance: Optional[np.ndarray] = None,
        config: Optional[CrossbarConfig] = None,
        rng: Optional[np.random.Generator] = None,
    ):
        self.config = config or CrossbarConfig(rows=rows, cols=cols)
        self.rows = self.config.rows
        self.cols = self.config.cols
        self.rng = rng or np.random.default_rng(0)
        if conductance is not None:
            self.G = np.asarray(conductance, dtype=float).reshape(self.rows, self.cols)
        else:
            self.G = self.rng.uniform(self.config.g_min, self.config.g_max, (self.rows, self.cols))
        self._stdp_delta: Optional[np.ndarray] = None

    def solve(self, voltages: np.ndarray) -> np.ndarray:
        """Compute column currents I = G @ V with IR-drop correction and read noise."""
        v = np.asarray(voltages, dtype=float).reshape(-1)
        if v.size < self.rows:
            v = np.pad(v, (0, self.rows - v.size))
        v = v[: self.rows]

        currents = self.G @ v
        if self.config.ir_drop > 0:
            row_currents = self.G.sum(axis=1) * v
            v_eff = v - self.config.ir_drop * row_currents
            currents = self.G @ v_eff

        if self.config.read_noise_std > 0:
            currents = currents + self.rng.normal(0.0, self.config.read_noise_std, self.cols)

        return currents

    def apply_stdp_delta(self, delta: np.ndarray) -> None:
        """Apply accumulated STDP weight updates to conductance matrix."""
        d = np.asarray(delta, dtype=float)
        if d.shape != self.G.shape:
            return
        self.G = np.clip(self.G + d, self.config.g_min, self.config.g_max)

    def consume_stdp_delta(self) -> None:
        if self._stdp_delta is not None:
            self.apply_stdp_delta(self._stdp_delta)
            self._stdp_delta = None

    def queue_stdp_delta(self, delta: np.ndarray) -> None:
        d = np.asarray(delta, dtype=float)
        if self._stdp_delta is None:
            self._stdp_delta = np.zeros_like(self.G)
        if d.shape == self.G.shape:
            self._stdp_delta += d

    def stamp_mna(
        self,
        n: int,
        row_nodes: List[int],
        col_nodes: List[int],
    ) -> Tuple[np.ndarray, np.ndarray]:
        """Stamp crossbar conductances into an n×n MNA conductance matrix and RHS."""
        g_stamp = np.zeros((n, n))
        i_rhs = np.zeros(n)

        for i in range(min(self.rows, len(row_nodes))):
            ni = row_nodes[i]
            if ni < 0 or ni >= n:
                continue
            for j in range(min(self.cols, len(col_nodes))):
                nj = col_nodes[j]
                if nj < 0 or nj >= n:
                    continue
                g = float(self.G[i, j])
                g_stamp[ni, ni] += g
                g_stamp[nj, nj] += g
                g_stamp[ni, nj] -= g
                g_stamp[nj, ni] -= g

        return g_stamp, i_rhs

    def stamp_for_component(
        self,
        node_map: Dict[str, int],
        comp_id: str,
        n: int,
    ) -> Tuple[np.ndarray, np.ndarray]:
        """Build row/col node lists from schematic port naming convention."""
        row_nodes = [node_map.get(f"{comp_id}:R{i}", -1) for i in range(self.rows)]
        col_nodes = [node_map.get(f"{comp_id}:C{j}", -1) for j in range(self.cols)]
        return self.stamp_mna(n, row_nodes, col_nodes)
