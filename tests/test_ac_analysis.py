"""Tests for small-signal AC analysis."""

import numpy as np
import pytest

from egottol.engines.analog.ac_analysis import ACAnalysisEngine
from egottol.engines.orchestrator import MultiDomainOrchestrator
from egottol.models.base import Circuit, Component, ComponentType, Port, Wire


def _rc_lowpass_circuit() -> Circuit:
    return Circuit(
        id="rc_lp",
        name="RC Lowpass",
        components=[
            Component(
                id="V1",
                name="Voltage Source",
                type=ComponentType.SOURCE,
                ports=[
                    Port(name="+", direction="inout"),
                    Port(name="-", direction="inout"),
                ],
                parameters={"V": 0.0, "v_ac": 1.0},
            ),
            Component(
                id="R1",
                name="Resistor",
                type=ComponentType.PASSIVE,
                ports=[
                    Port(name="1", direction="inout"),
                    Port(name="2", direction="inout"),
                ],
                parameters={"R": 1000.0},
            ),
            Component(
                id="C1",
                name="Capacitor",
                type=ComponentType.PASSIVE,
                ports=[
                    Port(name="1", direction="inout"),
                    Port(name="2", direction="inout"),
                ],
                parameters={"C": 1e-7},
            ),
            Component(
                id="GND1",
                name="Ground",
                type=ComponentType.POWER,
                ports=[Port(name="G", direction="inout")],
                parameters={},
            ),
        ],
        wires=[
            Wire(id="w1", from_component="V1", from_port="+", to_component="R1", to_port="1"),
            Wire(id="w2", from_component="R1", from_port="2", to_component="C1", to_port="1"),
            Wire(id="w3", from_component="C1", from_port="2", to_component="GND1", to_port="G"),
            Wire(id="w4", from_component="V1", from_port="-", to_component="GND1", to_port="G"),
        ],
    )


def test_rc_lowpass_magnitude():
    circuit = _rc_lowpass_circuit()
    engine = ACAnalysisEngine(circuit)
    result = engine.solve_ac(10.0, 1e6, 10)

    freqs = result["frequencies"]
    out_node = "C1:1"
    assert out_node in result["nodes"]

    mags = result["nodes"][out_node]["magnitude"]
    r = 1000.0
    c = 1e-7
    fc = 1.0 / (2.0 * np.pi * r * c)

    assert mags[0] == pytest.approx(1.0, rel=0.05)
    idx_fc = int(np.argmin(np.abs(freqs - fc)))
    assert mags[idx_fc] == pytest.approx(1.0 / np.sqrt(2.0), rel=0.08)
    assert mags[-1] < 0.2


def test_orchestrator_run_ac():
    orch = MultiDomainOrchestrator(_rc_lowpass_circuit(), use_nonlinear=False)
    result = orch.run_ac(100.0, 100e3, 8)
    assert "frequencies" in result
    assert "C1:1" in result["nodes"]
    assert len(result["frequencies"]) >= 2
