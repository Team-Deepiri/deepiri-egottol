#include "hopfield.h"

#include <algorithm>
#include <numeric>

namespace deepiri {

HopfieldNetwork::HopfieldNetwork(size_t nNeurons, uint64_t rngSeed)
    : n_(nNeurons),
      W_(nNeurons * nNeurons, 0.0),
      bias_(nNeurons, 0.0),
      state_(nNeurons, 1.0),
      rng_(rngSeed) {}

std::vector<double> HopfieldNetwork::asBipolar(const std::vector<double>& state) {
    std::vector<double> s(state.size());
    for (size_t i = 0; i < state.size(); ++i) {
        s[i] = state[i] >= 0.0 ? 1.0 : -1.0;
    }
    return s;
}

std::vector<double> HopfieldNetwork::asBinary(const std::vector<double>& state) {
    std::vector<double> s(state.size());
    for (size_t i = 0; i < state.size(); ++i) {
        s[i] = state[i] >= 0.0 ? 1.0 : 0.0;
    }
    return s;
}

double HopfieldNetwork::energy() const {
    return energy(state_);
}

double HopfieldNetwork::energy(const std::vector<double>& state) const {
    std::vector<double> s = asBipolar(state);
    double quad = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        double row = 0.0;
        for (size_t j = 0; j < n_; ++j) {
            row += W_[i * n_ + j] * s[j];
        }
        quad += s[i] * row;
    }
    double linear = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        linear += bias_[i] * s[i];
    }
    return -0.5 * quad - linear;
}

void HopfieldNetwork::storePattern(const std::vector<double>& pattern) {
    std::vector<double> p = asBinary(pattern);
    std::vector<double> bp(n_);
    for (size_t i = 0; i < n_; ++i) {
        bp[i] = 2.0 * p[i] - 1.0;
    }
    for (size_t i = 0; i < n_; ++i) {
        for (size_t j = 0; j < n_; ++j) {
            W_[i * n_ + j] += bp[i] * bp[j];
        }
    }
    for (size_t i = 0; i < n_; ++i) {
        W_[i * n_ + i] = 0.0;
    }
}

void HopfieldNetwork::storePatterns(const std::vector<std::vector<double>>& patterns) {
    for (const auto& pat : patterns) {
        storePattern(pat);
    }
}

std::vector<double> HopfieldNetwork::updateAsync(int maxSteps) {
    std::vector<size_t> order(n_);
    std::iota(order.begin(), order.end(), 0);

    for (int step = 0; step < maxSteps; ++step) {
        std::shuffle(order.begin(), order.end(), rng_);
        bool changed = false;
        for (size_t i : order) {
            std::vector<double> s = asBipolar(state_);
            double h = bias_[i];
            for (size_t j = 0; j < n_; ++j) {
                h += W_[i * n_ + j] * s[j];
            }
            double newVal = h >= 0.0 ? 1.0 : -1.0;
            if (newVal != s[i]) {
                state_[i] = newVal;
                changed = true;
            }
        }
        if (!changed) {
            break;
        }
    }
    return state_;
}

std::vector<double> HopfieldNetwork::recall(const std::vector<double>& cue, int maxSteps, double noise) {
    std::vector<double> c = cue;
    if (noise > 0.0) {
        std::uniform_real_distribution<double> uni(0.0, 1.0);
        for (size_t i = 0; i < n_; ++i) {
            if (uni(rng_) < noise) {
                c[i] = -c[i];
            }
        }
    }
    state_ = asBipolar(c);
    return updateAsync(maxSteps);
}

}
