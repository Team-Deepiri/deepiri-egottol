#include "inference.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace deepiri {

std::vector<double> softmax(const std::vector<double>& logits, double temperature) {
    double t = std::max(temperature, 1e-9);
    std::vector<double> scaled(logits.size());
    for (size_t i = 0; i < logits.size(); ++i) scaled[i] = logits[i] / t;
    double maxVal = scaled.empty() ? 0.0 : *std::max_element(scaled.begin(), scaled.end());
    std::vector<double> exp(scaled.size());
    double sum = 0.0;
    for (size_t i = 0; i < scaled.size(); ++i) {
        exp[i] = std::exp(scaled[i] - maxVal);
        sum += exp[i];
    }
    if (sum <= 0.0) sum = 1e-9;
    for (double& v : exp) v /= sum;
    return exp;
}

std::pair<std::vector<double>, double> InferenceEngine::infer(const std::vector<double>& z) const {
    switch (config_.backend) {
        case InferenceBackend::Analog:
            return inferAnalog(z);
        case InferenceBackend::EnergyBased:
            return inferEnergyBased(z);
        case InferenceBackend::Digital:
        default:
            return inferDigital(z);
    }
}

Matrix InferenceEngine::matchWeights(size_t zSize) const {
    if (weights_.cols() == zSize) return weights_;
    Matrix w(weights_.rows(), zSize, 0.0);
    size_t cols = std::min(weights_.cols(), zSize);
    for (size_t i = 0; i < weights_.rows(); ++i) {
        for (size_t j = 0; j < cols; ++j) w.at(i, j) = weights_.at(i, j);
    }
    return w;
}

Matrix InferenceEngine::matchConductance(size_t zSize) const {
    if (conductance_.cols() == zSize) return conductance_;
    Matrix g(conductance_.rows(), zSize, 0.0);
    size_t cols = std::min(conductance_.cols(), zSize);
    for (size_t i = 0; i < conductance_.rows(); ++i) {
        for (size_t j = 0; j < cols; ++j) g.at(i, j) = conductance_.at(i, j);
    }
    return g;
}

std::vector<double> InferenceEngine::dacEncode(const std::vector<double>& z) const {
    double maxAbs = 1e-9;
    for (double v : z) maxAbs = std::max(maxAbs, std::abs(v));
    std::vector<double> out(z.size());
    for (size_t i = 0; i < z.size(); ++i) {
        double zn = z[i] / maxAbs;
        out[i] = config_.vDd * std::clamp(zn, 0.0, 1.0);
    }
    return out;
}

std::vector<double> InferenceEngine::quantizeAdc(const std::vector<double>& currents) const {
    double levels = static_cast<double>((1 << config_.adcBits) - 1);
    double vRef = std::max(config_.vRef, 1e-9);
    std::vector<double> out(currents.size());
    for (size_t i = 0; i < currents.size(); ++i) {
        double scaled = currents[i] / vRef;
        double clipped = std::clamp(scaled, 0.0, 1.0);
        out[i] = std::round(clipped * levels) / levels;
    }
    return out;
}

std::pair<std::vector<double>, double> InferenceEngine::inferAnalog(const std::vector<double>& z) const {
    Matrix g = matchConductance(z.size());
    std::vector<double> vRow = dacEncode(z);
    std::vector<double> iCol = g * vRow;
    std::vector<double> yHat = quantizeAdc(iCol);

    std::vector<double> probs;
    if (config_.digitalHead == DigitalHead::Softmax) {
        probs = softmax(yHat, config_.temperature);
    } else {
        double sumAbs = 1e-9;
        for (double v : yHat) sumAbs += std::abs(v);
        probs.resize(yHat.size());
        double sum = 0.0;
        for (size_t i = 0; i < yHat.size(); ++i) {
            double p = std::max(yHat[i] / sumAbs, 0.0);
            probs[i] = p;
            sum += p;
        }
        sum = std::max(sum, 1e-9);
        for (double& p : probs) p /= sum;
    }
    double confidence = probs.empty() ? 0.0 : *std::max_element(probs.begin(), probs.end());
    return {probs, confidence};
}

std::pair<std::vector<double>, double> InferenceEngine::inferDigital(const std::vector<double>& z) const {
    Matrix w = matchWeights(z.size());
    std::vector<double> logits = w * z;
    for (size_t i = 0; i < logits.size() && i < bias_.size(); ++i) logits[i] += bias_[i];
    // Both digital_head branches in inference.py call _softmax; ported verbatim.
    std::vector<double> probs = softmax(logits, config_.temperature);
    double confidence = probs.empty() ? 0.0 : *std::max_element(probs.begin(), probs.end());
    return {probs, confidence};
}

std::pair<std::vector<double>, double> InferenceEngine::inferEnergyBased(const std::vector<double>& z) const {
    Matrix w = matchWeights(z.size());
    size_t numClasses = w.rows();
    std::vector<double> y(numClasses, numClasses > 0 ? 1.0 / static_cast<double>(numClasses) : 0.0);
    std::vector<double> wz = w * z;

    for (int iter = 0; iter < config_.ebmIterations; ++iter) {
        std::vector<double> grad(numClasses);
        for (size_t i = 0; i < numClasses; ++i) grad[i] = -wz[i] + y[i];
        for (size_t i = 0; i < numClasses; ++i) y[i] -= config_.ebmLearningRate * grad[i];
        for (double& v : y) v = std::max(v, 0.0);
        double sumY = std::accumulate(y.begin(), y.end(), 0.0);
        for (double& v : y) v += config_.ebmLambda * (1.0 - sumY);
        double total = std::accumulate(y.begin(), y.end(), 0.0);
        if (total > 0.0) {
            for (double& v : y) v /= total;
        }
    }
    double confidence = y.empty() ? 0.0 : *std::max_element(y.begin(), y.end());
    return {y, confidence};
}

}  // namespace deepiri
