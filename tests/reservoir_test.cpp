// Tests for core/ai/reservoir.{h,cpp}: sparse reservoir recurrence and
// ridge-regression readout, ported from egottol/engines/ai/reservoir.py.
#include "../core/ai/reservoir.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

Reservoir::Trace makeSineTrace(double freq, size_t steps) {
    Reservoir::Trace trace(steps, std::vector<double>(1, 0.0));
    for (size_t t = 0; t < steps; ++t) {
        trace[t][0] = std::sin(freq * static_cast<double>(t));
    }
    return trace;
}

void test_reservoir_learns_periodic_classes() {
    ReservoirConfig config;
    config.n_reservoir = 40;
    config.spectral_radius = 0.9;
    config.leak_rate = 0.3;
    config.sparsity = 0.2;
    config.seed = 7;

    Reservoir reservoir(config);

    Reservoir::Batch traces;
    std::vector<int> labels;
    for (int i = 0; i < 6; ++i) {
        reservoir.reset();
        traces.push_back(makeSineTrace(0.5, 30));
        labels.push_back(0);
        reservoir.reset();
        traces.push_back(makeSineTrace(1.7, 30));
        labels.push_back(1);
    }

    reservoir.fit(traces, labels);
    expect(reservoir.isTrained(), "reservoir readout should be trained after fit()");

    reservoir.reset();
    auto [probsLow, confLow] = reservoir.infer(makeSineTrace(0.5, 30));
    reservoir.reset();
    auto [probsHigh, confHigh] = reservoir.infer(makeSineTrace(1.7, 30));

    expect(probsLow.size() == 2, "probability vector should have 2 classes");
    expect(probsLow[0] > probsLow[1], "low-frequency trace should be classified as class 0");
    expect(probsHigh[1] > probsHigh[0], "high-frequency trace should be classified as class 1");
    expect(confLow > 0.5 && confHigh > 0.5, "confidence should favor the correct class");
}

void test_reservoir_state_updates_are_bounded() {
    ReservoirConfig config;
    config.n_reservoir = 20;
    config.seed = 3;
    Reservoir reservoir(config);

    auto trace = makeSineTrace(0.3, 50);
    auto states = reservoir.collectStates(trace);

    expect(states.size() == trace.size(), "state trajectory should have one row per input step");
    for (const auto& row : states) {
        for (double v : row) {
            expect(std::isfinite(v), "reservoir state must stay finite");
            expect(v >= -1.0 - 1e-9 && v <= 1.0 + 1e-9, "leaky tanh state must stay within [-1, 1]");
        }
    }
}

}  // namespace

int main() {
    test_reservoir_learns_periodic_classes();
    test_reservoir_state_updates_are_bounded();

    if (failures == 0) {
        std::printf("All reservoir tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
