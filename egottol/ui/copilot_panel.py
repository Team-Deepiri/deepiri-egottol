from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QDialogButtonBox,
    QDockWidget,
    QFormLayout,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPushButton,
    QTabWidget,
    QTextEdit,
    QVBoxLayout,
    QWidget,
)

from egottol.assistant.copilot import Copilot, CopilotResponse
from egottol.assistant.providers import ProviderRegistry
from egottol.assistant.settings import CopilotSettings
from egottol.models.base import Circuit


def _empty_circuit() -> Circuit:
    return Circuit(id="main", name="untitled", components=[], wires=[])

COLORS = {
    "bg": "#1a1b2e",
    "bg2": "#12121e",
    "accent": "#bd93f9",
    "text": "#f8f8f2",
    "input": "#2a2a42",
    "border": "#44475a",
    "success": "#50fa7b",
    "error": "#ff5555",
    "muted": "#6272a4",
}

_PANEL_STYLE = f"""
QWidget {{
    background: {COLORS["bg2"]};
    color: {COLORS["text"]};
}}
QTextEdit {{
    background: {COLORS["bg2"]};
    color: {COLORS["text"]};
    font-family: monospace;
    font-size: 11px;
    border: 1px solid {COLORS["border"]};
}}
QLineEdit {{
    background: {COLORS["input"]};
    color: {COLORS["text"]};
    border: 1px solid {COLORS["border"]};
    padding: 4px 6px;
    font-family: monospace;
}}
QPushButton {{
    background: {COLORS["input"]};
    color: {COLORS["text"]};
    border: 1px solid {COLORS["border"]};
    padding: 4px 10px;
}}
QPushButton:hover {{
    background: {COLORS["border"]};
}}
QComboBox {{
    background: {COLORS["input"]};
    color: {COLORS["text"]};
    border: 1px solid {COLORS["border"]};
    padding: 2px 6px;
}}
QTabWidget::pane {{
    border: 1px solid {COLORS["border"]};
    background: {COLORS["bg2"]};
}}
QTabBar::tab {{
    background: {COLORS["input"]};
    color: {COLORS["text"]};
    padding: 6px 12px;
    border: 1px solid {COLORS["border"]};
}}
QTabBar::tab:selected {{
    background: {COLORS["border"]};
}}
"""


class _AsyncWorker(QThread):
    finished_ok = pyqtSignal(object)
    finished_err = pyqtSignal(str)

    def __init__(self, fn, *args, **kwargs):
        super().__init__()
        self._fn = fn
        self._args = args
        self._kwargs = kwargs

    def run(self):
        try:
            result = self._fn(*self._args, **self._kwargs)
            self.finished_ok.emit(result)
        except Exception as exc:
            self.finished_err.emit(str(exc))


