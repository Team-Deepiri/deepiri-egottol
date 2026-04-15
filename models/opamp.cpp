#include "opamp.h"

namespace deepiri {

OpAmp::OpAmp(double gain)
    : name_("OP"), gain_(gain), rin_(1e6), rout_(100.0), slewRate_(1e6),
      vOut_(0.0), vIn_(0.0) {}

OpAmp::OpAmp(const std::string& name, double gain)
    : name_(name), gain_(gain), rin_(1e6), rout_(100.0), slewRate_(1e6),
      vOut_(0.0), vIn_(0.0) {}

void OpAmp::initializeDC() {
    vOut_ = 0.0;
    vIn_ = 0.0;
}

std::vector<double> OpAmp::getCurrent() const {
    return {0.0, 0.0, 0.0};
}

std::vector<std::vector<double>> OpAmp::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(3, std::vector<double>(3, 0.0));
    return G;
}

void OpAmp::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = 0.0;
    }
}

void OpAmp::updateState(const std::vector<double>& state) {
    if (nodeP_ > 0 && nodeN_ > 0 && nodeP_ - 1 < state.size() && nodeN_ - 1 < state.size()) {
        vIn_ = state[nodeP_ - 1] - state[nodeN_ - 1];
        vOut_ = gain_ * vIn_;
    }
}

}