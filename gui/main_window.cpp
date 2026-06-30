#include "main_window.h"
#include "component_palette.h"
#include "console_panel.h"
#include "egottol_theme.h"
#include "scene.h"
#include "schematic_document.h"
#include "schematic_view.h"
#include "selection_tool.h"
#include "waveform_panel.h"
#include "wire_tool.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QStatusBar>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

namespace deepiri {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setWindowTitle(QStringLiteral("deepiri-egottol  —  Schematic & Simulation"));
  resize(1600, 950);
  setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
  applyDarkTheme();

  document_ = new SchematicDocument();
  setupCentralView();
  setupTools();
  setupDocks();
  setupToolbar();
  setupStatusBar();

  connect(palette_, &ComponentPalette::componentRequested, this,
          &MainWindow::onComponentRequested);
  connect(scene_, &SchematicScene::placeModeChanged, this,
          &MainWindow::onPlaceModeChanged);

  console_->appendLine(tr("deepiri-egottol C++ GUI — Stage 1 shell"));
  console_->appendLine(
      tr("Click palette or toolbar to place | Middle-drag pan | Scroll zoom"));
}

MainWindow::~MainWindow() = default;

SchematicScene *MainWindow::schematicScene() const { return scene_; }
SchematicDocument *MainWindow::document() const { return document_; }

void MainWindow::applyDarkTheme() {
  QPalette pal;
  pal.setColor(QPalette::Window, ui::colorBackground());
  pal.setColor(QPalette::WindowText, ui::colorText());
  pal.setColor(QPalette::Base, ui::colorBackgroundDeep());
  pal.setColor(QPalette::Text, ui::colorText());
  pal.setColor(QPalette::Button, ui::colorToolbarBg());
  pal.setColor(QPalette::ButtonText, ui::colorText());
  pal.setColor(QPalette::Highlight, ui::colorHighlight());
  pal.setColor(QPalette::HighlightedText, ui::colorBackground());
  qApp->setPalette(pal);
}

void MainWindow::setupCentralView() {
  scene_ = new SchematicScene(this);
  scene_->set_document(document_);

  view_ = new SchematicView(scene_, this);
  view_->setBackgroundBrush(ui::colorBackground());
  setCentralWidget(view_);

  // Python: QTimer.singleShot(0, centerOn origin)
  view_->centerOn(0, 0);
}

void MainWindow::setupTools() {
  wire_tool_ = new WireTool(this);
  wire_tool_->set_scene(scene_);
  scene_->set_wire_tool(wire_tool_);

  selection_tool_ = new SelectionTool(this);
  selection_tool_->set_scene(scene_);
  selection_tool_->activate();
  scene_->set_selection_tool(selection_tool_);
}

void MainWindow::setupDocks() {
  // --- Left: Components (Python left dock) ---
  palette_ = new ComponentPalette(this);
  auto *compDock = new QDockWidget(tr("Components"), this);
  compDock->setWidget(palette_);
  addDockWidget(Qt::LeftDockWidgetArea, compDock);

  // --- Right: Services placeholder (Python service LEDs) ---
  services_dock_ = new QDockWidget(tr("Services"), this);
  auto *svcWidget = new QWidget(this);
  svcWidget->setStyleSheet(
      QStringLiteral("background:%1;").arg(ui::colorDockBg().name()));
  auto *svcLayout = new QVBoxLayout(svcWidget);
  for (const char *name : {"ZEPGPU", "UQE"}) {
    auto *row = new QHBoxLayout();
    auto *lbl = new QLabel(QString::fromLatin1(name), svcWidget);
    lbl->setStyleSheet(QStringLiteral("color:%1;font-family:monospace;")
                           .arg(ui::colorText().name()));
    auto *led = new QFrame(svcWidget);
    led->setFixedSize(12, 12);
    led->setStyleSheet(QStringLiteral("background:gray;border-radius:6px;"));
    row->addWidget(lbl);
    row->addWidget(led);
    svcLayout->addLayout(row);
  }
  svcLayout->addStretch();
  services_dock_->setWidget(svcWidget);
  addDockWidget(Qt::RightDockWidgetArea, services_dock_);

  // --- Bottom: Waveform + Console (Python bottom splitter) ---
  console_ = new ConsolePanel(this);
  waveform_ = new WaveformPanel(this);
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(waveform_);
  splitter->addWidget(console_);
  splitter->setSizes({900, 400});

  auto *btmDock = new QDockWidget(tr("Waveform / Console"), this);
  btmDock->setWidget(splitter);
  addDockWidget(Qt::BottomDockWidgetArea, btmDock);
}

