import sys
import asyncio
import pyqtgraph as pg
from PyQt6.QtWidgets import (QApplication, QMainWindow, QGraphicsView, QGraphicsScene, 
                             QVBoxLayout, QHBoxLayout, QWidget, QToolBar, QListWidget, 
                             QDockWidget, QLabel, QPushButton, QFrame)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QAction, QColor

from egottol.engines.discovery import ServiceDiscovery
from egottol.engines.solver import AdvancedMNASolver

class IntegrationStatusPanel(QWidget):
    """LED-style status indicators for infrastructure connectivity."""
    def __init__(self, discovery: ServiceDiscovery):
        super().__init__()
        self.discovery = discovery
        self.layout = QVBoxLayout(self)
        self.indicators = {}
        self._init_ui()

    def _init_ui(self):
        for svc in self.discovery.DEFAULT_ENDPOINTS:
            container = QHBoxLayout()
            label = QLabel(f"{svc.upper()}:")
            status_led = QFrame()
            status_led.setFixedSize(12, 12)
            status_led.setStyleSheet("background-color: gray; border-radius: 6px;")
            
            container.addWidget(label)
            container.addWidget(status_led)
            self.layout.addLayout(container)
            self.indicators[svc] = status_led

    def update_status(self):
        for svc, status in self.discovery.status.items():
            color = "gray"
            if status == "online": color = "green"
            elif status == "degraded": color = "orange"
            elif status == "offline": color = "red"
            self.indicators[svc].setStyleSheet(f"background-color: {color}; border-radius: 6px;")

class EgottolApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("deepiri-egottol — Infrastructure-Aware GUI")
        self.resize(1600, 1000)
        self.setStyle("Fusion")

        # Core Engines
        self.discovery = ServiceDiscovery()
        self.solver = None

        # UI Setup
        self._setup_ui()
        
        # Real-time discovery timer
        self.timer = QTimer(self)
        self.timer.timeout.connect(self._run_discovery_task)
        self.timer.start(5000) # Every 5s
        self._run_discovery_task()

    def _setup_ui(self):
        self.central = QGraphicsView(QGraphicsScene())
        self.setCentralWidget(self.central)

        # Integration Dock
        self.status_panel = IntegrationStatusPanel(self.discovery)
        dock = QDockWidget("Integration Status", self)
        dock.setWidget(self.status_panel)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, dock)

        # Component Library
        self.palette = QListWidget()
        from egottol.models.registry import COMPONENT_LIBRARY
        self.palette.addItems(COMPONENT_LIBRARY.keys())
        comp_dock = QDockWidget("Components", self)
        comp_dock.setWidget(self.palette)
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, comp_dock)

        # Plotter
        self.plotter = pg.PlotWidget(title="Real-time Signal Analysis")
        plot_dock = QDockWidget("Waveform Viewer", self)
        plot_dock.setWidget(self.plotter)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, plot_dock)

        self._setup_toolbar()

    def _setup_toolbar(self):
        tb = self.addToolBar("Simulate")
        run_act = QAction("▶ RUN (SMART ROUTE)", self)
        run_act.triggered.connect(self._smart_run)
        tb.addAction(run_act)

    def _smart_run(self):
        """Intelligently routes simulation based on service availability."""
        if self.discovery.is_available("zepgpu"):
            print("🚀 Offloading heavy matrix to zepGPU...")
            # gpu_solver.solve(...)
        else:
            print("💻 Running simulation on local CPU...")
            # solver.solve_dc(...)

    def _run_discovery_task(self):
        """Spawns asynchronous discovery probe."""
        asyncio.run(self.discovery.probe_services())
        self.status_panel.update_status()

def main():
    app = QApplication(sys.argv)
    window = EgottolApp()
    window.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
