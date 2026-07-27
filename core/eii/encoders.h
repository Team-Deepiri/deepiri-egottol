#pragma once

#include <random>
#include <vector>

#include "types.h"

namespace deepiri {

// Encoding manifold Phi: impulse events + probe voltages -> embedding z.
// Port of egottol/engines/eii/encoders.py::EncodingManifold.
class EncodingManifold {
public:
    explicit EncodingManifold(const EIIConfig& config) : config_(config), rng_(0) {}

    std::vector<double> encode(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);

    double tauLat = 5e-3;
    double tauM = 2e-3;

private:
    std::vector<double> rate(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);
    std::vector<double> latency(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);
    std::vector<double> filter(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);
    std::vector<double> population(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);
    std::vector<double> continuous(EIIState& state, const std::vector<ImpulseEvent>& events, double windowStart);

    EIIConfig config_;
    std::mt19937 rng_;
};

}  // namespace deepiri
