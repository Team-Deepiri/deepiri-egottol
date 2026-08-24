#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <string>

namespace deepiri {

class SchematicScene;

// Runs simulations through the native core solvers. Prefer the schematic when
// present; fall back to demo circuits for smoke-testing the engine.
class SimulationController : public QObject {
    Q_OBJECT

public:
    explicit SimulationController(QObject* parent = nullptr);

    struct DcResult {
        bool success = false;
        QString message;
        QString summary;
    };

    struct TransientResult {
        bool converged = false;
        QString message;
        std::vector<double> timePoints;
        std::vector<double> values;
        QString traceName;
    };

    struct AcResult {
        bool success = false;
        QString message;
        std::vector<double> frequenciesHz;
        std::vector<double> magnitude;
        QString traceName;
    };

    struct EiiResult {
        bool ran = false;
        QString summary;
    };

    // Simulate whatever is drawn. Falls back to demos only when the scene is
    // null/empty and allowDemoFallback is true.
    DcResult runDcOperatingPoint(const SchematicScene* scene, bool allowDemoFallback = true);
    TransientResult runTransient(const SchematicScene* scene, bool allowDemoFallback = true);
    AcResult runAcAnalysis(const SchematicScene* scene, bool allowDemoFallback = true);

    // Explicit netlist path (CLI / tests).
    DcResult runDcFromNetlist(const std::string& netlist);
    TransientResult runTransientFromNetlist(const std::string& netlist);
    AcResult runAcFromNetlist(const std::string& netlist);

    DcResult runDemoDcOperatingPoint();
    TransientResult runDemoTransient();
    AcResult runDemoAcAnalysis();
    EiiResult runDemoEiiPipeline();
};

}
