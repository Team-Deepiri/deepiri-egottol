import asyncio
import aiohttp
import logging
from typing import Dict, Any

logger = logging.getLogger(__name__)

class ServiceDiscovery:
    """Detects and monitors deepiri infrastructure services (zepGPU, UQE)."""
    
    DEFAULT_ENDPOINTS = {
        "zepgpu": "http://localhost:8000/api/v1/health",
        "uqe": "http://localhost:8080/api/v1/health" # Assuming standard UQE server port
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
            "specs": self.specs
        }
