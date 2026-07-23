#include "ising.h"

#include <cmath>

namespace deepiri {

namespace {

std::vector<double> toBipolar(const std::vector<double>& spins) {
    std::vector<double> s(spins.size());
    for (size_t i = 0; i < spins.size(); ++i) {
        s[i] = spins[i] >= 0.0 ? 1.0 : -1.0;
    }
    return s;
}

}

IsingMachine::IsingMachine(size_t nSpins, uint64_t rngSeed)
    : n_(nSpins), J_(nSpins * nSpins, 0.0), h_(nSpins, 0.0), rng_(rngSeed) {}

IsingMachine IsingMachine::fromQubo(const std::vector<double>& Q, size_t n, uint64_t rngSeed) {
    IsingMachine m(n, rngSeed);
    std::vector<double> rowSum(n, 0.0);
    std::vector<double> colSum(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            rowSum[i] += Q[i * n + j];
            colSum[j] += Q[i * n + j];
        }
    }
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            m.J_[i * n + j] = Q[i * n + j] / 4.0;
        }
        m.J_[i * n + i] = 0.0;
        m.h_[i] = rowSum[i] / 2.0 - colSum[i] / 2.0;
    }
    return m;
}

double IsingMachine::energy(const std::vector<double>& spins) const {
    std::vector<double> s = toBipolar(spins);
    double pair = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        for (size_t j = 0; j < n_; ++j) {
            pair += J_[i * n_ + j] * s[i] * s[j];
        }
    }
    double field = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        field += h_[i] * s[i];
    }
    return -pair - field;
}

double IsingMachine::quboValue(const std::vector<double>& spins) const {
    std::vector<double> s = toBipolar(spins);
    std::vector<double> x(n_);
    for (size_t i = 0; i < n_; ++i) {
        x[i] = (s[i] + 1.0) / 2.0;
    }
    double quad = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        double row = 0.0;
        for (size_t j = 0; j < n_; ++j) {
            row += (J_[i * n_ + j] * 4.0) * x[j];
        }
        quad += x[i] * row;
    }
    double linear = 0.0;
    for (size_t i = 0; i < n_; ++i) {
        linear += (h_[i] / 2.0) * x[i];
    }
    return quad + 2.0 * linear;
}

double IsingMachine::deltaEnergy(const std::vector<double>& spins, size_t idx) const {
    std::vector<double> s = toBipolar(spins);
    double local = h_[idx];
    for (size_t j = 0; j < n_; ++j) {
        local += J_[idx * n_ + j] * s[j];
    }
    for (size_t i = 0; i < n_; ++i) {
        local += J_[i * n_ + idx] * s[i];
    }
    return 2.0 * s[idx] * local;
}

IsingResult IsingMachine::solve(int nSteps, double tStart, double tEnd, const std::vector<double>* initialSpins) {
    std::vector<double> spins(n_);
    if (initialSpins != nullptr) {
        spins = toBipolar(*initialSpins);
    } else {
        std::uniform_int_distribution<int> coin(0, 1);
        for (size_t i = 0; i < n_; ++i) {
            spins[i] = coin(rng_) == 0 ? -1.0 : 1.0;
        }
    }

    std::uniform_int_distribution<size_t> idxDist(0, n_ > 0 ? n_ - 1 : 0);
    std::uniform_real_distribution<double> uni(0.0, 1.0);

    int accepted = 0;
    int denom = nSteps - 1 > 1 ? nSteps - 1 : 1;
    for (int step = 0; step < nSteps; ++step) {
        double frac = static_cast<double>(step) / static_cast<double>(denom);
        double t = tStart * std::pow(tEnd / tStart, frac);
        size_t idx = idxDist(rng_);
        double de = deltaEnergy(spins, idx);
        if (de <= 0.0 || uni(rng_) < std::exp(-de / std::max(t, 1e-18))) {
            spins[idx] *= -1.0;
            ++accepted;
        }
    }

    return IsingResult{spins, energy(spins), accepted};
}

}
