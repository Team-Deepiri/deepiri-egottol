"""Dock panel for community GPU mesh status and controls."""

from __future__ import annotations

import asyncio
from typing import Callable, Optional

from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtWidgets import (
    QCheckBox,
    QDockWidget,
    QFrame,
    QHBoxLayout,
    QLabel,
    QListWidget,
    QVBoxLayout,
    QWidget,
)

from egottol.engines.discovery import ServiceDiscovery
from egottol.engines.gpu_mesh import GPUMeshClient

_STATUS_COLORS = {
    "online": "#50fa7b",
    "degraded": "#ffb86c",
    "offline": "#ff5555",
}


class GPUMeshPanel(QWidget):
    """Small status panel for the community GPU mesh."""

    def __init__(
        self,
        discovery: Optional[ServiceDiscovery] = None,
        client: Optional[GPUMeshClient] = None,
        on_community_toggle: Optional[Callable[[bool], None]] = None,
        parent: Optional[QWidget] = None,
    ):
        super().__init__(parent)
        self.discovery = discovery or ServiceDiscovery()
        self.client = client or GPUMeshClient(discovery=self.discovery)
        self._on_community_toggle = on_community_toggle

        self.setStyleSheet("background:#12121e;color:#f8f8f2;font-family:monospace;")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(6, 6, 6, 6)

        title = QLabel("Community GPU Mesh")
        title.setStyleSheet("color:#bd93f9;font-weight:bold;")
        layout.addWidget(title)

        self._mesh_led = QFrame()
        self._mesh_led.setFixedSize(12, 12)
        self._mesh_led.setStyleSheet("background:gray;border-radius:6px;")
        self._mesh_label = QLabel("Mesh: offline")
        status_row = QHBoxLayout()
        status_row.addWidget(self._mesh_led)
        status_row.addWidget(self._mesh_label)
        status_row.addStretch()
        layout.addLayout(status_row)

        self._backend_label = QLabel("Backend: numpy")
        self._backend_label.setStyleSheet("color:#6272a4;")
        layout.addWidget(self._backend_label)

        layout.addWidget(QLabel("Connected nodes:"))
        self._nodes = QListWidget()
        self._nodes.setStyleSheet(
            "QListWidget{background:#1a1b2e;border:1px solid #44475a;color:#f8f8f2;}"
        )
        self._nodes.setMaximumHeight(120)
        layout.addWidget(self._nodes)

        self._kernels_label = QLabel("")
        self._kernels_label.setWordWrap(True)
        self._kernels_label.setStyleSheet("color:#6272a4;font-size:10px;")
        layout.addWidget(self._kernels_label)

        self._community_cb = QCheckBox("Run on community GPU")
        self._community_cb.setStyleSheet("color:#f8f8f2;")
        self._community_cb.toggled.connect(self._handle_community_toggle)
        layout.addWidget(self._community_cb)

        layout.addStretch()
        self.refresh()

    def use_community_gpu(self) -> bool:
        return self._community_cb.isChecked()

    def set_use_community_gpu(self, enabled: bool) -> None:
        self._community_cb.setChecked(enabled)

    def _handle_community_toggle(self, checked: bool) -> None:
        self.client.prefer_remote = checked
        if self._on_community_toggle:
            self._on_community_toggle(checked)

    def refresh(self) -> None:
        try:
            asyncio.run(self.client.refresh_nodes())
        except Exception:
            pass

        status = self.client.mesh_status()
        mesh_state = status.get("gpu_mesh", "offline")
        zep_state = status.get("zepgpu", "offline")
        overall = "online" if status.get("available") else mesh_state
        if overall != "online" and zep_state == "online":
            overall = "online"

        color = _STATUS_COLORS.get(str(overall), "gray")
        self._mesh_led.setStyleSheet(f"background:{color};border-radius:6px;")
        self._mesh_label.setText(f"Mesh: {overall}")
        self._backend_label.setText(f"Backend: {status.get('last_backend', 'numpy')}")

        self._nodes.clear()
        nodes = status.get("connected_nodes") or []
        if nodes:
            for node in nodes:
                self._nodes.addItem(str(node))
        else:
            self._nodes.addItem("(no nodes discovered)")

        kernels = ", ".join(status.get("kernels", []))
        self._kernels_label.setText(f"Kernels: {kernels}")


class GPUMeshDockWidget(QDockWidget):
    """Dock wrapper for the GPU mesh panel."""

    def __init__(
        self,
        discovery: Optional[ServiceDiscovery] = None,
        on_community_toggle: Optional[Callable[[bool], None]] = None,
        parent: Optional[QWidget] = None,
    ):
        super().__init__("GPU Mesh", parent)
        self.panel = GPUMeshPanel(
            discovery=discovery,
            on_community_toggle=on_community_toggle,
            parent=self,
        )
        self.setWidget(self.panel)
        self.setAllowedAreas(
            Qt.DockWidgetArea.LeftDockWidgetArea | Qt.DockWidgetArea.RightDockWidgetArea
        )

        self._timer = QTimer(self)
        self._timer.timeout.connect(self.panel.refresh)
        self._timer.start(8000)

    def use_community_gpu(self) -> bool:
        return self.panel.use_community_gpu()

    def refresh(self) -> None:
        self.panel.refresh()
