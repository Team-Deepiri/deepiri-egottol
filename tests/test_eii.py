"""Unit tests for EII impulse detector and encoding manifold."""

import numpy as np
import pytest

from egottol.engines.eii.detectors import ImpulseDetector
from egottol.engines.eii.encoders import EncodingManifold
from egottol.engines.eii.types import EIIConfig, EIIState, ImpulseEvent


@pytest.fixture
def config() -> EIIConfig:
    return EIIConfig(
        dt=1e-3,
        window_T=0.05,
        embedding_dim=2,
        num_channels=2,
        detector_mode="threshold",
        encoder_mode="rate",
        stimulus_time=0.0,
    )


@pytest.fixture
def state() -> EIIState:
    return EIIState(
        t=0.0,
        voltages=np.array([0.0, 0.0]),
        refractory=np.array([0.0, 0.0]),
    )


def test_threshold_detector_emits_on_crossing(config, state):
    detector = ImpulseDetector(config)
    state.voltages = np.array([0.2, 0.8])
    events = detector.detect(state, config.dt)
    assert len(events) == 1
    assert events[0].channel == 1
    assert events[0].event_type == "threshold_cross"
    assert events[0].amplitude == pytest.approx(0.8)


def test_threshold_detector_respects_refractory(config, state):
    detector = ImpulseDetector(config)
    state.voltages = np.array([0.9, 0.9])
    first = detector.detect(state, config.dt)
    state.t += config.dt
    second = detector.detect(state, config.dt)
    assert len(first) == 2
    assert len(second) == 0


def test_comparator_detector_rising_edge_only(config, state):
    config.detector_mode = "comparator"
    detector = ImpulseDetector(config)
    state.voltages = np.array([0.2, 0.2])
    assert detector.detect(state, config.dt) == []
    state.voltages = np.array([0.6, 0.2])
    events = detector.detect(state, config.dt)
    assert len(events) == 1
    assert events[0].channel == 0


def test_rate_encoder_counts_spikes_in_window(config):
    encoder = EncodingManifold(config)
    events = [
        ImpulseEvent(t=0.01, channel=0, event_type="spike", amplitude=1.0),
        ImpulseEvent(t=0.02, channel=0, event_type="spike", amplitude=1.0),
        ImpulseEvent(t=0.03, channel=1, event_type="spike", amplitude=1.0),
    ]
    state = EIIState()
    z = encoder.encode(state, events, window_start=0.0)
    assert z[0] == pytest.approx(2.0 / config.window_T)
    assert z[1] == pytest.approx(1.0 / config.window_T)


def test_continuous_encoder_reads_probe_voltages(config):
    config.encoder_mode = "continuous"
    config.embedding_dim = 3
    encoder = EncodingManifold(config)
    state = EIIState(voltages=np.array([1.1, 2.2, 3.3]))
    z = encoder.encode(state, [], window_start=0.0)
    np.testing.assert_allclose(z, [1.1, 2.2, 3.3])


def test_filter_encoder_accumulates_decay(config):
    config.encoder_mode = "filter"
    encoder = EncodingManifold(config)
    state = EIIState(filter_state=np.zeros(2))
    events = [ImpulseEvent(t=0.001, channel=0, event_type="spike", amplitude=0.5)]
    z1 = encoder.encode(state, events, window_start=0.0)
    z2 = encoder.encode(state, [], window_start=0.0)
    assert z1[0] > 0
    assert z2[0] < z1[0]
