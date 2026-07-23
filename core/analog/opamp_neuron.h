#pragma once

#include "ota.h"
#include <vector>
#include <string>
#include <cstdint>

namespace deepiri {

enum class NeuronActivation { Tanh, Sigmoid };

class OpAmpNeuronLayer {
public:
    OpAmpNeuronLayer(size_t nIn, size_t nOut,
                      NeuronActivation activation = NeuronActivation::Tanh,
                      double gm = 1e-3, double vSat = 1.0,
                      uint64_t rngSeed = 0);

    OpAmpNeuronLayer(size_t nIn, size_t nOut,
                      const std::vector<double>& weights,
                      const std::vector<double>& bias,
                      NeuronActivation activation = NeuronActivation::Tanh,
                      double gm = 1e-3, double vSat = 1.0);

    std::vector<double> forward(const std::vector<double>& x) const;
    void setWeights(const std::vector<double>& weights);

    size_t nIn() const { return nIn_; }
    size_t nOut() const { return nOut_; }
    const std::vector<double>& weights() const { return W_; }
    const std::vector<double>& bias() const { return bias_; }

private:
    size_t nIn_, nOut_;
    NeuronActivation activation_;
    double gm_;
    double vSat_;
    std::vector<double> W_;
    std::vector<double> bias_;
    std::vector<OTA> otaCells_;

    static double sigmoid(double x);
    static double tanhAct(double x);
    double activate(double x) const;
};

}
