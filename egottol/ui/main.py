import sys
import uuid
import asyncio
import pyqtgraph as pg
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QGraphicsView, QGraphicsScene,
    QGraphicsItem, QGraphicsRectItem, QGraphicsEllipseItem,
    QGraphicsLineItem, QGraphicsTextItem, QGraphicsPathItem,
    QVBoxLayout, QHBoxLayout, QWidget, QToolBar, QListWidget,
    QDockWidget, QLabel, QPushButton, QFrame, QStyleFactory,
    QSplitter, QTextEdit, QListWidgetItem, QAbstractItemView,
    QGraphicsProxyWidget, QStatusBar, QDialog, QFormLayout,
    QLineEdit, QDialogButtonBox
)
from PyQt6.QtCore import Qt, QTimer, QPointF, QRectF, QLineF
from PyQt6.QtGui import (
    QAction, QColor, QPen, QBrush, QPainter, QPainterPath,
    QFont, QCursor, QTransform
)

from egottol.engines.discovery import ServiceDiscovery
from egottol.models.registry import COMPONENT_LIBRARY
from egottol.models.base import Circuit, Component, Wire, ComponentType, Port

GRID = 20          # grid snap size in pixels
COLORS = {
    "bg":        QColor("#1e1e2e"),
    "grid_dot":  QColor("#3a3a5c"),
    "grid_line": QColor("#2a2a42"),
    "wire":      QColor("#50fa7b"),
    "component": QColor("#8be9fd"),
    "port":      QColor("#ffb86c"),
    "selected":  QColor("#ff79c6"),
    "text":      QColor("#f8f8f2"),
    "label":     QColor("#bd93f9"),
    "gnd":       QColor("#ff5555"),
    "vcc":       QColor("#f1fa8c"),
}

SYMBOLS = {
    # key -> list of draw commands: ("line", x1,y1,x2,y2) | ("arc", x,y,w,h,start,span) | ("text", x,y,txt)
    "R": [
        ("line", 0,0, 0,10), ("rect", -8,10,16,30), ("line", 0,40, 0,50),
        ("text", -4, 55, "R"),
    ],
    "C": [
        ("line", 0,0, 0,20), ("line", -12,20,12,20),
        ("line", -12,26,12,26), ("line", 0,26, 0,50),
        ("text", -4, 55, "C"),
    ],
    "L": [
        ("line", 0,0, 0,10),
        ("arc", -8,10,16,10, 0, 180), ("arc", -8,20,16,10, 0, 180),
        ("arc", -8,30,16,10, 0, 180),
        ("line", 0,40, 0,50),
        ("text", -4, 55, "L"),
    ],
    "VSRC": [
        ("line", 0,0, 0,10), ("circle", -15,10,30,30),
        ("text", -5,20, "+"), ("text", -5,32, "−"),
        ("line", 0,40, 0,50), ("text", -6,55, "V"),
    ],
    "GND": [
        ("line", 0,0, 0,15), ("line", -15,15,15,15),
        ("line", -10,21,10,21), ("line", -5,27,5,27),
        ("text", -5, 35, "GND"),
    ],
    "VCC": [
        ("line", 0,20, 0,0), ("text", -8,-12, "VCC"),
        ("line", -12,20,12,20),
    ],
    "Q_NPN": [
        ("line", 0,0, 0,15), ("line", 0,15,-20,5), ("line", 0,15,-20,25),
        ("line", -20,0,-20,30), ("line", -20,15,-35,15),
        ("arrow", -20,25,0,35),
        ("text", -35,35, "Q"),
    ],
    "OPAMP": [
        ("line", 0,10, 0,15), ("line", 0,30, 0,35),
        ("triangle", -5,15, 25,22, -5,30),
        ("line", -5,17, 3,17), ("line", -5,28, 3,28), ("line", -5,22, 1,22),
        ("line", 25,22, 35,22),
        ("text", -2, 42, "A"),
    ],
    "GATE_AND": [
        ("line", -20,0,-20,40), ("line", -20,0,0,0),
        ("line", -20,40,0,40),
        ("arc", 0,0,20,40, -90, 180),
        ("line", -35,10,-20,10), ("line", -35,30,-20,30),
        ("line", 20,20,35,20),
        ("text", -8, 48, "AND"),
    ],
    "GATE_XOR": [
        ("arc", -25,0,20,40, -90, 180),
        ("line", -20,0,-5,0), ("line", -20,40,-5,40),
        ("arc", -5,0,20,40, -90, 180),
        ("line", -35,10,-20,10), ("line", -35,30,-20,30),
        ("line", 20,20,35,20),
        ("text", -8, 48, "XOR"),
    ],
    "DEFAULT": [
        ("rect", -20,0,40,50),
        ("text", -6, 20, "?"),
    ],
}

