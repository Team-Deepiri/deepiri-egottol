"""Auto-tune circuit parameters to match a target waveform."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Callable, Dict, Optional, Sequence, Union

import numpy as np
from scipy.optimize import least_squares


class AutoTuner:
    """Fit circuit parameters to a target waveform using least squares."""

    def __init__(
        self,
        simulate_fn: Callable[[np.ndarray], np.ndarray],
        param_names: Sequence[str],
        initial_params: Dict[str, float],
        bounds: Optional[tuple] = None,
    ):
        self.simulate_fn = simulate_fn
        self.param_names = list(param_names)
        self.initial = np.array([initial_params[n] for n in self.param_names], dtype=float)
        self.bounds = bounds

    @staticmethod
    def load_target(path_or_array: Union[str, Path, np.ndarray]) -> np.ndarray:
        """Load target waveform from .egt-cal JSON or a raw array."""
        if isinstance(path_or_array, (str, Path)):
            path = Path(path_or_array)
            with path.open(encoding="utf-8") as f:
                payload = json.load(f)
            if "waveform" in payload:
                return np.asarray(payload["waveform"], dtype=float).reshape(-1)
            if "samples" in payload:
                return np.asarray(payload["samples"], dtype=float).reshape(-1)
            raise ValueError(f"No waveform field in {path}")
        return np.asarray(path_or_array, dtype=float).reshape(-1)

    def _vec_to_params(self, vec: np.ndarray) -> Dict[str, float]:
        return {name: float(vec[i]) for i, name in enumerate(self.param_names)}

    def tune(
        self,
        target: Union[str, Path, np.ndarray],
        max_nfev: int = 200,
    ) -> Dict[str, float]:
        """Optimize parameters so simulate_fn(params) matches target."""
        target_wave = self.load_target(target)
        p0 = self.initial.copy()

        def residual(vec: np.ndarray) -> np.ndarray:
            params = self._vec_to_params(vec)
            simulated = np.asarray(self.simulate_fn(params), dtype=float).reshape(-1)
            n = min(simulated.size, target_wave.size)
            if n == 0:
                return np.array([1.0])
            return simulated[:n] - target_wave[:n]

        result = least_squares(
            residual,
            p0,
            bounds=self.bounds,
            max_nfev=max_nfev,
        )
        return self._vec_to_params(result.x)
