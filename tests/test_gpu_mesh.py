"""Tests for community GPU mesh client and crossbar integration."""

from __future__ import annotations

import asyncio
from unittest.mock import AsyncMock, MagicMock, patch

import cloudpickle
import numpy as np
import pytest

from egottol.engines.analog_compute.crossbar import CrossbarConfig, CrossbarEngine
from egottol.engines.discovery import MESH_KERNELS, ServiceDiscovery
from egottol.engines.gpu_mesh import GPUMeshClient, _pickle_b64, _unpickle_b64


def test_discovery_lists_mesh_kernels():
    assert ServiceDiscovery.list_kernels() == MESH_KERNELS
    manifest = ServiceDiscovery().get_manifest()
    assert "kernels" in manifest
    assert manifest["kernels"] == MESH_KERNELS


def test_pickle_roundtrip():
    payload = {"a": np.array([1.0, 2.0])}
    assert _unpickle_b64(_pickle_b64(payload))["a"].tolist() == [1.0, 2.0]


def test_solve_crossbar_local_numpy():
    client = GPUMeshClient(prefer_remote=False)
    g = np.array([[1.0, 0.0], [0.0, 2.0]])
    v = np.array([3.0, 4.0])
    out = client.solve_crossbar(g, v)
    assert out.shape == (2,)
    assert out[0] == pytest.approx(3.0)
    assert out[1] == pytest.approx(8.0)
    assert client.last_backend in ("numpy", "cupy")


def test_analog_matmul_local():
    client = GPUMeshClient(prefer_remote=False)
    a = np.ones((2, 3))
    b = np.ones((3, 2))
    out = client.analog_matmul(a, b)
    assert out.shape == (2, 2)
    assert np.all(out == 3.0)


def test_mzi_mesh_fft_local():
    client = GPUMeshClient(prefer_remote=False)
    x = np.array([1.0, 0.0, -1.0, 0.0])
    out = client.mzi_mesh_fft(x)
    assert out.shape == (4,)
    assert np.allclose(out, np.fft.fft(x))


def test_spike_batch_local():
    client = GPUMeshClient(prefer_remote=False)
    membrane = np.array([0.5, 1.2, -0.1])
    spikes = client.spike_batch(membrane, threshold=1.0)
    assert spikes.tolist() == [0.0, 1.0, 0.0]


def test_submit_task_remote_success():
    discovery = ServiceDiscovery()
    discovery.status["zepgpu"] = "online"
    client = GPUMeshClient(discovery=discovery, prefer_remote=True)

    expected = np.array([7.0, 8.0])
    response_payload = {"result": _pickle_b64(expected)}

    mock_response = AsyncMock()
    mock_response.status = 200
    mock_response.json = AsyncMock(return_value=response_payload)

    mock_post_ctx = AsyncMock()
    mock_post_ctx.__aenter__.return_value = mock_response
    mock_post_ctx.__aexit__.return_value = None

    mock_session = MagicMock()
    mock_session.post.return_value = mock_post_ctx
    mock_session.__aenter__ = AsyncMock(return_value=mock_session)
    mock_session.__aexit__ = AsyncMock(return_value=None)

    with patch("egottol.engines.gpu_mesh.aiohttp.ClientSession", return_value=mock_session):
        result = asyncio.run(
            client.submit_task_async(
                "crossbar_solve",
                lambda g, v: g @ v,
                (np.eye(2), np.array([7.0, 8.0])),
            )
        )

    assert np.allclose(result, expected)
    assert client.last_backend == "remote"
    mock_session.post.assert_called_once()
    call_kwargs = mock_session.post.call_args.kwargs
    assert "json" in call_kwargs
    assert call_kwargs["json"]["task_type"] == "crossbar_solve"
    assert "kernel" in call_kwargs["json"]


def test_submit_task_falls_back_on_remote_failure():
    discovery = ServiceDiscovery()
    discovery.status["gpu_mesh"] = "online"
    client = GPUMeshClient(discovery=discovery, prefer_remote=True)

    mock_response = AsyncMock()
    mock_response.status = 503

    mock_post_ctx = AsyncMock()
    mock_post_ctx.__aenter__.return_value = mock_response
    mock_post_ctx.__aexit__.return_value = None

    mock_session = MagicMock()
    mock_session.post.return_value = mock_post_ctx
    mock_session.__aenter__ = AsyncMock(return_value=mock_session)
    mock_session.__aexit__ = AsyncMock(return_value=None)

    with patch("egottol.engines.gpu_mesh.aiohttp.ClientSession", return_value=mock_session):
        result = asyncio.run(
            client.submit_task_async(
                "analog_matmul",
                lambda a, b: a @ b,
                (np.array([[2.0]]), np.array([[3.0]])),
            )
        )

    assert result == pytest.approx(6.0)
    assert client.last_backend in ("numpy", "cupy")


def test_volunteer_url_used_for_tasks():
    client = GPUMeshClient(volunteer_url="http://volunteer:9000", prefer_remote=True)
    assert client._tasks_url() == "http://volunteer:9000/api/v1/tasks"


def test_crossbar_engine_use_gpu():
    g = np.array([[1.0, 0.0], [0.0, 1.0]])
    mock_client = MagicMock()
    mock_client.solve_crossbar.return_value = np.array([1.0, 2.0])
    engine = CrossbarEngine(
        rows=2,
        cols=2,
        conductance=g,
        config=CrossbarConfig(rows=2, cols=2, ir_drop=0.0),
        use_gpu=True,
        gpu_client=mock_client,
    )
    out = engine.solve(np.array([1.0, 2.0]))
    mock_client.solve_crossbar.assert_called_once()
    assert out.tolist() == [1.0, 2.0]


def test_crossbar_engine_cpu_matches_gpu_stub():
    rng = np.random.default_rng(0)
    g = rng.uniform(1e-6, 1e-3, (4, 4))
    v = rng.uniform(0.0, 1.0, 4)
    cpu = CrossbarEngine(rows=4, cols=4, conductance=g, use_gpu=False, rng=rng)
    gpu = CrossbarEngine(rows=4, cols=4, conductance=g, use_gpu=True, gpu_client=GPUMeshClient(prefer_remote=False), rng=rng)
    assert np.allclose(cpu.solve(v), gpu.solve(v))
