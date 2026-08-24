#include "inductor.h"

namespace deepiri {

Inductor::Inductor(double inductance, double i)
    : name_("L"), L_(inductance), L0_(inductance), i_(i), iInitial_(i), flux_(inductance * i) {
    terminals_ = {nodeP_, nodeN_};
}

Inductor::Inductor(const std::string& name, double inductance, double initialCurrent)
    : name_(name), L_(inductance), L0_(inductance), i_(initialCurrent),
      iInitial_(initialCurrent), flux_(inductance * initialCurrent) {
    terminals_ = {nodeP_, nodeN_};
}

void Inductor::setCoupling(size_t otherInductor, double k) {
    couplingMap_[otherInductor] = k;
}

void Inductor::initializeDC() {
    i_ = iInitial_;
    flux_ = L_ * i_;
    transientActive_ = false;
}

std::vector<double> Inductor::getCurrent() const {
    if (!transientActive_) {
        return {0.0, 0.0};  // DC handled as short in DcOperatingPoint
    }
    // Backward Euler: i = (h/L)*v + i_prev = geq*v + i_prev
    // i_+ = geq*v + i_prev → RHS at + gets -i_prev if we define carefully.
    // i = geq*v - (-i_prev); stamp geq, RHS_+ = -i_prev
    return {ieq_, -ieq_};
}

std::vector<std::vector<double>> Inductor::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (!transientActive_) return G;
    G[0][0] = geq_;
    G[0][1] = -geq_;
    G[1][0] = -geq_;
    G[1][1] = geq_;
    return G;
}

void Inductor::getInitialGuess(std::vector<double>& guess) const {}

void Inductor::updateState(const std::vector<double>& state) {
    double vp = (nodeP_ > 0 && nodeP_ - 1 < state.size()) ? state[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < state.size()) ? state[nodeN_ - 1] : 0.0;
    (void)vp;
    (void)vn;
}

void Inductor::prepareTransientStep(double h, const std::vector<double>& /*prev*/) {
    if (h <= 0.0 || L_ == 0.0) {
        transientActive_ = false;
        return;
    }
    geq_ = h / L_;
    // i_new = geq * v_new + i_old  →  geq*v - (-i_old) 
    ieq_ = -i_;  // RHS at + terminal
    transientActive_ = true;
}

void Inductor::acceptTransientStep(const std::vector<double>& nodeVoltages) {
    double vp = (nodeP_ > 0 && nodeP_ - 1 < nodeVoltages.size()) ? nodeVoltages[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < nodeVoltages.size()) ? nodeVoltages[nodeN_ - 1] : 0.0;
    double v = vp - vn;
    if (transientActive_) {
        // i_new = geq*v + i_old, and ieq_ was set to -i_old at prepare.
        i_ = geq_ * v - ieq_;
    }
    flux_ = L_ * i_;
}

}
