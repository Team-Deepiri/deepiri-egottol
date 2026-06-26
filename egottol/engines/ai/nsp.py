"""Neural Signal Processor — implements NSP_AI component behavior."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Optional, Tuple

import numpy as np


@dataclass
class NSPConfig:
    moving_avg_window: int = 5
    spectral_gate_threshold: float = 0.15
    fft_bins: int = 32
    anomaly_z_threshold: float = 3.0


class NeuralSignalProcessor:
    """AI-driven signal cleanup, classification, and anomaly detection."""

    def __init__(self, config: Optional[NSPConfig] = None):
        self.config = config or NSPConfig()
        self._classifier_weights: Optional[np.ndarray] = None
        self._classifier_bias: Optional[np.ndarray] = None
        self._feature_mean: Optional[np.ndarray] = None
        self._feature_std: Optional[np.ndarray] = None

    def denoise(self, signal: np.ndarray, sample_rate: float = 1.0) -> np.ndarray:
        """Moving average followed by spectral gating."""
        x = np.asarray(signal, dtype=float).reshape(-1)
        if x.size == 0:
            return x

        window = max(int(self.config.moving_avg_window), 1)
        kernel = np.ones(window, dtype=float) / window
        smoothed = np.convolve(x, kernel, mode="same")

        n = smoothed.size
        spectrum = np.fft.rfft(smoothed)
        freqs = np.fft.rfftfreq(n, d=1.0 / max(sample_rate, 1e-9))
        magnitude = np.abs(spectrum)
        peak = np.max(magnitude) if magnitude.size else 1.0
        gate = magnitude >= self.config.spectral_gate_threshold * peak
        filtered = np.fft.irfft(spectrum * gate, n=n)
        return filtered

    def _fft_features(self, signal: np.ndarray) -> np.ndarray:
        x = np.asarray(signal, dtype=float).reshape(-1)
        spectrum = np.abs(np.fft.rfft(x))
        bins = self.config.fft_bins
        if spectrum.size >= bins:
            idx = np.linspace(0, spectrum.size - 1, bins, dtype=int)
            return spectrum[idx]
        out = np.zeros(bins, dtype=float)
        out[: spectrum.size] = spectrum
        return out

    def train_classifier(
        self,
        signals: np.ndarray,
        labels: np.ndarray,
    ) -> None:
        """Fit a linear classifier on FFT features (least squares)."""
        signals = np.atleast_2d(signals)
        labels = np.asarray(labels, dtype=int).reshape(-1)
        features = np.array([self._fft_features(s) for s in signals], dtype=float)
        self._feature_mean = features.mean(axis=0)
        self._feature_std = np.maximum(features.std(axis=0), 1e-9)
        normed = (features - self._feature_mean) / self._feature_std

        classes = np.unique(labels)
        n_classes = classes.size
        n_features = normed.shape[1]
        y_onehot = np.zeros((labels.size, n_classes), dtype=float)
        for i, c in enumerate(classes):
            y_onehot[labels == c, i] = 1.0

        aug = np.hstack([normed, np.ones((normed.shape[0], 1))])
        w_aug, _, _, _ = np.linalg.lstsq(aug, y_onehot, rcond=None)
        self._classifier_weights = w_aug[:n_features, :].T
        self._classifier_bias = w_aug[n_features, :]
        self._class_labels = classes

    def classify(self, signal: np.ndarray) -> Tuple[int, np.ndarray]:
        """Classify a signal using FFT features and the linear readout."""
        if self._classifier_weights is None:
            self.train_classifier(
                np.array([signal, signal * 0.5]),
                np.array([0, 1]),
            )

        feat = self._fft_features(signal)
        feat = (feat - self._feature_mean) / self._feature_std
        logits = self._classifier_weights @ feat + self._classifier_bias
        exp = np.exp(logits - np.max(logits))
        probs = exp / np.sum(exp)
        idx = int(np.argmax(probs))
        return int(self._class_labels[idx]), probs

    def anomaly_detect(
        self,
        signal: np.ndarray,
        reference: Optional[np.ndarray] = None,
    ) -> Tuple[bool, float]:
        """Z-score anomaly test against reference or running mean of signal."""
        x = np.asarray(signal, dtype=float).reshape(-1)
        ref = x if reference is None else np.asarray(reference, dtype=float).reshape(-1)
        mean = float(np.mean(ref))
        std = float(np.std(ref))
        if std < 1e-12:
            std = 1e-12
        z = float(np.max(np.abs(x - mean)) / std)
        return z > self.config.anomaly_z_threshold, z
