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
    vPrev_ = 0.0;
}

std::vector<double> Inductor::getCurrent() const {
    if (!transientActive_) {
        return {0.0, 0.0};
    }
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
    (void)state;
}

void Inductor::prepareTransientStep(double h, const std::vector<double>& prevNodeVoltages) {
    if (h <= 0.0 || L_ == 0.0) {
        transientActive_ = false;
        return;
    }
    double vp = (nodeP_ > 0 && nodeP_ - 1 < prevNodeVoltages.size()) ? prevNodeVoltages[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < prevNodeVoltages.size()) ? prevNodeVoltages[nodeN_ - 1] : 0.0;
    vPrev_ = vp - vn;
    if (useTrap_) {
        // Trap: i = (h/2L)v + i_prev + (h/2L)v_prev
        geq_ = h / (2.0 * L_);
        ieq_ = -(i_ + geq_ * vPrev_);
    } else {
        geq_ = h / L_;
        ieq_ = -i_;
    }
    transientActive_ = true;
}

void Inductor::acceptTransientStep(const std::vector<double>& nodeVoltages) {
    double vp = (nodeP_ > 0 && nodeP_ - 1 < nodeVoltages.size()) ? nodeVoltages[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < nodeVoltages.size()) ? nodeVoltages[nodeN_ - 1] : 0.0;
    double v = vp - vn;
    if (transientActive_) {
        i_ = geq_ * v - ieq_;
    }
    flux_ = L_ * i_;
}

}
