#pragma once

#include <utility>
#include <vector>

namespace deepiri {

struct NSPConfig {
    int moving_avg_window = 5;
    double spectral_gate_threshold = 0.15;
    size_t fft_bins = 32;
    double anomaly_z_threshold = 3.0;
};

// Ported from egottol/engines/ai/nsp.py (NeuralSignalProcessor): denoise,
// FFT-feature classification, and z-score anomaly detection for NSP_AI.
class NSP {
public:
    explicit NSP(const NSPConfig& config = NSPConfig());

    std::vector<double> denoise(const std::vector<double>& signal, double sampleRate = 1.0) const;

    void trainClassifier(const std::vector<std::vector<double>>& signals, const std::vector<int>& labels);
    std::pair<int, std::vector<double>> classify(const std::vector<double>& signal);

    std::pair<bool, double> anomalyDetect(const std::vector<double>& signal,
                                           const std::vector<double>* reference = nullptr) const;

private:
    NSPConfig config_;
    std::vector<std::vector<double>> classifierWeights_;  // [n_classes][n_features]
    std::vector<double> classifierBias_;                  // [n_classes]
    std::vector<double> featureMean_;
    std::vector<double> featureStd_;
    std::vector<int> classLabels_;
    bool classifierTrained_ = false;

    std::vector<double> fftFeatures(const std::vector<double>& signal) const;
};

}  // namespace deepiri
