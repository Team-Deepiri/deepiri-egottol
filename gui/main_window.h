#pragma once

#include <QMainWindow>
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
    void runDemoDcOperatingPoint();
    void runDemoTransient();
    void runDemoAcAnalysis();
    void runDemoEiiPipeline();
    void parseSampleVhdl();
    void decodeSampleAdsb();

private:
    void buildMenusAndToolbars();
    void buildDocks();

    SchematicScene* scene_ = nullptr;
    SchematicView* view_ = nullptr;
    SelectionTool* selectionTool_ = nullptr;
    WireTool* wireTool_ = nullptr;
    PropertyEditor* propertyEditor_ = nullptr;
    WaveformPlotter* waveformPlotter_ = nullptr;
    SimulationController* simController_ = nullptr;
    std::shared_ptr<SimulationData> simData_;
};

}
