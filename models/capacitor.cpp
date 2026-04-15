#include "capacitor.h"
#include <cmath>

namespace deepiri {

Capacitor::Capacitor(double capacitance, double v)
    : name_("C"), C_(capacitance), C0_(capacitance), v_(v), vInitial_(v), q_(capacitance * v), vc1_(0.0), vc2_(0.0) {}

Capacitor::Capacitor(const std::string& name, double capacitance, double initialVoltage)
    : name_(name), C_(capacitance), C0_(capacitance), v_(initialVoltage), vInitial_(initialVoltage), q_(capacitance * initialVoltage), vc1_(0.0), vc2_(0.0) {}

void Capacitor::initializeDC() {
    v_ = vInitial_;
    q_ = C_ * v_;
}

std::vector<double> Capacitor::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> Capacitor::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (nodeP_ > 0 && nodeN_ > 0) {
        G[0][0] = 0.0;
        G[0][1] = 0.0;
        G[1][0] = 0.0;
        G[1][1] = 0.0;
    }
    return G;
}

void Capacitor::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = vInitial_;
    }
}

void Capacitor::updateState(const std::vector<double>& state) {
    if (nodeP_ > 0 && nodeN_ > 0 && nodeP_ - 1 < state.size() && nodeN_ - 1 < state.size()) {
        v_ = state[nodeP_ - 1] - state[nodeN_ - 1];
        double dv = vc1_ + 2.0 * vc2_ * v_;
        C_ = C0_ * (1.0 + vc1_ * v_ + vc2_ * v_ * v_);
        q_ = C_ * v_;
    }
}

}