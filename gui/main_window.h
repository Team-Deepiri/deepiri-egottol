#pragma once

#include <QMainWindow>
#include <QString>
#include <memory>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
QT_END_NAMESPACE

namespace deepiri {

class SchematicScene;
class SchematicView;
class SelectionTool;
class WireTool;
class PropertyEditor;
class WaveformPlotter;
class SimulationController;
class SimulationData;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setSimulationData(std::shared_ptr<SimulationData> data);

private slots:
    void onSceneSelectionChanged();
    void insertComponent(int componentType);
    void runDcOperatingPoint();
    void runTransient();
    void runAcAnalysis();
    void runDemoEiiPipeline();
    void parseSampleVhdl();
    void decodeSampleAdsb();
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportWaveformCsv();
    void exportMermaid();

private:
    void buildMenusAndToolbars();
    void buildDocks();
    bool writeProjectTo(const QString& path);
    bool readProjectFrom(const QString& path);
    void clearSchematic();
    void setCurrentFile(const QString& path);

    SchematicScene* scene_ = nullptr;
    SchematicView* view_ = nullptr;
    SelectionTool* selectionTool_ = nullptr;
    WireTool* wireTool_ = nullptr;
    PropertyEditor* propertyEditor_ = nullptr;
    WaveformPlotter* waveformPlotter_ = nullptr;
    SimulationController* simController_ = nullptr;
    std::shared_ptr<SimulationData> simData_;
    QString currentFile_;
};

}
