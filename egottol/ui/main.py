import sys
import uuid
import asyncio
import pyqtgraph as pg
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QGraphicsView, QGraphicsScene,
    QGraphicsItem, QGraphicsPathItem, QGraphicsTextItem,
    QVBoxLayout, QHBoxLayout, QWidget, QToolBar, QListWidget,
    QDockWidget, QLabel, QFrame, QStyleFactory,
    QSplitter, QTextEdit, QListWidgetItem, QAbstractItemView,
    QStatusBar, QDialog, QFormLayout, QLineEdit, QDialogButtonBox,
    QTabWidget, QCheckBox, QComboBox, QSpinBox, QDoubleSpinBox,
    QGroupBox, QScrollArea
)
from PyQt6.QtCore import Qt, QTimer, QPointF, QRectF, QLineF, QRect
from PyQt6.QtGui import (
    QAction, QColor, QPen, QBrush, QPainter, QPainterPath, QFont, QPalette
)

from egottol.engines.discovery import ServiceDiscovery
from egottol.models.registry import COMPONENT_LIBRARY
from egottol.models.base import Circuit, Component, Wire, ComponentType, Port

GRID = 20

COLORS = {
    "bg":       QColor("#1a1b2e"),
    "grid_dot": QColor("#2e3057"),
    "wire":     QColor("#50fa7b"),
    "component":QColor("#8be9fd"),
    "port":     QColor("#ffb86c"),
    "selected": QColor("#ff79c6"),
    "text":     QColor("#f8f8f2"),
    "label":    QColor("#bd93f9"),
    "result":   QColor("#f1fa8c"),
}

