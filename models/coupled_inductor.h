#ifndef DEEPIRI_COUPLED_INDUCTOR_H
#define DEEPIRI_COUPLED_INDUCTOR_H

#include "device.h"
#include <string>

namespace deepiri {

// Two inductors with mutual coupling K (M = k·√(L1·L2)), 4 terminals.
class CoupledInductor : public Device {
public:
    CoupledInductor(const std::string& name,
                    double l1, double l2, double k,
                    size_t n1p, size_t n1n, size_t n2p, size_t n2n);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;
    std::vector<size_t> terminals() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "CoupledInductor"; }

    void getInitialGuess(std::vector<double>&) const override {}
    void updateState(const std::vector<double>&) override {}
    void setTrapezoidal(bool trap) override { useTrap_ = trap; }
    void prepareTransientStep(double h, const std::vector<double>& prev) override;
    void acceptTransientStep(const std::vector<double>& state) override;

private:
    std::string name_;
    double L1_, L2_, M_;
    size_t n1p_, n1n_, n2p_, n2n_;
    double i1_ = 0, i2_ = 0;
    bool transientActive_ = false;
    bool useTrap_ = false;
    // Companion stamps: i = Geq * v + Ieq (2 ports)
    double g11_ = 0, g12_ = 0, g21_ = 0, g22_ = 0;
    double ieq1_ = 0, ieq2_ = 0;
};

}

#endif
