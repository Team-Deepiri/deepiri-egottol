#include "main_window.h"

#include "scene.h"
#include "schematic_view.h"
#include "selection_tool.h"
#include "wire_tool.h"
#include "component_item.h"
#include "property_editor.h"
#include "waveform_plotter.h"
#include "simulation_controller.h"
#include "../io/simulation_data.h"
#include "../logic/vhdl_parser.h"
#include "../logic/subckt.h"
#include "../avionics/adsb_decoder.h"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QDockWidget>
#include <QStatusBar>
#include <QMessageBox>
#include <QAction>

namespace deepiri {

namespace {
int componentInsertCount = 0;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Deepiri Egottol");
    resize(1200, 800);

    scene_ = new SchematicScene(this);
    view_ = new SchematicView(scene_, this);
    setCentralWidget(view_);

    selectionTool_ = new SelectionTool(this);
    selectionTool_->set_scene(scene_);
    selectionTool_->activate();
    scene_->set_selection_tool(selectionTool_);

    wireTool_ = new WireTool(this);
    wireTool_->set_scene(scene_);
    scene_->set_wire_tool(wireTool_);

    simController_ = new SimulationController(this);

    connect(scene_, &SchematicScene::selectionChanged, this, &MainWindow::onSceneSelectionChanged);

    buildDocks();
    buildMenusAndToolbars();

    statusBar()->showMessage("Ready — native C++ engine loaded", 5000);
}

MainWindow::~MainWindow() = default;

void MainWindow::setSimulationData(std::shared_ptr<SimulationData> data) {
    simData_ = std::move(data);
}

void MainWindow::buildDocks() {
    propertyEditor_ = new PropertyEditor(this);
    auto* propertyDock = new QDockWidget("Properties", this);
    propertyDock->setWidget(propertyEditor_);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);

    waveformPlotter_ = new WaveformPlotter(this);
    auto* waveformDock = new QDockWidget("Waveform Viewer", this);
    waveformDock->setWidget(waveformPlotter_);
    addDockWidget(Qt::BottomDockWidgetArea, waveformDock);
}

void MainWindow::buildMenusAndToolbars() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    // View toolbar: zoom + tools
    QToolBar* viewToolbar = addToolBar("View");
    viewToolbar->addAction("Zoom In", view_, &SchematicView::zoom_in);
    viewToolbar->addAction("Zoom Out", view_, &SchematicView::zoom_out);
    viewToolbar->addAction("Fit", view_, &SchematicView::zoom_fit);
    viewToolbar->addAction("Reset", view_, &SchematicView::zoom_reset);

    // Insert toolbar: drop a few common components onto the canvas
    QToolBar* insertToolbar = addToolBar("Insert");
    struct Entry { const char* label; ComponentType type; };
    static const Entry entries[] = {
        {"Resistor", ComponentType::RESISTOR},
        {"Capacitor", ComponentType::CAPACITOR},
        {"Source", ComponentType::SOURCE},
        {"Ground", ComponentType::GROUND},
        {"LED", ComponentType::LED},
    };
    for (const auto& entry : entries) {
        QAction* action = insertToolbar->addAction(entry.label);
        ComponentType type = entry.type;
        connect(action, &QAction::triggered, this, [this, type]() {
            insertComponent(static_cast<int>(type));
        });
    }

    // Simulate menu
    QMenu* simMenu = menuBar()->addMenu("&Simulate");
    simMenu->addAction("Run DC Operating Point (demo circuit)", this, &MainWindow::runDemoDcOperatingPoint);
    simMenu->addAction("Run Transient (demo circuit)", this, &MainWindow::runDemoTransient);
    simMenu->addAction("Run AC/Bode Analysis (demo RC low-pass)", this, &MainWindow::runDemoAcAnalysis);
    simMenu->addAction("Run EII Pipeline (demo step signal)", this, &MainWindow::runDemoEiiPipeline);

    // Tools menu: wire in avionics/VHDL that already exist natively
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("Parse Sample VHDL", this, &MainWindow::parseSampleVhdl);
    toolsMenu->addAction("Decode Sample ADS-B Frame", this, &MainWindow::decodeSampleAdsb);
}

