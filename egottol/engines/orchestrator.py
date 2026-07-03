"""Multi-domain simulation orchestrator: analog, digital, analog-compute, AI, and EII."""

from __future__ import annotations

import logging
from typing import Any, Dict, List, Optional

import numpy as np

from egottol.engines.analog_compute.orchestrator import AnalogComputeOrchestrator
from egottol.engines.simulator import EventDrivenSimulator
from egottol.models.base import Circuit, ComponentType

logger = logging.getLogger(__name__)

try:
    from egottol.engines.solver import AdvancedMNASolver
except ImportError:
    AdvancedMNASolver = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.analog import NonlinearMNASolver
except ImportError:
    NonlinearMNASolver = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.ai.nsp import NeuralSignalProcessor, NSPConfig
except ImportError:
    NeuralSignalProcessor = None  # type: ignore[misc, assignment]
    NSPConfig = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.analog.hopfield import HopfieldNetwork
except ImportError:
    HopfieldNetwork = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.analog.ac_analysis import ACAnalysisEngine
except ImportError:
    ACAnalysisEngine = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.analog.ising import IsingMachine
except ImportError:
    IsingMachine = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.eii.pipeline import EIIPipeline
    from egottol.models.eii import EIIPipelineConfig

    _EII_AVAILABLE = True
except ImportError:
    EIIPipeline = None  # type: ignore[misc, assignment]
    EIIPipelineConfig = None  # type: ignore[misc, assignment]
    _EII_AVAILABLE = False

try:
    from egottol.engines.rtl_shadow import RTLShadowEngine

    _RTL_SHADOW_AVAILABLE = True
except ImportError:
    RTLShadowEngine = None  # type: ignore[misc, assignment]
    _RTL_SHADOW_AVAILABLE = False


