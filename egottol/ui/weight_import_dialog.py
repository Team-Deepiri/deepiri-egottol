"""PyQt6 dialog for importing AI readout weights onto schematic components."""

from __future__ import annotations

from pathlib import Path
from typing import Any, Callable, Dict, Optional

from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QFileDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QMessageBox,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QVBoxLayout,
    QWidget,
)

from egottol.engines.ai.onnx_import import import_weights, onnx_available, preview_weight_shapes
from egottol.engines.ai.weight_loader import apply_weights_to_engine, load_weights
from egottol.engines.eii.inference import InferenceEngine
from egottol.models.eii import EIIPipelineConfig

_WEIGHT_TARGET_KEYS = frozenset({"INFERENCE_ENGINE", "EII_PIPELINE"})

_FILE_FILTER = (
    "AI Weights (*.onnx *.egt-weights *.json *.npy);;"
    "ONNX Models (*.onnx);;"
    "Egottol Weights (*.egt-weights);;"
    "JSON (*.json);;"
    "NumPy (*.npy);;"
    "All Files (*)"
)


class WeightImportDialog(QDialog):
    """Browse weight files, preview shapes, and apply to an inference component."""

    def __init__(
        self,
        scene,
        log_fn: Optional[Callable[[str], None]] = None,
        parent: Optional[QWidget] = None,
    ):
        super().__init__(parent)
        self._scene = scene
        self._log = log_fn or (lambda _msg: None)
        self._payload: Optional[Dict[str, Any]] = None
        self._source_path: Optional[Path] = None
        self.setWindowTitle("Import AI Weights")
        self.setMinimumWidth(520)
        self._build_ui()
        self._refresh_targets()

    def _build_ui(self) -> None:
        root = QVBoxLayout(self)

        file_row = QHBoxLayout()
        self._path_label = QLabel("No file selected")
        self._path_label.setWordWrap(True)
        self._path_label.setStyleSheet("color:#bd93f9;font-family:monospace;")
        browse_btn = QPushButton("Browse…")
        browse_btn.clicked.connect(self._browse_file)
        file_row.addWidget(self._path_label, stretch=1)
        file_row.addWidget(browse_btn)
        root.addLayout(file_row)

        hint = QLabel(
            "Supported: .onnx (linear layers)"
            + (" — ONNX available" if onnx_available() else " — ONNX not installed; use JSON/NPY")
        )
        hint.setStyleSheet("color:#6272a4;font-size:11px;")
        root.addWidget(hint)

        preview_box = QGroupBox("Weight preview")
        preview_layout = QVBoxLayout(preview_box)
        self._preview_table = QTableWidget(0, 2)
        self._preview_table.setHorizontalHeaderLabels(["Tensor", "Shape"])
        self._preview_table.horizontalHeader().setStretchLastSection(True)
        self._preview_table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        preview_layout.addWidget(self._preview_table)
        root.addWidget(preview_box)

        form = QFormLayout()
        self._target_combo = QComboBox()
        self._target_combo.setMinimumWidth(280)
        form.addRow("Apply to component:", self._target_combo)
        root.addLayout(form)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Apply | QDialogButtonBox.StandardButton.Close
        )
        buttons.button(QDialogButtonBox.StandardButton.Apply).setEnabled(False)
        self._apply_btn = buttons.button(QDialogButtonBox.StandardButton.Apply)
        self._apply_btn.clicked.connect(self._apply_weights)
        buttons.rejected.connect(self.reject)
        root.addWidget(buttons)

    def _refresh_targets(self) -> None:
        self._target_combo.clear()
        placed = getattr(self._scene, "_placed_components", {})
        for comp_id, item in placed.items():
            key = getattr(item, "key", "")
            if key in _WEIGHT_TARGET_KEYS:
                label = f"{comp_id}  ({key})"
                self._target_combo.addItem(label, comp_id)

        if self._target_combo.count() == 0:
            self._target_combo.addItem("(place INFERENCE_ENGINE or EII_PIPELINE first)", None)

    def _browse_file(self) -> None:
        path, _ = QFileDialog.getOpenFileName(self, "Import AI Weights", "", _FILE_FILTER)
        if not path:
            return
        self._load_file(Path(path))

    def _load_file(self, path: Path) -> None:
        try:
            if path.suffix.lower() == ".egt-weights":
                payload = load_weights(path)
            else:
                payload = import_weights(path)
            self._payload = payload
            self._source_path = path
            self._path_label.setText(str(path))
            self._fill_preview(payload)
            self._apply_btn.setEnabled(self._target_combo.currentData() is not None)
            self._log(f"Loaded weights from {path.name}")
        except Exception as exc:
            self._payload = None
            self._source_path = None
            self._path_label.setText("Load failed")
            self._preview_table.setRowCount(0)
            self._apply_btn.setEnabled(False)
            QMessageBox.warning(self, "Import failed", str(exc))

    def _fill_preview(self, payload: Dict[str, Any]) -> None:
        rows = preview_weight_shapes(payload)
        self._preview_table.setRowCount(len(rows))
        for i, (name, shape) in enumerate(rows):
            self._preview_table.setItem(i, 0, QTableWidgetItem(name))
            shape_text = "×".join(str(d) for d in shape) if shape else "—"
            self._preview_table.setItem(i, 1, QTableWidgetItem(shape_text))

    def _apply_weights(self) -> None:
        if self._payload is None:
            return
        comp_id = self._target_combo.currentData()
        if comp_id is None:
            QMessageBox.information(
                self,
                "No target",
                "Place an INFERENCE_ENGINE or EII_PIPELINE component on the canvas first.",
            )
            return

        placed = getattr(self._scene, "_placed_components", {})
        item = placed.get(comp_id)
        if item is None:
            QMessageBox.warning(self, "Apply failed", f"Component {comp_id} not found.")
            return

        try:
            pipeline = EIIPipelineConfig()
            engine = InferenceEngine(
                pipeline.inference,
                pipeline.default_weights(),
                pipeline.default_bias(),
                pipeline.default_conductance(),
            )
            apply_weights_to_engine(engine, self._payload)
            self._attach_to_component(item, comp_id, engine)
            self._log(
                f"Applied weights to {comp_id}: "
                f"W={engine.weights.shape} backend={self._payload.get('backend')}"
            )
            QMessageBox.information(
                self,
                "Weights applied",
                f"Readout weights applied to {comp_id}.\n"
                f"Shape: {engine.weights.shape[0]}×{engine.weights.shape[1]}",
            )
            self.accept()
        except Exception as exc:
            QMessageBox.warning(self, "Apply failed", str(exc))

    def _attach_to_component(self, item, comp_id: str, engine: InferenceEngine) -> None:
        item._inference_engine = engine  # noqa: SLF001 — runtime handle for UI/sim hooks

        w_path = str(self._source_path) if self._source_path else ""
        item.params["weights_file"] = w_path
        item.params["num_classes"] = int(engine.weights.shape[0])
        item.params["embedding_dim"] = int(engine.weights.shape[1])
        item.params["backend"] = str(self._payload.get("backend", "digital_linear"))
        if "temperature" in self._payload:
            item.params["temperature"] = float(self._payload["temperature"])

        circuit = getattr(self._scene, "_circuit", None)
        if circuit is None:
            return
        for comp in circuit.components:
            if comp.id != comp_id:
                continue
            comp.parameters.update(dict(item.params))
            comp.metadata["registry_key"] = item.key
            comp.metadata["eii_weights"] = {
                "format": self._payload["format"],
                "backend": self._payload.get("backend"),
                "embedding_dim": self._payload["embedding_dim"],
                "output_dim": self._payload["output_dim"],
                "weights": self._payload["weights"],
                "temperature": self._payload.get("temperature", 1.0),
            }
            if w_path:
                comp.metadata["weights_source"] = w_path
            break
        item.update()


def open_weight_import_dialog(scene, log_fn=None, parent=None) -> None:
    """Show the weight import dialog modally."""
    dlg = WeightImportDialog(scene, log_fn=log_fn, parent=parent)
    dlg.exec()
