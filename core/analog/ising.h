#pragma once

#include <vector>
#include <random>
#include <cstddef>
#include <cstdint>

namespace deepiri {

struct IsingResult {
    std::vector<double> spins;
    double energy;
    int nAccepted;
};

class IsingMachine {
public:
    explicit IsingMachine(size_t nSpins, uint64_t rngSeed = 0);

    static IsingMachine fromQubo(const std::vector<double>& Q, size_t n, uint64_t rngSeed = 0);

    double energy(const std::vector<double>& spins) const;
    double quboValue(const std::vector<double>& spins) const;

    IsingResult solve(
        int nSteps = 10000,
        double tStart = 10.0,
        double tEnd = 0.01,
        const std::vector<double>* initialSpins = nullptr
    );

    void setCoupling(const std::vector<double>& J) { J_ = J; }
    void setField(const std::vector<double>& h) { h_ = h; }
    const std::vector<double>& coupling() const { return J_; }
    const std::vector<double>& field() const { return h_; }
    size_t size() const { return n_; }

private:
    size_t n_;
    std::vector<double> J_;
    std::vector<double> h_;
    std::mt19937_64 rng_;

    double deltaEnergy(const std::vector<double>& spins, size_t idx) const;
};

}