class MultiDomainOrchestrator:
    """Coordinates MNA, nonlinear analog, logic, analog-compute, NSP, and EII."""

    def __init__(
        self,
        circuit: Circuit,
        eii_config: Optional[Any] = None,
        analog_compute: Optional[AnalogComputeOrchestrator] = None,
        use_nonlinear: bool = True,
    ):
        self.circuit = circuit
        self.mna = self._build_mna_solver(circuit, use_nonlinear)
        self.logic = EventDrivenSimulator(circuit)
        self.analog_compute = analog_compute or self._build_analog_compute()
        self.eii = self._init_eii(eii_config)
        self.nsp = self._init_nsp()
        self.hopfield = self._init_hopfield()
        self.ising = self._init_ising()
        self.rtl_shadow = self._init_rtl_shadow()
        self.t = 0.0
        self._dc_result: Dict[str, float] = {}
        self._ac_result: Dict[str, Any] = {}
        self._transient_history: List[Dict[str, Any]] = []
        self._last_spikes: List[Dict[str, Any]] = []
        self._last_nsp_output: Optional[np.ndarray] = None

    def _build_mna_solver(self, circuit: Circuit, use_nonlinear: bool):
        if use_nonlinear and NonlinearMNASolver is not None:
            try:
                return NonlinearMNASolver(circuit)
            except Exception as exc:
                logger.warning("NonlinearMNASolver unavailable: %s", exc)
        if AdvancedMNASolver is None:
            raise RuntimeError("No MNA solver available")
        return AdvancedMNASolver(circuit)

    def _build_analog_compute(self) -> AnalogComputeOrchestrator:
        n = 4
        for comp in self.circuit.components:
            if comp.type == ComponentType.ANALOG_COMPUTE:
                n = max(n, int(comp.parameters.get("rows", comp.parameters.get("n_modes", n))))
            if comp.metadata.get("registry_key") == "CROSSBAR":
                n = max(n, int(comp.parameters.get("rows", n)))
        return AnalogComputeOrchestrator(n_modes=n)

    def _init_eii(self, eii_config: Optional[Any]):
        if not _EII_AVAILABLE or EIIPipeline is None:
            return None
        try:
            if any(
                c.metadata.get("registry_key") in ("EII_PIPELINE", "IMPULSE_DETECTOR")
                or c.type == ComponentType.ANALOG_COMPUTE
                for c in self.circuit.components
            ):
                if eii_config is not None:
                    return EIIPipeline(eii_config)
                return EIIPipeline.from_circuit(self.circuit)
            cfg = eii_config if eii_config is not None else EIIPipelineConfig()
            return EIIPipeline(cfg)
        except Exception as exc:
            logger.warning("Failed to initialize EII pipeline: %s", exc)
            return None

    def _init_nsp(self):
        if NeuralSignalProcessor is None:
            return None
        for comp in self.circuit.components:
            key = comp.metadata.get("registry_key", comp.name)
            if key == "NSP_AI" or comp.name == "NSP_AI":
                mode = str(comp.parameters.get("mode", "denoise"))
                window = int(comp.parameters.get("window", 32))
                threshold = float(comp.parameters.get("threshold", 0.15))
                return NeuralSignalProcessor(
                    NSPConfig(
                        moving_avg_window=max(window // 4, 1),
                        spectral_gate_threshold=threshold,
                        fft_bins=window,
                    )
                ), mode
        return None

    def _init_hopfield(self):
        if HopfieldNetwork is None:
            return None
        for comp in self.circuit.components:
            if comp.metadata.get("registry_key") == "HOPFIELD_NET":
                n = int(comp.parameters.get("n_neurons", 4))
                net = HopfieldNetwork(n)
                patterns = comp.parameters.get("patterns", [])
                for p in patterns:
                    if isinstance(p, (list, tuple)) and len(p) == n:
                        net.store_pattern(np.asarray(p, dtype=float))
                return net
        return None

    def _init_ising(self):
        if IsingMachine is None:
            return None
        for comp in self.circuit.components:
            if comp.metadata.get("registry_key") == "ISING_CELL":
                n = 4
                J = float(comp.parameters.get("J", 1.0))
                coupling = np.full((n, n), J) - np.diag(np.full(n, J))
                return IsingMachine(n, coupling=coupling)
        return None

    def _init_rtl_shadow(self):
        if not _RTL_SHADOW_AVAILABLE or RTLShadowEngine is None:
            return None
        trigger_keys = {"RTL_SHADOW", "PIM_PERIPHERAL", "CROSSBAR"}
        if not any(
            c.metadata.get("registry_key") in trigger_keys
            or c.type == ComponentType.LOGIC
            for c in self.circuit.components
        ):
            return None
        try:
            return RTLShadowEngine(self.circuit)
        except Exception as exc:
            logger.warning("Failed to initialize RTL shadow engine: %s", exc)
            return None

    @property
    def eii_available(self) -> bool:
        return self.eii is not None

    def solve_dc(self) -> Dict[str, float]:
        self._dc_result = self.mna.solve_dc()
        ac = self.analog_compute.last_state
        for i, v in enumerate(ac.membrane):
            self._dc_result[f"analog_compute:membrane:{i}"] = float(v)
        for i, c in enumerate(ac.crossbar_currents):
            self._dc_result[f"analog_compute:I_col:{i}"] = float(c)
        if self.hopfield is not None:
            self._dc_result["hopfield:energy"] = float(self.hopfield.energy())
        return dict(self._dc_result)

    def run_ac(self, freq_start: float, freq_stop: float, points: int) -> Dict[str, Any]:
        """Small-signal AC sweep using the active MNA solver node map."""
        if ACAnalysisEngine is None:
            raise RuntimeError("ACAnalysisEngine is not available")
        engine = ACAnalysisEngine(self.circuit, solver=self.mna)
        result = engine.solve_ac(freq_start, freq_stop, points)
        self._ac_result = {
            "frequencies": result["frequencies"].tolist(),
            "dc_op": result["dc_op"],
            "reference_ac_v": result["reference_ac_v"],
            "nodes": {
                name: {
                    "magnitude": data["magnitude"].tolist(),
                    "phase_deg": data["phase_deg"].tolist(),
                }
                for name, data in result["nodes"].items()
            },
        }
        return dict(self._ac_result)

    def _run_nsp_on_probe(self, probe: np.ndarray, sample_rate: float = 1e6) -> Dict[str, Any]:
        if self.nsp is None:
            return {}
        nsp, mode = self.nsp
        x = np.asarray(probe, dtype=float).reshape(-1)
        if x.size == 0:
            return {}
        if mode == "classify":
            label, probs = nsp.classify(x)
            self._last_nsp_output = probs
            return {"mode": "classify", "label": int(label), "probs": probs.tolist()}
        if mode == "anomaly":
            is_anomaly, score = nsp.anomaly_detect(x)
            self._last_nsp_output = np.array([score])
            return {"mode": "anomaly", "is_anomaly": is_anomaly, "score": float(score)}
        cleaned = nsp.denoise(x, sample_rate=sample_rate)
        self._last_nsp_output = cleaned
        return {"mode": "denoise", "rms_in": float(np.sqrt(np.mean(x**2))), "rms_out": float(np.sqrt(np.mean(cleaned**2)))}

    def step_transient(self, dt: float) -> Dict[str, Any]:
        self.logic.step()
        ac_state = self.analog_compute.step(dt)
        self.t += dt

        snapshot: Dict[str, Any] = {
            "t": self.t,
            "logic_state": {cid: dict(ports) for cid, ports in self.logic.state.items()},
            "analog_compute": {
                "membrane": ac_state.membrane.tolist(),
                "crossbar_currents": ac_state.crossbar_currents.tolist(),
                "n_spikes": len(ac_state.spikes),
                "spikes": [
                    {"t": s.t, "neuron": s.neuron, "amplitude": s.amplitude}
                    for s in ac_state.spikes[-20:]
                ],
            },
        }
        self._last_spikes = snapshot["analog_compute"]["spikes"]

        if self.nsp is not None:
            snapshot["nsp"] = self._run_nsp_on_probe(ac_state.membrane)

        if self.hopfield is not None and ac_state.membrane.size > 0:
            state = np.sign(ac_state.membrane[: self.hopfield.n])
            recalled = self.hopfield.recall(state, max_steps=5)
            snapshot["hopfield"] = {
                "input": state.tolist(),
                "recalled": recalled.tolist(),
                "energy": float(self.hopfield.energy(recalled)),
            }

        if self.eii is not None:
            try:
                eii_out = self.eii.step(ac_state.membrane, ac_state.crossbar_currents, dt=dt)
                snapshot["eii"] = eii_out
            except Exception as exc:
                logger.warning("EII step failed: %s", exc)

        if self.rtl_shadow is not None:
            try:
                snapshot["rtl_shadow"] = self.rtl_shadow.step(
                    cycles=1,
                    crossbar_currents=ac_state.crossbar_currents.tolist(),
                )
            except Exception as exc:
                logger.warning("RTL shadow step failed: %s", exc)

        self._transient_history.append(snapshot)
        return snapshot

    def solve_transient(self, t_stop: float, dt: float) -> List[Dict[str, Any]]:
        """Full coupled transient — uses nonlinear MNA when available."""
        if hasattr(self.mna, "solve_transient"):
            mna_trace = self.mna.solve_transient(t_stop, dt)
            self._transient_history = []
            for row in mna_trace:
                snap = self.step_transient(dt)
                snap["mna"] = row.get("v", {})
                self._transient_history[-1] = snap
            return self._transient_history

        self.reset()
        steps = max(int(round(t_stop / dt)), 1)
        for _ in range(steps):
            self.step_transient(dt)
        return self._transient_history

    def run_eii_window(self, T: float, target_class: int = 0) -> Optional[List[Dict[str, Any]]]:
        if self.eii is None:
            return None
        dt = self.eii.config.dt
        steps = max(int(round(T / dt)), 1)
        outputs: List[Dict[str, Any]] = []
        self.eii.reset()
        for _ in range(steps):
            ac_state = self.analog_compute.step(dt)
            self.t += dt
            out = self.eii.step(
                ac_state.membrane,
                ac_state.crossbar_currents,
                dt=dt,
                target_class=target_class,
            )
            outputs.append(out)
        return outputs

    def analyze_spikes(self) -> Dict[str, Any]:
        spikes = self._last_spikes
        if not spikes and self._transient_history:
            spikes = self._transient_history[-1].get("analog_compute", {}).get("spikes", [])
        if not spikes:
            return {"ok": False, "message": "No spikes recorded — run transient or EII first."}
        times = [s["t"] for s in spikes]
        neurons = [s["neuron"] for s in spikes]
        isi = np.diff(times) if len(times) > 1 else []
        return {
            "ok": True,
            "count": len(spikes),
            "mean_isi": float(np.mean(isi)) if len(isi) else 0.0,
            "unique_neurons": len(set(neurons)),
            "spike_rate_hz": len(spikes) / max(times[-1] - times[0], 1e-9) if len(times) > 1 else 0.0,
            "spikes": spikes[-50:],
        }

    def reset(self) -> None:
        self.t = 0.0
        self._dc_result.clear()
        self._ac_result.clear()
        self._transient_history.clear()
        self._last_spikes.clear()
        self._last_nsp_output = None
        self.analog_compute.reset()
        if self.eii is not None:
            self.eii.reset()
