#pragma once

#include <utility>
#include <vector>

#include "../matrix.h"
#include "types.h"

namespace deepiri {

struct InferenceConfig {
    InferenceBackend backend = InferenceBackend::Digital;
    DigitalHead digitalHead = DigitalHead::Linear;
    int numClasses = 4;
    double temperature = 1.0;
    int adcBits = 8;
    double vRef = 1.0;
    double vDd = 1.0;
    double ebmLambda = 0.01;
    int ebmIterations = 50;
    double ebmLearningRate = 0.1;
};

// Inference operator Psi: embedding z -> (prediction y_hat, confidence p).
// Port of egottol/engines/eii/inference.py::InferenceEngine.
//
// Backends ported: analog crossbar readout, digital linear/softmax head,
// energy-based classifier. The RESERVOIR and HOPFIELD backends in the Python
// source delegate to egottol/engines/ai/reservoir.py and hopfield_infer.py,
// which live outside the eii/ pipeline and are not part of docs/eii-math.md's
// Psi table; they are deferred (see report), not silently dropped.
class InferenceEngine {
public:
    InferenceEngine(InferenceConfig config, Matrix weights, std::vector<double> bias, Matrix conductance)
        : config_(std::move(config)), weights_(std::move(weights)), bias_(std::move(bias)),
          conductance_(std::move(conductance)) {}

    std::pair<std::vector<double>, double> infer(const std::vector<double>& z) const;

private:
    std::pair<std::vector<double>, double> inferAnalog(const std::vector<double>& z) const;
    std::pair<std::vector<double>, double> inferDigital(const std::vector<double>& z) const;
    std::pair<std::vector<double>, double> inferEnergyBased(const std::vector<double>& z) const;

    std::vector<double> dacEncode(const std::vector<double>& z) const;
    std::vector<double> quantizeAdc(const std::vector<double>& currents) const;
    Matrix matchWeights(size_t zSize) const;
    Matrix matchConductance(size_t zSize) const;

    InferenceConfig config_;
    Matrix weights_;
    std::vector<double> bias_;
    Matrix conductance_;
};

std::vector<double> softmax(const std::vector<double>& logits, double temperature);

}  // namespace deepiri
