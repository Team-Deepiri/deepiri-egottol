"""Integration tests for analog and AI orchestration."""

import numpy as np
import pytest

from egottol.engines.analog import (
    GilbertCell,
    HopfieldNetwork,
    IsingMachine,
    NonlinearMNASolver,
    OpAmpNeuronLayer,
    OTACell,
    add_noise_to_trace,
    thermal_noise,
)
from egottol.engines.orchestrator import MultiDomainOrchestrator
from egottol.models.base import Circuit, Component, ComponentType, Port


def _minimal_circuit() -> Circuit:
    return Circuit(
        id="t",
        name="test",
        components=[
            Component(
                id="R1",
                name="Resistor",
                type=ComponentType.PASSIVE,
                ports=[Port(name="1", direction="inout"), Port(name="2", direction="inout")],
                parameters={"R": 1000.0},
            ),
            Component(
                id="EII1",
                name="EII Pipeline",
                type=ComponentType.ANALOG_COMPUTE,
                metadata={"registry_key": "EII_PIPELINE"},
                parameters={"window_T": 1e-3},
            ),
        ],
        wires=[],
    )


def test_opamp_neuron_layer_forward():
    layer = OpAmpNeuronLayer(n_in=3, n_out=2, activation="tanh")
    out = layer.forward(np.array([0.1, 0.2, -0.1]))
    assert out.shape == (2,)
    assert np.all(np.abs(out) <= 1.0 + 1e-9)


def test_hopfield_store_recall():
    net = HopfieldNetwork(4)
    net.store_pattern(np.array([1, 1, -1, -1]))
    recalled = net.recall(np.array([1, 1, -1, 0]), max_steps=20)
    assert recalled.shape == (4,)


def test_ising_anneal():
    J = np.array([[0, 1], [1, 0]], dtype=float)
    machine = IsingMachine(2, coupling=J)
    result = machine.solve(n_steps=200, t_start=2.0, t_end=0.01)
    assert result.spins.shape == (2,)
    assert np.isfinite(result.energy)


def test_ota_and_gilbert():
    ota = OTACell(gm=1e-3)
    i = ota.output_current(0.5, 0.2)
    assert i == pytest.approx(3e-4)
    mult = GilbertCell(k=1.0)
    assert mult.multiply(0.3, 0.4) == pytest.approx(0.12)


def test_thermal_noise():
    n = thermal_noise(resistance=1e3, temperature=300, bandwidth=1e6, n_samples=1000)
    assert n.shape == (1000,)
    assert np.std(n) > 0


def test_orchestrator_step():
    orch = MultiDomainOrchestrator(_minimal_circuit(), use_nonlinear=False)
    snap = orch.step_transient(1e-4)
    assert "analog_compute" in snap
    assert orch.eii_available


def test_orchestrator_analyze_spikes_empty():
    orch = MultiDomainOrchestrator(_minimal_circuit(), use_nonlinear=False)
    result = orch.analyze_spikes()
    assert result["ok"] is False
