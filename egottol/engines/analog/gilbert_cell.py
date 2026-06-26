"""Gilbert cell four-quadrant analog multiplier."""

from __future__ import annotations

import numpy as np


class GilbertCell:
    """
    Four-quadrant analog multiplier based on the Gilbert cell topology.

    I_out = K * Vx * Vy  (idealized transfer characteristic).
    """

    def __init__(self, k: float = 1e-3, v_limit: float = 0.5):
        self.k = float(k)
        self.v_limit = float(v_limit)

    def multiply(self, vx: float, vy: float) -> float:
        """Compute four-quadrant product with input range limiting."""
        x = np.clip(float(vx), -self.v_limit, self.v_limit)
        y = np.clip(float(vy), -self.v_limit, self.v_limit)
        return self.k * x * y

    def multiply_array(self, vx: np.ndarray, vy: np.ndarray) -> np.ndarray:
        """Element-wise four-quadrant multiplication."""
        x = np.clip(np.asarray(vx, dtype=float), -self.v_limit, self.v_limit)
        y = np.clip(np.asarray(vy, dtype=float), -self.v_limit, self.v_limit)
        return self.k * x * y

    def small_signal_gain(self, vx: float, vy: float) -> tuple:
        """Return partial derivatives dI/dVx and dI/dVy at operating point."""
        x = np.clip(float(vx), -self.v_limit, self.v_limit)
        y = np.clip(float(vy), -self.v_limit, self.v_limit)
        return self.k * y, self.k * x
