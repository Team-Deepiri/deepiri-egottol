from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

import numpy as np


@dataclass
class ImpulseEvent:
    """Detected information event extracted from continuous waveforms."""

    t: float
    channel: int
    event_type: str
    amplitude: float
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class EIIConfig:
    """Runtime configuration for the EII pipeline."""

    dt: float = 1e-4
    window_T: float = 10e-3
    embedding_dim: int = 8
    num_channels: int = 4
    detector_mode: str = "threshold"
    encoder_mode: str = "filter"
    inference_backend: str = "digital"
    digital_head: str = "linear"
    actuator_mode: str = "dac"
    stimulus_time: float = 0.0
    read_noise_std: float = 0.0


@dataclass
class EIIState:
    """Coupled system state at simulation time t."""

    t: float = 0.0
    voltages: np.ndarray = field(default_factory=lambda: np.zeros(4))
    currents: np.ndarray = field(default_factory=lambda: np.zeros(4))
    conductances: Optional[np.ndarray] = None
    optical_phases: Optional[np.ndarray] = None
    logic_registers: Dict[str, float] = field(default_factory=dict)
    refractory: np.ndarray = field(default_factory=lambda: np.zeros(4))
    prev_voltages: np.ndarray = field(default_factory=lambda: np.zeros(4))
    prev_envelope: np.ndarray = field(default_factory=lambda: np.zeros(4))
    prev_diff_signal: np.ndarray = field(default_factory=lambda: np.zeros(4))
    prev_comparator: np.ndarray = field(default_factory=lambda: np.zeros(4))
    prev_memristor_current: np.ndarray = field(default_factory=lambda: np.zeros(4))
    event_history: List[ImpulseEvent] = field(default_factory=list)
    filter_state: np.ndarray = field(default_factory=lambda: np.zeros(4))
    embedding: Optional[np.ndarray] = None
    prediction: Optional[np.ndarray] = None
    confidence: float = 0.0
    control: np.ndarray = field(default_factory=lambda: np.zeros(4))
    window_events: List[ImpulseEvent] = field(default_factory=list)