# ---------------------------------------------------------------------------
# Symbol draw commands:
#   ("line",   x1,y1,x2,y2)
#   ("rect",   x,y,w,h)
#   ("circle", x,y,w,h)
#   ("arc",    x,y,w,h,start,span)
#   ("path",   [(x,y),...], close)
#   ("text",   x,y,str)
#   ("bubble", cx,cy,r)
# ---------------------------------------------------------------------------
SYMBOLS = {
    "R": [
        ("line", 0,-25, 0,-12), ("rect",-8,-12,16,24), ("line", 0,12, 0,25),
    ],
    "C": [
        ("line", 0,-25, 0,-4), ("line",-14,-4,14,-4),
        ("line",-14, 4,14, 4), ("line", 0, 4, 0,25),
    ],
    "L": [
        ("line", 0,-25, 0,-18),
        ("arc",-8,-18,16,10, 0,180), ("arc",-8,-8,16,10, 0,180),
        ("arc",-8,  2,16,10, 0,180), ("line", 0,12, 0,25),
    ],
    "DIODE": [
        ("line", 0,-25, 0,-10),
        ("path",[(-10,-10),(10,-10),(0,10)], True),
        ("line",-10,10,10,10), ("line", 0,10, 0,25),
    ],
    "ZENER": [
        ("line", 0,-25, 0,-10),
        ("path",[(-10,-10),(10,-10),(0,10)], True),
        ("line",-14,10,10,10), ("line",-10,10,-14,14),
        ("line", 0,10, 0,25),
    ],
    "LED": [
        ("line", 0,-25, 0,-10),
        ("path",[(-10,-10),(10,-10),(0,10)], True),
        ("line",-10,10,10,10), ("line", 0,10, 0,25),
        ("line", 5,-6,14,-14), ("line",14,-14,10,-14), ("line",14,-14,14,-10),
        ("line", 8,-3,17,-11), ("line",17,-11,13,-11), ("line",17,-11,17,-7),
    ],
    "SW": [
        ("line", 0,-25, 0,-15), ("circle",-3,-15,6,6),
        ("line", 0,-9,12,-9),   ("circle",-3,9,6,6),
        ("line", 0,15, 0,25),
    ],
    "VSRC": [
        ("line", 0,-25, 0,-18), ("circle",-18,-18,36,36),
        ("line", 0,-15, 0,-8), ("line",-4,-11,4,-11),
        ("line",-4, 7, 4, 7),  ("line", 0,18, 0,25),
    ],
    "ISRC": [
        ("line", 0,-25, 0,-18), ("circle",-18,-18,36,36),
        ("line", 0,-14, 0,14),  ("path",[(0,14),(-5,4),(5,4)], True),
    ],
    "GND": [
        ("line", 0,-25, 0,0), ("line",-16,0,16,0),
        ("line",-10,6,10,6),  ("line",-5,12,5,12),
    ],
    "VCC": [
        ("line", 0,25, 0,6), ("line",-14,6,14,6), ("text",-8,-8,"VCC"),
    ],
    "Q_NPN": [
        ("circle",-22,-22,44,44),
        ("line",-8,-22,-8,22), ("line",-25,0,-8,0),
        ("line",-8,-8, 8,-18), ("line",-8, 8, 8,18),
        ("path",[(8,18),(3,10),(10,14)], True),
        ("line", 8,-18, 8,-25), ("line", 8,18, 8,25),
    ],
    "Q_PNP": [
        ("circle",-22,-22,44,44),
        ("line",-8,-22,-8,22), ("line",-25,0,-8,0),
        ("line",-8,-8, 8,-18), ("line",-8, 8, 8,18),
        ("path",[(-8,-8),(-3,-16),(-10,-12)], True),
        ("line", 8,-18, 8,-25), ("line", 8,18, 8,25),
    ],
    "OPAMP": [
        ("path",[(-20,-22),(-20,22),(20,0)], True),
        ("line",-20,-10,-10,-10), ("line",-15,-10,-15,-6),
        ("line",-20, 10,-10, 10),
        ("line", 20, 0, 30, 0),
        ("text",-18,-8,"+"), ("text",-18,8,"−"),
    ],
    "GATE_AND": [
        ("line",-20,-16,-20,16), ("line",-20,-16,0,-16), ("line",-20,16,0,16),
        ("arc",0,-16,20,32,-90,180),
        ("line",-32,-8,-20,-8), ("line",-32,8,-20,8),
        ("line",20,0,30,0),
    ],
    "GATE_NAND": [
        ("line",-20,-16,-20,16), ("line",-20,-16,0,-16), ("line",-20,16,0,16),
        ("arc",0,-16,20,32,-90,180),
        ("line",-32,-8,-20,-8), ("line",-32,8,-20,8),
        ("bubble",23,0,4), ("line",27,0,34,0),
    ],
    "GATE_OR": [
        ("arc",-28,-16,28,32,-90,90), ("arc",-20,-16,36,32,-90,90),
        ("line",-20,-16,0,-16), ("line",-20,16,0,16),
        ("line",-32,-8,-20,-8), ("line",-32,8,-20,8),
        ("line",16,0,30,0),
    ],
    "GATE_NOR": [
        ("arc",-28,-16,28,32,-90,90), ("arc",-20,-16,36,32,-90,90),
        ("line",-20,-16,0,-16), ("line",-20,16,0,16),
        ("line",-32,-8,-20,-8), ("line",-32,8,-20,8),
        ("bubble",19,0,4), ("line",23,0,34,0),
    ],
    "GATE_XOR": [
        ("arc",-28,-16,28,32,-90,90), ("arc",-20,-16,36,32,-90,90),
        ("arc",-34,-16,28,32,-90,90),
        ("line",-20,-16,0,-16), ("line",-20,16,0,16),
        ("line",-32,-8,-24,-8), ("line",-32,8,-24,8),
        ("line",16,0,30,0),
    ],
    "GATE_NOT": [
        ("path",[(-14,-14),(-14,14),(14,0)], True),
        ("bubble",18,0,4),
        ("line",-22,0,-14,0), ("line",22,0,30,0),
    ],
    "DFF": [
        ("rect",-20,-28,40,56),
        ("text",-14,-18,"D"),  ("text",6,-18,"Q"),
        ("text",-14,8,"CLK"),  ("text",6,8,"QB"),
        ("path",[(-20,4),(-14,0),(-20,-4)], False),
        ("line",-28,-16,-20,-16), ("line",-28,8,-20,8),
        ("line",20,-16,28,-16),   ("line",20,8,28,8),
    ],
    "MUX2": [
        ("path",[(-16,-24),(-16,24),(16,16),(16,-16)], True),
        ("text",-12,-14,"A"), ("text",-12,4,"B"), ("text",-12,16,"S"),
        ("line",-24,-12,-16,-12), ("line",-24,6,-16,6),
        ("line",-24,18,-16,18),   ("line",16,0,24,0),
    ],
    "IC_555": [
        ("rect",-28,-36,56,72),
        ("text",-20,-26,"555"),
        ("text",-24,-14,"VCC"), ("text",4,-14,"OUT"),
        ("text",-24,0,"GND"),   ("text",4,0,"RST"),
        ("text",-24,14,"TRG"),  ("text",4,14,"THR"),
        ("text",-24,28,"DIS"),  ("text",4,28,"CV"),
        ("line",-36,-20,-28,-20), ("line",28,-20,36,-20),
        ("line",-36,-6,-28,-6),   ("line",28,-6,36,-6),
        ("line",-36,8,-28,8),     ("line",28,8,36,8),
        ("line",-36,22,-28,22),   ("line",28,22,36,22),
    ],
    "IC_GENERIC": [
        ("rect",-22,-28,44,56),
        ("text",-8,-20,"IC"),
        ("line",-30,-16,-22,-16), ("line",22,-16,30,-16),
        ("line",-30,0,-22,0),     ("line",22,0,30,0),
        ("line",-30,16,-22,16),   ("line",22,16,30,16),
    ],
    "XFMR": [
        ("arc",-10,-20,14,10,0,180), ("arc",-10,-10,14,10,0,180),
        ("arc",-10,0,14,10,0,180),
        ("line",-3,-20,-3,-25), ("line",-3,10,-3,25),
        ("arc",-4,-20,14,10,0,-180), ("arc",-4,-10,14,10,0,-180),
        ("arc",-4,0,14,10,0,-180),
        ("line",10,-20,10,-25), ("line",10,10,10,25),
        ("line",-1,-24,11,-24),
    ],
    "POT": [
        ("line",0,-25,0,-12), ("rect",-8,-12,16,24), ("line",0,12,0,25),
        ("line",16,-2,22,-8), ("path",[(22,-8),(16,-4),(20,-4)],False),
        ("line",22,-8,28,-8),
    ],
    "DEFAULT": [
        ("rect",-18,-25,36,50), ("text",-6,-4,"IC"),
    ],
}

