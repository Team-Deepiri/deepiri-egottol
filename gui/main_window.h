#pragma once

#include <QMainWindow>
#include <memory>

class QToolBar;
class QStatusBar;
class QLabel;
class QDockWidget;

namespace deepiri {

class SchematicScene;
class SchematicView;
class SchematicDocument;
class ComponentPalette;
class ConsolePanel;
class WaveformPanel;
class WireTool;
class SelectionTool;

/**
 * MainWindow — C++ equivalent of Python EgottolApp (egottol/ui/main.py).
 *
 * STAGE 1 responsibilities (this file):
 *   - Assemble central schematic view + docks + toolbar + status bar
 *   - Route palette/toolbar actions to place mode on SchematicScene
 *   - Log stub messages for DC/Transient until Stage 5/6
 *
 * YOU IMPLEMENT next (see scope.md):
 *   - runDcAnalysis()        → Stage 5 (schematic_to_circuit + MNASolver)
 *   - runTransientAnalysis() → Stage 6
 *   - openSimConfig()        → Stage 6 SimConfigDialog
 *   - probeServices()        → Stage 9 ServiceDiscovery port
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    SchematicScene* schematicScene() const;
    SchematicDocument* document() const;

private slots:
    void onComponentRequested(const QString& registryKey);
    void onPlaceModeChanged(const QString& modeText);
    void runDcAnalysis();
    void runTransientAnalysis();
    void openSimConfig();
    void clearCanvas();
    void zoomIn();
    void zoomOut();
    void zoomFit();

private:
    void applyDarkTheme();
    void setupCentralView();
    void setupDocks();
    void setupToolbar();
    void setupStatusBar();
    void setupTools();

    SchematicScene* scene_ = nullptr;
    SchematicView* view_ = nullptr;
    SchematicDocument* document_ = nullptr;

    ComponentPalette* palette_ = nullptr;
    ConsolePanel* console_ = nullptr;
    WaveformPanel* waveform_ = nullptr;

    WireTool* wire_tool_ = nullptr;
    SelectionTool* selection_tool_ = nullptr;

    QLabel* mode_label_ = nullptr;
    QDockWidget* services_dock_ = nullptr;
};

} // namespace deepiri
