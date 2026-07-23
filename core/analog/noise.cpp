#include "noise.h"
#include <cmath>
#include <complex>
#include <algorithm>

namespace deepiri {

std::vector<double> thermalNoise(double resistance, double temperature, double bandwidth,
                                  size_t nSamples, std::mt19937_64& rng) {
    double r = std::max(resistance, 1e-18);
    double bw = std::max(bandwidth, 1e-18);
    double vRms = std::sqrt(4.0 * kBoltzmann * temperature * r * bw);
    std::normal_distribution<double> dist(0.0, vRms);
    std::vector<double> out(std::max<size_t>(nSamples, 1));
    for (auto& v : out) {
        v = dist(rng);
    }
    return out;
}

std::vector<double> thermalNoise(double resistance, double temperature, double bandwidth,
                                  size_t nSamples) {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    return thermalNoise(resistance, temperature, bandwidth, nSamples, rng);
}

std::vector<double> flickerNoise(size_t nSamples, double dt, double cornerFreq,
                                  double amplitude, std::mt19937_64& rng) {
    size_t n = std::max<size_t>(nSamples, 1);
    double dtSafe = std::max(dt, 1e-18);
    double cornerSafe = std::max(cornerFreq, 1e-18);

    size_t m = n / 2 + 1;
    std::vector<double> freqs(m);
    for (size_t k = 0; k < m; ++k) {
        freqs[k] = static_cast<double>(k) / (static_cast<double>(n) * dtSafe);
    }

    std::normal_distribution<double> dist(0.0, 1.0);
    std::vector<std::complex<double>> shaped(m);
    for (size_t k = 0; k < m; ++k) {
        std::complex<double> white(dist(rng), dist(rng));
        double psd = 1.0;
        if (k > 0) {
            psd = 1.0 / std::sqrt(freqs[k] / cornerSafe);
        } else {
            psd = 0.0;
        }
        shaped[k] = white * psd;
    }

    std::vector<std::complex<double>> full(n);
    for (size_t k = 0; k < n; ++k) {
        if (k < m) {
            full[k] = shaped[k];
        } else {
            full[k] = std::conj(shaped[n - k]);
        }
    }

    std::vector<double> trace(n, 0.0);
    const double twoPiOverN = 2.0 * M_PI / static_cast<double>(n);
    for (size_t j = 0; j < n; ++j) {
        std::complex<double> acc(0.0, 0.0);
        for (size_t k = 0; k < n; ++k) {
            double angle = twoPiOverN * static_cast<double>(k) * static_cast<double>(j);
            acc += full[k] * std::complex<double>(std::cos(angle), std::sin(angle));
        }
        trace[j] = acc.real() / static_cast<double>(n);
    }

    double mean = 0.0;
    for (double v : trace) mean += v;
    mean /= static_cast<double>(n);
    double variance = 0.0;
    for (double v : trace) variance += (v - mean) * (v - mean);
    variance /= static_cast<double>(n);
    double stddev = std::max(std::sqrt(variance), 1e-18);

    for (double& v : trace) {
        v = v / stddev * amplitude;
    }
    return trace;
}

std::vector<double> flickerNoise(size_t nSamples, double dt, double cornerFreq, double amplitude) {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    return flickerNoise(nSamples, dt, cornerFreq, amplitude, rng);
}

std::vector<double> addNoiseToTrace(const std::vector<double>& trace, double thermalR,
                                     double temperature, double bandwidth, double flickerAmp,
                                     double dt, double flickerCorner, std::mt19937_64& rng) {
    std::vector<double> y = trace;
    size_t n = y.size();

    if (thermalR > 0.0) {
        auto tn = thermalNoise(thermalR, temperature, bandwidth, n, rng);
        for (size_t i = 0; i < n; ++i) {
            y[i] += tn[i];
        }
    }

    if (flickerAmp > 0.0) {
        auto fn = flickerNoise(n, dt, flickerCorner, flickerAmp, rng);
        for (size_t i = 0; i < n; ++i) {
            y[i] += fn[i];
        }
    }

    return y;
}

std::vector<double> addNoiseToTrace(const std::vector<double>& trace, double thermalR,
                                     double temperature, double bandwidth, double flickerAmp,
                                     double dt, double flickerCorner) {
    std::random_device rd;
    std::mt19937_64 rng(rd());
    return addNoiseToTrace(trace, thermalR, temperature, bandwidth, flickerAmp, dt, flickerCorner, rng);
}

}
