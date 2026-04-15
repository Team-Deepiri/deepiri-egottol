#include "vccs.h"

namespace deepiri {

VCCS::VCCS(double transconductance)
    : name_("G"), gm_(transconductance), nodeCP_(0), nodeCN_(0) {}

VCCS::VCCS(const std::string& name, double transconductance)
    : name_(name), gm_(transconductance), nodeCP_(0), nodeCN_(0) {}

void VCCS::setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN) {
    nodeP_ = nP;
    nodeN_ = nN;
    nodeCP_ = nCP;
    nodeCN_ = nCN;
}

void VCCS::initializeDC() {}

std::vector<double> VCCS::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> VCCS::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    return G;
}

void VCCS::getInitialGuess(std::vector<double>& guess) const {}

void VCCS::updateState(const std::vector<double>& state) {}

}