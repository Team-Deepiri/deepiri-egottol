#include "encoders.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace deepiri {

std::vector<double> EncodingManifold::encode(EIIState& state, const std::vector<ImpulseEvent>& events,
                                              double windowStart) {
    std::vector<double> z;
    switch (config_.encoderMode) {
        case EncoderMode::Rate:
            z = rate(state, events, windowStart);
            break;
        case EncoderMode::Latency:
            z = latency(state, events, windowStart);
            break;
        case EncoderMode::Filter:
            z = filter(state, events, windowStart);
            break;
        case EncoderMode::Population:
            z = population(state, events, windowStart);
            break;
        case EncoderMode::Continuous:
            z = continuous(state, events, windowStart);
            break;
    }
    if (config_.readNoiseStd > 0.0) {
        std::normal_distribution<double> dist(0.0, config_.readNoiseStd);
        for (double& v : z) v += dist(rng_);
    }
    return z;
}

std::vector<double> EncodingManifold::rate(EIIState&, const std::vector<ImpulseEvent>& events, double windowStart) {
    int d = config_.embeddingDim;
    std::vector<double> counts(d, 0.0);
    for (const auto& e : events) {
        if (e.t < windowStart) continue;
        int ch = ((e.channel % d) + d) % d;
        counts[ch] += 1.0;
    }
    double windowT = std::max(config_.windowT, 1e-12);
    for (double& c : counts) c /= windowT;
    return counts;
}

std::vector<double> EncodingManifold::latency(EIIState&, const std::vector<ImpulseEvent>& events,
                                               double windowStart) {
    int d = config_.embeddingDim;
    std::vector<double> z(d, 0.0);
    for (int ch = 0; ch < d; ++ch) {
        bool found = false;
        double tFirst = 0.0;
        for (const auto& e : events) {
            if (e.t < windowStart) continue;
            int c = ((e.channel % d) + d) % d;
            if (c != ch) continue;
            if (!found || e.t < tFirst) {
                tFirst = e.t;
                found = true;
            }
        }
        if (found) {
            z[ch] = std::exp(-(tFirst - config_.stimulusTime) / tauLat);
        }
    }
    return z;
}

std::vector<double> EncodingManifold::filter(EIIState& state, const std::vector<ImpulseEvent>& events,
                                              double windowStart) {
    int d = config_.embeddingDim;
    std::vector<double> z = state.filterState;
    if (static_cast<int>(z.size()) != d) {
        z.assign(d, 0.0);
    }
    double dt = config_.dt;
    double decay = std::exp(-dt / std::max(tauM, dt));
    for (double& v : z) v *= decay;
    for (const auto& e : events) {
        if (e.t < windowStart) continue;
        int ch = ((e.channel % d) + d) % d;
        z[ch] += e.amplitude / std::max(tauM, dt);
    }
    state.filterState = z;
    return z;
}

std::vector<double> EncodingManifold::population(EIIState& state, const std::vector<ImpulseEvent>& events,
                                                   double windowStart) {
    std::vector<double> perChannel = rate(state, events, windowStart);
    int d = config_.embeddingDim;
    if (d == 1) return perChannel;
    std::vector<double> pooled(d, 0.0);
    size_t chunk = std::max<size_t>(1, perChannel.size() / static_cast<size_t>(d));
    for (int i = 0; i < d; ++i) {
        size_t start = static_cast<size_t>(i) * chunk;
        size_t end = (i < d - 1) ? start + chunk : perChannel.size();
        if (end > start && start < perChannel.size()) {
            end = std::min(end, perChannel.size());
            double sum = std::accumulate(perChannel.begin() + start, perChannel.begin() + end, 0.0);
            pooled[i] = sum / static_cast<double>(end - start);
        } else {
            pooled[i] = 0.0;
        }
    }
    return pooled;
}

std::vector<double> EncodingManifold::continuous(EIIState& state, const std::vector<ImpulseEvent>&, double) {
    int d = config_.embeddingDim;
    std::vector<double> probes(d, 0.0);
    size_t n = std::min(state.voltages.size(), static_cast<size_t>(d));
    for (size_t i = 0; i < n; ++i) probes[i] = state.voltages[i];
    return probes;
}

}  // namespace deepiri
