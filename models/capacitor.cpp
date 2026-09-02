#include "capacitor.h"
#include <cmath>

namespace deepiri {

Capacitor::Capacitor(double capacitance, double v)
    : name_("C"), C_(capacitance), C0_(capacitance), v_(v), vInitial_(v),
      q_(capacitance * v), vc1_(0.0), vc2_(0.0) {
    terminals_ = {nodeP_, nodeN_};
}

Capacitor::Capacitor(const std::string& name, double capacitance, double initialVoltage)
    : name_(name), C_(capacitance), C0_(capacitance), v_(initialVoltage), vInitial_(initialVoltage),
      q_(capacitance * initialVoltage), vc1_(0.0), vc2_(0.0) {
    terminals_ = {nodeP_, nodeN_};
}

void Capacitor::initializeDC() {
    v_ = vInitial_;
    q_ = C_ * v_;
    i_ = 0.0;
    transientActive_ = false;
    geq_ = 0.0;
    ieq_ = 0.0;
}

std::vector<double> Capacitor::getCurrent() const {
    if (!transientActive_) {
        return {0.0, 0.0};  // open at DC
    }
    return {ieq_, -ieq_};
}

std::vector<std::vector<double>> Capacitor::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (!transientActive_) return G;
    G[0][0] = geq_;
    G[0][1] = -geq_;
    G[1][0] = -geq_;
    G[1][1] = geq_;
    return G;
}

void Capacitor::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = vInitial_;
    }
}

void Capacitor::updateState(const std::vector<double>& state) {
    double vp = (nodeP_ > 0 && nodeP_ - 1 < state.size()) ? state[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < state.size()) ? state[nodeN_ - 1] : 0.0;
    v_ = vp - vn;
    C_ = C0_ * (1.0 + vc1_ * v_ + vc2_ * v_ * v_);
    q_ = C_ * v_;
}

void Capacitor::prepareTransientStep(double h, const std::vector<double>& prevNodeVoltages) {
    if (h <= 0.0) {
        transientActive_ = false;
        return;
    }
    double vp = (nodeP_ > 0 && nodeP_ - 1 < prevNodeVoltages.size()) ? prevNodeVoltages[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < prevNodeVoltages.size()) ? prevNodeVoltages[nodeN_ - 1] : 0.0;
    double vPrev = vp - vn;
    if (useTrap_) {
        // Trapezoidal: i = (2C/h)v − [(2C/h)v_prev + i_prev]
        geq_ = 2.0 * C_ / h;
        ieq_ = geq_ * vPrev + i_;
    } else {
        // Backward Euler: i = (C/h)(v − v_prev)
        geq_ = C_ / h;
        ieq_ = geq_ * vPrev;
    }
    transientActive_ = true;
}

void Capacitor::acceptTransientStep(const std::vector<double>& nodeVoltages) {
    updateState(nodeVoltages);
    if (transientActive_) {
        i_ = geq_ * v_ - ieq_;
    }
}

}
