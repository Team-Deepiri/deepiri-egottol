#include "main_window.h"

#include "scene.h"
#include "schematic_view.h"
#include "selection_tool.h"
#include "wire_tool.h"
#include "component_item.h"
#include "wire_item.h"
#include "property_editor.h"
#include "waveform_plotter.h"
#include "simulation_controller.h"
#include "../io/simulation_data.h"
#include "../io/project_loader.h"
#include "schematic_mermaid.h"
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
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QMap>
#include <QIODevice>

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

    statusBar()->showMessage("Ready — draw a circuit, then Simulate → Run", 5000);
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
    QMenu* fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&New", QKeySequence::New, this, &MainWindow::newProject);
    fileMenu->addAction("&Open…", QKeySequence::Open, this, &MainWindow::openProject);
    fileMenu->addAction("&Save", QKeySequence::Save, this, &MainWindow::saveProject);
    fileMenu->addAction("Save &As…", QKeySequence::SaveAs, this, &MainWindow::saveProjectAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Export Waveform &CSV…", this, &MainWindow::exportWaveformCsv);
    fileMenu->addAction("Export Schematic &Mermaid…", this, &MainWindow::exportMermaid);
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", QKeySequence::Quit, this, &QWidget::close);

    QToolBar* viewToolbar = addToolBar("View");
    viewToolbar->addAction("Zoom In", view_, &SchematicView::zoom_in);
    viewToolbar->addAction("Zoom Out", view_, &SchematicView::zoom_out);
    viewToolbar->addAction("Fit", view_, &SchematicView::zoom_fit);
    viewToolbar->addAction("Reset", view_, &SchematicView::zoom_reset);

    QToolBar* insertToolbar = addToolBar("Insert");
    struct Entry { const char* label; ComponentType type; };
    static const Entry entries[] = {
        {"Resistor", ComponentType::RESISTOR},
        {"Capacitor", ComponentType::CAPACITOR},
        {"Inductor", ComponentType::INDUCTOR},
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

    QMenu* simMenu = menuBar()->addMenu("&Simulate");
    simMenu->addAction("Run DC Operating Point", this, &MainWindow::runDcOperatingPoint);
    simMenu->addAction("Run Transient", this, &MainWindow::runTransient);
    simMenu->addAction("Run AC/Bode Analysis", this, &MainWindow::runAcAnalysis);
    simMenu->addSeparator();
    simMenu->addAction("Run EII Pipeline (demo)", this, &MainWindow::runDemoEiiPipeline);

    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    toolsMenu->addAction("Parse Sample VHDL", this, &MainWindow::parseSampleVhdl);
    toolsMenu->addAction("Decode Sample ADS-B Frame", this, &MainWindow::decodeSampleAdsb);
}

void MainWindow::setCurrentFile(const QString& path) {
    currentFile_ = path;
    if (path.isEmpty()) {
        setWindowTitle("Deepiri Egottol");
    } else {
        setWindowTitle(QString("Deepiri Egottol — %1").arg(QFileInfo(path).fileName()));
    }
}

void MainWindow::clearSchematic() {
    const auto comps = scene_->components();
    for (ComponentItem* c : comps) {
        scene_->remove_component(c);
        delete c;
    }
    const auto wires = scene_->wires();
    for (WireItem* w : wires) {
        scene_->remove_wire(w);
        delete w;
    }
    componentInsertCount = 0;
    propertyEditor_->clearComponent();
}

void MainWindow::newProject() {
    clearSchematic();
    setCurrentFile(QString());
    statusBar()->showMessage("New project", 3000);
}

bool MainWindow::writeProjectTo(const QString& path) {
    Project project;
    project.name = QFileInfo(path).completeBaseName().toStdString();
    project.version = "1.0";
    project.format = "egottol-project";

    for (ComponentItem* c : scene_->components()) {
        SchematicComponentData data;
        data.type = static_cast<int>(c->component_type());
        data.label = c->label().toStdString();
        data.x = c->pos().x();
        data.y = c->pos().y();
        const auto props = c->properties();
        for (auto it = props.begin(); it != props.end(); ++it) {
            data.properties[it.key().toStdString()] = it.value().toStdString();
        }
        project.components.push_back(std::move(data));
    }

    for (WireItem* w : scene_->wires()) {
        SchematicWireData wd;
        for (const QPointF& p : w->points()) {
            wd.points.emplace_back(p.x(), p.y());
        }
        project.wires.push_back(std::move(wd));
    }

    ProjectLoader loader;
    loader.setProject(project);
    if (!loader.save(path.toStdString())) {
        return false;
    }
    setCurrentFile(path);
    return true;
}

bool MainWindow::readProjectFrom(const QString& path) {
    ProjectLoader loader;
    if (!loader.load(path.toStdString())) {
        return false;
    }
    Project project = loader.getProject();
    clearSchematic();

    for (const auto& c : project.components) {
        auto* item = new ComponentItem(static_cast<ComponentType>(c.type),
                                       QString::fromStdString(c.label));
        item->setPos(c.x, c.y);
        QMap<QString, QString> props;
        for (const auto& kv : c.properties) {
            props[QString::fromStdString(kv.first)] = QString::fromStdString(kv.second);
        }
        if (!props.isEmpty()) {
            item->set_properties(props);
        }
        scene_->add_component(item);
        ++componentInsertCount;
    }

    for (const auto& w : project.wires) {
        if (w.points.size() < 2) continue;
        QPointF start(w.points.front().first, w.points.front().second);
        QPointF end(w.points.back().first, w.points.back().second);
        auto* wire = new WireItem(start, end);
        if (w.points.size() > 2) {
            QList<QPointF> pts;
            for (const auto& p : w.points) {
                pts.append(QPointF(p.first, p.second));
            }
            wire->set_points(pts);
        }
        scene_->add_wire(wire);
    }

    setCurrentFile(path);
    return true;
}

void MainWindow::openProject() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Project", QString(), "Egottol Project (*.egt *.json);;All Files (*)");
    if (path.isEmpty()) return;
    if (!readProjectFrom(path)) {
        QMessageBox::warning(this, "Open failed", "Could not load project file.");
        return;
    }
    statusBar()->showMessage(QString("Opened %1").arg(path), 4000);
}