void MainWindow::setupToolbar() {
  auto *tb = addToolBar(tr("Main"));
  tb->setMovable(false);
  tb->setStyleSheet(
      QStringLiteral("background:%1;color:%2;spacing:3px;")
          .arg(ui::colorToolbarBg().name(), ui::colorText().name()));

  auto addAct = [tb](const QString &text, const QString &tip, auto slot,
                     QWidget *receiver) {
    auto *a = new QAction(text, receiver);
    a->setToolTip(tip);
    QObject::connect(a, &QAction::triggered, receiver, slot);
    tb->addAction(a);
  };

  addAct(QStringLiteral("▶ DC"), tr("Run DC operating point"),
         &MainWindow::runDcAnalysis, this);
  addAct(QStringLiteral("⟳ Tran"), tr("Run transient simulation"),
         &MainWindow::runTransientAnalysis, this);
  addAct(QStringLiteral("⚙ Cfg"), tr("Simulation settings"),
         &MainWindow::openSimConfig, this);
  tb->addSeparator();

  struct Quick {
    const char *key;
    const char *label;
    const char *tip;
  };
  const Quick passives[] = {
      {"RES", "R", "Resistor"},        {"CAP", "C", "Capacitor"},
      {"IND", "L", "Inductor"},        {"DIODE", "D", "Diode"},
      {"VSRC", "V", "Voltage Source"}, {"ISRC", "I", "Current Source"},
      {"GND", "⏚", "Ground"},          {"VCC", "VCC", "VCC Rail"},
  };
  for (const Quick &q : passives) {
    addAct(
        QString::fromUtf8(q.label), tr(q.tip),
        [this, k = QString::fromLatin1(q.key)] { onComponentRequested(k); },
        this);
  }
  tb->addSeparator();

  const Quick active[] = {
      {"NPN", "NPN", "NPN BJT"},     {"PNP", "PNP", "PNP BJT"},
      {"AND", "AND", "AND Gate"},    {"OR", "OR", "OR Gate"},
      {"NOT", "NOT", "Inverter"},    {"XOR", "XOR", "XOR Gate"},
      {"DFF", "DFF", "D Flip-Flop"}, {"LM741", "OpAmp", "Op-Amp"},
  };
  for (const Quick &q : active) {
    addAct(
        QString::fromUtf8(q.label), tr(q.tip),
        [this, k = QString::fromLatin1(q.key)] { onComponentRequested(k); },
        this);
  }
  tb->addSeparator();

  addAct(QStringLiteral("🗑"), tr("Clear canvas"), &MainWindow::clearCanvas,
         this);
  addAct(QStringLiteral("−"), tr("Zoom out"), &MainWindow::zoomOut, this);
  addAct(QStringLiteral("+"), tr("Zoom in"), &MainWindow::zoomIn, this);
  addAct(QStringLiteral("⤢"), tr("Fit view"), &MainWindow::zoomFit, this);
}

void MainWindow::setupStatusBar() {
  mode_label_ = new QLabel(QStringLiteral("SELECT"), this);
  mode_label_->setStyleSheet(
      QStringLiteral("color:%1;font-family:monospace;padding:0 8px;")
          .arg(ui::colorLabel().name()));
  statusBar()->addPermanentWidget(mode_label_);
  statusBar()->showMessage(tr(
      "Click palette/toolbar to place | Esc cancel | Del delete | Space fit"));
}

void MainWindow::onComponentRequested(const QString &registryKey) {
  scene_->set_place_mode(registryKey);
  QTimer::singleShot(0, view_, [this]() { view_->setFocus(Qt::OtherFocusReason); });
}

void MainWindow::onPlaceModeChanged(const QString &modeText) {
  mode_label_->setText(modeText);
}

void MainWindow::runDcAnalysis() {
  console_->appendLine(
      QStringLiteral("─── DC Analysis ───────────────────────"));
  // TODO Stage 5: schematic_to_circuit + MNASolver; annotate scene +
  // waveform_->showDcResults
  console_->appendLine(
      tr("Not implemented — see gui/schematic_to_circuit.h (Stage 5)"));
}

void MainWindow::runTransientAnalysis() {
  console_->appendLine(
      QStringLiteral("─── Transient ─────────────────────────"));
  console_->appendLine(tr("Not implemented — Stage 6"));
}

void MainWindow::openSimConfig() {
  console_->appendLine(tr("Simulation config dialog — Stage 6"));
}

void MainWindow::clearCanvas() {
  scene_->clear_canvas();
  document_->clear();
  waveform_->clear();
  console_->appendLine(tr("Canvas cleared."));
}

void MainWindow::zoomIn() { view_->zoom_in(); }
void MainWindow::zoomOut() { view_->zoom_out(); }
void MainWindow::zoomFit() { view_->zoom_fit(); }

} // namespace deepiri
