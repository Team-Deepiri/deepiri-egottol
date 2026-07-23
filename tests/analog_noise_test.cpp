#include "../core/analog/noise.h"

#include <cmath>
#include <cstdio>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

double variance(const std::vector<double>& v) {
    double mean = 0.0;
    for (double x : v) mean += x;
    mean /= static_cast<double>(v.size());
    double var = 0.0;
    for (double x : v) var += (x - mean) * (x - mean);
    return var / static_cast<double>(v.size());
}

void test_thermal_noise_matches_johnson_formula() {
    std::mt19937_64 rng(42);
    double r = 1000.0;
    double t = 300.0;
    double bw = 1e6;
    auto samples = thermalNoise(r, t, bw, 20000, rng);

    expect(samples.size() == 20000, "thermalNoise should produce the requested sample count");

    double expectedVar = 4.0 * kBoltzmann * t * r * bw;
    double sampleVar = variance(samples);

    expect(expectedVar > 0.0, "Johnson noise variance should be strictly positive for R,T,BW > 0");
    expect(sampleVar > 0.0, "generated thermal noise must have nonzero variance across many samples");
    double ratio = sampleVar / expectedVar;
    expect(ratio > 0.5 && ratio < 1.5,
           "sample variance should be within 2x of the theoretical Johnson-Nyquist variance");
}

void test_thermal_noise_zero_for_degenerate_inputs() {
    std::mt19937_64 rng(1);
    auto samples = thermalNoise(0.0, 300.0, 0.0, 1000, rng);
    double sampleVar = variance(samples);
    expect(sampleVar < 1e-20, "zero resistance/bandwidth should collapse thermal noise toward zero variance");
}

void test_flicker_noise_nonzero_variance() {
    std::mt19937_64 rng(7);
    auto trace = flickerNoise(256, 1e-3, 1.0, 1e-6, rng);
    expect(trace.size() == 256, "flickerNoise should produce n_samples points");

    double var = variance(trace);
    expect(var > 0.0, "flicker noise trace should have nonzero variance");

    double meanAbs = 0.0;
    for (double v : trace) meanAbs += std::abs(v);
    meanAbs /= static_cast<double>(trace.size());
    expect(meanAbs > 0.0 && meanAbs < 1e-4,
           "flicker noise amplitude should be normalized to roughly the requested scale");
}

void test_add_noise_to_trace_perturbs_signal() {
    std::mt19937_64 rng(99);
    std::vector<double> clean(500, 1.0);
    auto noisy = addNoiseToTrace(clean, 1000.0, 300.0, 1e6, 1e-3, 1e-3, 1.0, rng);

    expect(noisy.size() == clean.size(), "addNoiseToTrace should preserve trace length");

    bool anyDifferent = false;
    for (size_t i = 0; i < noisy.size(); ++i) {
        if (std::abs(noisy[i] - clean[i]) > 1e-15) {
            anyDifferent = true;
            break;
        }
    }
    expect(anyDifferent, "addNoiseToTrace should perturb at least some samples away from the clean signal");
}

void test_add_noise_to_trace_passthrough_when_disabled() {
    std::mt19937_64 rng(3);
    std::vector<double> clean{1.0, 2.0, 3.0};
    auto noisy = addNoiseToTrace(clean, 0.0, 300.0, 1.0, 0.0, 1e-3, 1.0, rng);
    for (size_t i = 0; i < clean.size(); ++i) {
        expect(noisy[i] == clean[i], "with thermal_r=0 and flicker_amp=0 the trace should pass through unchanged");
    }
}

}  // namespace

int main() {
    test_thermal_noise_matches_johnson_formula();
    test_thermal_noise_zero_for_degenerate_inputs();
    test_flicker_noise_nonzero_variance();
    test_add_noise_to_trace_perturbs_signal();
    test_add_noise_to_trace_passthrough_when_disabled();

    if (failures == 0) {
        std::printf("All noise model tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
