#include "reservoir.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace deepiri {

namespace {

// Power iteration estimate of the dominant eigenvalue magnitude (spectral
// radius). matrix.h has no eigensolver, and reservoir.py only needs the
// magnitude (to rescale w_res), not the eigenvector, so this avoids pulling
// in a full eigen-decomposition for one scalar.
double estimateSpectralRadius(const Matrix& w, std::mt19937& rng) {
    const size_t n = w.rows();
    std::uniform_real_distribution<double> dist(-1.0, 1.0);
    std::vector<double> v(n);
    for (size_t i = 0; i < n; ++i) v[i] = dist(rng);

    double norm = std::sqrt(std::inner_product(v.begin(), v.end(), v.begin(), 0.0));
    if (norm < 1e-12) norm = 1.0;
    for (double& x : v) x /= norm;

    double radius = 0.0;
    const int iterations = 200;
    for (int it = 0; it < iterations; ++it) {
        std::vector<double> wv = w * v;
        double wvNorm = std::sqrt(std::inner_product(wv.begin(), wv.end(), wv.begin(), 0.0));
        if (wvNorm < 1e-12) {
            radius = 0.0;
            break;
        }
        for (size_t i = 0; i < n; ++i) v[i] = wv[i] / wvNorm;
        radius = wvNorm;
    }
    return radius;
}

std::vector<double> tanhVec(std::vector<double> x) {
    for (double& v : x) v = std::tanh(v);
    return x;
}

}  // namespace

Reservoir::Reservoir(const ReservoirConfig& config)
    : config_(config), rng_(config.seed), wRes_(config.n_reservoir, config.n_reservoir) {
    initReservoir();
    state_.assign(config_.n_reservoir, 0.0);
}

void Reservoir::initReservoir() {
    const size_t n = config_.n_reservoir;
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, 1.0);

    Matrix w(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            bool connected = uniform(rng_) < config_.sparsity;
            w.at(i, j) = connected ? normal(rng_) : 0.0;
        }
    }

    double radius = std::max(estimateSpectralRadius(w, rng_), 1e-9);
    double scale = config_.spectral_radius / radius;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            w.at(i, j) *= scale;
        }
    }
    wRes_ = w;
}

void Reservoir::reset() {
    std::fill(state_.begin(), state_.end(), 0.0);
}

void Reservoir::ensureInputWeights(size_t inputDim) {
    if (wInInitialized_ && wIn_.cols() == inputDim) return;
    std::normal_distribution<double> normal(0.0, 1.0);
    Matrix w(config_.n_reservoir, inputDim);
    for (size_t i = 0; i < config_.n_reservoir; ++i) {
        for (size_t j = 0; j < inputDim; ++j) {
            w.at(i, j) = normal(rng_) * config_.input_scaling;
        }
    }
    wIn_ = w;
    wInInitialized_ = true;
}

std::vector<std::vector<double>> Reservoir::collectStates(const Trace& trace) {
    if (trace.empty()) return {};
    const size_t inputDim = trace[0].size();
    ensureInputWeights(inputDim);

    std::vector<std::vector<double>> states(trace.size());
    std::vector<double> h = state_;
    const double leak = config_.leak_rate;

    for (size_t t = 0; t < trace.size(); ++t) {
        std::vector<double> pre = wIn_ * trace[t];
        std::vector<double> resPart = wRes_ * h;
        for (size_t i = 0; i < pre.size(); ++i) pre[i] += resPart[i];
        std::vector<double> activated = tanhVec(std::move(pre));
        for (size_t i = 0; i < h.size(); ++i) {
            h[i] = (1.0 - leak) * h[i] + leak * activated[i];
        }
        states[t] = h;
    }
    state_ = h;
    return states;
}