class CopilotSettingsDialog(QDialog):
    def __init__(self, settings: CopilotSettings | None = None, parent=None):
        super().__init__(parent)
        self.setWindowTitle("AI Provider Settings")
        self.setMinimumWidth(480)
        self.settings = settings or CopilotSettings.load()
        self._copilot = Copilot(_empty_circuit(), settings=self.settings)
        self._key_fields: dict[str, QLineEdit] = {}
        self._model_fields: dict[str, QLineEdit] = {}
        self._status_labels: dict[str, QLabel] = {}
        self._test_workers: list[_AsyncWorker] = []
        self._build_ui()
        self._load_fields()

    def _build_ui(self):
        root = QVBoxLayout(self)
        default_box = QWidget()
        default_form = QFormLayout(default_box)
        self._provider_combo = QComboBox()
        labels = ProviderRegistry.labels()
        for name in ProviderRegistry.all_providers():
            self._provider_combo.addItem(labels[name], name)
        idx = self._provider_combo.findData(self.settings.selected_provider)
        if idx >= 0:
            self._provider_combo.setCurrentIndex(idx)
        self._default_model = QLineEdit()
        self._default_model.setPlaceholderText("Default model for selected provider")
        default_form.addRow("Default provider:", self._provider_combo)
        default_form.addRow("Default model:", self._default_model)
        root.addWidget(default_box)

        tabs = QTabWidget()
        for name in ProviderRegistry.all_providers():
            tabs.addTab(self._provider_tab(name), labels[name])
        root.addWidget(tabs)

        btns = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Save
            | QDialogButtonBox.StandardButton.Cancel
        )
        btns.button(QDialogButtonBox.StandardButton.Save).setText("Save")
        btns.accepted.connect(self._save)
        btns.rejected.connect(self.reject)
        root.addWidget(btns)
        self.setStyleSheet(_PANEL_STYLE)

    def _provider_tab(self, provider: str) -> QWidget:
        w = QWidget()
        layout = QFormLayout(w)
        key_edit = QLineEdit()
        key_edit.setEchoMode(QLineEdit.EchoMode.Password)
        key_edit.setPlaceholderText("API key (leave blank to keep existing)")
        model_edit = QLineEdit()
        model_edit.setPlaceholderText(ProviderRegistry.get_default_model(provider))
        status = QLabel("")
        status.setWordWrap(True)
        status.setStyleSheet(f"color:{COLORS['muted']};font-family:monospace;font-size:10px;")
        test_btn = QPushButton("Test Connection")
        test_btn.clicked.connect(lambda _=False, p=provider: self._test_provider(p))
        layout.addRow("API key:", key_edit)
        layout.addRow("Default model:", model_edit)
        layout.addRow(test_btn)
        layout.addRow("Status:", status)
        self._key_fields[provider] = key_edit
        self._model_fields[provider] = model_edit
        self._status_labels[provider] = status
        return w

    def _load_fields(self):
        self._default_model.setText(self.settings.selected_model)
        for name in ProviderRegistry.all_providers():
            existing = self.settings.api_keys.get(name, "")
            if existing:
                self._key_fields[name].setPlaceholderText("••••••••  (saved)")
            model = self.settings.default_models.get(
                name, ProviderRegistry.get_default_model(name)
            )
            self._model_fields[name].setText(model)

    def _test_provider(self, provider: str):
        status = self._status_labels[provider]
        status.setText("Testing…")
        status.setStyleSheet(f"color:{COLORS['muted']};font-family:monospace;font-size:10px;")
        key_text = self._key_fields[provider].text().strip()
        api_key = key_text or self.settings.get_api_key(provider)
        model = self._model_fields[provider].text().strip() or None

        worker = _AsyncWorker(
            self._copilot.test_connection,
            provider,
            api_key or None,
            model,
        )
        worker.finished_ok.connect(
            lambda result, p=provider: self._on_test_result(p, result)
        )
        worker.finished_err.connect(
            lambda err, p=provider: self._on_test_error(p, err)
        )
        worker.finished.connect(lambda w=worker: self._test_workers.remove(w) if w in self._test_workers else None)
        self._test_workers.append(worker)
        worker.start()

    def _on_test_result(self, provider: str, result):
        ok, msg = result
        lbl = self._status_labels[provider]
        color = COLORS["success"] if ok else COLORS["error"]
        prefix = "OK" if ok else "Failed"
        lbl.setStyleSheet(f"color:{color};font-family:monospace;font-size:10px;")
        lbl.setText(f"{prefix}: {msg}")

    def _on_test_error(self, provider: str, err: str):
        lbl = self._status_labels[provider]
        lbl.setStyleSheet(f"color:{COLORS['error']};font-family:monospace;font-size:10px;")
        lbl.setText(f"Failed: {err}")

    def _save(self):
        for name in ProviderRegistry.all_providers():
            key = self._key_fields[name].text().strip()
            if key:
                self.settings.api_keys[name] = key
            model = self._model_fields[name].text().strip()
            if model:
                self.settings.default_models[name] = model
        self.settings.selected_provider = self._provider_combo.currentData()
        self.settings.selected_model = self._default_model.text().strip()
        self.settings.save()
        self.accept()


