"""Op-amp based analog neuron layer with tanh/sigmoid activation."""

from __future__ import annotations

from typing import Literal, Optional

import numpy as np

from egottol.engines.analog.ota import OTACell


ActivationFn = Literal["tanh", "sigmoid"]


class OpAmpNeuronLayer:
    """
    Analog neuron layer: weighted transconductance summation followed by
    op-amp saturation (tanh or sigmoid).

    Each output neuron j computes y_j = act(gain * sum_i W_ji * x_i + b_j).
    """

    def __init__(
        self,
        n_in: int,
        n_out: int,
        weights: Optional[np.ndarray] = None,
        bias: Optional[np.ndarray] = None,
        activation: ActivationFn = "tanh",
        gm: float = 1e-3,
        v_sat: float = 1.0,
        rng: Optional[np.random.Generator] = None,
    ):
        self.n_in = n_in
        self.n_out = n_out
        self.activation = activation
        self.gm = gm
        self.v_sat = v_sat
        self.rng = rng or np.random.default_rng(0)

        if weights is not None:
            self.W = np.asarray(weights, dtype=float).reshape(n_out, n_in)
        else:
            self.W = self.rng.normal(0.0, 0.1, (n_out, n_in))

        if bias is not None:
            self.bias = np.asarray(bias, dtype=float).reshape(n_out)
        else:
            self.bias = np.zeros(n_out)

        self._ota_cells = [
            OTACell(gm=gm) for _ in range(n_out)
        ]

    @staticmethod
    def _sigmoid(x: np.ndarray) -> np.ndarray:
        x = np.clip(x, -500.0, 500.0)
        return 1.0 / (1.0 + np.exp(-x))

    @staticmethod
    def _tanh(x: np.ndarray) -> np.ndarray:
        return np.tanh(x)

    def _activate(self, x: np.ndarray) -> np.ndarray:
        scaled = x / max(self.v_sat, 1e-12)
        if self.activation == "sigmoid":
            return self._sigmoid(scaled)
        return self._tanh(scaled)

    def forward(self, x: np.ndarray) -> np.ndarray:
        """
        Forward pass through the analog neuron layer.

        Uses OTA transconductance summation per output, then op-amp saturation.
        """
        inp = np.asarray(x, dtype=float).reshape(-1)
        if inp.size < self.n_in:
            inp = np.pad(inp, (0, self.n_in - inp.size))
        inp = inp[: self.n_in]

        outputs = np.zeros(self.n_out)
        for j in range(self.n_out):
            i_sum = 0.0
            for i in range(self.n_in):
                i_sum += self.W[j, i] * self._ota_cells[j].output_current(inp[i], 0.0)
            v_net = i_sum / max(self.gm, 1e-18) + self.bias[j]
            outputs[j] = self._activate(np.array([v_net]))[0]

        return outputs

    def set_weights(self, weights: np.ndarray) -> None:
        self.W = np.asarray(weights, dtype=float).reshape(self.n_out, self.n_in)
