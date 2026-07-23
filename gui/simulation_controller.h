#pragma once

#include <QObject>
#include <QString>
#include <vector>

namespace deepiri {

// Runs demo circuits directly through the native core/ solver (MNASolver,
// Transient) so the desktop app exercises the real C++ engine end to end,
// independent of the Python engines.
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
    };

    // 5V source into a 1k/1k divider, solved via MNASolver.
    DcResult runDemoDcOperatingPoint();

    // Constant current into a floating node pair, integrated via Transient.
    TransientResult runDemoTransient();

    struct AcResult {
        bool success = false;
        QString message;
        std::vector<double> frequenciesHz;
        std::vector<double> magnitude;
    };

    // RC low-pass (R=1k, C=1uF) swept via the native ACAnalysis solver.
    AcResult runDemoAcAnalysis();

    struct EiiResult {
        bool ran = false;
        QString summary;
    };

    // Feeds a synthetic step signal through the native Phi->Psi->Gamma EII pipeline.
    EiiResult runDemoEiiPipeline();
};

}
