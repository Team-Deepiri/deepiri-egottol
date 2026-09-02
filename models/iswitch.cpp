#include "iswitch.h"
#include <cmath>

namespace deepiri {

ISwitch::ISwitch(const std::string& name, double it, double ron, double roff)
    : name_(name), it_(it), ron_(ron > 0 ? ron : 1.0), roff_(roff > 0 ? roff : 1e12) {
    g_ = 1.0 / roff_;
}

void ISwitch::setNodes(size_t nP, size_t nN) {
    nodeP_ = nP;
    nodeN_ = nN;
    terminals_ = {nP, nN};
}

std::vector<size_t> ISwitch::terminals() const {
    return {nodeP_, nodeN_};
}

void ISwitch::initializeDC() {
    g_ = 1.0 / roff_;
}

std::vector<double> ISwitch::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> ISwitch::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    G[0][0] = g_;
    G[0][1] = -g_;
    G[1][0] = -g_;
    G[1][1] = g_;
    return G;
}

void ISwitch::updateFromSenseCurrent(double senseCurrent) {
    g_ = (std::abs(senseCurrent) >= it_) ? (1.0 / ron_) : (1.0 / roff_);
}

}
