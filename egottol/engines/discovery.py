import asyncio
import aiohttp
import logging
from typing import Dict, Any, List, Optional
from urllib.parse import urlparse

logger = logging.getLogger(__name__)

MESH_KERNELS: List[str] = [
    "analog_matmul",
    "crossbar_solve",
    "mzi_mesh_fft",
    "spike_batch",
]


class ServiceDiscovery:
    """Detects and monitors deepiri infrastructure services (zepGPU, GPU mesh, UQE)."""
    
    DEFAULT_ENDPOINTS = {
        "zepgpu": "http://localhost:8000/api/v1/health",
        "gpu_mesh": "http://localhost:8000/api/v1/mesh/health",
        "uqe": "http://localhost:8080/api/v1/health",  # Assuming standard UQE server port
    }

    def __init__(self):
        self.status = {svc: "offline" for svc in self.DEFAULT_ENDPOINTS}
        self.specs = {svc: {} for svc in self.DEFAULT_ENDPOINTS}

    async def probe_services(self):
        """Asynchronously probes all services for connectivity and specs."""
        async with aiohttp.ClientSession() as session:
            tasks = [self._probe_service(session, svc, url) 
                     for svc, url in self.DEFAULT_ENDPOINTS.items()]
            await asyncio.gather(*tasks)

    async def _probe_service(self, session, svc, url):
        try:
            async with session.get(url, timeout=2.0) as response:
                if response.status == 200:
                    data = await response.json()
                    self.status[svc] = "online"
                    self.specs[svc] = data.get("specs", data)
                    logger.info(f"Service {svc} detected: ONLINE")
                else:
                    self.status[svc] = "degraded"
        except Exception:
            self.status[svc] = "offline"

    def is_available(self, service: str) -> bool:
        return self.status.get(service) == "online"

    def get_manifest(self) -> Dict[str, Any]:
        return {
            "status": self.status,
            "specs": self.specs,
            "kernels": self.list_kernels(),
        }

    @staticmethod
    def list_kernels() -> List[str]:
        return list(MESH_KERNELS)

    def get_service_base_url(self, service: str) -> Optional[str]:
        endpoint = self.DEFAULT_ENDPOINTS.get(service)
        if not endpoint:
            return None
        parsed = urlparse(endpoint)
        if not parsed.scheme or not parsed.netloc:
            return None
        return f"{parsed.scheme}://{parsed.netloc}"