void MainWindow::saveProject() {
    if (currentFile_.isEmpty()) {
        saveProjectAs();
        return;
    }
    if (!writeProjectTo(currentFile_)) {
        QMessageBox::warning(this, "Save failed", "Could not write project file.");
        return;
    }
    statusBar()->showMessage(QString("Saved %1").arg(currentFile_), 4000);
}

void MainWindow::saveProjectAs() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Project As", "untitled.egt", "Egottol Project (*.egt);;JSON (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".egt") && !path.endsWith(".json")) {
        path += ".egt";
    }
    if (!writeProjectTo(path)) {
        QMessageBox::warning(this, "Save failed", "Could not write project file.");
        return;
    }
    statusBar()->showMessage(QString("Saved %1").arg(path), 4000);
}

void MainWindow::exportWaveformCsv() {
    if (waveformPlotter_->traces().empty()) {
        QMessageBox::information(this, "Export CSV", "No waveform data to export — run a simulation first.");
        return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, "Export Waveform CSV", "waveform.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".csv")) path += ".csv";
    if (!waveformPlotter_->exportCSV(path)) {
        QMessageBox::warning(this, "Export failed", "Could not write CSV file.");
        return;
    }
    statusBar()->showMessage(QString("Exported %1").arg(path), 4000);
}

void MainWindow::exportMermaid() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export Schematic Mermaid", "schematic.mmd", "Mermaid (*.mmd *.md);;All Files (*)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".mmd") && !path.endsWith(".md")) path += ".mmd";
    std::string mermaid = schematicToMermaid(scene_);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export failed", "Could not write Mermaid file.");
        return;
    }
    f.write(QByteArray::fromStdString(mermaid));
    f.close();
    statusBar()->showMessage(QString("Exported %1").arg(path), 4000);
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
    QString label;
    switch (type) {
        case ComponentType::RESISTOR: label = QString("R%1").arg(++componentInsertCount); break;
        case ComponentType::CAPACITOR: label = QString("C%1").arg(++componentInsertCount); break;
        case ComponentType::INDUCTOR: label = QString("L%1").arg(++componentInsertCount); break;
        case ComponentType::SOURCE: label = QString("V%1").arg(++componentInsertCount); break;
        case ComponentType::GROUND: label = QString("GND%1").arg(++componentInsertCount); break;
        case ComponentType::LED: label = QString("D%1").arg(++componentInsertCount); break;
        default: label = QString("X%1").arg(++componentInsertCount); break;
    }
    auto* item = new ComponentItem(type, label);
    item->setPos(componentInsertCount * 20 % 400, componentInsertCount * 35 % 300);
    scene_->add_component(item);
    statusBar()->showMessage(QString("Inserted %1").arg(item->label()), 3000);
}

void MainWindow::runDcOperatingPoint() {
    // Empty canvas → demo; drawn circuit → never silently substitute a demo.
    const bool empty = scene_->items().isEmpty();
    auto result = simController_->runDcOperatingPoint(scene_, /*allowDemoFallback=*/empty);
    if (result.success) {
        statusBar()->showMessage(result.summary, 8000);
        QMessageBox::information(this, "DC Operating Point", result.summary);
    } else {
        QMessageBox::warning(this, "DC Operating Point failed", result.message);
    }
}

void MainWindow::runTransient() {
    const bool empty = scene_->items().isEmpty();
    auto result = simController_->runTransient(scene_, /*allowDemoFallback=*/empty);
    if (!result.converged) {
        QMessageBox::warning(this, "Transient simulation failed", result.message);
        return;
    }
    waveformPlotter_->clear();
    waveformPlotter_->setTrace(result.traceName.isEmpty() ? "transient" : result.traceName,
                               result.timePoints, result.values);
    waveformPlotter_->animateSweep();
    statusBar()->showMessage("Transient simulation complete", 5000);
}

void MainWindow::runAcAnalysis() {
    const bool empty = scene_->items().isEmpty();
    auto result = simController_->runAcAnalysis(scene_, /*allowDemoFallback=*/empty);
    if (!result.success) {
        QMessageBox::warning(this, "AC/Bode analysis failed", result.message);
        return;
    }
    waveformPlotter_->clear();
    waveformPlotter_->setTrace(result.traceName.isEmpty() ? "|H(f)|" : result.traceName,
                               result.frequenciesHz, result.magnitude);
    waveformPlotter_->animateSweep();
    statusBar()->showMessage("AC/Bode sweep complete", 5000);
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
    demod.processIQ({});
    QMessageBox::information(this, "ADS-B Decoder (native)",
        QString("Native ADSBDemodulator ran — %1 aircraft tracked so far.")
            .arg(demod.getDetectedCount()));
}

}