std::vector<std::vector<double>> Reservoir::collectFinalStates(const Batch& traces) {
    std::vector<std::vector<double>> finals;
    finals.reserve(traces.size());
    for (const Trace& trace : traces) {
        std::vector<std::vector<double>> states = collectStates(trace);
        finals.push_back(states.empty() ? std::vector<double>(config_.n_reservoir, 0.0) : states.back());
    }
    return finals;
}

void Reservoir::fitOnStates(const std::vector<std::vector<double>>& states,
                             const std::vector<std::vector<double>>& targets) {
    const size_t n = config_.n_reservoir;
    const size_t nSamples = states.size();
    const size_t nClasses = targets.empty() ? 0 : targets[0].size();

    Matrix xtx(n, n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            double sum = 0.0;
            for (size_t s = 0; s < nSamples; ++s) sum += states[s][i] * states[s][j];
            xtx.at(i, j) = sum;
        }
        xtx.at(i, i) += config_.ridge_alpha;
    }

    Matrix xty(n, nClasses, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t c = 0; c < nClasses; ++c) {
            double sum = 0.0;
            for (size_t s = 0; s < nSamples; ++s) sum += states[s][i] * targets[s][c];
            xty.at(i, c) = sum;
        }
    }

    Matrix readout(n, nClasses, 0.0);
    for (size_t c = 0; c < nClasses; ++c) {
        std::vector<double> col(n);
        for (size_t i = 0; i < n; ++i) col[i] = xty.at(i, c);
        std::vector<double> w = xtx.solveGaussian(col);
        for (size_t i = 0; i < n; ++i) readout.at(i, c) = w[i];
    }

    readout_ = readout;
    readoutTrained_ = true;
}

void Reservoir::fit(const Batch& traces, const std::vector<int>& labels) {
    std::vector<int> classes = labels;
    std::sort(classes.begin(), classes.end());
    classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
    classLabels_ = classes;

    std::vector<std::vector<double>> targets(labels.size(), std::vector<double>(classes.size(), 0.0));
    for (size_t i = 0; i < labels.size(); ++i) {
        auto it = std::find(classes.begin(), classes.end(), labels[i]);
        targets[i][static_cast<size_t>(it - classes.begin())] = 1.0;
    }
    fit(traces, targets);
}

void Reservoir::fit(const Batch& traces, const std::vector<std::vector<double>>& targets) {
    std::vector<std::vector<double>> states = collectFinalStates(traces);
    fitOnStates(states, targets);
}

std::vector<double> Reservoir::predict(const Trace& trace) {
    if (!readoutTrained_) {
        throw std::runtime_error("Reservoir readout not trained; call fit() first.");
    }
    std::vector<std::vector<double>> states = collectStates(trace);
    std::vector<double> feature = states.empty() ? std::vector<double>(config_.n_reservoir, 0.0) : states.back();

    const size_t nClasses = readout_.cols();
    std::vector<double> logits(nClasses, 0.0);
    for (size_t c = 0; c < nClasses; ++c) {
        double sum = 0.0;
        for (size_t i = 0; i < feature.size(); ++i) sum += feature[i] * readout_.at(i, c);
        logits[c] = sum;
    }

    double maxLogit = *std::max_element(logits.begin(), logits.end());
    std::vector<double> expVals(nClasses);
    double sumExp = 0.0;
    for (size_t c = 0; c < nClasses; ++c) {
        expVals[c] = std::exp(logits[c] - maxLogit);
        sumExp += expVals[c];
    }
    for (double& e : expVals) e /= sumExp;
    return expVals;
}

std::pair<std::vector<double>, double> Reservoir::infer(const Trace& trace) {
    std::vector<double> probs = predict(trace);
    double confidence = probs.empty() ? 0.0 : *std::max_element(probs.begin(), probs.end());
    return {probs, confidence};
}

std::pair<std::vector<double>, double> Reservoir::inferEmbedding(const std::vector<double>& z) {
    Trace trace{z};
    return infer(trace);
}

}  // namespace deepiri
