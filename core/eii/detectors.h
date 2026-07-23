#pragma once

#include <vector>

#include "types.h"

namespace deepiri {

// Impulse extraction operator: continuous waveforms -> discrete ImpulseEvents.
// Port of egottol/engines/eii/detectors.py::ImpulseDetector.
class ImpulseDetector {
public:
    explicit ImpulseDetector(const EIIConfig& config) : config_(config) {}

    std::vector<ImpulseEvent> detect(EIIState& state, double dt);

    double threshold = 0.5;
    double tauD = 1e-3;
    double thetaRf = 0.1;
    double iSet = 1e-6;

private:
    std::vector<ImpulseEvent> detectThreshold(EIIState& state, double dt);
    std::vector<ImpulseEvent> detectDifferentiator(EIIState& state, double dt);
    std::vector<ImpulseEvent> detectComparator(EIIState& state, double dt);
    std::vector<ImpulseEvent> detectRfEnvelope(EIIState& state, double dt);
    std::vector<ImpulseEvent> detectMemristorSwitch(EIIState& state, double dt);

    EIIConfig config_;
};

}  // namespace deepiri