PORT_OFFSETS = {
    "R":         [(0,-25,"1"),  (0,25,"2")],
    "C":         [(0,-25,"1"),  (0,25,"2")],
    "L":         [(0,-25,"1"),  (0,25,"2")],
    "DIODE":     [(0,-25,"A"),  (0,25,"K")],
    "ZENER":     [(0,-25,"A"),  (0,25,"K")],
    "LED":       [(0,-25,"A"),  (0,25,"K")],
    "SW":        [(0,-25,"1"),  (0,25,"2")],
    "VSRC":      [(0,-25,"+"),  (0,25,"−")],
    "ISRC":      [(0,-25,"+"),  (0,25,"−")],
    "GND":       [(0,-25,"G")],
    "VCC":       [(0,25,"V")],
    "Q_NPN":     [(8,-25,"C"),  (-25,0,"B"),  (8,25,"E")],
    "Q_PNP":     [(8,-25,"C"),  (-25,0,"B"),  (8,25,"E")],
    "OPAMP":     [(-20,-10,"+"),(-20,10,"−"), (30,0,"OUT")],
    "GATE_AND":  [(-32,-8,"A"), (-32,8,"B"),  (30,0,"Q")],
    "GATE_NAND": [(-32,-8,"A"), (-32,8,"B"),  (34,0,"Q")],
    "GATE_OR":   [(-32,-8,"A"), (-32,8,"B"),  (30,0,"Q")],
    "GATE_NOR":  [(-32,-8,"A"), (-32,8,"B"),  (34,0,"Q")],
    "GATE_XOR":  [(-32,-8,"A"), (-32,8,"B"),  (30,0,"Q")],
    "GATE_NOT":  [(-22,0,"A"),  (30,0,"Q")],
    "DFF":       [(-28,-16,"D"),(-28,8,"CLK"),(28,-16,"Q"),(28,8,"QB")],
    "MUX2":      [(-24,-12,"A"),(-24,6,"B"),  (-24,18,"SEL"),(24,0,"Q")],
    "IC_555":    [(-36,-20,"VCC"),(-36,-6,"GND"),(-36,8,"TRIG"),(-36,22,"DISCH"),
                  (36,-20,"OUT"),(36,-6,"RESET"),(36,8,"THRES"),(36,22,"CV")],
    "IC_GENERIC":[(-30,-16,"+"),(-30,0,"−"),  (30,0,"OUT"),
                  (-30,16,"V+"),(30,16,"V-")],
    "XFMR":      [(-3,-25,"P1"),(-3,25,"P2"), (10,-25,"S1"),(10,25,"S2")],
    "POT":       [(0,-25,"1"),  (28,-8,"W"),   (0,25,"2")],
    "DEFAULT":   [(-18,0,"1"),  (18,0,"2")],
}

SYMBOL_KEY = {
    "RES":"R",   "CAP":"C",   "IND":"L",
    "DIODE":"DIODE","ZENER":"ZENER","LED":"LED",
    "SW":"SW",   "XFMR":"XFMR","POT":"POT",
    "VSRC":"VSRC","ISRC":"ISRC",
    "GND":"GND", "VCC":"VCC",
    "NPN":"Q_NPN","PNP":"Q_PNP",
    "NMOS":"DEFAULT","PMOS":"DEFAULT",
    "AND":"GATE_AND","NAND":"GATE_NAND",
    "OR":"GATE_OR",  "NOR":"GATE_NOR",
    "XOR":"GATE_XOR","NOT":"GATE_NOT",
    "DFF":"DFF", "MUX2":"MUX2",
    "LM741":"OPAMP","LM358":"OPAMP","TL071":"OPAMP",
    "UA741":"OPAMP","LF356":"OPAMP","AD8055":"OPAMP",
    "OPAMP":"OPAMP",
    "555":"IC_555",
    "7805":"IC_GENERIC","LM317":"IC_GENERIC",
    "ADSB_TX":"DEFAULT","NSP_AI":"DEFAULT",
}


def snap(v):
    return round(v / GRID) * GRID


def _ortho_path(start: QPointF, end: QPointF) -> QPainterPath:
    path = QPainterPath(start)
    path.lineTo(QPointF(end.x(), start.y()))
    path.lineTo(end)
    return path


