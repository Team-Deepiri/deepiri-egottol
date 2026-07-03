"""Tests for ONNX/simple weight import (JSON roundtrip without ONNX installed)."""

import json
from pathlib import Path

import numpy as np
import pytest

from egottol.engines.ai.onnx_import import (
    EGT_FORMAT,
    build_egt_payload,
    export_egt_weights,
    import_weights,
    onnx_available,
    preview_weight_shapes,
)
from egottol.engines.ai.weight_loader import apply_weights_to_engine, load_weights
from egottol.engines.eii.inference import InferenceEngine
from egottol.models.eii import EIIPipelineConfig


def test_build_and_export_json_roundtrip(tmp_path: Path):
    w = np.array([[1.0, 0.5, -0.2], [0.0, 1.0, 0.3]], dtype=float)
    b = np.array([0.1, -0.1], dtype=float)
    payload = build_egt_payload(w, b, name="test-readout", backend="digital_softmax", head="softmax")

    out_path = tmp_path / "roundtrip.egt-weights"
    export_egt_weights(payload, out_path)

    loaded = load_weights(out_path)
    assert loaded["format"] == EGT_FORMAT
    assert loaded["backend"] == "digital_softmax"
    np.testing.assert_allclose(loaded["weights"]["W"], w)
    np.testing.assert_allclose(loaded["weights"]["b"], b)

    reimported = import_weights(out_path)
    assert reimported["embedding_dim"] == 3
    assert reimported["output_dim"] == 2


def test_import_simple_json_without_format(tmp_path: Path):
    raw = {
        "W": [[2.0, 0.0], [0.0, 3.0]],
        "b": [0.5, -0.5],
        "backend": "digital_linear",
    }
    path = tmp_path / "simple.json"
    path.write_text(json.dumps(raw), encoding="utf-8")

    payload = import_weights(path)
    assert payload["format"] == EGT_FORMAT
    assert payload["embedding_dim"] == 2
    assert payload["output_dim"] == 2
    shapes = dict(preview_weight_shapes(payload))
    assert shapes["W"] == (2, 2)
    assert shapes["b"] == (2,)


def test_import_npy_matrix(tmp_path: Path):
    w = np.eye(4)
    path = tmp_path / "W.npy"
    np.save(path, w)

    payload = import_weights(path)
    assert payload["output_dim"] == 4
    assert payload["embedding_dim"] == 4
    np.testing.assert_allclose(payload["weights"]["W"], w)


def test_apply_imported_weights_to_engine(tmp_path: Path):
    payload = build_egt_payload(np.eye(2), np.zeros(2))
    path = tmp_path / "engine.egt-weights"
    export_egt_weights(payload, path)

    pipeline = EIIPipelineConfig()
    engine = InferenceEngine(
        pipeline.inference,
        pipeline.default_weights(),
        pipeline.default_bias(),
        pipeline.default_conductance(),
    )
    apply_weights_to_engine(engine, load_weights(path))
    probs, conf = engine.infer(np.array([1.0, 0.0]))
    assert probs.shape[0] == 2
    assert conf >= 0.0


@pytest.mark.skipif(not onnx_available(), reason="onnx not installed")
def test_onnx_extract_if_available(tmp_path: Path):
    pytest.importorskip("onnx")
    from onnx import TensorProto, helper, numpy_helper

    w = np.array([[1.0, 0.0], [0.0, 1.0]], dtype=np.float32)
    b = np.array([0.1, 0.2], dtype=np.float32)
    w_init = numpy_helper.from_array(w, name="linear_weight")
    b_init = numpy_helper.from_array(b, name="linear_bias")
    node = helper.make_node("Gemm", ["input", "linear_weight", "linear_bias"], ["output"], transB=1)
    graph = helper.make_graph(
        [node],
        "test_linear",
        [helper.make_tensor_value_info("input", TensorProto.FLOAT, [None, 2])],
        [helper.make_tensor_value_info("output", TensorProto.FLOAT, [None, 2])],
        [w_init, b_init],
    )
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 13)])
    onnx_path = tmp_path / "linear.onnx"
    import onnx

    onnx.save(model, str(onnx_path))

    payload = import_weights(onnx_path)
    assert payload["output_dim"] == 2
    assert payload["embedding_dim"] == 2
    np.testing.assert_allclose(payload["weights"]["W"], w, rtol=1e-5)
    np.testing.assert_allclose(payload["weights"]["b"], b, rtol=1e-5)
