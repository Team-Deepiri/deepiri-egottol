#pragma once

#include <vector>
#include <random>
#include <cstddef>

namespace deepiri {

class HopfieldNetwork {
public:
    explicit HopfieldNetwork(size_t nNeurons, uint64_t rngSeed = 0);

    double energy() const;
    double energy(const std::vector<double>& state) const;

    void storePattern(const std::vector<double>& pattern);
    void storePatterns(const std::vector<std::vector<double>>& patterns);

    std::vector<double> updateAsync(int maxSteps = 100);
    std::vector<double> recall(const std::vector<double>& cue, int maxSteps = 100, double noise = 0.0);

    const std::vector<double>& weights() const { return W_; }
    const std::vector<double>& bias() const { return bias_; }
    const std::vector<double>& state() const { return state_; }
    size_t size() const { return n_; }

private:
    size_t n_;
    std::vector<double> W_;
    std::vector<double> bias_;
    std::vector<double> state_;
    std::mt19937_64 rng_;

    static std::vector<double> asBipolar(const std::vector<double>& state);
    static std::vector<double> asBinary(const std::vector<double>& state);
};

}
