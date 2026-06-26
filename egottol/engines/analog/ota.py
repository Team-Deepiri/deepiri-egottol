"""Operational transconductance amplifier cell."""

from __future__ import annotations

import numpy as np


class OTACell:
    """
    Ideal OTA: I_out = gm * (V+ - V-).

    Small-signal transconductance amplifier used in analog neuron summation.
    """

    def __init__(self, gm: float = 1e-3, v_limit: float = 1.0):
        self.gm = float(gm)
        self.v_limit = float(v_limit)

    def output_current(self, v_plus: float, v_minus: float) -> float:
        """Compute output current with soft clipping at v_limit."""
        vd = float(v_plus) - float(v_minus)
        vd = np.clip(vd, -self.v_limit, self.v_limit)
        return self.gm * vd

    def stamp_mna(self, n: int, n_plus: int, n_minus: int, n_out: int) -> tuple:
        """
        Stamp OTA as Norton equivalent: current source gm*(V+ - V-) into output node.

        Returns (G, I) for n x n node subset; output node receives transconductance.
        """
        g = np.zeros((n, n))
        i_vec = np.zeros(n)
        if n_out < 0:
            return g, i_vec

        if n_plus >= 0:
            g[n_out, n_plus] += self.gm
        if n_minus >= 0:
            g[n_out, n_minus] -= self.gm

        return g, i_vec
