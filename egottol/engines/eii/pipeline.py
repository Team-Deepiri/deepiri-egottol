import logging
from typing import Any, Dict, List, Optional

import numpy as np

from egottol.engines.eii.actuators import FeedbackActuator
from egottol.engines.eii.detectors import ImpulseDetector
from egottol.engines.eii.encoders import EncodingManifold
from egottol.engines.eii.inference import InferenceEngine
from egottol.engines.eii.types import EIIConfig, EIIState
from egottol.models.eii import EIIPipelineConfig

logger = logging.getLogger(__name__)


class EIIPipeline:
    """Runs the full Phi -> Psi -> Gamma loop per timestep."""

    def __init__(self, config: Optional[EIIPipelineConfig] = None):
        self.pipeline_config = config or EIIPipelineConfig()
        self.config = EIIConfig(**self.pipeline_config.to_runtime_config())
        # Detector / encoder take the runtime EIIConfig, not the Pydantic sub-models.
        self.detector = ImpulseDetector(self.config)
        self.encoder = EncodingManifold(self.config)
        if hasattr(self.pipeline_config.detector, "threshold"):
            self.detector.threshold = float(self.pipeline_config.detector.threshold)
        self.inference = InferenceEngine(
            self.pipeline_config.inference,
            self.pipeline_config.default_weights(),
            self.pipeline_config.default_bias(),
            self.pipeline_config.default_conductance(),
        )
        self.actuator = FeedbackActuator(self.pipeline_config.actuator)
        self.state = self._initial_state()
        self.history: List[Dict[str, Any]] = []

    def _initial_state(self) -> EIIState:
        n = self.pipeline_config.detector.num_channels
        dim = self.pipeline_config.encoder.embedding_dim
        classes = self.pipeline_config.inference.num_classes
        return EIIState(
            voltages=np.zeros(n),
            currents=np.zeros(n),
            conductances=self.pipeline_config.default_conductance(),
            optical_phases=np.zeros(classes),
            refractory=np.zeros(n),
            prev_voltages=np.zeros(n),
            prev_envelope=np.zeros(n),
            prev_diff_signal=np.zeros(n),
            prev_comparator=np.zeros(n),
            prev_memristor_current=np.zeros(n),
            filter_state=np.zeros(n),
            control=np.zeros(classes),
        )

    def reset(self) -> None:
        self.state = self._initial_state()
        self.encoder.reset_filter(self.state)
        self.history.clear()

    def step(
        self,
        voltages: np.ndarray,
        currents: Optional[np.ndarray] = None,
        dt: Optional[float] = None,
        target_class: int = 0,
    ) -> Dict[str, Any]:
        dt = self.config.dt if dt is None else dt
        n = self.pipeline_config.detector.num_channels
        v = np.asarray(voltages, dtype=float).reshape(-1)
        if v.size < n:
            v = np.pad(v, (0, n - v.size))
        i = np.zeros(n) if currents is None else np.asarray(currents, dtype=float).reshape(-1)[:n]

        self.state.t += dt
        self.state.voltages = v[:n]
        self.state.currents = i[:n]

        events = self.detector.detect(self.state, dt)
        self.state.event_history.extend(events)
        self.state.window_events.extend(events)

        result: Dict[str, Any] = {
            "t": self.state.t,
            "events": events,
            "voltages": self.state.voltages.copy(),
            "embedding": None,
            "prediction": None,
            "confidence": 0.0,
            "control": self.state.control.copy(),
            "inference_ran": False,
        }

        window = self.config.window_T
        if window > 0 and self._window_boundary(self.state.t, window):
            window_start = self.state.t - window
            z = self.encoder.encode(
                self.state, self.state.window_events, window_start=window_start
            )
            y_hat, confidence = self.inference.infer(z)
            control = self.actuator.actuate(
                self.state,
                y_hat,
                confidence,
                self.state.window_events,
                target_class=target_class,
            )

            self.state.embedding = z
            self.state.prediction = y_hat
            self.state.confidence = confidence
            self.state.control = control
            self.state.window_events.clear()
            if self.config.encoder_mode == "filter":
                self.encoder.reset_filter(self.state)

            result.update(
                {
                    "embedding": z.copy(),
                    "prediction": y_hat.copy(),
                    "confidence": confidence,
                    "control": control.copy(),
                    "inference_ran": True,
                }
            )

        self.history.append(result)
        return result

    def run(
        self,
        voltage_trace: np.ndarray,
        current_trace: Optional[np.ndarray] = None,
        dt: Optional[float] = None,
        target_class: int = 0,
    ) -> List[Dict[str, Any]]:
        dt = self.config.dt if dt is None else dt
        trace = np.asarray(voltage_trace, dtype=float)
        if trace.ndim == 1:
            trace = trace.reshape(-1, 1)
        currents = None if current_trace is None else np.asarray(current_trace, dtype=float)
        outputs: List[Dict[str, Any]] = []

        for idx in range(trace.shape[0]):
            v = trace[idx]
            i = None if currents is None else currents[idx]
            outputs.append(self.step(v, i, dt=dt, target_class=target_class))
        return outputs

    def _window_boundary(self, t: float, window: float) -> bool:
        steps = int(round(t / max(self.config.dt, 1e-12)))
        window_steps = max(int(round(window / max(self.config.dt, 1e-12))), 1)
        return steps > 0 and steps % window_steps == 0

    @classmethod
    def from_circuit(cls, circuit) -> "EIIPipeline":
        """Build pipeline config from EII-tagged components on a schematic."""
        from egottol.models.base import ComponentType

        config = EIIPipelineConfig()
        eii_keys = {
            "IMPULSE_DETECTOR", "INFERENCE_ENCODER", "INFERENCE_ENGINE",
            "EII_PIPELINE", "MEMRISTOR", "CROSSBAR", "LIF_NEURON",
        }
        for comp in getattr(circuit, "components", []):
            key = comp.metadata.get("registry_key", comp.name)
            if key not in eii_keys and comp.type != ComponentType.ANALOG_COMPUTE:
                continue
            params = comp.parameters or {}
            if key in ("IMPULSE_DETECTOR", "EII_PIPELINE"):
                if "mode" in params:
                    from egottol.models.eii import DetectorMode
                    try:
                        config.detector.mode = DetectorMode(str(params["mode"]))
                    except ValueError:
                        pass
                config.detector.num_channels = int(
                    params.get("num_channels", config.detector.num_channels)
                )
                config.detector.threshold = float(
                    params.get("threshold", config.detector.threshold)
                )
            if key in ("INFERENCE_ENCODER", "EII_PIPELINE"):
                if "mode" in params:
                    from egottol.models.eii import EncoderMode
                    try:
                        config.encoder.mode = EncoderMode(str(params["mode"]))
                    except ValueError:
                        pass
                config.encoder.window_T = float(
                    params.get("window_T", config.encoder.window_T)
                )
            if key in ("INFERENCE_ENGINE", "EII_PIPELINE"):
                if "backend" in params:
                    from egottol.models.eii import InferenceBackend
                    try:
                        config.inference.backend = InferenceBackend(str(params["backend"]))
                    except ValueError:
                        pass
                config.inference.num_classes = int(
                    params.get("num_classes", config.inference.num_classes)
                )
        return cls(config)

    def run_duration(self, duration: float, dt: Optional[float] = None) -> List[Dict[str, Any]]:
        """Simulate a sinusoidal probe trace for the given duration."""
        dt = dt or self.config.dt
        steps = max(int(round(duration / dt)), 1)
        t = np.arange(steps) * dt
        n = self.pipeline_config.detector.num_channels
        trace = np.column_stack(
            [0.3 * np.sin(2 * np.pi * 50 * t + i * 0.5) + 0.2 * (i + 1) for i in range(n)]
        )
        return self.run(trace, dt=dt)
