// Tests for core/ai/nsp.{h,cpp} and core/ai/fft.h, ported from
// egottol/engines/ai/nsp.py.
#include "../core/ai/fft.h"
#include "../core/ai/nsp.h"

#include <cmath>
#include <complex>
#include <cstdio>
#include <random>
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

void test_fft_impulse_is_flat_magnitude() {
    const size_t n = 16;
    std::vector<std::complex<double>> x(n, std::complex<double>(0.0, 0.0));
    x[0] = std::complex<double>(1.0, 0.0);
    auto spectrum = fft(x);
    for (const auto& c : spectrum) {
        expect(std::abs(std::abs(c) - 1.0) < 1e-9, "FFT of a unit impulse must have flat unit magnitude");
    }
}

void test_fft_sine_peaks_at_bin() {
    const size_t n = 64;
    const size_t k = 5;
    std::vector<std::complex<double>> x(n);
    for (size_t i = 0; i < n; ++i) {
        double angle = 2.0 * M_PI * static_cast<double>(k) * static_cast<double>(i) / static_cast<double>(n);
        x[i] = std::complex<double>(std::sin(angle), 0.0);
    }
    auto spectrum = fft(x);

    size_t peakBin = 0;
    double peakMag = 0.0;
    for (size_t i = 0; i < n / 2; ++i) {
        double mag = std::abs(spectrum[i]);
        if (mag > peakMag) {
            peakMag = mag;
            peakBin = i;
        }
    }
    expect(peakBin == k, "FFT of a pure sine at bin k must peak at bin k");

    auto restored = ifft(spectrum);
    for (size_t i = 0; i < n; ++i) {
        expect(std::abs(restored[i].real() - x[i].real()) < 1e-9, "ifft(fft(x)) must reconstruct x");
    }
}

void test_denoise_reduces_high_frequency_noise() {
    NSPConfig config;
    config.moving_avg_window = 5;
    config.spectral_gate_threshold = 0.2;
    NSP nsp(config);

    const size_t n = 128;
    std::mt19937 rng(11);
    std::normal_distribution<double> noise(0.0, 0.3);
    std::vector<double> signal(n);
    for (size_t i = 0; i < n; ++i) {
        double clean = std::sin(2.0 * M_PI * 3.0 * static_cast<double>(i) / static_cast<double>(n));
        signal[i] = clean + noise(rng);
    }

    auto filtered = nsp.denoise(signal, 1.0);
    expect(filtered.size() == signal.size(), "denoise should preserve signal length");

    auto variance = [](const std::vector<double>& v) {
        double mean = 0.0;
        for (double x : v) mean += x;
        mean /= static_cast<double>(v.size());
        double acc = 0.0;
        for (double x : v) acc += (x - mean) * (x - mean);
        return acc / static_cast<double>(v.size());
    };

    std::vector<double> rawDiff(n), filteredDiff(n);
    for (size_t i = 1; i < n; ++i) {
        rawDiff[i] = signal[i] - signal[i - 1];
        filteredDiff[i] = filtered[i] - filtered[i - 1];
    }
    expect(variance(filteredDiff) < variance(rawDiff),
           "denoised signal should have smoother (lower variance) sample-to-sample deltas");
}

void test_classify_distinguishes_frequencies() {
    NSPConfig config;
    config.fft_bins = 16;
    NSP nsp(config);

    const size_t n = 64;
    std::vector<std::vector<double>> signals;
    std::vector<int> labels;
    for (int rep = 0; rep < 4; ++rep) {
        std::vector<double> low(n), high(n);
        for (size_t i = 0; i < n; ++i) {
            low[i] = std::sin(2.0 * M_PI * 2.0 * static_cast<double>(i) / static_cast<double>(n));
            high[i] = std::sin(2.0 * M_PI * 12.0 * static_cast<double>(i) / static_cast<double>(n));
        }
        signals.push_back(low);
        labels.push_back(0);
        signals.push_back(high);
        labels.push_back(1);
    }
    nsp.trainClassifier(signals, labels);

    std::vector<double> testLow(n), testHigh(n);
    for (size_t i = 0; i < n; ++i) {
        testLow[i] = std::sin(2.0 * M_PI * 2.0 * static_cast<double>(i) / static_cast<double>(n));
        testHigh[i] = std::sin(2.0 * M_PI * 12.0 * static_cast<double>(i) / static_cast<double>(n));
    }

    auto [labelLow, probsLow] = nsp.classify(testLow);
    auto [labelHigh, probsHigh] = nsp.classify(testHigh);
    expect(labelLow == 0, "low-frequency signal should classify as class 0");
    expect(labelHigh == 1, "high-frequency signal should classify as class 1");
}

void test_anomaly_detect_flags_outlier() {
    NSPConfig config;
    config.anomaly_z_threshold = 3.0;
    NSP nsp(config);

    std::vector<double> reference;
    std::mt19937 rng(5);
    std::uniform_real_distribution<double> noise(-1.0, 1.0);
    for (int i = 0; i < 200; ++i) reference.push_back(noise(rng));

    std::vector<double> normalSignal(reference.begin(), reference.begin() + 50);
    auto [flaggedNormal, zNormal] = nsp.anomalyDetect(normalSignal, &reference);
    expect(!flaggedNormal, "in-distribution signal should not be flagged as anomalous");

    std::vector<double> spikySignal = normalSignal;
    spikySignal[10] = 50.0;
    auto [flaggedSpike, zSpike] = nsp.anomalyDetect(spikySignal, &reference);
    expect(flaggedSpike, "signal with a large outlier spike should be flagged as anomalous");
    expect(zSpike > zNormal, "spiky signal should have a higher z-score than the normal one");
}

}  // namespace

int main() {
    test_fft_impulse_is_flat_magnitude();
    test_fft_sine_peaks_at_bin();
    test_denoise_reduces_high_frequency_noise();
    test_classify_distinguishes_frequencies();
    test_anomaly_detect_flags_outlier();

    if (failures == 0) {
        std::printf("All NSP tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