PORT_OFFSETS = {
    "R":        [(0,0,"1"), (0,50,"2")],
    "C":        [(0,0,"1"), (0,50,"2")],
    "L":        [(0,0,"1"), (0,50,"2")],
    "VSRC":     [(0,0,"+"), (0,50,"−")],
    "GND":      [(0,0,"G")],
    "VCC":      [(0,0,"V")],
    "Q_NPN":    [(0,0,"C"), (-35,15,"B"), (0,50,"E")],
    "OPAMP":    [(0,10,"+"), (0,30,"−"), (35,22,"OUT")],
    "GATE_AND": [(-35,10,"A"), (-35,30,"B"), (35,20,"Q")],
    "GATE_XOR": [(-35,10,"A"), (-35,30,"B"), (35,20,"Q")],
    "DEFAULT":  [(0,0,"1"), (0,50,"2")],
}

SYMBOL_KEY = {
    "R": "R", "CAP": "C", "IND": "L", "VSRC": "VSRC", "GND": "GND", "VCC": "VCC",
    "NPN": "Q_NPN", "OPAMP_0": "OPAMP",
    "AND": "GATE_AND", "XOR": "GATE_XOR",
}

def snap(v):
    return round(v / GRID) * GRID


class SchematicScene(QGraphicsScene):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSceneRect(-2000, -2000, 4000, 4000)
        self.setBackgroundBrush(QBrush(COLORS["bg"]))
        self._placed_components = {}   # id -> ComponentItem
        self._wires = []
        self._pending_wire = None      # (start_item, port_name, QPointF)
        self._wire_preview = None
        self._place_mode = None        # component key to place next
        self._circuit = Circuit(id="main", name="untitled", components=[], wires=[])
        self._node_counter = 0
        self._results = {}             # node -> voltage

    def drawBackground(self, painter, rect):
        super().drawBackground(painter, rect)
        # dot grid
        pen = QPen(COLORS["grid_dot"])
        pen.setWidth(1)
        painter.setPen(pen)
        left = int(rect.left() / GRID) * GRID
        top  = int(rect.top()  / GRID) * GRID
        x = left
        while x <= rect.right():
            y = top
            while y <= rect.bottom():
                painter.drawPoint(int(x), int(y))
                y += GRID
            x += GRID

    def set_place_mode(self, key):
        self._place_mode = key
        self.views()[0].setCursor(Qt.CursorShape.CrossCursor)

    def clear_place_mode(self):
        self._place_mode = None
        self.views()[0].setCursor(Qt.CursorShape.ArrowCursor)

    def mousePressEvent(self, event):
        if self._place_mode and event.button() == Qt.MouseButton.LeftButton:
            pos = event.scenePos()
            sx, sy = snap(pos.x()), snap(pos.y())
            self._drop_component(self._place_mode, sx, sy)
            if not (event.modifiers() & Qt.KeyboardModifier.ShiftModifier):
                self.clear_place_mode()
            return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        super().mouseReleaseEvent(event)

    def _drop_component(self, key, x, y):
        defn = COMPONENT_LIBRARY.get(key)
        if defn is None:
            return
        sym_key = SYMBOL_KEY.get(key, "DEFAULT")
        comp_id = f"{key}_{uuid.uuid4().hex[:6]}"
        item = ComponentItem(comp_id, key, defn, sym_key, x, y, self)
        self.addItem(item)
        self._placed_components[comp_id] = item
        model_comp = Component(
            id=comp_id, name=defn.name, type=defn.category,
            ports=list(defn.ports), parameters=dict(defn.parameters)
        )
        self._circuit.components.append(model_comp)
        return item

    def start_wire(self, comp_id, port_name, scene_pos):
        self._pending_wire = (comp_id, port_name, scene_pos)
        pen = QPen(COLORS["wire"], 2, Qt.PenStyle.DashLine)
        self._wire_preview = self.addLine(QLineF(scene_pos, scene_pos), pen)

    def update_wire_preview(self, scene_pos):
        if self._wire_preview and self._pending_wire:
            start = self._pending_wire[2]
            self._wire_preview.setLine(QLineF(start, scene_pos))

    def finish_wire(self, to_comp_id, to_port_name, scene_pos):
        if not self._pending_wire:
            return
        from_id, from_port, start_pos = self._pending_wire
        if self._wire_preview:
            self.removeItem(self._wire_preview)
            self._wire_preview = None
        self._pending_wire = None
        if from_id == to_comp_id:
            return
        wire_id = f"W_{uuid.uuid4().hex[:6]}"
        pen = QPen(COLORS["wire"], 2)
        line = self.addLine(QLineF(start_pos, scene_pos), pen)
        self._wires.append(line)
        self._circuit.wires.append(Wire(
            id=wire_id,
            from_component=from_id, from_port=from_port,
            to_component=to_comp_id, to_port=to_port_name
        ))

    def cancel_wire(self):
        if self._wire_preview:
            self.removeItem(self._wire_preview)
            self._wire_preview = None
        self._pending_wire = None

    def run_simulation(self):
        from egottol.engines.solver import AdvancedMNASolver
        solver = AdvancedMNASolver(self._circuit)
        try:
            result = solver.solve_dc()
            self._results = result or {}
            return result
        except Exception as e:
            return {"error": str(e)}

    def clear_canvas(self):
        self.clear()
        self._placed_components.clear()
        self._wires.clear()
        self._pending_wire = None
        self._wire_preview = None
        self._circuit = Circuit(id="main", name="untitled", components=[], wires=[])
        self._results = {}


