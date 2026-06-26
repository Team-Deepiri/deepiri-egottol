"""Couples crossbar, spiking, and photonic sub-engines per timestep."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional

import numpy as np

from egottol.engines.analog_compute.crossbar import CrossbarEngine
from egottol.engines.analog_compute.photonic import PhotonicEngine
from egottol.engines.analog_compute.spiking import SpikeEvent, SpikingEngine


@dataclass
class AnalogComputeState:
    t: float = 0.0
    photonic: np.ndarray = field(default_factory=lambda: np.zeros(4, dtype=complex))
    electrical: np.ndarray = field(default_factory=lambda: np.zeros(4))
    crossbar_currents: np.ndarray = field(default_factory=lambda: np.zeros(4))
    membrane: np.ndarray = field(default_factory=lambda: np.zeros(4))
    spikes: List[SpikeEvent] = field(default_factory=list)


class AnalogComputeOrchestrator:
    """Hybrid photonic-electronic neuromorphic compute loop."""

    def __init__(
        self,
        n_modes: int = 4,
        crossbar: Optional[CrossbarEngine] = None,
        spiking: Optional[SpikingEngine] = None,
        photonic: Optional[PhotonicEngine] = None,
    ):
        self.crossbar = crossbar or CrossbarEngine(rows=n_modes, cols=n_modes)
        self.spiking = spiking or SpikingEngine(n_neurons=n_modes)
        self.photonic = photonic or PhotonicEngine(n_modes=n_modes)
        self.t = 0.0
        self.last_state = AnalogComputeState()

    def reset(self) -> None:
        self.t = 0.0
        self.spiking.reset()
        self.last_state = AnalogComputeState()

    def step(
        self,
        dt: float,
        optical_in: Optional[np.ndarray] = None,
        row_voltages: Optional[np.ndarray] = None,
    ) -> AnalogComputeState:
        photonic_out = self.photonic.step(dt, optical_in=optical_in)
        elec_in = self.photonic.to_electrical(photonic_out)

        if row_voltages is not None:
            v_drive = np.asarray(row_voltages, dtype=float)
        else:
            v_drive = elec_in[: self.crossbar.rows]

        crossbar_out = self.crossbar.solve(v_drive)
        membrane, spikes = self.spiking.step(crossbar_out, dt)

        if spikes:
            pre, post = self.spiking.spikes_from_events(spikes)
            delta = self.spiking.apply_stdp(pre, post, dt)
            if delta.shape == self.crossbar.G.shape:
                self.crossbar.queue_stdp_delta(delta)
            elif delta.size > 0:
                d = np.zeros_like(self.crossbar.G)
                rows = min(delta.shape[0], d.shape[0])
                cols = min(delta.shape[1], d.shape[1])
                d[:rows, :cols] = delta[:rows, :cols]
                self.crossbar.queue_stdp_delta(d)

        self.crossbar.consume_stdp_delta()
        self.t += dt

        self.last_state = AnalogComputeState(
            t=self.t,
            photonic=photonic_out,
            electrical=elec_in,
            crossbar_currents=crossbar_out,
            membrane=membrane,
            spikes=list(spikes),
        )
        return self.last_state

    def stamp_mna(self, n: int, node_map: Dict[str, int], comp_id: str):
        return self.crossbar.stamp_for_component(node_map, comp_id, n)

    def summary(self) -> Dict[str, Any]:
        return {
            "t": self.t,
            "crossbar_shape": self.crossbar.G.shape,
            "n_spikes": len(self.spiking.state.spikes),
            "last_membrane_mean": float(np.mean(self.last_state.membrane)),
        }