class CopilotDockWidget(QDockWidget):
    settings_changed = pyqtSignal(CopilotSettings)

    def __init__(self, get_circuit_fn=None, get_sim_results_fn=None, parent=None):
        super().__init__("Copilot", parent)
        self.setObjectName("CopilotDock")
        self.setAllowedAreas(
            Qt.DockWidgetArea.LeftDockWidgetArea | Qt.DockWidgetArea.RightDockWidgetArea
        )
        self._get_circuit = get_circuit_fn or (lambda: None)
        self._get_sim_results = get_sim_results_fn or (lambda: {})
        self.settings = CopilotSettings.load()
        circuit = self._get_circuit() or _empty_circuit()
        self._copilot = Copilot(
            circuit,
            settings=self.settings,
            sim_results=self._get_sim_results(),
        )
        self._chat_worker: _AsyncWorker | None = None
        self._build_ui()
        self._refresh_provider_label()

    def _build_ui(self):
        panel = QWidget()
        panel.setStyleSheet(_PANEL_STYLE)
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(6, 6, 6, 6)

        header = QHBoxLayout()
        self._provider_lbl = QLabel()
        self._provider_lbl.setStyleSheet(
            f"color:{COLORS['accent']};font-family:monospace;font-size:11px;"
        )
        settings_btn = QPushButton("⚙")
        settings_btn.setFixedWidth(32)
        settings_btn.setToolTip("AI provider settings")
        settings_btn.clicked.connect(self._open_settings)
        header.addWidget(self._provider_lbl, stretch=1)
        header.addWidget(settings_btn)
        layout.addLayout(header)

        self._history = QTextEdit()
        self._history.setReadOnly(True)
        self._history.setPlaceholderText("Ask Egottol Copilot about your schematic…")
        layout.addWidget(self._history, stretch=1)

        input_row = QHBoxLayout()
        self._input = QLineEdit()
        self._input.setPlaceholderText("Message…")
        self._input.returnPressed.connect(self._send_message)
        self._send_btn = QPushButton("Send")
        self._send_btn.clicked.connect(self._send_message)
        input_row.addWidget(self._input, stretch=1)
        input_row.addWidget(self._send_btn)
        layout.addLayout(input_row)

        self.setWidget(panel)

    def reload_settings(self, settings: CopilotSettings | None = None):
        self.settings = settings or CopilotSettings.load()
        circuit = self._get_circuit() or _empty_circuit()
        self._copilot = Copilot(
            circuit,
            settings=self.settings,
            sim_results=self._get_sim_results(),
        )
        self._refresh_provider_label()
        self.settings_changed.emit(self.settings)

    def _refresh_provider_label(self):
        provider = self.settings.selected_provider
        label = ProviderRegistry.labels().get(provider, provider)
        model = self.settings.get_model(provider)
        self._provider_lbl.setText(f"{label}  ·  {model}")

    def _open_settings(self):
        dlg = CopilotSettingsDialog(self.settings, self)
        if dlg.exec():
            self.reload_settings(dlg.settings)

    def _append_message(self, role: str, text: str):
        color = COLORS["accent"] if role == "user" else COLORS["text"]
        name = "You" if role == "user" else "Copilot"
        self._history.append(
            f'<span style="color:{COLORS["muted"]}">{name}:</span> '
            f'<span style="color:{color}">{text}</span>'
        )

    def _send_message(self):
        text = self._input.text().strip()
        if not text or (self._chat_worker and self._chat_worker.isRunning()):
            return
        self._input.clear()
        self._append_message("user", text)
        self._copilot.update_context(
            circuit=self._get_circuit() or _empty_circuit(),
            sim_results=self._get_sim_results(),
        )
        self._set_chat_busy(True)
        self._chat_worker = _AsyncWorker(self._copilot.chat_sync, text)
        self._chat_worker.finished_ok.connect(self._on_chat_response)
        self._chat_worker.finished_err.connect(self._on_chat_error)
        self._chat_worker.finished.connect(lambda: self._set_chat_busy(False))
        self._chat_worker.start()

    def _set_chat_busy(self, busy: bool):
        self._input.setEnabled(not busy)
        self._send_btn.setEnabled(not busy)
        if busy:
            self._provider_lbl.setText(
                self._provider_lbl.text().split("  ·  ")[0] + "  ·  …"
            )
        else:
            self._refresh_provider_label()

    def _on_chat_response(self, response: CopilotResponse):
        self._append_message("assistant", response.message)
        if response.tool_results:
            for tr in response.tool_results:
                tool = tr.get("tool", "?")
                ok = tr.get("result", {}).get("ok", True)
                mark = "ok" if ok else "fail"
                self._history.append(
                    f'<span style="color:{COLORS["muted"]}">  tool {tool}: {mark}</span>'
                )

    def _on_chat_error(self, err: str):
        self._history.append(f'<span style="color:{COLORS["error"]}">Error: {err}</span>')


_EII_REGISTRY_KEYS = frozenset({
    "IMPULSE_DETECTOR", "INFERENCE_ENCODER", "INFERENCE_ENGINE",
    "EII_PIPELINE", "MEMRISTOR", "CROSSBAR", "LIF_NEURON", "MZI_MESH",
})


def circuit_has_eii_components(circuit) -> bool:
    if circuit is None:
        return False
    markers = ("eii", "impulse", "memristor", "encoding", "inference", "actuator", "detector")
    for comp in getattr(circuit, "components", []):
        if comp.metadata.get("eii") or comp.metadata.get("eii_role"):
            return True
        reg_key = comp.metadata.get("registry_key", "")
        if reg_key in _EII_REGISTRY_KEYS:
            return True
        comp_id = getattr(comp, "id", "").lower()
        comp_name = getattr(comp, "name", "").lower()
        if any(m in comp_id or m in comp_name for m in markers):
            return True
    return False


def run_eii_pipeline(circuit, log_fn=print) -> bool:
    if not circuit_has_eii_components(circuit):
        log_fn("No EII components found in circuit.")
        return False
    try:
        from egottol.engines.orchestrator import MultiDomainOrchestrator

        orch = MultiDomainOrchestrator(circuit)
        t_stop = 1e-3
        log_fn("─── EII Pipeline ─────────────────────")
        if hasattr(orch, "run_eii_window"):
            result = orch.run_eii_window(t_stop)
        else:
            result = orch.step_transient(t_stop)
        log_fn(f"EII run complete: {result}")
        return True
    except ImportError:
        try:
            from egottol.engines.eii.pipeline import EIIPipeline

            pipeline = EIIPipeline.from_circuit(circuit)
            log_fn("─── EII Pipeline ─────────────────────")
            result = pipeline.run_duration(1e-3)
            log_fn(f"EII run complete: {result}")
            return True
        except ImportError:
            log_fn("EII engine not installed — add egottol/engines/eii or orchestrator.")
            return False
        except Exception as exc:
            log_fn(f"EII error: {exc}")
            return False
    except Exception as exc:
        log_fn(f"EII error: {exc}")
        return False
