"""Community GPU mesh client for analog compute kernels."""

from __future__ import annotations

import asyncio
import base64
import logging
from typing import Any, Callable, Dict, List, Optional, Sequence, Tuple

import aiohttp
import cloudpickle
import numpy as np

from egottol.engines.discovery import ServiceDiscovery

logger = logging.getLogger(__name__)

MESH_KERNELS: Tuple[str, ...] = (
    "analog_matmul",
    "crossbar_solve",
    "mzi_mesh_fft",
    "spike_batch",
)


def _pickle_b64(obj: Any) -> str:
    return base64.b64encode(cloudpickle.dumps(obj)).decode("ascii")


def _unpickle_b64(data: str) -> Any:
    return cloudpickle.loads(base64.b64decode(data.encode("ascii")))


def _local_array_module():
    try:
        import cupy as cp

        return cp
    except ImportError:
        return np


class GPUMeshClient:
    """Submit analog compute tasks to zepGPU / community volunteer nodes."""

    TASKS_PATH = "/api/v1/tasks"

    def __init__(
        self,
        discovery: Optional[ServiceDiscovery] = None,
        volunteer_url: Optional[str] = None,
        prefer_remote: bool = True,
    ):
        self.discovery = discovery or ServiceDiscovery()
        self.volunteer_url = volunteer_url.rstrip("/") if volunteer_url else None
        self.prefer_remote = prefer_remote
        self.connected_nodes: List[str] = []
        self.last_backend: str = "numpy"

    @staticmethod
    def list_kernels() -> List[str]:
        return list(MESH_KERNELS)

    def mesh_status(self) -> Dict[str, Any]:
        zep = self.discovery.is_available("zepgpu")
        mesh = self.discovery.is_available("gpu_mesh")
        return {
            "zepgpu": self.discovery.status.get("zepgpu", "offline"),
            "gpu_mesh": self.discovery.status.get("gpu_mesh", "offline"),
            "available": zep or mesh or bool(self.volunteer_url),
            "kernels": self.list_kernels(),
            "connected_nodes": list(self.connected_nodes),
            "last_backend": self.last_backend,
        }

    def _tasks_url(self) -> Optional[str]:
        if self.volunteer_url:
            return f"{self.volunteer_url}{self.TASKS_PATH}"
        if self.discovery.is_available("gpu_mesh"):
            base = self.discovery.get_service_base_url("gpu_mesh")
            if base:
                return f"{base}{self.TASKS_PATH}"
        if self.discovery.is_available("zepgpu"):
            base = self.discovery.get_service_base_url("zepgpu")
            if base:
                return f"{base}{self.TASKS_PATH}"
        return None

    async def refresh_nodes(self) -> List[str]:
        await self.discovery.probe_services()
        nodes: List[str] = []
        manifest = self.discovery.get_manifest()
        for svc, status in manifest.get("status", {}).items():
            if status == "online" and svc in ("zepgpu", "gpu_mesh"):
                spec = manifest.get("specs", {}).get(svc, {})
                node_id = spec.get("node_id") or spec.get("device") or svc
                nodes.append(str(node_id))
        if self.volunteer_url:
            nodes.append(self.volunteer_url)
        self.connected_nodes = nodes
        return nodes

    async def submit_task_async(
        self,
        task_type: str,
        kernel: Callable[..., Any],
        args: Sequence[Any] = (),
        kwargs: Optional[Dict[str, Any]] = None,
    ) -> Any:
        if task_type not in MESH_KERNELS:
            raise ValueError(f"Unknown task type {task_type!r}; expected one of {MESH_KERNELS}")

        kwargs = kwargs or {}
        url = self._tasks_url() if self.prefer_remote else None
        if url:
            payload = {
                "task_type": task_type,
                "kernel": _pickle_b64(kernel),
                "args": _pickle_b64(tuple(args)),
                "kwargs": _pickle_b64(kwargs),
            }
            try:
                async with aiohttp.ClientSession() as session:
                    async with session.post(url, json=payload, timeout=30.0) as response:
                        if response.status == 200:
                            data = await response.json()
                            if "result" in data:
                                result = data["result"]
                                if isinstance(result, str):
                                    self.last_backend = "remote"
                                    return _unpickle_b64(result)
                                self.last_backend = "remote"
                                return np.asarray(result)
                        logger.warning(
                            "GPU mesh task %s failed (%s); falling back locally",
                            task_type,
                            response.status,
                        )
            except Exception as exc:
                logger.warning("GPU mesh submit failed (%s); falling back locally", exc)

        return self._run_local(kernel, *args, **kwargs)

    def submit_task(
        self,
        task_type: str,
        kernel: Callable[..., Any],
        args: Sequence[Any] = (),
        kwargs: Optional[Dict[str, Any]] = None,
    ) -> Any:
        return asyncio.run(self.submit_task_async(task_type, kernel, args, kwargs))

    def _run_local(self, kernel: Callable[..., Any], *args: Any, **kwargs: Any) -> Any:
        xp = _local_array_module()
        self.last_backend = "cupy" if xp.__name__ == "cupy" else "numpy"
        return kernel(*args, **kwargs)

    def solve_crossbar(self, G: np.ndarray, V: np.ndarray) -> np.ndarray:
        g = np.asarray(G, dtype=float)
        v = np.asarray(V, dtype=float).reshape(-1)
        if v.size < g.shape[0]:
            v = np.pad(v, (0, g.shape[0] - v.size))
        v = v[: g.shape[0]]

        def crossbar_kernel(conductance: np.ndarray, voltages: np.ndarray) -> np.ndarray:
            xp = _local_array_module()
            g_arr = xp.asarray(conductance)
            v_arr = xp.asarray(voltages)
            out = g_arr @ v_arr
            if xp.__name__ == "cupy":
                return xp.asnumpy(out)
            return np.asarray(out)

        return np.asarray(
            self.submit_task("crossbar_solve", crossbar_kernel, (g, v)),
            dtype=float,
        )

    def analog_matmul(self, A: np.ndarray, B: np.ndarray) -> np.ndarray:
        a = np.asarray(A, dtype=float)
        b = np.asarray(B, dtype=float)

        def matmul_kernel(left: np.ndarray, right: np.ndarray) -> np.ndarray:
            xp = _local_array_module()
            out = xp.asarray(left) @ xp.asarray(right)
            if xp.__name__ == "cupy":
                return xp.asnumpy(out)
            return np.asarray(out)

        return np.asarray(self.submit_task("analog_matmul", matmul_kernel, (a, b)), dtype=float)

    def mzi_mesh_fft(self, signal: np.ndarray) -> np.ndarray:
        x = np.asarray(signal, dtype=complex)

        def fft_kernel(values: np.ndarray) -> np.ndarray:
            xp = _local_array_module()
            arr = xp.asarray(values)
            out = xp.fft.fft(arr)
            if xp.__name__ == "cupy":
                return xp.asnumpy(out)
            return np.asarray(out)

        return np.asarray(self.submit_task("mzi_mesh_fft", fft_kernel, (x,)), dtype=complex)

    def spike_batch(
        self,
        membrane: np.ndarray,
        threshold: float,
        reset: float = 0.0,
    ) -> np.ndarray:
        m = np.asarray(membrane, dtype=float)

        def spike_kernel(values: np.ndarray, thr: float, rst: float) -> np.ndarray:
            xp = _local_array_module()
            arr = xp.asarray(values)
            spikes = (arr >= thr).astype(xp.float64)
            arr = xp.where(arr >= thr, rst, arr)
            if xp.__name__ == "cupy":
                return xp.asnumpy(spikes)
            return np.asarray(spikes)

        return np.asarray(
            self.submit_task("spike_batch", spike_kernel, (m, threshold, reset)),
            dtype=float,
        )
