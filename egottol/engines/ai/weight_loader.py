"""Load .egt-weights files and apply them to an InferenceEngine."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, Union

import numpy as np

from egottol.engines.eii.inference import InferenceEngine
from egottol.models.eii import DigitalHead, InferenceBackend, InferenceEngineConfig


_BACKEND_MAP = {
    "digital_linear": (InferenceBackend.DIGITAL, DigitalHead.LINEAR),
    "digital_softmax": (InferenceBackend.DIGITAL, DigitalHead.SOFTMAX),
    "digital_mlp": (InferenceBackend.DIGITAL, DigitalHead.LINEAR),
    "energy_based": (InferenceBackend.ENERGY_BASED, DigitalHead.LINEAR),
}


def load_weights(path: Union[str, Path]) -> Dict[str, Any]:
    """Load and minimally validate a .egt-weights JSON file."""
    path = Path(path)
    with path.open(encoding="utf-8") as f:
        payload = json.load(f)

    if payload.get("format") != "egt-weights-v1":
        raise ValueError(f"Unsupported weights format in {path}")
    if "weights" not in payload or "W" not in payload["weights"]:
        raise ValueError(f"Missing weights.W in {path}")

    w = np.asarray(payload["weights"]["W"], dtype=float)
    embedding_dim = int(payload["embedding_dim"])
    output_dim = int(payload["output_dim"])
    if w.shape != (output_dim, embedding_dim):
        raise ValueError(
            f"W shape {w.shape} != ({output_dim}, {embedding_dim}) in {path}"
        )
    return payload


def apply_weights_to_engine(
    engine: InferenceEngine,
    payload: Dict[str, Any],
) -> InferenceEngine:
    """Apply loaded weights to an existing InferenceEngine in place."""
    w = np.asarray(payload["weights"]["W"], dtype=float)
    b_raw = payload["weights"].get("b")
    bias = np.zeros(w.shape[0], dtype=float) if b_raw is None else np.asarray(b_raw, dtype=float)

    backend_name = payload.get("backend", "digital_linear")
    backend, head = _BACKEND_MAP.get(
        backend_name,
        (InferenceBackend.DIGITAL, DigitalHead.LINEAR),
    )
    head_name = payload.get("head")
    if head_name == "softmax":
        head = DigitalHead.SOFTMAX

    engine.config = InferenceEngineConfig(
        **{
            **engine.config.model_dump(),
            "backend": backend,
            "digital_head": head,
            "num_classes": w.shape[0],
            "temperature": float(payload.get("temperature", engine.config.temperature)),
            "weights": w.tolist(),
            "bias": bias.tolist(),
        }
    )
    engine.weights = w
    engine.bias = bias
    return engine


def load_and_apply(path: Union[str, Path], engine: InferenceEngine) -> InferenceEngine:
    """Convenience: load file and apply to engine."""
    return apply_weights_to_engine(engine, load_weights(path))