void MainWindow::onSceneSelectionChanged() {
    QList<QGraphicsItem*> selected = scene_->selectedItems();
    ComponentItem* component = selected.isEmpty() ? nullptr : dynamic_cast<ComponentItem*>(selected.first());
    if (component) {
        propertyEditor_->setComponent(component);
    } else {
        propertyEditor_->clearComponent();
    }
}

void MainWindow::insertComponent(int componentType) {
    auto type = static_cast<ComponentType>(componentType);
    auto* item = new ComponentItem(type, QString("C%1").arg(++componentInsertCount));
    item->setPos(componentInsertCount * 20 % 400, componentInsertCount * 35 % 300);
    scene_->add_component(item);
    statusBar()->showMessage(QString("Inserted %1").arg(item->label()), 3000);
}

void MainWindow::runDemoDcOperatingPoint() {
    auto result = simController_->runDemoDcOperatingPoint();
    if (result.success) {
        statusBar()->showMessage(result.summary, 8000);
        QMessageBox::information(this, "DC Operating Point (native solver)", result.summary);
    } else {
        QMessageBox::warning(this, "DC Operating Point failed", result.message);
    }
}

void MainWindow::runDemoTransient() {
    auto result = simController_->runDemoTransient();
    if (!result.converged) {
        QMessageBox::warning(this, "Transient simulation failed", result.message);
        return;
    }
    waveformPlotter_->clear();
    waveformPlotter_->setTrace("node1 (I1 = 1mA into open node)", result.timePoints, result.values);
    waveformPlotter_->animateSweep();
    statusBar()->showMessage("Transient simulation complete — native core/Transient", 5000);
}

void MainWindow::runDemoAcAnalysis() {
    auto result = simController_->runDemoAcAnalysis();
    if (!result.success) {
        QMessageBox::warning(this, "AC/Bode analysis failed", result.message);
        return;
    }
    waveformPlotter_->clear();
    waveformPlotter_->setTrace("|H(f)| at C1 (RC low-pass)", result.frequenciesHz, result.magnitude);
    waveformPlotter_->animateSweep();
    statusBar()->showMessage("AC/Bode sweep complete — native core/ACAnalysis", 5000);
}

void MainWindow::runDemoEiiPipeline() {
    auto result = simController_->runDemoEiiPipeline();
    statusBar()->showMessage(result.summary, 8000);
    QMessageBox::information(this, "EII Pipeline (native)", result.summary);
}

void MainWindow::parseSampleVhdl() {
    static const char* kSampleVhdl =
        "entity AND2 is\n"
        "  port (A : in bit; B : in bit; Y : out bit);\n"
        "end entity AND2;\n";

    VHDLParser parser;
    bool ok = parser.parse(kSampleVhdl);
    if (ok && parser.getSubckt()) {
        QMessageBox::information(this, "VHDL Parser (native)",
            QString("Parsed subcircuit: %1").arg(QString::fromStdString(parser.getSubckt()->getName())));
    } else {
        QMessageBox::warning(this, "VHDL Parser (native)",
            QString("Parse failed: %1").arg(QString::fromStdString(parser.getLastError())));
    }
}

void MainWindow::decodeSampleAdsb() {
    ADSBDemodulator demod;
    // No real IQ capture wired up yet — this proves the native avionics
    // library links and runs inside the desktop app, not a full RF pipeline.
    demod.processIQ({});
    QMessageBox::information(this, "ADS-B Decoder (native)",
        QString("Native ADSBDemodulator ran — %1 aircraft tracked so far.")
            .arg(demod.getDetectedCount()));
}

}
