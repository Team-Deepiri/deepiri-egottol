#include "nsp.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numeric>
#include <stdexcept>

#include "../matrix.h"
#include "fft.h"

namespace deepiri {

namespace {

// numpy's np.convolve(x, kernel, mode="same") for kernel.size() <= x.size():
// take the central `x.size()` samples of the full convolution.
std::vector<double> convolveSame(const std::vector<double>& x, const std::vector<double>& kernel) {
    const size_t m = x.size();
    const size_t k = kernel.size();
    std::vector<double> full(m + k - 1, 0.0);
    for (size_t i = 0; i < m; ++i) {
        for (size_t j = 0; j < k; ++j) {
            full[i + j] += x[i] * kernel[j];
        }
    }
    const size_t offset = (k - 1) / 2;
    return std::vector<double>(full.begin() + static_cast<long>(offset),
                                full.begin() + static_cast<long>(offset + m));
}

// fft.h only supports power-of-two sizes, whereas numpy's rfft/irfft handle
// any length; signals are zero-padded up to the next power of two here and
// truncated back afterward. This changes the effective frequency bin
// spacing versus numpy for non-power-of-two inputs — an intentional
// deviation flagged in the port notes rather than adding a mixed-radix FFT.
std::vector<std::complex<double>> paddedForwardFFT(const std::vector<double>& x, size_t& npad) {
    npad = nextPowerOfTwo(std::max<size_t>(x.size(), 1));
    std::vector<std::complex<double>> padded(npad, std::complex<double>(0.0, 0.0));
    for (size_t i = 0; i < x.size(); ++i) padded[i] = std::complex<double>(x[i], 0.0);
    return fft(std::move(padded));
}

size_t linspaceIndex(size_t i, size_t bins, size_t spectrumSize) {
    if (bins <= 1 || spectrumSize <= 1) return 0;
    double value = static_cast<double>(i) * static_cast<double>(spectrumSize - 1) /
                   static_cast<double>(bins - 1);
    return static_cast<size_t>(value);  // truncation matches numpy's astype(int)
}

}  // namespace

NSP::NSP(const NSPConfig& config) : config_(config) {}

std::vector<double> NSP::denoise(const std::vector<double>& signal, double sampleRate) const {
    (void)sampleRate;
    if (signal.empty()) return signal;

    const size_t window = std::max<size_t>(static_cast<size_t>(config_.moving_avg_window), 1);
    std::vector<double> kernel(window, 1.0 / static_cast<double>(window));
    std::vector<double> smoothed = convolveSame(signal, kernel);

    const size_t n = smoothed.size();
    size_t npad = 0;
    std::vector<std::complex<double>> spectrum = paddedForwardFFT(smoothed, npad);

    double peak = 0.0;
    for (const auto& c : spectrum) peak = std::max(peak, std::abs(c));
    if (peak <= 0.0) peak = 1.0;

    std::vector<std::complex<double>> gated(npad);
    for (size_t i = 0; i < npad; ++i) {
        bool pass = std::abs(spectrum[i]) >= config_.spectral_gate_threshold * peak;
        gated[i] = pass ? spectrum[i] : std::complex<double>(0.0, 0.0);
    }

    std::vector<std::complex<double>> restored = ifft(std::move(gated));
    std::vector<double> filtered(n);
    for (size_t i = 0; i < n; ++i) filtered[i] = restored[i].real();
    return filtered;
}

std::vector<double> NSP::fftFeatures(const std::vector<double>& signal) const {
    size_t npad = 0;
    std::vector<std::complex<double>> spectrum = paddedForwardFFT(signal, npad);
    const size_t spectrumSize = npad / 2 + 1;

    const size_t bins = config_.fft_bins;
    std::vector<double> out(bins, 0.0);
    if (spectrumSize >= bins) {
        for (size_t i = 0; i < bins; ++i) {
            out[i] = std::abs(spectrum[linspaceIndex(i, bins, spectrumSize)]);
        }
    } else {
        for (size_t i = 0; i < spectrumSize; ++i) out[i] = std::abs(spectrum[i]);
    }
    return out;
}

void NSP::trainClassifier(const std::vector<std::vector<double>>& signals, const std::vector<int>& labels) {
    const size_t nSamples = signals.size();
    const size_t bins = config_.fft_bins;

    std::vector<std::vector<double>> features(nSamples);
    for (size_t s = 0; s < nSamples; ++s) features[s] = fftFeatures(signals[s]);

    featureMean_.assign(bins, 0.0);
    featureStd_.assign(bins, 0.0);
    for (size_t j = 0; j < bins; ++j) {
        double sum = 0.0;
        for (size_t s = 0; s < nSamples; ++s) sum += features[s][j];
        featureMean_[j] = sum / static_cast<double>(nSamples);
    }
    for (size_t j = 0; j < bins; ++j) {
        double sumSq = 0.0;
        for (size_t s = 0; s < nSamples; ++s) {
            double d = features[s][j] - featureMean_[j];
            sumSq += d * d;
        }
        featureStd_[j] = std::max(std::sqrt(sumSq / static_cast<double>(nSamples)), 1e-9);
    }

    std::vector<int> classes = labels;
    std::sort(classes.begin(), classes.end());
    classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
    classLabels_ = classes;
    const size_t nClasses = classes.size();

    std::vector<std::vector<double>> yOneHot(nSamples, std::vector<double>(nClasses, 0.0));
    for (size_t s = 0; s < nSamples; ++s) {
        auto it = std::find(classes.begin(), classes.end(), labels[s]);
        yOneHot[s][static_cast<size_t>(it - classes.begin())] = 1.0;
    }

    const size_t nAug = bins + 1;
    Matrix aug(nSamples, nAug);
    for (size_t s = 0; s < nSamples; ++s) {
        for (size_t j = 0; j < bins; ++j) {
            aug.at(s, j) = (features[s][j] - featureMean_[j]) / featureStd_[j];
        }
        aug.at(s, bins) = 1.0;
    }

    Matrix augT = aug.transpose();
    Matrix ata = augT * aug;
    Matrix aty = augT * Matrix(yOneHot);

    // matrix.h has no SVD/pseudoinverse, so unlike numpy's np.linalg.lstsq
    // (which is rank-deficiency-safe), the normal equations here need a
    // tiny Tikhonov nudge to stay solvable when samples < features + 1.
    for (size_t i = 0; i < nAug; ++i) ata.at(i, i) += 1e-8;

    Matrix wAug(nAug, nClasses, 0.0);
    for (size_t c = 0; c < nClasses; ++c) {
        std::vector<double> col(nAug);
        for (size_t i = 0; i < nAug; ++i) col[i] = aty.at(i, c);
        std::vector<double> w = ata.solveGaussian(col);
        for (size_t i = 0; i < nAug; ++i) wAug.at(i, c) = w[i];
    }

    classifierWeights_.assign(nClasses, std::vector<double>(bins, 0.0));
    classifierBias_.assign(nClasses, 0.0);
    for (size_t c = 0; c < nClasses; ++c) {
        for (size_t j = 0; j < bins; ++j) classifierWeights_[c][j] = wAug.at(j, c);
        classifierBias_[c] = wAug.at(bins, c);
    }
    classifierTrained_ = true;
}

std::pair<int, std::vector<double>> NSP::classify(const std::vector<double>& signal) {
    if (!classifierTrained_) {
        std::vector<double> halved(signal.size());
        for (size_t i = 0; i < signal.size(); ++i) halved[i] = signal[i] * 0.5;
        trainClassifier({signal, halved}, {0, 1});
    }

    std::vector<double> feat = fftFeatures(signal);
    for (size_t j = 0; j < feat.size(); ++j) feat[j] = (feat[j] - featureMean_[j]) / featureStd_[j];

    const size_t nClasses = classifierWeights_.size();
    std::vector<double> logits(nClasses, 0.0);
    for (size_t c = 0; c < nClasses; ++c) {
        double sum = classifierBias_[c];
        for (size_t j = 0; j < feat.size(); ++j) sum += classifierWeights_[c][j] * feat[j];
        logits[c] = sum;
    }

    double maxLogit = *std::max_element(logits.begin(), logits.end());
    std::vector<double> probs(nClasses);
    double sumExp = 0.0;
    for (size_t c = 0; c < nClasses; ++c) {
        probs[c] = std::exp(logits[c] - maxLogit);
        sumExp += probs[c];
    }
    for (double& p : probs) p /= sumExp;

    size_t argmax = static_cast<size_t>(std::max_element(probs.begin(), probs.end()) - probs.begin());
    return {classLabels_[argmax], probs};
}

std::pair<bool, double> NSP::anomalyDetect(const std::vector<double>& signal,
                                            const std::vector<double>* reference) const {
    const std::vector<double>& ref = reference ? *reference : signal;
    double mean = std::accumulate(ref.begin(), ref.end(), 0.0) / static_cast<double>(ref.size());
    double sumSq = 0.0;
    for (double v : ref) sumSq += (v - mean) * (v - mean);
    double stddev = std::sqrt(sumSq / static_cast<double>(ref.size()));
    if (stddev < 1e-12) stddev = 1e-12;

    double maxAbsDev = 0.0;
    for (double v : signal) maxAbsDev = std::max(maxAbsDev, std::abs(v - mean));
    double z = maxAbsDev / stddev;
    return {z > config_.anomaly_z_threshold, z};
}

}  // namespace deepiri
