"""Tests for AI engine modules (NSP, reservoir, hopfield)."""

import json
from pathlib import Path

import numpy as np
import pytest

from egottol.engines.ai.auto_tune import AutoTuner
from egottol.engines.ai.hopfield_infer import HopfieldInference, HopfieldNetwork
from egottol.engines.ai.nsp import NeuralSignalProcessor
from egottol.engines.ai.reservoir import EchoStateReservoir, ReservoirConfig
from egottol.engines.ai.weight_loader import apply_weights_to_engine, load_weights
from egottol.engines.eii.inference import InferenceEngine
from egottol.models.eii import EIIPipelineConfig, InferenceBackend, InferenceEngineConfig


@pytest.fixture
def sine_noisy() -> np.ndarray:
    t = np.linspace(0, 1, 256)
    clean = np.sin(2 * np.pi * 10 * t)
    noise = 0.3 * np.random.default_rng(0).standard_normal(clean.size)
    return clean + noise


def test_nsp_denoise_reduces_high_frequency_noise(sine_noisy):
    nsp = NeuralSignalProcessor()
    out = nsp.denoise(sine_noisy, sample_rate=256.0)
    assert out.shape == sine_noisy.shape
    assert np.std(out - np.mean(out)) <= np.std(sine_noisy - np.mean(sine_noisy)) + 0.05


def test_nsp_classify_separates_frequencies():
    nsp = NeuralSignalProcessor()
    t = np.linspace(0, 1, 128)
    low = np.sin(2 * np.pi * 5 * t)
    high = np.sin(2 * np.pi * 30 * t)
    nsp.train_classifier(np.array([low, high, low, high]), np.array([0, 1, 0, 1]))
    label, probs = nsp.classify(high)
    assert label == 1
    assert probs[1] > probs[0]


def test_nsp_anomaly_detect_flags_spike():
    nsp = NeuralSignalProcessor()
    baseline = np.zeros(64)
    spike = baseline.copy()
    spike[32] = 10.0
    is_anomaly, z = nsp.anomaly_detect(spike, reference=baseline)
    assert is_anomaly
    assert z > nsp.config.anomaly_z_threshold


def test_reservoir_trains_and_predicts():
    rng = np.random.default_rng(1)
    cfg = ReservoirConfig(n_reservoir=50, seed=7)
    esn = EchoStateReservoir(cfg)
    traces = []
    labels = []
    for cls in range(2):
        for _ in range(6):
            trace = rng.standard_normal((20, 2))
            trace[:, cls % 2] += 2.0 * (1 if cls == 0 else -1)
            traces.append(trace)
            labels.append(cls)
    esn.fit(np.array(traces), np.array(labels))
    probs, conf = esn.infer(traces[0])
    assert probs.shape[0] == 2
    assert conf == pytest.approx(float(np.max(probs)))


def test_hopfield_recalls_stored_pattern():
    patterns = np.array([[1, 1, -1, -1], [-1, -1, 1, 1]], dtype=float)
    net = HopfieldNetwork(4)
    net.store(patterns)
    probe = np.array([1, 1, -1, 1], dtype=float)
    recalled = net.recall(probe)
    np.testing.assert_array_equal(recalled, patterns[0])


def test_hopfield_inference_returns_class_probs():
    patterns = np.array([[1, 1, -1, -1], [-1, -1, 1, 1]], dtype=float)
    infer = HopfieldInference(patterns)
    probs, conf = infer.infer(np.array([1, 1, -1, 1], dtype=float))
    assert probs.shape[0] == 2
    assert np.argmax(probs) == 0
    assert conf > 0.5


def test_inference_engine_reservoir_backend():
    cfg = InferenceEngineConfig(backend=InferenceBackend.RESERVOIR, num_classes=2, reservoir_size=30)
    pipeline = EIIPipelineConfig(inference=cfg)
    engine = InferenceEngine(cfg, pipeline.default_weights(), pipeline.default_bias(), pipeline.default_conductance())
    z = np.array([0.5, -0.5, 0.2, 0.1, 0.0, 0.3, -0.2, 0.4])
    probs, conf = engine.infer(z)
    assert probs.size == cfg.num_classes
    assert np.isclose(np.sum(probs), 1.0)
    assert conf >= 0.0


def test_inference_engine_hopfield_backend():
    patterns = [[1, 1, -1, -1], [-1, -1, 1, 1]]
    cfg = InferenceEngineConfig(
        backend=InferenceBackend.HOPFIELD,
        num_classes=2,
        hopfield_patterns=patterns,
    )
    engine = InferenceEngine(
        cfg,
        np.asarray(patterns, dtype=float),
        np.zeros(2),
        np.zeros((2, 4)),
    )
    probs, conf = engine.infer(np.array([1, 1, -1, 1], dtype=float))
    assert np.argmax(probs) == 0
    assert conf > 0.5


def test_weight_loader_applies_to_engine(tmp_path: Path):
    payload = {
        "format": "egt-weights-v1",
        "backend": "digital_softmax",
        "embedding_dim": 2,
        "output_dim": 2,
        "weights": {"W": [[1.0, 0.0], [0.0, 1.0]], "b": [0.0, 0.0]},
        "temperature": 1.0,
    }
    path = tmp_path / "test.egt-weights"
    path.write_text(json.dumps(payload), encoding="utf-8")
    loaded = load_weights(path)
    cfg = InferenceEngineConfig()
    engine = InferenceEngine(cfg, np.zeros((2, 2)), np.zeros(2), np.zeros((2, 2)))
    apply_weights_to_engine(engine, loaded)
    probs, _ = engine.infer(np.array([1.0, 0.0]))
    assert np.argmax(probs) == 0


def test_auto_tuner_fits_parameters():
    target = np.sin(np.linspace(0, 2 * np.pi, 32))

    def simulate(params):
        amp = params["amp"]
        freq = params["freq"]
        t = np.linspace(0, 2 * np.pi, 32)
        return amp * np.sin(freq * t)

    tuner = AutoTuner(
        simulate,
        param_names=["amp", "freq"],
        initial_params={"amp": 0.5, "freq": 0.5},
        bounds=([0.1, 0.5], [2.0, 2.0]),
    )
    result = tuner.tune(target)
    assert result["amp"] == pytest.approx(1.0, rel=0.2)
    assert result["freq"] == pytest.approx(1.0, rel=0.2)
