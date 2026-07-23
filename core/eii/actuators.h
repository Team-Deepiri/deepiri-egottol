#pragma once

#include <map>
#include <utility>
#include <vector>

#include "types.h"

namespace deepiri {

struct ActuatorConfig {
    ActuatorMode mode = ActuatorMode::Dac;
    double vDd = 1.0;
    double stdpEta = 0.01;
    double stdpAPlus = 0.001;
    double stdpAMinus = 0.0012;
    double stdpTauPlus = 20e-3;
    double stdpTauMinus = 20e-3;
    double opticalPhaseScale = 0.1;
    double digitalThreshold = 0.5;
    double sourceFrequency = 1e6;
    double sourceAmplitudeScale = 1.0;
};

// Feedback operator Gamma: (y_hat, p, x(t)) -> circuit drive u(t + dt).
// Port of egottol/engines/eii/actuators.py::FeedbackActuator.
class FeedbackActuator {
public:
    explicit FeedbackActuator(ActuatorConfig config) : config_(std::move(config)) {}

    std::vector<double> actuate(EIIState& state, const std::vector<double>& prediction, double confidence,
                                 const std::vector<ImpulseEvent>& events, int targetClass = 0);

    double sourceModulation(const std::vector<double>& prediction, double t) const;

private:
    std::vector<double> actuateDac(const std::vector<double>& prediction) const;
    std::vector<double> actuateStdp(EIIState& state, const std::vector<double>& prediction, double confidence,
                                     const std::vector<ImpulseEvent>& events, int targetClass);
    std::vector<double> actuateDigital(EIIState& state, const std::vector<double>& prediction) const;
    std::vector<double> actuateOptical(EIIState& state, const std::vector<double>& prediction) const;
    double stdpDelta(double deltaT) const;

    ActuatorConfig config_;
    std::map<int, double> lastPreSpikeTime_;
};

}  // namespace deepiri
