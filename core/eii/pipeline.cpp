#include "pipeline.h"

#include <algorithm>
#include <cmath>

namespace deepiri {

EIIPipeline::EIIPipeline(EIIPipelineConfig config)
    : config_(std::move(config)),
      detector_(config_.eii),
      encoder_(config_.eii),
      inference_(config_.inference, config_.weights, config_.bias, config_.conductance),
      actuator_(config_.actuator),
      state_(initialState()) {}

EIIState EIIPipeline::initialState() const {
    EIIState s;
    int n = config_.eii.numChannels;
    int classes = config_.inference.numClasses;
    s.voltages.assign(n, 0.0);
    s.currents.assign(n, 0.0);
    s.conductances.assign(static_cast<size_t>(config_.conductance.rows()) * config_.conductance.cols(), 0.0);
    for (size_t i = 0; i < config_.conductance.rows(); ++i) {
        for (size_t j = 0; j < config_.conductance.cols(); ++j) {
            s.conductances[i * config_.conductance.cols() + j] = config_.conductance.at(i, j);
        }
    }
    s.opticalPhases.assign(classes, 0.0);
    s.refractory.assign(n, 0.0);
    s.prevVoltages.assign(n, 0.0);
    s.prevEnvelope.assign(n, 0.0);
    s.prevDiffSignal.assign(n, 0.0);
    s.prevComparator.assign(n, 0.0);
    s.prevMemristorCurrent.assign(n, 0.0);
    s.filterState.assign(config_.eii.embeddingDim, 0.0);
    s.control.assign(classes, 0.0);
    return s;
}

void EIIPipeline::reset() {
    state_ = initialState();
    history_.clear();
}

bool EIIPipeline::windowBoundary(double t, double window) const {
    double dt = std::max(config_.eii.dt, 1e-12);
    long long steps = static_cast<long long>(std::llround(t / dt));
    long long windowSteps = std::max<long long>(static_cast<long long>(std::llround(window / dt)), 1);
    return steps > 0 && steps % windowSteps == 0;
}

EIIStepResult EIIPipeline::step(const std::vector<double>& voltages, const std::vector<double>& currents, double dt,
                                 int targetClass) {
    int n = config_.eii.numChannels;
    std::vector<double> v = voltages;
    if (static_cast<int>(v.size()) < n) v.resize(n, 0.0);
    v.resize(n);
    std::vector<double> i = currents;
    if (static_cast<int>(i.size()) < n) i.resize(n, 0.0);
    i.resize(n);

    state_.t += dt;
    state_.voltages = v;
    state_.currents = i;

    std::vector<ImpulseEvent> events = detector_.detect(state_, dt);
    state_.eventHistory.insert(state_.eventHistory.end(), events.begin(), events.end());
    state_.windowEvents.insert(state_.windowEvents.end(), events.begin(), events.end());

    EIIStepResult result;
    result.t = state_.t;
    result.events = events;
    result.voltages = state_.voltages;
    result.control = state_.control;
    result.inferenceRan = false;

    double window = config_.eii.windowT;
    if (window > 0.0 && windowBoundary(state_.t, window)) {
        double windowStart = state_.t - window;
        std::vector<double> z = encoder_.encode(state_, state_.windowEvents, windowStart);
        auto [yHat, confidence] = inference_.infer(z);
        std::vector<double> control = actuator_.actuate(state_, yHat, confidence, state_.windowEvents, targetClass);

        state_.embedding = z;
        state_.prediction = yHat;
        state_.confidence = confidence;
        state_.control = control;
        state_.windowEvents.clear();

        result.embedding = z;
        result.prediction = yHat;
        result.confidence = confidence;
        result.control = control;
        result.inferenceRan = true;
    }

    history_.push_back(result);
    return result;
}

std::vector<EIIStepResult> EIIPipeline::run(const std::vector<std::vector<double>>& voltageTrace,
                                             const std::vector<std::vector<double>>& currentTrace, double dt,
                                             int targetClass) {
    std::vector<EIIStepResult> outputs;
    outputs.reserve(voltageTrace.size());
    for (size_t idx = 0; idx < voltageTrace.size(); ++idx) {
        const std::vector<double>& i = (idx < currentTrace.size()) ? currentTrace[idx] : std::vector<double>();
        outputs.push_back(step(voltageTrace[idx], i, dt, targetClass));
    }
    return outputs;
}

}  // namespace deepiri
