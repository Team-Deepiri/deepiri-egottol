#include "simulation_controller.h"

#include "schematic_netlist.h"
#include "scene.h"

#include "../core/mna_solver.h"
#include "../core/spice_engine.h"
#include "../core/ac_analysis.h"
#include "../core/eii/pipeline.h"
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../models/resistor.h"
#include "../models/capacitor.h"

#include <map>
#include <memory>
#include <cmath>

namespace deepiri {

namespace {

double controlNumber(const NetlistParser& parser, const char* kind, size_t index, double fallback) {
    for (const auto& d : parser.getControlDirectives()) {
        if (d.kind == kind && index < d.numbers.size()) {
            return d.numbers[index];
        }
    }
    return fallback;
}

size_t pickProbeNode(const BuiltCircuit& circuit) {
    // Prefer a non-source node named "out" / "mid", else highest index.
    for (const char* name : {"out", "mid", "n2", "n1"}) {
        auto it = circuit.nodeMap.find(name);
        if (it != circuit.nodeMap.end() && it->second > 0) {
            return it->second;
        }
    }
    size_t best = 0;
    for (const auto& kv : circuit.nodeMap) {
        if (kv.second > best) best = kv.second;
    }
    return best;
}

QString nodeLabel(const BuiltCircuit& circuit, size_t nodeId) {
    for (const auto& kv : circuit.nodeMap) {
        if (kv.second == nodeId) {
            return QString::fromStdString(kv.first);
        }
    }
    return QString("node%1").arg(nodeId);
}

}  // namespace

SimulationController::SimulationController(QObject* parent) : QObject(parent) {}

SimulationController::DcResult SimulationController::runDcFromNetlist(const std::string& netlist) {
    DcResult result;
    NetlistParser parser;
    if (!parser.parse(netlist)) {
        result.message = "Failed to parse netlist";
        return result;
    }

    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    if (!circuit.ok) {
        result.message = QString::fromStdString(circuit.error);
        return result;
    }

    // Production nonlinear DC (Newton + gmin/source stepping); linear MNA fallback.
    auto solution = DcOperatingPoint().solve(circuit.devices, circuit.nodeMap);
    if (!solution.success) {
        solution = MNASolver().solve(circuit.devices, circuit.nodeMap, {});
    }
    result.success = solution.success;
    result.message = QString::fromStdString(solution.message);
    if (!solution.success) return result;

    QStringList parts;
    for (const auto& kv : circuit.nodeMap) {
        if (kv.second == 0) continue;
        size_t idx = kv.second - 1;
        if (idx < solution.voltages.size()) {
            parts << QString("%1=%2V")
                         .arg(QString::fromStdString(kv.first))
                         .arg(solution.voltages[idx], 0, 'f', 4);
        }
    }
    result.summary = QString("DC OP (%1 devices):  %2")
                         .arg(circuit.devices.size())
                         .arg(parts.join("   "));
    return result;
}

SimulationController::TransientResult SimulationController::runTransientFromNetlist(
    const std::string& netlist) {
    TransientResult result;
    NetlistParser parser;
    if (!parser.parse(netlist)) {
        result.message = "Failed to parse netlist";
        return result;
    }

    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    if (!circuit.ok) {
        result.message = QString::fromStdString(circuit.error);
        return result;
    }

    double tstep = controlNumber(parser, "tran", 0, 1e-5);
    double tstop = controlNumber(parser, "tran", 1, 1e-3);
    if (tstep <= 0.0) tstep = 1e-5;
    if (tstop <= tstep) tstop = tstep * 100.0;

    size_t probe = pickProbeNode(circuit);
    if (circuit.numNodes == 0 && probe == 0) {
        result.message = "No nodes to simulate";
        return result;
    }

    SpiceTransient::Options opts;
    opts.tolerance = 1e-5;
    auto sim = SpiceTransient(opts).simulate(0.0, tstop, tstep, circuit.devices, circuit.nodeMap);

    result.converged = sim.converged;
    result.message = QString::fromStdString(sim.message);
    result.timePoints = sim.timePoints;
    result.traceName = QString("V(%1)").arg(nodeLabel(circuit, probe));
    result.values.reserve(sim.nodeVoltages.size());
    for (const auto& state : sim.nodeVoltages) {
        double v = 0.0;
        if (probe >= 1 && probe - 1 < state.size()) {
            v = state[probe - 1];
        } else if (!state.empty()) {
            v = state[0];
        }
        result.values.push_back(v);
    }
    return result;
}

SimulationController::AcResult SimulationController::runAcFromNetlist(const std::string& netlist) {
    AcResult result;
    NetlistParser parser;
    if (!parser.parse(netlist)) {
        result.message = "Failed to parse netlist";
        return result;
    }

    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    if (!circuit.ok) {
        result.message = QString::fromStdString(circuit.error);
        return result;
    }

    // .ac dec|oct|lin np fstart fstop — numbers may skip the string keyword.
    double fstart = 1.0;
    double fstop = 1e6;
    int npoints = 60;
    for (const auto& d : parser.getControlDirectives()) {
        if (d.kind != "ac") continue;
        if (d.numbers.size() >= 3) {
            // np, fstart, fstop (when type token is non-numeric)
            npoints = std::max(2, static_cast<int>(d.numbers[0]));
            fstart = d.numbers[1];
            fstop = d.numbers[2];
        } else if (d.numbers.size() == 2) {
            fstart = d.numbers[0];
            fstop = d.numbers[1];
        }
    }

    ACAnalysis ac;
    auto sweep = ac.sweep(circuit.devices, circuit.nodeMap, fstart, fstop, npoints);
    result.success = sweep.success;
    result.message = QString::fromStdString(sweep.message);
    if (!sweep.success) return result;

    size_t probe = pickProbeNode(circuit);
    result.frequenciesHz = sweep.frequenciesHz;
    result.traceName = QString("|V(%1)|").arg(nodeLabel(circuit, probe));
    if (probe >= 1 && probe - 1 < sweep.magnitude.size()) {
        result.magnitude = sweep.magnitude[probe - 1];
    } else if (!sweep.magnitude.empty()) {
        result.magnitude = sweep.magnitude.back();
    }
    return result;
}

SimulationController::DcResult SimulationController::runDcOperatingPoint(
    const SchematicScene* scene, bool allowDemoFallback) {
    if (scene) {
        auto extracted = extractNetlistFromScene(scene);
        if (extracted.ok) {
            return runDcFromNetlist(extracted.netlist);
        }
        if (!allowDemoFallback) {
            DcResult r;
            r.message = QString::fromStdString(extracted.error);
            return r;
        }
    }
    return runDemoDcOperatingPoint();
}

SimulationController::TransientResult SimulationController::runTransient(
    const SchematicScene* scene, bool allowDemoFallback) {
    if (scene) {
        auto extracted = extractNetlistFromScene(scene);
        if (extracted.ok) {
            return runTransientFromNetlist(extracted.netlist);
        }
        if (!allowDemoFallback) {
            TransientResult r;
            r.message = QString::fromStdString(extracted.error);
            return r;
        }
    }
    return runDemoTransient();
}

SimulationController::AcResult SimulationController::runAcAnalysis(
    const SchematicScene* scene, bool allowDemoFallback) {
    if (scene) {
        auto extracted = extractNetlistFromScene(scene);
        if (extracted.ok) {
            return runAcFromNetlist(extracted.netlist);
        }
        if (!allowDemoFallback) {
            AcResult r;
            r.message = QString::fromStdString(extracted.error);
            return r;
        }
    }
    return runDemoAcAnalysis();
}

SimulationController::DcResult SimulationController::runDemoDcOperatingPoint() {
    const char* nl =
        "V1 1 0 5\n"
        "R1 1 2 1k\n"
        "R2 2 0 1k\n"
        ".op\n";
    auto result = runDcFromNetlist(nl);
    if (result.success) {
        result.summary = QString("Demo divider (V1=5V, R1=R2=1k):  %1").arg(result.summary);
    }
    return result;
}

SimulationController::TransientResult SimulationController::runDemoTransient() {
    const char* nl =
        "V1 1 0 PULSE(0 5 0 1n 1n 0.5m 1m)\n"
        "R1 1 2 1k\n"
        "C1 2 0 1n\n"
        ".tran 1u 1m\n";
    auto result = runTransientFromNetlist(nl);
    if (result.converged) {
        result.traceName = QString("Demo RC: %1").arg(result.traceName);
    }
    return result;
}

SimulationController::AcResult SimulationController::runDemoAcAnalysis() {
    const char* nl =
        "V1 1 0 1 1\n"
        "R1 1 2 1k\n"
        "C1 2 0 1u\n"
        ".ac dec 10 1 1Meg\n";
    auto result = runAcFromNetlist(nl);
    if (result.success) {
        result.traceName = "|H(f)| at C1 (RC low-pass)";
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
        voltageTrace[i][0] = 1.0;
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
        result.summary = QString("EII pipeline: %1 impulse events detected, last confidence = %2")
                              .arg(totalEvents)
                              .arg(lastInference->confidence, 0, 'f', 4);
    } else {
        result.summary = QString("EII pipeline: %1 impulse events detected, no inference window completed")
                              .arg(totalEvents);
    }
    return result;
}

}
