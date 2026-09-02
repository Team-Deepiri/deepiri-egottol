#include "vcvs.h"

namespace deepiri {

VCVS::VCVS(double gain) : name_("E"), gain_(gain) {}

VCVS::VCVS(const std::string& name, double gain) : name_(name), gain_(gain) {}

void VCVS::setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN) {
    nodeP_ = nP;
    nodeN_ = nN;
    nodeCP_ = nCP;
    nodeCN_ = nCN;
    terminals_ = {nP, nN, nCP, nCN};
}

std::vector<size_t> VCVS::terminals() const {
    return {nodeP_, nodeN_, nodeCP_, nodeCN_};
}

void VCVS::initializeDC() {}

std::vector<double> VCVS::getCurrent() const {
    // Branch current lives in the MNA auxiliary unknown.
    return {0.0, 0.0, 0.0, 0.0};
}

std::vector<std::vector<double>> VCVS::getConductanceMatrix() const {
    // Stamped via aux branch in MNA / spice_engine (like an ideal Vsrc).
    return std::vector<std::vector<double>>(4, std::vector<double>(4, 0.0));
}

void VCVS::getInitialGuess(std::vector<double>& /*guess*/) const {}

void VCVS::updateState(const std::vector<double>& /*state*/) {}

}