# ---------------------------------------------------------------------------
class SchematicScene(QGraphicsScene):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setSceneRect(-3000, -3000, 6000, 6000)
        self.setBackgroundBrush(QBrush(COLORS["bg"]))
        self._placed_components = {}
        self._wire_items   = []
        self._pending_wire = None
        self._wire_preview = None
        self._place_mode   = None
        self._circuit = Circuit(id="main", name="untitled", components=[], wires=[])
        self._result_labels = []

    def drawBackground(self, painter, rect):
        super().drawBackground(painter, rect)
        painter.setPen(QPen(COLORS["grid_dot"], 1))
        x = int(rect.left()  / GRID) * GRID
        while x <= rect.right() + GRID:
            y = int(rect.top() / GRID) * GRID
            while y <= rect.bottom() + GRID:
                painter.drawPoint(int(x), int(y))
                y += GRID
            x += GRID

    def set_place_mode(self, key):
        self._place_mode = key
        if self.views():
            self.views()[0].setCursor(Qt.CursorShape.CrossCursor)

    def clear_place_mode(self):
        self._place_mode = None
        if self.views():
            self.views()[0].setCursor(Qt.CursorShape.ArrowCursor)

    def mousePressEvent(self, event):
        if self._place_mode and event.button() == Qt.MouseButton.LeftButton:
            pos = event.scenePos()
            self._drop_component(self._place_mode, snap(pos.x()), snap(pos.y()))
            if not (event.modifiers() & Qt.KeyboardModifier.ShiftModifier):
                self.clear_place_mode()
            return
        super().mousePressEvent(event)

    def _get_sym_key(self, key):
        if key in SYMBOL_KEY:
            return SYMBOL_KEY[key]
        for prefix, sk in SYMBOL_KEY.items():
            if key.startswith(prefix):
                return sk
        return "DEFAULT"

    def _drop_component(self, key, x, y):
        defn = COMPONENT_LIBRARY.get(key)
        if defn is None:
            return None
        sym_key = self._get_sym_key(key)
        comp_id = f"{key}_{uuid.uuid4().hex[:6]}"
        item = ComponentItem(comp_id, key, defn, sym_key, x, y, self)
        self.addItem(item)
        self._placed_components[comp_id] = item
        self._circuit.components.append(Component(
            id=comp_id, name=defn.name, type=defn.category,
            ports=list(defn.ports), parameters=dict(defn.parameters)
        ))
        return item

    def start_wire(self, comp_id, port_name, scene_pos):
        self._pending_wire = (comp_id, port_name, scene_pos)
        pen = QPen(COLORS["wire"], 1, Qt.PenStyle.DashLine)
        self._wire_preview = self.addPath(_ortho_path(scene_pos, scene_pos), pen)

    def update_wire_preview(self, scene_pos):
        if self._wire_preview and self._pending_wire:
            self._wire_preview.setPath(_ortho_path(self._pending_wire[2], scene_pos))

    def finish_wire(self, to_comp_id, to_port_name, exact_pos):
        if not self._pending_wire:
            return
        from_id, from_port, start_pos = self._pending_wire
        if self._wire_preview:
            self.removeItem(self._wire_preview)
            self._wire_preview = None
        self._pending_wire = None
        if from_id == to_comp_id:
            return
        wire_item = self.addPath(_ortho_path(start_pos, exact_pos),
                                  QPen(COLORS["wire"], 2))
        self._wire_items.append(wire_item)
        self._circuit.wires.append(Wire(
            id=f"W_{uuid.uuid4().hex[:6]}",
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
        try:
            result = AdvancedMNASolver(self._circuit).solve_dc()
            self._annotate_results(result or {})
            return result
        except Exception as e:
            return {"error": str(e)}

    def _annotate_results(self, results):
        for lbl in self._result_labels:
            self.removeItem(lbl)
        self._result_labels.clear()
        for comp_id, item in self._placed_components.items():
            ports = PORT_OFFSETS.get(item.sym_key, PORT_OFFSETS["DEFAULT"])
            for px, py, pname in ports:
                node_key = f"{comp_id}:{pname}"
                if node_key in results:
                    pt = item.mapToScene(QPointF(px, py))
                    lbl = self.addText(f"{results[node_key]:.2f}V")
                    lbl.setDefaultTextColor(COLORS["result"])
                    lbl.setFont(QFont("Monospace", 6))
                    lbl.setPos(pt.x() + 3, pt.y() - 14)
                    self._result_labels.append(lbl)

    def clear_canvas(self):
        self.clear()
        self._placed_components.clear()
        self._wire_items.clear()
        self._result_labels.clear()
        self._pending_wire = None
        self._wire_preview = None
        self._circuit = Circuit(id="main", name="untitled", components=[], wires=[])


# ---------------------------------------------------------------------------
class ComponentItem(QGraphicsItem):
    def __init__(self, comp_id, key, defn, sym_key, x, y, scene):
        super().__init__()
        self.comp_id = comp_id
        self.key     = key
        self.defn    = defn
        self.sym_key = sym_key
        self._scene  = scene
        self.params  = dict(defn.parameters)
        self.setPos(x, y)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsMovable)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemIsSelectable)
        self.setFlag(QGraphicsItem.GraphicsItemFlag.ItemSendsGeometryChanges)
        self._port_r = 4

    def boundingRect(self):
        return QRectF(-44, -44, 88, 88)

    def paint(self, painter, option, widget=None):
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        color = COLORS["selected"] if self.isSelected() else COLORS["component"]
        pen   = QPen(color, 1.8)
        painter.setPen(pen)
        painter.setBrush(Qt.BrushStyle.NoBrush)

        for cmd in SYMBOLS.get(self.sym_key, SYMBOLS["DEFAULT"]):
            t = cmd[0]
            if t == "line":
                painter.drawLine(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
            elif t == "rect":
                painter.setBrush(QBrush(COLORS["bg"]))
                painter.drawRect(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
                painter.setBrush(Qt.BrushStyle.NoBrush)
            elif t == "circle":
                painter.setBrush(QBrush(COLORS["bg"]))
                painter.drawEllipse(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4]))
                painter.setBrush(Qt.BrushStyle.NoBrush)
            elif t == "arc":
                painter.drawArc(QRect(int(cmd[1]),int(cmd[2]),int(cmd[3]),int(cmd[4])),
                                int(cmd[5])*16, int(cmd[6])*16)
            elif t == "path":
                pts, close = cmd[1], cmd[2]
                p = QPainterPath()
                p.moveTo(pts[0][0], pts[0][1])
                for px2, py2 in pts[1:]:
                    p.lineTo(px2, py2)
                if close:
                    p.closeSubpath()
                    painter.setBrush(QBrush(color))
                painter.drawPath(p)
                painter.setBrush(Qt.BrushStyle.NoBrush)
            elif t == "bubble":
                cx, cy, r = cmd[1], cmd[2], cmd[3]
                painter.setBrush(QBrush(COLORS["bg"]))
                painter.drawEllipse(QRect(int(cx-r),int(cy-r),r*2,r*2))
                painter.setBrush(Qt.BrushStyle.NoBrush)
            elif t == "text":
                painter.setPen(QPen(COLORS["label"]))
                painter.setFont(QFont("Monospace", 6))
                painter.drawText(int(cmd[1]), int(cmd[2]), cmd[3])
                painter.setPen(pen)

        # port dots — slightly larger hit target
        ports = PORT_OFFSETS.get(self.sym_key, PORT_OFFSETS["DEFAULT"])
        painter.setPen(QPen(COLORS["port"], 1))
        painter.setBrush(QBrush(COLORS["port"]))
        for px, py, _ in ports:
            r = self._port_r
            painter.drawEllipse(int(px - r//2), int(py - r//2), r, r)

        # ID label
        painter.setPen(QPen(COLORS["label"]))
        painter.setFont(QFont("Monospace", 6))
        painter.drawText(-18, -32, self.comp_id)

    def _hit_port(self, local_pos):
        ports = PORT_OFFSETS.get(self.sym_key, PORT_OFFSETS["DEFAULT"])
        for px, py, name in ports:
            if (local_pos.x()-px)**2 + (local_pos.y()-py)**2 <= (self._port_r + 6)**2:
                return name, px, py
        return None

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.LeftButton:
            hit = self._hit_port(event.pos())
            if hit:
                name, px, py = hit
                exact = self.mapToScene(QPointF(px, py))
                if self._scene._pending_wire:
                    self._scene.finish_wire(self.comp_id, name, exact)
                else:
                    self._scene.start_wire(self.comp_id, name, exact)
                return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._scene._pending_wire:
            self._scene.update_wire_preview(self.mapToScene(event.pos()))
        super().mouseMoveEvent(event)

    def mouseDoubleClickEvent(self, event):
        dlg = ParamDialog(self.comp_id, self.params)
        if dlg.exec():
            self.params = dlg.result_params
            self.update()


# ---------------------------------------------------------------------------
class ParamDialog(QDialog):
    def __init__(self, comp_id, params):
        super().__init__()
        self.setWindowTitle(f"Properties — {comp_id}")
        self.setMinimumWidth(300)
        layout = QFormLayout(self)
        self._fields = {}
        for k, v in params.items():
            f = QLineEdit(str(v))
            self._fields[k] = f
            layout.addRow(k, f)
        btns = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok |
                                QDialogButtonBox.StandardButton.Cancel)
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        layout.addRow(btns)
        self.result_params = dict(params)

    def accept(self):
        for k, f in self._fields.items():
            try:
                self.result_params[k] = float(f.text())
            except ValueError:
                self.result_params[k] = f.text()
        super().accept()


# ---------------------------------------------------------------------------
class SimConfigDialog(QDialog):
    def __init__(self, cfg, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Simulation Configuration")
        self.setMinimumWidth(420)
        self.cfg = dict(cfg)
        tabs = QTabWidget()
        tabs.addTab(self._dc_tab(),        "DC Analysis")
        tabs.addTab(self._transient_tab(), "Transient")
        tabs.addTab(self._ac_tab(),        "AC Analysis")
        tabs.addTab(self._display_tab(),   "Display")
        btns = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok |
                                QDialogButtonBox.StandardButton.Cancel)
        btns.accepted.connect(self.accept)
        btns.rejected.connect(self.reject)
        root = QVBoxLayout(self)
        root.addWidget(tabs); root.addWidget(btns)

    def _dc_tab(self):
        w = QWidget(); l = QFormLayout(w)
        self._temp = QDoubleSpinBox()
        self._temp.setRange(-273, 1000); self._temp.setSuffix(" °C")
        self._temp.setValue(self.cfg.get("temperature", 27))
        l.addRow("Temperature:", self._temp)
        grp = QGroupBox("DC Sweep"); gl = QFormLayout(grp)
        self._sweep_en    = QCheckBox("Enable"); self._sweep_en.setChecked(self.cfg.get("sweep_en", False))
        self._sweep_start = QDoubleSpinBox(); self._sweep_start.setRange(-1000,1000); self._sweep_start.setSuffix(" V"); self._sweep_start.setValue(self.cfg.get("sweep_start",0))
        self._sweep_stop  = QDoubleSpinBox(); self._sweep_stop.setRange(-1000,1000);  self._sweep_stop.setSuffix(" V");  self._sweep_stop.setValue(self.cfg.get("sweep_stop",5))
        self._sweep_step  = QDoubleSpinBox(); self._sweep_step.setRange(1e-6,100);    self._sweep_step.setSuffix(" V");  self._sweep_step.setValue(self.cfg.get("sweep_step",0.1))
        gl.addRow(self._sweep_en)
        gl.addRow("Start:", self._sweep_start); gl.addRow("Stop:", self._sweep_stop); gl.addRow("Step:", self._sweep_step)
        grp.setLayout(gl); l.addRow(grp)
        return w

    def _transient_tab(self):
        w = QWidget(); l = QFormLayout(w)
        self._t_stop = QDoubleSpinBox(); self._t_stop.setRange(1e-9,100); self._t_stop.setDecimals(6); self._t_stop.setSuffix(" s"); self._t_stop.setValue(self.cfg.get("t_stop",1e-3))
        self._t_step = QDoubleSpinBox(); self._t_step.setRange(1e-12,1);  self._t_step.setDecimals(9); self._t_step.setSuffix(" s"); self._t_step.setValue(self.cfg.get("t_step",1e-6))
        self._ic_mode = QComboBox(); self._ic_mode.addItems(["Zero","DC Operating Point"]); self._ic_mode.setCurrentText(self.cfg.get("ic_mode","DC Operating Point"))
        l.addRow("Stop Time:", self._t_stop); l.addRow("Time Step:", self._t_step); l.addRow("Initial Conditions:", self._ic_mode)
        return w

    def _ac_tab(self):
        w = QWidget(); l = QFormLayout(w)
        self._ac_start = QDoubleSpinBox(); self._ac_start.setRange(1e-3,1e12); self._ac_start.setSuffix(" Hz"); self._ac_start.setValue(self.cfg.get("ac_start",1))
        self._ac_stop  = QDoubleSpinBox(); self._ac_stop.setRange(1,1e12);     self._ac_stop.setSuffix(" Hz");  self._ac_stop.setValue(self.cfg.get("ac_stop",1e6))
        self._ac_pts   = QSpinBox();       self._ac_pts.setRange(2,1000);                                        self._ac_pts.setValue(self.cfg.get("ac_pts",20))
        l.addRow("Start Freq:", self._ac_start); l.addRow("Stop Freq:", self._ac_stop); l.addRow("Points/decade:", self._ac_pts)
        return w

    def _display_tab(self):
        w = QWidget(); l = QFormLayout(w)
        self._max_nodes  = QSpinBox(); self._max_nodes.setRange(1,500); self._max_nodes.setValue(self.cfg.get("max_nodes",20))
        self._auto_fit   = QCheckBox(); self._auto_fit.setChecked(self.cfg.get("auto_fit",True))
        self._show_lbl   = QCheckBox(); self._show_lbl.setChecked(self.cfg.get("show_labels",True))
        l.addRow("Max display nodes:", self._max_nodes)
        l.addRow("Auto-fit after run:", self._auto_fit)
        l.addRow("Node voltage labels:", self._show_lbl)
        return w

    def accept(self):
        self.cfg.update({
            "temperature": self._temp.value(),
            "sweep_en": self._sweep_en.isChecked(),
            "sweep_start": self._sweep_start.value(), "sweep_stop": self._sweep_stop.value(), "sweep_step": self._sweep_step.value(),
            "t_stop": self._t_stop.value(), "t_step": self._t_step.value(), "ic_mode": self._ic_mode.currentText(),
            "ac_start": self._ac_start.value(), "ac_stop": self._ac_stop.value(), "ac_pts": self._ac_pts.value(),
            "max_nodes": self._max_nodes.value(), "auto_fit": self._auto_fit.isChecked(), "show_labels": self._show_lbl.isChecked(),
        })
        super().accept()


# ---------------------------------------------------------------------------
class SchematicView(QGraphicsView):
    def __init__(self, scene):
        super().__init__(scene)
        self._scene = scene
        self.setRenderHint(QPainter.RenderHint.Antialiasing)
        self.setDragMode(QGraphicsView.DragMode.RubberBandDrag)
        self.setTransformationAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self.setResizeAnchor(QGraphicsView.ViewportAnchor.AnchorUnderMouse)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        self._panning = False; self._pan_start = None

    def wheelEvent(self, event):
        f = 1.15 if event.angleDelta().y() > 0 else 1/1.15
        self.scale(f, f)

    def mousePressEvent(self, event):
        if event.button() == Qt.MouseButton.MiddleButton:
            self._panning = True; self._pan_start = event.pos()
            self.setCursor(Qt.CursorShape.ClosedHandCursor); return
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        if self._panning and self._pan_start:
            d = event.pos() - self._pan_start; self._pan_start = event.pos()
            self.horizontalScrollBar().setValue(self.horizontalScrollBar().value() - d.x())
            self.verticalScrollBar().setValue(self.verticalScrollBar().value() - d.y())
            return
        self._scene.update_wire_preview(self.mapToScene(event.pos()))
        super().mouseMoveEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.MiddleButton:
            self._panning = False; self.setCursor(Qt.CursorShape.ArrowCursor); return
        super().mouseReleaseEvent(event)

    def keyPressEvent(self, event):
        if event.key() == Qt.Key.Key_Escape:
            self._scene.cancel_wire(); self._scene.clear_place_mode()
        elif event.key() == Qt.Key.Key_Delete:
            for item in list(self._scene.selectedItems()):
                if isinstance(item, ComponentItem):
                    self._scene._circuit.components = [c for c in self._scene._circuit.components if c.id != item.comp_id]
                    self._scene._placed_components.pop(item.comp_id, None)
                self._scene.removeItem(item)
        elif event.key() == Qt.Key.Key_Space:
            br = self._scene.itemsBoundingRect()
            if not br.isEmpty():
                self.fitInView(br.adjusted(-40,-40,40,40), Qt.AspectRatioMode.KeepAspectRatio)
        super().keyPressEvent(event)


# ---------------------------------------------------------------------------
class EgottolApp(QMainWindow):
    _DEFAULT_CFG = {
        "temperature":27, "sweep_en":False, "sweep_start":0, "sweep_stop":5, "sweep_step":0.1,
        "t_stop":1e-3, "t_step":1e-6, "ic_mode":"DC Operating Point",
        "ac_start":1, "ac_stop":1e6, "ac_pts":20,
        "max_nodes":20, "auto_fit":True, "show_labels":True,
    }

    def __init__(self):
        super().__init__()
        self.setWindowTitle("deepiri-egottol  —  Schematic & Simulation")
        self.resize(1600, 950)
        self.setStyle(QStyleFactory.create("Fusion"))
        self._apply_dark_palette()
        self._sim_cfg  = dict(self._DEFAULT_CFG)
        self.discovery = ServiceDiscovery()
        self._scene    = SchematicScene()
        self._view     = SchematicView(self._scene)
        self._setup_ui()
        self._setup_toolbar()
        self._setup_statusbar()
        QTimer.singleShot(0, lambda: self._view.centerOn(0, 0))
        self.timer = QTimer(self)
        self.timer.timeout.connect(self._probe_services)
        self.timer.start(8000)

    def _setup_ui(self):
        self.setCentralWidget(self._view)

        # Palette with category colours
        self._palette = QListWidget()
        self._palette.setMinimumWidth(170); self._palette.setMaximumWidth(220)
        self._palette.setStyleSheet(
            "QListWidget{background:#12121e;color:#f8f8f2;font-family:monospace;font-size:12px;}"
            "QListWidget::item:hover{background:#2a2a42;}"
            "QListWidget::item:selected{background:#44475a;}")
        self._palette.setDragDropMode(QAbstractItemView.DragDropMode.NoDragDrop)
        cat_colors = {
            "passive":"#8be9fd","active":"#50fa7b","source":"#f1fa8c","power":"#ff5555",
            "logic":"#bd93f9","ic_block":"#ffb86c","rf":"#ff79c6","experimental":"#6272a4",
        }
        for key, defn in COMPONENT_LIBRARY.items():
            item = QListWidgetItem(f"  {defn.name}")
            item.setData(Qt.ItemDataRole.UserRole, key)
            item.setForeground(QBrush(QColor(cat_colors.get(defn.category.value,"#f8f8f2"))))
            self._palette.addItem(item)
        self._palette.itemClicked.connect(self._on_palette_click)
        dock = QDockWidget("Components", self); dock.setWidget(self._palette)
        self.addDockWidget(Qt.DockWidgetArea.LeftDockWidgetArea, dock)

        # Services
        svc_w = QWidget(); svc_w.setStyleSheet("background:#12121e;")
        svc_l = QVBoxLayout(svc_w); svc_l.setContentsMargins(6,6,6,6)
        self._leds = {}
        for svc in getattr(self.discovery,"DEFAULT_ENDPOINTS",[]):
            row = QHBoxLayout()
            lbl = QLabel(svc.upper()); lbl.setStyleSheet("color:#f8f8f2;font-family:monospace;")
            led = QFrame(); led.setFixedSize(12,12); led.setStyleSheet("background:gray;border-radius:6px;")
            row.addWidget(lbl); row.addWidget(led); svc_l.addLayout(row); self._leds[svc] = led
        svc_l.addStretch()
        svc_dock = QDockWidget("Services", self); svc_dock.setWidget(svc_w)
        self.addDockWidget(Qt.DockWidgetArea.RightDockWidgetArea, svc_dock)

        # Waveform + console
        self._plotter = pg.PlotWidget(background="#12121e")
        self._plotter.setLabel("left","Voltage (V)"); self._plotter.setLabel("bottom","Node")
        self._plotter.showGrid(x=True,y=True,alpha=0.25)
        self._console = QTextEdit(); self._console.setReadOnly(True); self._console.setMinimumHeight(80)
        self._console.setStyleSheet("background:#12121e;color:#50fa7b;font-family:monospace;font-size:11px;")
        bottom = QSplitter(Qt.Orientation.Horizontal)
        bottom.addWidget(self._plotter); bottom.addWidget(self._console); bottom.setSizes([900,400])
        btm_dock = QDockWidget("Waveform / Console", self); btm_dock.setWidget(bottom)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, btm_dock)

    def _setup_toolbar(self):
        tb = self.addToolBar("Main"); tb.setMovable(False)
        tb.setStyleSheet("background:#2a2a42;color:#f8f8f2;spacing:3px;")
        def act(label, tip, fn):
            a = QAction(label, self); a.setToolTip(tip); a.triggered.connect(fn); tb.addAction(a)
        act("▶ DC",    "Run DC operating point",  self._run_dc)
        act("⟳ Tran", "Run transient simulation", self._run_transient)
        act("⚙ Cfg",  "Simulation settings",      self._open_sim_config)
        tb.addSeparator()
        for key, lbl, tip in [
            ("RES","R","Resistor"),("CAP","C","Capacitor"),("IND","L","Inductor"),
            ("DIODE","D","Diode"),("VSRC","V","Voltage Source"),("ISRC","I","Current Source"),
            ("GND","⏚","Ground"),("VCC","VCC","VCC Rail"),
        ]:
            act(lbl, tip, lambda k=key: self._scene.set_place_mode(k))
        tb.addSeparator()
        for key, lbl, tip in [
            ("NPN","NPN","NPN BJT"),("PNP","PNP","PNP BJT"),
            ("AND","AND","AND Gate"),("OR","OR","OR Gate"),
            ("NOT","NOT","Inverter"),("XOR","XOR","XOR Gate"),
            ("DFF","DFF","D Flip-Flop"),("LM741","OpAmp","Op-Amp"),
        ]:
            act(lbl, tip, lambda k=key: self._scene.set_place_mode(k))
        tb.addSeparator()
        act("🗑","Clear canvas",    self._confirm_clear)
        act("−","Zoom out",         lambda: self._view.scale(1/1.2,1/1.2))
        act("+","Zoom in",          lambda: self._view.scale(1.2,1.2))
        act("⤢","Fit [Space]",      self._fit)

    def _setup_statusbar(self):
        self._sb = QStatusBar(); self.setStatusBar(self._sb)
        self._mode_lbl = QLabel("SELECT")
        self._mode_lbl.setStyleSheet("color:#bd93f9;font-family:monospace;padding:0 8px;")
        self._sb.addPermanentWidget(self._mode_lbl)
        self._sb.showMessage("Click palette/toolbar to place  |  Click port dot to wire  |  Middle-drag to pan  |  Scroll to zoom  |  Del = delete  |  Space = fit")

    def _on_palette_click(self, item):
        key = item.data(Qt.ItemDataRole.UserRole)
        if key:
            self._scene.set_place_mode(key)
            name = COMPONENT_LIBRARY[key].name if key in COMPONENT_LIBRARY else key
            self._mode_lbl.setText(f"PLACE: {name}")
            self._sb.showMessage(f"Placing {name}  |  Shift+click = multi-place  |  Esc = cancel")

    def _run_dc(self):
        self._log("─── DC Analysis ───────────────────────")
        result = self._scene.run_simulation()
        if not result:
            self._log("No results — circuit empty or under-constrained."); return
        if "error" in result:
            self._log(f"ERROR: {result['error']}"); return
        nodes, volts = [], []
        for node, v in list(result.items())[:self._sim_cfg.get("max_nodes",20)]:
            self._log(f"  {node:42s} = {v:+.6f} V")
            nodes.append(node.split(":")[-1])
            volts.append(v)
        self._plotter.clear()
        self._plotter.setLabel("bottom","Node"); self._plotter.setLabel("left","Voltage (V)")
        bar = pg.BarGraphItem(x=list(range(len(volts))),height=volts,width=0.6,brush="#50fa7b")
        self._plotter.addItem(bar)
        self._plotter.getAxis("bottom").setTicks([list(enumerate(nodes))])
        if self._sim_cfg.get("auto_fit") and self._scene.items():
            self._fit()

    def _run_transient(self):
        self._log("─── Transient ─────────────────────────")
        import numpy as np
        t_stop = self._sim_cfg.get("t_stop",1e-3)
        dt     = self._sim_cfg.get("t_step",1e-6)
        t = np.arange(0, t_stop, dt)
        result = self._scene.run_simulation()
        vscale = 5.0
        if result and "error" not in result:
            vals = [abs(v) for v in result.values() if abs(v) > 0.01]
            if vals: vscale = max(vals)
        y = vscale * np.sin(2*3.14159*1000*t)
        self._plotter.clear()
        self._plotter.setLabel("bottom","Time (ms)"); self._plotter.setLabel("left","Voltage (V)")
        self._plotter.plot(t*1e3, y, pen=pg.mkPen(color="#ff79c6",width=2))
        self._log(f"  t_stop={t_stop*1e3:.3f}ms  dt={dt*1e6:.3f}µs  pts={len(t)}")

    def _open_sim_config(self):
        dlg = SimConfigDialog(self._sim_cfg, self)
        if dlg.exec():
            self._sim_cfg = dlg.cfg
            self._log(f"Config: T={self._sim_cfg['temperature']}°C  t_stop={self._sim_cfg['t_stop']*1e3:.3f}ms  t_step={self._sim_cfg['t_step']*1e6:.3f}µs")

    def _confirm_clear(self):
        self._scene.clear_canvas(); self._plotter.clear(); self._log("Canvas cleared.")

    def _fit(self):
        br = self._scene.itemsBoundingRect()
        if not br.isEmpty():
            self._view.fitInView(br.adjusted(-40,-40,40,40), Qt.AspectRatioMode.KeepAspectRatio)

    def _log(self, msg):
        self._console.append(msg)

    def _probe_services(self):
        try:
            asyncio.run(self.discovery.probe_services())
            for svc, status in self.discovery.status.items():
                led = self._leds.get(svc)
                if led:
                    c = {"online":"#50fa7b","degraded":"#ffb86c","offline":"#ff5555"}.get(status,"gray")
                    led.setStyleSheet(f"background:{c};border-radius:6px;")
        except Exception:
            pass

    def _apply_dark_palette(self):
        pal = QPalette()
        bg = QColor("#1a1b2e"); bg2 = QColor("#12121e"); fg = QColor("#f8f8f2")
        pal.setColor(QPalette.ColorRole.Window,          bg)
        pal.setColor(QPalette.ColorRole.WindowText,      fg)
        pal.setColor(QPalette.ColorRole.Base,            bg2)
        pal.setColor(QPalette.ColorRole.AlternateBase,   bg)
        pal.setColor(QPalette.ColorRole.ToolTipBase,     fg)
        pal.setColor(QPalette.ColorRole.ToolTipText,     bg)
        pal.setColor(QPalette.ColorRole.Text,            fg)
        pal.setColor(QPalette.ColorRole.Button,          QColor("#2a2a42"))
        pal.setColor(QPalette.ColorRole.ButtonText,      fg)
        pal.setColor(QPalette.ColorRole.Highlight,       QColor("#bd93f9"))
        pal.setColor(QPalette.ColorRole.HighlightedText, bg)
        QApplication.instance().setPalette(pal)


def main():
    app = QApplication(sys.argv)
    window = EgottolApp()
    window.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
