#include "vcvs.h"

namespace deepiri {

VCVS::VCVS(double gain)
    : name_("E"), gain_(gain), nodeCP_(0), nodeCN_(0), vOut_(0.0) {}

VCVS::VCVS(const std::string& name, double gain)
    : name_(name), gain_(gain), nodeCP_(0), nodeCN_(0), vOut_(0.0) {}

void VCVS::setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN) {
    nodeP_ = nP;
    nodeN_ = nN;
    nodeCP_ = nCP;
    nodeCN_ = nCN;
}

void VCVS::initializeDC() {
    vOut_ = 0.0;
}

std::vector<double> VCVS::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> VCVS::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    return G;
}

void VCVS::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = 0.0;
    }
}

void VCVS::updateState(const std::vector<double>& state) {
    if (nodeCP_ > 0 && nodeCN_ > 0 && nodeCP_ - 1 < state.size() && nodeCN_ - 1 < state.size()) {
        vIn_ = state[nodeCP_ - 1] - state[nodeCN_ - 1];
        vOut_ = gain_ * vIn_;
    }
}

}