#include "detectors.h"

#include <algorithm>
#include <cmath>

namespace deepiri {

namespace {
double sign(double v) {
    if (v > 0.0) return 1.0;
    if (v < 0.0) return -1.0;
    return 0.0;
}
}  // namespace

std::vector<ImpulseEvent> ImpulseDetector::detect(EIIState& state, double dt) {
    switch (config_.detectorMode) {
        case DetectorMode::Threshold:
            return detectThreshold(state, dt);
        case DetectorMode::Differentiator:
            return detectDifferentiator(state, dt);
        case DetectorMode::Comparator:
            return detectComparator(state, dt);
        case DetectorMode::RfEnvelope:
            return detectRfEnvelope(state, dt);
        case DetectorMode::MemristorSwitch:
            return detectMemristorSwitch(state, dt);
    }
    return {};
}

std::vector<ImpulseEvent> ImpulseDetector::detectThreshold(EIIState& state, double dt) {
    std::vector<ImpulseEvent> events;
    size_t n = std::min(state.voltages.size(), static_cast<size_t>(config_.numChannels));
    for (size_t ch = 0; ch < n; ++ch) {
        if (state.refractory[ch] > 0.0) {
            state.refractory[ch] = std::max(0.0, state.refractory[ch] - dt);
            continue;
        }
        double v = state.voltages[ch];
        if (v >= threshold) {
            events.push_back({state.t, static_cast<int>(ch), "threshold_cross", v});
            state.refractory[ch] = config_.windowT * 0.1;
        }
    }
    return events;
}

std::vector<ImpulseEvent> ImpulseDetector::detectDifferentiator(EIIState& state, double dt) {
    std::vector<ImpulseEvent> events;
    size_t n = std::min(state.voltages.size(), static_cast<size_t>(config_.numChannels));
    for (size_t ch = 0; ch < n; ++ch) {
        double dv = (state.voltages[ch] - state.prevVoltages[ch]) / std::max(dt, 1e-12);
        double a = tauD * dv + state.voltages[ch];
        double prevA = state.prevDiffSignal[ch];
        if (a > threshold && a < prevA) {
            events.push_back({state.t, static_cast<int>(ch), "spike", a});
        }
        state.prevDiffSignal[ch] = a;
    }
    state.prevVoltages = state.voltages;
    return events;
}

std::vector<ImpulseEvent> ImpulseDetector::detectComparator(EIIState& state, double dt) {
    (void)dt;
    std::vector<ImpulseEvent> events;
    size_t n = std::min(state.voltages.size(), static_cast<size_t>(config_.numChannels));
    for (size_t ch = 0; ch < n; ++ch) {
        bool high = state.voltages[ch] >= threshold;
        bool wasHigh = state.prevComparator[ch] >= 0.5;
        if (high && !wasHigh) {
            events.push_back({state.t, static_cast<int>(ch), "spike", state.voltages[ch]});
        }
        state.prevComparator[ch] = high ? 1.0 : 0.0;
    }
    return events;
}

std::vector<ImpulseEvent> ImpulseDetector::detectRfEnvelope(EIIState& state, double dt) {
    std::vector<ImpulseEvent> events;
    double alpha = dt / std::max(tauD, dt);
    size_t n = std::min(state.voltages.size(), static_cast<size_t>(config_.numChannels));
    for (size_t ch = 0; ch < n; ++ch) {
        double target = state.voltages[ch] * state.voltages[ch];
        double env = (1.0 - alpha) * state.prevEnvelope[ch] + alpha * target;
        if (env > thetaRf && env < state.prevEnvelope[ch]) {
            events.push_back({state.t, static_cast<int>(ch), "rf_burst_peak", std::sqrt(env)});
        }
        state.prevEnvelope[ch] = env;
    }
    return events;
}

std::vector<ImpulseEvent> ImpulseDetector::detectMemristorSwitch(EIIState& state, double dt) {
    (void)dt;
    std::vector<ImpulseEvent> events;
    size_t n = std::min(state.currents.size(), static_cast<size_t>(config_.numChannels));
    for (size_t ch = 0; ch < n; ++ch) {
        double iNow = state.currents[ch];
        double iPrev = state.prevMemristorCurrent[ch];
        if (std::abs(iNow) > iSet && sign(iNow) != sign(iPrev)) {
            events.push_back({state.t, static_cast<int>(ch), "memristor_switch", iNow});
        }
        state.prevMemristorCurrent[ch] = iNow;
    }
    return events;
}

}  // namespace deepiri
