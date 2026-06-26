"""Import ONNX models and simple weight files into .egt-weights format."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple, Union

import numpy as np

EGT_FORMAT = "egt-weights-v1"


def onnx_available() -> bool:
    """Return True if ONNX graph parsing is available."""
    try:
        import onnx  # noqa: F401

        return True
    except ImportError:
        return False


def onnxruntime_available() -> bool:
    """Return True if onnxruntime is installed."""
    try:
        import onnxruntime  # noqa: F401

        return True
    except ImportError:
        return False


def _resolve_tensor(name: str, tensors: Dict[str, np.ndarray]) -> Optional[np.ndarray]:
    if name in tensors:
        return tensors[name]
    base = name.split("/")[-1]
    for key, value in tensors.items():
        if key == base or key.endswith("/" + base):
            return value
    return None


def extract_linear_layers_from_onnx(path: Union[str, Path]) -> List[Dict[str, Any]]:
    """Parse an ONNX model and collect weight matrices from Gemm/MatMul nodes."""
    try:
        import onnx
        from onnx import numpy_helper
    except ImportError as exc:
        raise ImportError(
            "ONNX import requires the 'onnx' package. "
            "Install optional AI deps: pip install 'deepiri-egottol[ai]'"
        ) from exc

    model = onnx.load(str(path))
    tensors = {
        init.name: np.asarray(numpy_helper.to_array(init), dtype=float)
        for init in model.graph.initializer
    }
    layers: List[Dict[str, Any]] = []

    for node in model.graph.node:
        if node.op_type == "Gemm":
            if len(node.input) < 2:
                continue
            a_name, b_name = node.input[0], node.input[1]
            b = _resolve_tensor(b_name, tensors)
            if b is None:
                continue
            trans_b = 0
            for attr in node.attribute:
                if attr.name == "transB":
                    trans_b = int(attr.i)
            w = b.T if trans_b else b
            bias = None
            if len(node.input) >= 3:
                c = _resolve_tensor(node.input[2], tensors)
                if c is not None and c.ndim == 1:
                    bias = c
            layers.append(
                {
                    "name": node.name or b_name,
                    "W": np.asarray(w, dtype=float),
                    "b": None if bias is None else np.asarray(bias, dtype=float),
                    "op": "Gemm",
                }
            )
        elif node.op_type == "MatMul":
            if len(node.input) < 2:
                continue
            left = _resolve_tensor(node.input[0], tensors)
            right = _resolve_tensor(node.input[1], tensors)
            if right is None:
                continue
            if left is not None and left.ndim == 2 and right.ndim == 2:
                if left.shape[1] == right.shape[0]:
                    w = right.T
                else:
                    w = right
            else:
                w = right if right.ndim == 2 else right.reshape(-1, 1)
            layers.append(
                {
                    "name": node.name or node.input[1],
                    "W": np.asarray(w, dtype=float),
                    "b": None,
                    "op": "MatMul",
                }
            )

    if not layers:
        for name, arr in tensors.items():
            if arr.ndim == 2:
                layers.append({"name": name, "W": arr, "b": None, "op": "initializer"})
    return layers


def build_egt_payload(
    w: np.ndarray,
    bias: Optional[np.ndarray] = None,
    *,
    name: str = "imported",
    backend: str = "digital_linear",
    head: str = "linear",
    temperature: float = 1.0,
    metadata: Optional[Dict[str, Any]] = None,
) -> Dict[str, Any]:
    """Build a schema-compliant .egt-weights payload from W and optional bias."""
    w = np.asarray(w, dtype=float)
    if w.ndim != 2:
        raise ValueError(f"Weight matrix must be 2-D, got shape {w.shape}")
    output_dim, embedding_dim = w.shape
    b = np.zeros(output_dim, dtype=float) if bias is None else np.asarray(bias, dtype=float).reshape(-1)
    if b.shape[0] != output_dim:
        raise ValueError(f"Bias length {b.shape[0]} != output_dim {output_dim}")

    payload: Dict[str, Any] = {
        "format": EGT_FORMAT,
        "name": name,
        "backend": backend,
        "head": head,
        "embedding_dim": int(embedding_dim),
        "output_dim": int(output_dim),
        "weights": {"W": w.tolist(), "b": b.tolist()},
        "temperature": float(temperature),
    }
    if metadata:
        payload["metadata"] = metadata
    return payload


def _parse_simple_json(data: Dict[str, Any], path: Path) -> Dict[str, Any]:
    if data.get("format") == EGT_FORMAT:
        return data
    if "weights" in data and isinstance(data["weights"], dict) and "W" in data["weights"]:
        w = np.asarray(data["weights"]["W"], dtype=float)
        b_raw = data["weights"].get("b")
        return build_egt_payload(
            w,
            None if b_raw is None else np.asarray(b_raw, dtype=float),
            name=str(data.get("name", path.stem)),
            backend=str(data.get("backend", "digital_linear")),
            head=str(data.get("head", "linear")),
            temperature=float(data.get("temperature", 1.0)),
            metadata=data.get("metadata"),
        )
    if "W" in data:
        w = np.asarray(data["W"], dtype=float)
        b_raw = data.get("b")
        return build_egt_payload(
            w,
            None if b_raw is None else np.asarray(b_raw, dtype=float),
            name=str(data.get("name", path.stem)),
            backend=str(data.get("backend", "digital_linear")),
            head=str(data.get("head", "linear")),
            temperature=float(data.get("temperature", 1.0)),
            metadata=data.get("metadata"),
        )
    raise ValueError(f"Unrecognized JSON weight layout in {path}")


def _load_npy_weights(path: Path) -> Dict[str, Any]:
    arr = np.load(path, allow_pickle=True)
    if isinstance(arr, np.ndarray) and arr.dtype == object:
        obj = arr.item() if arr.ndim == 0 else arr
        if isinstance(obj, dict):
            w = np.asarray(obj.get("W", obj.get("weights")), dtype=float)
            b = obj.get("b")
            return build_egt_payload(
                w,
                None if b is None else np.asarray(b, dtype=float),
                name=path.stem,
            )
    if isinstance(arr, np.ndarray) and arr.ndim == 2:
        return build_egt_payload(arr, name=path.stem)
    raise ValueError(f"Unsupported .npy weight layout in {path} (expected 2-D array or dict)")


def import_weights(
    path: Union[str, Path],
    *,
    layer_index: int = -1,
    backend: str = "digital_linear",
) -> Dict[str, Any]:
    """Load weights from .onnx, .egt-weights, .json, or .npy and return egt payload."""
    path = Path(path)
    suffix = path.suffix.lower()
    if suffix == ".onnx":
        layers = extract_linear_layers_from_onnx(path)
        if not layers:
            raise ValueError(f"No linear layers found in ONNX model {path}")
        layer = layers[layer_index]
        meta = {"source": str(path), "onnx_layer": layer["name"], "op": layer["op"]}
        return build_egt_payload(
            layer["W"],
            layer.get("b"),
            name=path.stem,
            backend=backend,
            metadata=meta,
        )
    if suffix in (".egt-weights", ".json"):
        with path.open(encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            raise ValueError(f"Expected JSON object in {path}")
        payload = _parse_simple_json(data, path)
        _validate_payload(payload, path)
        return payload
    if suffix == ".npy":
        payload = _load_npy_weights(path)
        _validate_payload(payload, path)
        return payload
    raise ValueError(f"Unsupported weight file type: {suffix}")


def _validate_payload(payload: Dict[str, Any], path: Path) -> None:
    if payload.get("format") != EGT_FORMAT:
        raise ValueError(f"Unsupported weights format in {path}")
    w = np.asarray(payload["weights"]["W"], dtype=float)
    embedding_dim = int(payload["embedding_dim"])
    output_dim = int(payload["output_dim"])
    if w.shape != (output_dim, embedding_dim):
        raise ValueError(
            f"W shape {w.shape} != ({output_dim}, {embedding_dim}) in {path}"
        )


def export_egt_weights(payload: Dict[str, Any], path: Union[str, Path]) -> Path:
    """Write payload to a .egt-weights JSON file."""
    path = Path(path)
    _validate_payload(payload, path)
    with path.open("w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
        f.write("\n")
    return path


def preview_weight_shapes(payload: Dict[str, Any]) -> List[Tuple[str, Tuple[int, ...]]]:
    """Return human-readable (name, shape) tuples for UI preview."""
    rows: List[Tuple[str, Tuple[int, ...]]] = []
    weights = payload.get("weights", {})
    for key in ("W", "b", "W1", "b1", "W2", "b2", "L"):
        if key in weights and weights[key] is not None:
            arr = np.asarray(weights[key], dtype=float)
            rows.append((key, tuple(arr.shape)))
    rows.append(("embedding_dim", (int(payload.get("embedding_dim", 0)),)))
    rows.append(("output_dim", (int(payload.get("output_dim", 0)),)))
    return rows
