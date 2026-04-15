#include "inductor.h"

namespace deepiri {

Inductor::Inductor(double inductance, double i)
    : name_("L"), L_(inductance), L0_(inductance), i_(i), iInitial_(i), flux_(inductance * i) {}

Inductor::Inductor(const std::string& name, double inductance, double initialCurrent)
    : name_(name), L_(inductance), L0_(inductance), i_(initialCurrent), iInitial_(initialCurrent), flux_(inductance * initialCurrent) {}

void Inductor::setCoupling(size_t otherInductor, double k) {
    couplingMap_[otherInductor] = k;
}

void Inductor::initializeDC() {
    i_ = iInitial_;
    flux_ = L_ * i_;
}

std::vector<double> Inductor::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> Inductor::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (nodeP_ > 0 && nodeN_ > 0) {
        G[0][0] = 0.0;
        G[0][1] = 0.0;
        G[1][0] = 0.0;
        G[1][1] = 0.0;
    }
    return G;
}

void Inductor::getInitialGuess(std::vector<double>& guess) const {
}

void Inductor::updateState(const std::vector<double>& state) {
    if (nodeP_ > 0 && nodeN_ > 0 && nodeP_ - 1 < state.size() && nodeN_ - 1 < state.size()) {
        double v = state[nodeP_ - 1] - state[nodeN_ - 1];
        double dt = 1e-6;
        i_ += (v / L_) * dt;
        flux_ = L_ * i_;
    }
}

}