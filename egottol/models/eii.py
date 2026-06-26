from enum import Enum
from typing import Any, Dict, List, Optional

import numpy as np
from pydantic import BaseModel, Field


class DetectorMode(str, Enum):
    THRESHOLD = "threshold"
    DIFFERENTIATOR = "differentiator"
    COMPARATOR = "comparator"
    RF_ENVELOPE = "rf_envelope"
    MEMRISTOR_SWITCH = "memristor_switch"


class EncoderMode(str, Enum):
    RATE = "rate"
    LATENCY = "latency"
    FILTER = "filter"
    POPULATION = "population"
    CONTINUOUS = "continuous"


class InferenceBackend(str, Enum):
    ANALOG = "analog"
    DIGITAL = "digital"
    ENERGY_BASED = "energy_based"
    RESERVOIR = "reservoir"
    HOPFIELD = "hopfield"


class DigitalHead(str, Enum):
    LINEAR = "linear"
    SOFTMAX = "softmax"


class ActuatorMode(str, Enum):
    DAC = "dac"
    STDP = "stdp"
    DIGITAL = "digital"
    OPTICAL = "optical"


class ImpulseDetectorConfig(BaseModel):
    mode: DetectorMode = DetectorMode.THRESHOLD
    num_channels: int = 4
    threshold: float = 0.5
    refractory_tau: float = 2e-3
    diff_tau: float = 1e-3
    v_th_low: float = 0.8
    v_th_high: float = 2.0
    rf_threshold: float = 0.01
    envelope_tau: float = 1e-4
    i_set: float = 1e-6
    delta_g: float = 1e-6


class EncodingManifoldConfig(BaseModel):
    mode: EncoderMode = EncoderMode.FILTER
    embedding_dim: int = 8
    window_T: float = 10e-3
    latency_tau: float = 5e-3
    filter_tau: List[float] = Field(default_factory=lambda: [1e-3, 5e-3, 20e-3])
    population_pool: int = 2
    read_noise_std: float = 0.0
    probe_channels: List[int] = Field(default_factory=lambda: [0, 1, 2, 3])


class InferenceEngineConfig(BaseModel):
    backend: InferenceBackend = InferenceBackend.DIGITAL
    digital_head: DigitalHead = DigitalHead.LINEAR
    num_classes: int = 4
    temperature: float = 1.0
    adc_bits: int = 8
    v_ref: float = 1.0
    v_dd: float = 1.0
    weights: Optional[List[List[float]]] = None
    bias: Optional[List[float]] = None
    conductance_matrix: Optional[List[List[float]]] = None
    ebm_lambda: float = 0.01
    ebm_iterations: int = 50
    ebm_learning_rate: float = 0.1
    reservoir_size: int = 100
    reservoir_spectral_radius: float = 0.9
    reservoir_leak: float = 0.3
    reservoir_ridge_alpha: float = 1e-6
    reservoir_input_scaling: float = 1.0
    reservoir_sparsity: float = 0.1
    hopfield_max_iters: int = 20
    hopfield_beta: float = 1.0
    hopfield_patterns: Optional[List[List[float]]] = None


class FeedbackActuatorConfig(BaseModel):
    mode: ActuatorMode = ActuatorMode.DAC
    v_dd: float = 1.0
    stdp_eta: float = 0.01
    stdp_a_plus: float = 0.001
    stdp_a_minus: float = 0.0012
    stdp_tau_plus: float = 20e-3
    stdp_tau_minus: float = 20e-3
    optical_phase_scale: float = 0.1
    digital_threshold: float = 0.5
    source_frequency: float = 1e6
    source_amplitude_scale: float = 1.0


class EIIPipelineConfig(BaseModel):
    dt: float = 1e-4
    stimulus_time: float = 0.0
    detector: ImpulseDetectorConfig = Field(default_factory=ImpulseDetectorConfig)
    encoder: EncodingManifoldConfig = Field(default_factory=EncodingManifoldConfig)
    inference: InferenceEngineConfig = Field(default_factory=InferenceEngineConfig)
    actuator: FeedbackActuatorConfig = Field(default_factory=FeedbackActuatorConfig)

    def to_runtime_config(self) -> Dict[str, Any]:
        return {
            "dt": self.dt,
            "window_T": self.encoder.window_T,
            "embedding_dim": self.encoder.embedding_dim,
            "num_channels": self.detector.num_channels,
            "detector_mode": self.detector.mode.value,
            "encoder_mode": self.encoder.mode.value,
            "inference_backend": self.inference.backend.value,
            "digital_head": self.inference.digital_head.value,
            "actuator_mode": self.actuator.mode.value,
            "stimulus_time": self.stimulus_time,
            "read_noise_std": self.encoder.read_noise_std,
        }

    def default_weights(self) -> np.ndarray:
        if self.inference.weights is not None:
            return np.asarray(self.inference.weights, dtype=float)
        dim = self.encoder.embedding_dim
        classes = self.inference.num_classes
        rng = np.random.default_rng(0)
        return rng.normal(0.0, 0.1, size=(classes, dim))

    def default_bias(self) -> np.ndarray:
        if self.inference.bias is not None:
            return np.asarray(self.inference.bias, dtype=float)
        return np.zeros(self.inference.num_classes, dtype=float)

    def default_conductance(self) -> np.ndarray:
        if self.inference.conductance_matrix is not None:
            return np.asarray(self.inference.conductance_matrix, dtype=float)
        rows = self.inference.num_classes
        cols = self.encoder.embedding_dim
        rng = np.random.default_rng(1)
        return np.abs(rng.normal(1e-3, 2e-4, size=(rows, cols)))
