#pragma once

#include <vector>

#include "../matrix.h"
#include "actuators.h"
#include "detectors.h"
#include "encoders.h"
#include "inference.h"
#include "types.h"

namespace deepiri {

struct EIIPipelineConfig {
    EIIConfig eii;
    InferenceConfig inference;
    ActuatorConfig actuator;
    // Weights [numClasses x embeddingDim], bias [numClasses], conductance
    // [numClasses x embeddingDim]. If left empty, seeded pseudo-random defaults
    // are generated (deterministic, but NOT bit-identical to numpy's
    // default_rng — see report).
    Matrix weights;
    std::vector<double> bias;
    Matrix conductance;
};

struct EIIStepResult {
    double t = 0.0;
    std::vector<ImpulseEvent> events;
    std::vector<double> voltages;
    std::vector<double> embedding;
    std::vector<double> prediction;
    double confidence = 0.0;
    std::vector<double> control;
    bool inferenceRan = false;
};

// Orchestrates the closed Phi -> Psi -> Gamma loop, one simulation window at a
// time. Port of egottol/engines/eii/pipeline.py::EIIPipeline, with the
// detector/encoder call sites corrected to match the real detectors.py /
// encoders.py signatures (the Python pipeline.py calls
// `detector.detect(state, v, i, dt)` and `encoder.encode(state, events, v,
// window_start)` / `encoder.reset_filter(state)`, none of which exist on the
// actual ImpulseDetector / EncodingManifold classes — confirmed by running it;
// see report). The window-boundary bookkeeping and step()/run() structure
// otherwise follow pipeline.py directly.
class EIIPipeline {
public:
    explicit EIIPipeline(EIIPipelineConfig config);

    void reset();

    EIIStepResult step(const std::vector<double>& voltages, const std::vector<double>& currents, double dt,
                        int targetClass = 0);

    std::vector<EIIStepResult> run(const std::vector<std::vector<double>>& voltageTrace,
                                    const std::vector<std::vector<double>>& currentTrace, double dt,
                                    int targetClass = 0);

    const EIIState& state() const { return state_; }
    const std::vector<EIIStepResult>& history() const { return history_; }

private:
    EIIState initialState() const;
    bool windowBoundary(double t, double window) const;

    EIIPipelineConfig config_;
    ImpulseDetector detector_;
    EncodingManifold encoder_;
    InferenceEngine inference_;
    FeedbackActuator actuator_;
    EIIState state_;
    std::vector<EIIStepResult> history_;
};

}  // namespace deepiri
