import logging
from typing import Tuple

import numpy as np

from egottol.models.eii import DigitalHead, InferenceBackend, InferenceEngineConfig

logger = logging.getLogger(__name__)


def _softmax(logits: np.ndarray, temperature: float = 1.0) -> np.ndarray:
    t = max(temperature, 1e-9)
    shifted = logits / t - np.max(logits / t)
    exp = np.exp(shifted)
    return exp / np.sum(exp)


class InferenceEngine:
    """Inference operator Psi: embedding z -> prediction y_hat and confidence p."""

    def __init__(self, config: InferenceEngineConfig, weights: np.ndarray, bias: np.ndarray, conductance: np.ndarray):
        self.config = config
        self.weights = weights
        self.bias = bias
        self.conductance = conductance
        self._reservoir = None
        self._hopfield = None
        if self.config.backend == InferenceBackend.RESERVOIR:
            self._init_reservoir()
        elif self.config.backend == InferenceBackend.HOPFIELD:
            self._init_hopfield()

    def _init_reservoir(self) -> None:
        from egottol.engines.ai.reservoir import EchoStateReservoir, ReservoirConfig

        cfg = ReservoirConfig(
            n_reservoir=self.config.reservoir_size,
            spectral_radius=self.config.reservoir_spectral_radius,
            leak_rate=self.config.reservoir_leak,
            ridge_alpha=self.config.reservoir_ridge_alpha,
            input_scaling=self.config.reservoir_input_scaling,
            sparsity=self.config.reservoir_sparsity,
        )
        self._reservoir = EchoStateReservoir(cfg)
        if self.weights.size > 0 and self.weights.ndim == 2:
            dim = self.weights.shape[1]
            n = cfg.n_reservoir
            self._reservoir._ensure_input_weights(dim)
            win = np.zeros((n, dim))
            rows = min(n, dim)
            win[:rows, :rows] = np.eye(rows) * cfg.input_scaling
            self._reservoir.w_in = win
            readout = np.zeros((n, self.weights.shape[0]))
            readout[:rows, :] = self.weights[:, :rows].T
            self._reservoir._readout = readout

    def _init_hopfield(self) -> None:
        from egottol.engines.ai.hopfield_infer import HopfieldConfig, HopfieldInference

        patterns = self.config.hopfield_patterns
        if patterns is None and self.weights.size > 0:
            patterns = self.weights.tolist()
        if not patterns:
            dim = max(self.weights.shape[1] if self.weights.ndim == 2 else 4, 2)
            patterns = [np.ones(dim).tolist(), (-np.ones(dim)).tolist()]
        cfg = HopfieldConfig(
            max_iters=self.config.hopfield_max_iters,
            beta=self.config.hopfield_beta,
        )
        self._hopfield = HopfieldInference(np.asarray(patterns, dtype=float), cfg)

    def infer(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        z = np.asarray(z, dtype=float).reshape(-1)
        if self.config.backend == InferenceBackend.ANALOG:
            return self._infer_analog(z)
        if self.config.backend == InferenceBackend.ENERGY_BASED:
            return self._infer_energy_based(z)
        if self.config.backend == InferenceBackend.RESERVOIR:
            return self._infer_reservoir(z)
        if self.config.backend == InferenceBackend.HOPFIELD:
            return self._infer_hopfield(z)
        return self._infer_digital(z)

    def _infer_reservoir(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        if self._reservoir is None:
            self._init_reservoir()
        return self._reservoir.infer_embedding(z)

    def _infer_hopfield(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        if self._hopfield is None:
            self._init_hopfield()
        return self._hopfield.infer(z)

    def _infer_analog(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        g = self._match_conductance(z)
        v_row = self._dac_encode(z)
        i_col = g @ v_row
        y_hat = self._quantize_adc(i_col)
        if self.config.digital_head == DigitalHead.SOFTMAX:
            probs = _softmax(y_hat, self.config.temperature)
        else:
            probs = y_hat / max(np.sum(np.abs(y_hat)), 1e-9)
            probs = np.clip(probs, 0.0, None)
            probs = probs / max(np.sum(probs), 1e-9)
        confidence = float(np.max(probs))
        return probs, confidence

    def _infer_digital(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        w = self._match_weights(z)
        logits = w @ z + self.bias[: w.shape[0]]
        if self.config.digital_head == DigitalHead.SOFTMAX:
            probs = _softmax(logits, self.config.temperature)
        else:
            probs = _softmax(logits, self.config.temperature)
        confidence = float(np.max(probs))
        return probs, confidence

    def _infer_energy_based(self, z: np.ndarray) -> Tuple[np.ndarray, float]:
        w = self._match_weights(z)
        num_classes = w.shape[0]
        y = np.full(num_classes, 1.0 / num_classes, dtype=float)
        lr = self.config.ebm_learning_rate
        lam = self.config.ebm_lambda

        for _ in range(self.config.ebm_iterations):
            energy_grad = -w @ z + y
            y = y - lr * energy_grad
            y = np.clip(y, 0.0, None)
            y = y + lam * (1.0 - np.sum(y))
            total = np.sum(y)
            if total > 0:
                y = y / total

        confidence = float(np.max(y))
        return y, confidence

    def _dac_encode(self, z: np.ndarray) -> np.ndarray:
        z_norm = z / max(np.max(np.abs(z)), 1e-9)
        return self.config.v_dd * np.clip(z_norm, 0.0, 1.0)

    def _quantize_adc(self, currents: np.ndarray) -> np.ndarray:
        bits = self.config.adc_bits
        levels = 2**bits - 1
        scaled = currents / max(self.config.v_ref, 1e-9)
        quantized = np.round(np.clip(scaled, 0.0, 1.0) * levels) / levels
        return quantized

    def _match_weights(self, z: np.ndarray) -> np.ndarray:
        if self.weights.shape[1] == z.size:
            return self.weights
        w = np.zeros((self.weights.shape[0], z.size), dtype=float)
        cols = min(self.weights.shape[1], z.size)
        w[:, :cols] = self.weights[:, :cols]
        return w

    def _match_conductance(self, z: np.ndarray) -> np.ndarray:
        if self.conductance.shape[1] == z.size:
            return self.conductance
        g = np.zeros((self.conductance.shape[0], z.size), dtype=float)
        cols = min(self.conductance.shape[1], z.size)
        g[:, :cols] = self.conductance[:, :cols]
        return g
