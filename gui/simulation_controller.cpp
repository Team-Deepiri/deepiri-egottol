#include "simulation_controller.h"

#include "../core/mna_solver.h"
#include "../core/transient.h"
#include "../core/ac_analysis.h"
#include "../core/eii/pipeline.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "../models/resistor.h"
#include "../models/capacitor.h"

#include <map>
#include <memory>

namespace deepiri {

SimulationController::SimulationController(QObject* parent) : QObject(parent) {}

SimulationController::DcResult SimulationController::runDemoDcOperatingPoint() {
    DcResult result;

    auto vsrc = std::make_shared<Vsrc>("V1", 5.0);
    vsrc->setNodes(1, 0);
    auto r1 = std::make_shared<Resistor>("R1", 1000.0);
    r1->setNodes(1, 2);
    auto r2 = std::make_shared<Resistor>("R2", 1000.0);
    r2->setNodes(2, 0);

    std::vector<std::shared_ptr<Device>> devices{vsrc, r1, r2};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    MNASolver solver;
    auto solution = solver.solve(devices, nodeMap, {});

    result.success = solution.success;
    result.message = QString::fromStdString(solution.message);
    if (solution.success && solution.voltages.size() >= 2) {
        result.summary = QString("Demo divider (V1=5V, R1=R2=1k):  node1 = %1 V   node2 = %2 V")
                              .arg(solution.voltages[0], 0, 'f', 4)
                              .arg(solution.voltages[1], 0, 'f', 4);
    }
    return result;
}

SimulationController::TransientResult SimulationController::runDemoTransient() {
    TransientResult result;

    auto isrc = std::make_shared<Isrc>("I1", 1e-3);
    isrc->setNodes(1, 2);

    std::vector<std::shared_ptr<Device>> devices{isrc};

    Transient transient;
    auto sim = transient.simulate(0.0, 1e-3, 1e-5, devices, 2);

    result.converged = sim.converged;
    result.message = QString::fromStdString(sim.message);
    result.timePoints = sim.timePoints;
    result.values.reserve(sim.nodeVoltages.size());
    for (const auto& state : sim.nodeVoltages) {
        result.values.push_back(state.empty() ? 0.0 : state[0]);
    }
    return result;
}

SimulationController::AcResult SimulationController::runDemoAcAnalysis() {
    AcResult result;

    // RC low-pass: Vsrc(1V AC) -> R(1k) -> node2 -> C(1uF) -> ground.
    auto vsrc = std::make_shared<Vsrc>("V1", 1.0);
    vsrc->setNodes(1, 0);
    auto r1 = std::make_shared<Resistor>("R1", 1000.0);
    r1->setNodes(1, 2);
    auto c1 = std::make_shared<Capacitor>("C1", 1e-6);
    c1->setNodes(2, 0);

    std::vector<std::shared_ptr<Device>> devices{vsrc, r1, c1};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    ACAnalysis ac;
    auto sweep = ac.sweep(devices, nodeMap, 1.0, 1.0e6, 60);

    result.success = sweep.success;
    result.message = QString::fromStdString(sweep.message);
    if (sweep.success && sweep.magnitude.size() >= 2) {
        result.frequenciesHz = sweep.frequenciesHz;
        result.magnitude = sweep.magnitude[1];  // node2 (across the capacitor)
    }
    return result;
}

SimulationController::EiiResult SimulationController::runDemoEiiPipeline() {
    EiiResult result;

    EIIPipelineConfig config;
    config.eii.numChannels = 4;
    config.eii.embeddingDim = 8;
    config.inference.numClasses = 4;
    config.inference.backend = InferenceBackend::Digital;
    config.inference.digitalHead = DigitalHead::Softmax;

    config.weights = Matrix(config.inference.numClasses, config.eii.embeddingDim, 0.0);
    for (int i = 0; i < config.inference.numClasses; ++i) {
        config.weights.at(i, i % config.eii.embeddingDim) = 1.0;
    }
    config.bias.assign(config.inference.numClasses, 0.0);
    config.conductance = Matrix(config.inference.numClasses, config.eii.embeddingDim, 0.0);

    EIIPipeline pipeline(std::move(config));

    const double dt = 1e-4;
    const int numSteps = 100;
    std::vector<std::vector<double>> voltageTrace(numSteps, std::vector<double>(4, 0.0));
    std::vector<std::vector<double>> currentTrace(numSteps, std::vector<double>(4, 0.0));
    for (int i = 20; i < numSteps; ++i) {
        voltageTrace[i][0] = 1.0;  // step input on channel 0
    }

    auto history = pipeline.run(voltageTrace, currentTrace, dt, 0);

    int totalEvents = 0;
    const EIIStepResult* lastInference = nullptr;
    for (const auto& step : history) {
        totalEvents += static_cast<int>(step.events.size());
        if (step.inferenceRan) lastInference = &step;
    }

    result.ran = true;
    if (lastInference) {
        result.summary = QString("EII pipeline: %1 impulse events detected, %2 window(s) inferred, "
                                  "last confidence = %3")
                              .arg(totalEvents)
                              .arg(lastInference != nullptr ? 1 : 0)
                              .arg(lastInference->confidence, 0, 'f', 4);
    } else {
        result.summary = QString("EII pipeline: %1 impulse events detected, no inference window completed")
                              .arg(totalEvents);
    }
    return result;
}

}
