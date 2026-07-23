#include "opamp_neuron.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace deepiri {

OpAmpNeuronLayer::OpAmpNeuronLayer(size_t nIn, size_t nOut,
                                    NeuronActivation activation,
                                    double gm, double vSat,
                                    uint64_t rngSeed)
    : nIn_(nIn), nOut_(nOut), activation_(activation), gm_(gm), vSat_(vSat),
      W_(nOut * nIn, 0.0), bias_(nOut, 0.0) {
    std::mt19937_64 rng(rngSeed);
    std::normal_distribution<double> dist(0.0, 0.1);
    for (auto& w : W_) {
        w = dist(rng);
    }
    for (size_t j = 0; j < nOut_; ++j) {
        otaCells_.emplace_back(gm_);
    }
}

OpAmpNeuronLayer::OpAmpNeuronLayer(size_t nIn, size_t nOut,
                                    const std::vector<double>& weights,
                                    const std::vector<double>& bias,
                                    NeuronActivation activation,
                                    double gm, double vSat)
    : nIn_(nIn), nOut_(nOut), activation_(activation), gm_(gm), vSat_(vSat),
      W_(weights), bias_(bias) {
    W_.resize(nOut_ * nIn_, 0.0);
    bias_.resize(nOut_, 0.0);
    for (size_t j = 0; j < nOut_; ++j) {
        otaCells_.emplace_back(gm_);
    }
}

double OpAmpNeuronLayer::sigmoid(double x) {
    x = std::clamp(x, -500.0, 500.0);
    return 1.0 / (1.0 + std::exp(-x));
}

double OpAmpNeuronLayer::tanhAct(double x) {
    return std::tanh(x);
}

double OpAmpNeuronLayer::activate(double x) const {
    double scaled = x / std::max(vSat_, 1e-12);
    if (activation_ == NeuronActivation::Sigmoid) {
        return sigmoid(scaled);
    }
    return tanhAct(scaled);
}

std::vector<double> OpAmpNeuronLayer::forward(const std::vector<double>& x) const {
    std::vector<double> inp(nIn_, 0.0);
    for (size_t i = 0; i < nIn_ && i < x.size(); ++i) {
        inp[i] = x[i];
    }

    std::vector<double> outputs(nOut_, 0.0);
    for (size_t j = 0; j < nOut_; ++j) {
        double iSum = 0.0;
        for (size_t i = 0; i < nIn_; ++i) {
            iSum += W_[j * nIn_ + i] * otaCells_[j].outputCurrent(inp[i], 0.0);
        }
        double vNet = iSum / std::max(gm_, 1e-18) + bias_[j];
        outputs[j] = activate(vNet);
    }
    return outputs;
}

void OpAmpNeuronLayer::setWeights(const std::vector<double>& weights) {
    W_ = weights;
    W_.resize(nOut_ * nIn_, 0.0);
}

}