class ComponentItem(QGraphicsItem):
    def __init__(self, comp_id, key, defn, sym_key, x, y, scene):
        super().__init__()
        self.comp_id = comp_id
        self.key = key
        self.defn = defn
        self.sym_key = sym_key
        self._scene = scene
        self.params = dict(defn.parameters)
        self._label = f"{key}\n" + ", ".join(
            f"{k}={v}" for k, v in list(self.params.items())[:2]
        )
        self.setPos(x, y)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsMovable)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsSelectable)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemSendsGeometryChanges)
        self._port_radius = 5
        self._dragging_port = None

    def boundingRect(self):
        return QRectF(-40, -15, 80, 90)

    def paint(self, painter, option, widget=None):
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        selected = self.isSelected()
        body_color = COLORS["selected"] if selected else COLORS["component"]
        cmds = SYMBOLS.get(self.sym_key, SYMBOLS["DEFAULT"])
        pen = QPen(body_color, 2)
        painter.setPen(pen)
        painter.setBrush(Qt.BrushStyle.NoBrush)
        for cmd in cmds:
            t = cmd[0]
            if t == "line":
                painter.drawLine(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
            elif t == "rect":
                painter.drawRect(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
            elif t == "circle":
                painter.drawEllipse(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
            elif t == "arc":
                from PyQt6.QtCore import QRect
                painter.drawArc(QRect(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4])),
                                int(cmd[5])*16, int(cmd[6])*16)
            elif t == "triangle":
                path = QPainterPath()
                path.moveTo(cmd[1], cmd[2])
                path.lineTo(cmd[3], cmd[4])
                path.lineTo(cmd[5], cmd[6])
                path.closeSubpath()
                painter.drawPath(path)
            elif t == "arrow":
                painter.drawLine(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
            elif t == "text":
                painter.setPen(QPen(COLORS["label"]))
                f = QFont("Monospace", 7)
                painter.setFont(f)
                painter.drawText(int(cmd[1]), int(cmd[2]), cmd[3])
                painter.setPen(pen)
        # ports
        ports = PORT_OFFSETS.get(self.sym_key, PORT_OFFSETS["DEFAULT"])
        for px, py, pname in ports:
            painter.setPen(QPen(COLORS["port"], 1))
            painter.setBrush(QBrush(COLORS["port"]))
            r = self._port_radius
            painter.drawEllipse(int(px - r//2), int(py - r//2), r, r)
        # ID label
        painter.setPen(QPen(COLORS["text"]))
        f = QFont("Monospace", 7)
        painter.setFont(f)
        painter.drawText(-20, -5, self.comp_id)

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            port = self._hit_port(event.pos())
            if port:
                name, px, py = port
                scene_pos = self.mapToScene(QPointF(px, py))
                if self._scene._pending_wire:
                    self._scene.finish_wire(self.comp_id, name, scene_pos)
                else:
                    self._scene.start_wire(self.comp_id, name, scene_pos)
                return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        self._scene.update_wire_preview(self.mapToScene(event.pos()))
        super().mouseMoveEvent(event)

    def mouseDoubleClickEvent(self, event):
        dlg = ParamDialog(self.comp_id, self.params)
        if dlg.exec():
            self.params = dlg.result_params
            self._label = f"{self.key}\n" + ", ".join(
                f"{k}={v}" for k, v in list(self.params.items())[:2]
            )
            self.update()

    def _hit_port(self, local_pos):
        ports = PORT_OFFSETS.get(self.sym_key, PORT_OFFSETS["DEFAULT"])
        r = self._port_radius + 4
        for px, py, name in ports:
            if (local_pos.x() - px)**2 + (local_pos.y() - py)**2 <= r*r:
                return name, px, py
        return None

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self._scene.cancel_wire()


class ParamDialog(QDialog):
    def __init__(self, comp_id, params):
        super().__init__()
        self.setWindowTitle(f"Edit {comp_id}")
        layout = QFormLayout(self)
        self._fields = {}
        for k, v in params.items():
            field = QLineEdit(str(v))
            self._fields[k] = field
            layout.addRow(k, field)
        btns = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok |
                                QDialogButtonBox.StandardButton.Cancel)
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        layout.addRow(btns)
        self.result_params = dict(params)

    def accept(self):
        for k, field in self._fields.items():
            try:
                self.result_params[k] = float(field.text())
            except ValueError:
                self.result_params[k] = field.text()
        super().accept()


class SchematicView(QGraphicsView):
    def __init__(self, scene):
        super().__init__(scene)
        self._scene = scene
        self.setRenderHint(QPainter.RenderHint.Antialiasing)
        self.setDragMode(QGraphicsView.DragMode.RubberBandDrag)
        self.setTransformationAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self.setResizeAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self._zoom = 1.0

    def wheelEvent(self, event):
        factor = 1.15 if event.angleDelta().y() > 0 else 1/1.15
        self._zoom *= factor
        self.scale(factor, factor)

    def mouseMoveEvent(self, event):
        pos = self.mapToScene(event.pos())
        self._scene.update_wire_preview(pos)
        super().mouseMoveEvent(event)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self._scene.cancel_wire()
            self._scene.clear_place_mode()
        elif event.key() == Qt.Key.Key_Delete:
            for item in self._scene.selectedItems():
                if isinstance(item, ComponentItem):
                    self._scene._circuit.components = [
                        c for c in self._scene._circuit.components
                        if c.id != item.comp_id
                    ]
                    del self._scene._placed_components[item.comp_id]
                self._scene.removeItem(item)
        super().keyPressEvent(event)

    def contextMenuEvent(self, event):
        self._scene.cancel_wire()
        self._scene.clear_place_mode()


class EgottolApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("deepiri-egottol — Schematic & Simulation")
        self.resize(1600, 950)
        self.setStyle(QStyleFactory.create("Fusion"))
        self._apply_dark_palette()

        self.discovery = ServiceDiscovery()
        self._scene = SchematicScene()
        self._view  = SchematicView(self._scene)

        self._setup_ui()
        self._setup_toolbar()
        self._setup_statusbar()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self._probe_services)
        self.timer.start(8000)

    # ------------------------------------------------------------------ layout

    def _setup_ui(self):
        self.setCentralWidget(self._view)

        # Left: component palette grouped by category
        self._palette = QListWidget()
        self._palette.setMinimumWidth(160)
        self._palette.setMaximumWidth(220)
        self._palette.setStyleSheet("background:#12121e; color:#f8f8f2; font-family:monospace;")
        self._palette.setDragDropMode(QAbstractItemView.DragDropMode.NoDragDrop)
        for key, defn in COMPONENT_LIBRARY.items():
            item = QListWidgetItem(f"  {defn.name}")
            item.setData(Qt.ItemDataRole.UserRole, key)
            item.setForeground(QBrush(COLORS["component"]))
            self._palette.addItem(item)
        self._palette.itemClicked.connect(self._on_palette_click)
        palette_dock = QDockWidget("Components", self)
        palette_dock.setWidget(self._palette)
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, palette_dock)

        # Right: service status
        self._status_panel = self._build_status_panel()
        status_dock = QDockWidget("Services", self)
        status_dock.setWidget(self._status_panel)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, status_dock)

        # Bottom: waveform + console
        self._plotter = pg.PlotWidget(background="#1e1e2e")
        self._plotter.setLabel("left", "Voltage (V)")
        self._plotter.setLabel("bottom", "Node")
        self._plotter.showGrid(x=True, y=True, alpha=0.3)
        self._console = QTextEdit()
        self._console.setReadOnly(True)
        self._console.setStyleSheet("background:#12121e; color:#50fa7b; font-family:monospace; font-size:11px;")
        self._console.setMaximumHeight(140)

        bottom = QSplitter(Qt.Orientation.Horizontal)
        bottom.addWidget(self._plotter)
        bottom.addWidget(self._console)
        bottom.setSizes([900, 400])

        bottom_dock = QDockWidget("Waveform / Console", self)
        bottom_dock.setWidget(bottom)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, bottom_dock)

    def _build_status_panel(self):
        w = QWidget()
        w.setStyleSheet("background:#12121e;")
        layout = QVBoxLayout(w)
        layout.setContentsMargins(6, 6, 6, 6)
        self._leds = {}
        for svc in getattr(self.discovery, "DEFAULT_ENDPOINTS", []):
            row = QHBoxLayout()
            lbl = QLabel(svc.upper())
            lbl.setStyleSheet("color:#f8f8f2; font-family:monospace;")
            led = QFrame()
            led.setFixedSize(12, 12)
            led.setStyleSheet("background-color:gray; border-radius:6px;")
            row.addWidget(lbl)
            row.addWidget(led)
            layout.addLayout(row)
            self._leds[svc] = led
        layout.addStretch()
        return w

    def _setup_toolbar(self):
        tb = self.addToolBar("Main")
        tb.setMovable(False)
        tb.setStyleSheet("background:#2a2a42; color:#f8f8f2; spacing:4px;")

        def act(label, tip, fn):
            a = QAction(label, self)
            a.setToolTip(tip)
            a.triggered.connect(fn)
            tb.addAction(a)

        act("▶ Run DC", "Solve DC operating point", self._run_dc)
        act("⟳ Transient", "Run transient simulation", self._run_transient)
        tb.addSeparator()
        act("+ R", "Place Resistor [shift=multi]", lambda: self._scene.set_place_mode("R"))
        act("+ C", "Place Capacitor", lambda: self._scene.set_place_mode("CAP"))
        act("+ L", "Place Inductor",  lambda: self._scene.set_place_mode("IND"))
        act("+ V", "Place Voltage Source", lambda: self._scene.set_place_mode("VSRC"))
        act("GND", "Place Ground", lambda: self._scene.set_place_mode("GND"))
        act("VCC", "Place VCC",    lambda: self._scene.set_place_mode("VCC"))
        tb.addSeparator()
        act("🗑 Clear", "Clear canvas", self._confirm_clear)
        act("⊖ Zoom–", "Zoom out", lambda: self._view.scale(1/1.2, 1/1.2))
        act("⊕ Zoom+", "Zoom in",  lambda: self._view.scale(1.2, 1.2))
        act("⤢ Fit",   "Fit view",  self._fit)

    def _setup_statusbar(self):
        self._sb = QStatusBar()
        self.setStatusBar(self._sb)
        self._mode_label = QLabel("Mode: SELECT")
        self._mode_label.setStyleSheet("color:#bd93f9; font-family:monospace;")
        self._sb.addPermanentWidget(self._mode_label)
        self._sb.showMessage("Click a component in the palette or toolbar to place. Click a port dot to start a wire.")

    # ------------------------------------------------------------------ actions

    def _on_palette_click(self, item):
        key = item.data(Qt.ItemDataRole.UserRole)
        if key:
            self._scene.set_place_mode(key)
            self._mode_label.setText(f"Mode: PLACE {key}  (Esc to cancel, Shift=multi)")
            self._sb.showMessage(f"Placing {key} — click on canvas. Shift+click for multiple.")

    def _run_dc(self):
        self._log("Running DC analysis...")
        result = self._scene.run_simulation()
        if not result:
            self._log("No results (circuit may be empty or under-constrained).")
            return
        if "error" in result:
            self._log(f"Solver error: {result['error']}")
            return
        self._log("DC Results:")
        nodes, voltages = [], []
        for node, v in result.items():
            self._log(f"  {node} = {v:.6f} V")
            nodes.append(node)
            voltages.append(v)
        if voltages:
            self._plotter.clear()
            bar = pg.BarGraphItem(
                x=list(range(len(voltages))), height=voltages,
                width=0.6, brush="#50fa7b"
            )
            self._plotter.addItem(bar)
            ax = self._plotter.getAxis("bottom")
            ax.setTicks([list(enumerate(nodes))])

    def _run_transient(self):
        self._log("Transient simulation not yet wired to solver — showing placeholder waveform.")
        import numpy as np
        t = np.linspace(0, 1e-3, 500)
        self._plotter.clear()
        self._plotter.plot(t * 1e3, np.sin(2 * 3.14159 * 1000 * t),
                           pen=pg.mkPen(color="#ff79c6", width=2))
        self._plotter.setLabel("bottom", "Time (ms)")

    def _confirm_clear(self):
        self._scene.clear_canvas()
        self._plotter.clear()
        self._log("Canvas cleared.")

    def _fit(self):
        self._view.fitInView(self._scene.itemsBoundingRect(),
                             Qt.AspectRatioMode.KeepAspectRatio)

    def _log(self, msg):
        self._console.append(msg)

    def _probe_services(self):
        try:
            asyncio.run(self.discovery.probe_services())
            for svc, status in self.discovery.status.items():
                led = self._leds.get(svc)
                if led:
                    color = {"online":"#50fa7b","degraded":"#ffb86c","offline":"#ff5555"}.get(status,"gray")
                    led.setStyleSheet(f"background-color:{color}; border-radius:6px;")
        except Exception:
            pass

    def _apply_dark_palette(self):
        from PyQt6.QtGui import QPalette
        pal = QPalette()
        pal.setColor(QPalette.ColorRole.Window,          QColor("#1e1e2e"))
        pal.setColor(QPalette.ColorRole.WindowText,      QColor("#f8f8f2"))
        pal.setColor(QPalette.ColorRole.Base,            QColor("#12121e"))
        pal.setColor(QPalette.ColorRole.AlternateBase,   QColor("#1e1e2e"))
        pal.setColor(QPalette.ColorRole.ToolTipBase,     QColor("#f8f8f2"))
        pal.setColor(QPalette.ColorRole.ToolTipText,     QColor("#1e1e2e"))
        pal.setColor(QPalette.ColorRole.Text,            QColor("#f8f8f2"))
        pal.setColor(QPalette.ColorRole.Button,          QColor("#2a2a42"))
        pal.setColor(QPalette.ColorRole.ButtonText,      QColor("#f8f8f2"))
        pal.setColor(QPalette.ColorRole.Highlight,       QColor("#bd93f9"))
        pal.setColor(QPalette.ColorRole.HighlightedText, QColor("#1e1e2e"))
        QApplication.instance().setPalette(pal)


def main():
    app = QApplication(sys.argv)
    window = EgottolApp()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
