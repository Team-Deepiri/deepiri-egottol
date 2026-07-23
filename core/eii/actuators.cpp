#include "actuators.h"

#include <algorithm>
#include <cmath>

namespace deepiri {

std::vector<double> FeedbackActuator::actuate(EIIState& state, const std::vector<double>& prediction,
                                               double confidence, const std::vector<ImpulseEvent>& events,
                                               int targetClass) {
    std::vector<double> control;
    switch (config_.mode) {
        case ActuatorMode::Stdp:
            control = actuateStdp(state, prediction, confidence, events, targetClass);
            break;
        case ActuatorMode::Digital:
            control = actuateDigital(state, prediction);
            break;
        case ActuatorMode::Optical:
            control = actuateOptical(state, prediction);
            break;
        case ActuatorMode::Dac:
        default:
            control = actuateDac(prediction);
            break;
    }
    state.control = control;
    return control;
}

std::vector<double> FeedbackActuator::actuateDac(const std::vector<double>& prediction) const {
    std::vector<double> out(prediction.size());
    for (size_t i = 0; i < prediction.size(); ++i) out[i] = config_.vDd * std::clamp(prediction[i], 0.0, 1.0);
    return out;
}

double FeedbackActuator::stdpDelta(double deltaT) const {
    if (deltaT > 0.0) return config_.stdpAPlus * std::exp(-deltaT / std::max(config_.stdpTauPlus, 1e-9));
    if (deltaT < 0.0) return -config_.stdpAMinus * std::exp(deltaT / std::max(config_.stdpTauMinus, 1e-9));
    return 0.0;
}

std::vector<double> FeedbackActuator::actuateStdp(EIIState& state, const std::vector<double>& prediction,
                                                   double confidence, const std::vector<ImpulseEvent>& events,
                                                   int targetClass) {
    (void)confidence;
    if (state.conductances.empty()) {
        return actuateDac(prediction);
    }
    size_t idxTarget = prediction.empty() ? 0 : static_cast<size_t>(((targetClass % static_cast<int>(prediction.size())) + static_cast<int>(prediction.size())) % static_cast<int>(prediction.size()));
    double pCorrect = prediction.empty() ? 0.0 : prediction[idxTarget];
    double eInference = 1.0 - pCorrect;
    std::vector<double> control = actuateDac(prediction);

    for (const auto& event : events) {
        int key = event.channel;
        double lastT = lastPreSpikeTime_.count(key) ? lastPreSpikeTime_[key] : state.t;
        double dtSpike = state.t - lastT;
        lastPreSpikeTime_[key] = state.t;
        double deltaT = (event.eventType != "memristor_switch") ? dtSpike : 0.0;
        double deltaG = stdpDelta(deltaT) * eInference * config_.stdpEta;
        size_t idx = static_cast<size_t>(((event.channel % static_cast<int>(state.conductances.size())) +
                                           static_cast<int>(state.conductances.size())) %
                                          static_cast<int>(state.conductances.size()));
        state.conductances[idx] = std::clamp(state.conductances[idx] + deltaG, 1e-9, 1.0);
    }
    return control;
}

std::vector<double> FeedbackActuator::actuateDigital(EIIState& state, const std::vector<double>& prediction) const {
    std::vector<double> control(prediction.size(), 0.0);
    for (size_t idx = 0; idx < prediction.size(); ++idx) {
        double bit = prediction[idx] >= config_.digitalThreshold ? 1.0 : 0.0;
        state.logicRegisters["OUT_" + std::to_string(idx)] = bit;
        control[idx] = bit;
    }
    return control;
}

std::vector<double> FeedbackActuator::actuateOptical(EIIState& state, const std::vector<double>& prediction) const {
    if (state.opticalPhases.size() != prediction.size()) {
        state.opticalPhases.assign(prediction.size(), 0.0);
    }
    std::vector<double> deltaPhi(prediction.size());
    for (size_t i = 0; i < prediction.size(); ++i) {
        deltaPhi[i] = config_.opticalPhaseScale * prediction[i];
        state.opticalPhases[i] += deltaPhi[i];
    }
    return deltaPhi;
}

double FeedbackActuator::sourceModulation(const std::vector<double>& prediction, double t) const {
    if (prediction.empty()) return 0.0;
    size_t cls = static_cast<size_t>(std::max_element(prediction.begin(), prediction.end()) - prediction.begin());
    double amplitude = config_.sourceAmplitudeScale * prediction[cls];
    double omega = 2.0 * M_PI * config_.sourceFrequency;
    return amplitude * std::sin(omega * t);
}

}  // namespace deepiri
