#include "ota.h"
#include <algorithm>

namespace deepiri {

OTA::OTA(double gm, double vLimit)
    : name_("OTA"), gm_(gm), vLimit_(vLimit), nodeOut_(0) {}

OTA::OTA(const std::string& name, double gm, double vLimit)
    : name_(name), gm_(gm), vLimit_(vLimit), nodeOut_(0) {}

void OTA::setNodes(size_t nPlus, size_t nMinus, size_t nOut) {
    nodeP_ = nPlus;
    nodeN_ = nMinus;
    nodeOut_ = nOut;
}

double OTA::outputCurrent(double vPlus, double vMinus) const {
    double vd = vPlus - vMinus;
    vd = std::clamp(vd, -vLimit_, vLimit_);
    return gm_ * vd;
}

void OTA::initializeDC() {}

std::vector<double> OTA::getCurrent() const {
    return {0.0, 0.0, 0.0};
}

std::vector<std::vector<double>> OTA::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(3, std::vector<double>(3, 0.0));
    G[2][0] += gm_;
    G[2][1] -= gm_;
    return G;
}

void OTA::getInitialGuess(std::vector<double>& guess) const {}

void OTA::updateState(const std::vector<double>& state) {}

}
