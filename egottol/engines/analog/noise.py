"""Analog noise models for circuit simulation traces."""

from __future__ import annotations

from typing import Optional, Union

import numpy as np

KB = 1.380649e-23
Q_E = 1.602176634e-19


def thermal_noise(
    resistance: float,
    temperature: float = 300.0,
    bandwidth: float = 1.0,
    n_samples: int = 1,
    rng: Optional[np.random.Generator] = None,
) -> np.ndarray:
    """
    Johnson-Nyquist thermal noise voltage RMS: V_n = sqrt(4 k T R BW).

    Returns zero-mean Gaussian samples with that standard deviation.
    """
    r = max(float(resistance), 1e-18)
    bw = max(float(bandwidth), 1e-18)
    v_rms = np.sqrt(4.0 * KB * temperature * r * bw)
    gen = rng or np.random.default_rng()
    return gen.normal(0.0, v_rms, n_samples)


def flicker_noise(
    n_samples: int,
    dt: float,
    corner_freq: float = 1.0,
    amplitude: float = 1e-6,
    rng: Optional[np.random.Generator] = None,
) -> np.ndarray:
    """
    1/f (flicker) noise via spectral shaping of white noise.

    PSD ~ 1/f below corner_freq; generates a time-domain trace of length n_samples.
    """
    gen = rng or np.random.default_rng()
    n = max(int(n_samples), 1)
    dt = max(float(dt), 1e-18)
    freqs = np.fft.rfftfreq(n, dt)
    white = gen.normal(0.0, 1.0, freqs.size) + 1j * gen.normal(0.0, 1.0, freqs.size)
    psd = np.ones_like(freqs, dtype=float)
    mask = freqs > 0
    psd[mask] = 1.0 / np.sqrt(freqs[mask] / max(corner_freq, 1e-18))
    psd[0] = 0.0
    shaped = white * psd
    trace = np.fft.irfft(shaped, n=n)
    trace = trace / max(np.std(trace), 1e-18) * amplitude
    return trace


def add_noise_to_trace(
    trace: np.ndarray,
    thermal_r: float = 0.0,
    temperature: float = 300.0,
    bandwidth: float = 1.0,
    flicker_amp: float = 0.0,
    dt: float = 1e-3,
    flicker_corner: float = 1.0,
    rng: Optional[np.random.Generator] = None,
) -> np.ndarray:
    """Add thermal and optional flicker noise to a voltage/current trace."""
    y = np.asarray(trace, dtype=float).copy()
    n = y.size
    gen = rng or np.random.default_rng()

    if thermal_r > 0.0:
        y += thermal_noise(thermal_r, temperature, bandwidth, n_samples=n, rng=gen)

    if flicker_amp > 0.0:
        y += flicker_noise(n, dt, corner_freq=flicker_corner, amplitude=flicker_amp, rng=gen)

    return y
