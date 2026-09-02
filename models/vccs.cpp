#include "vccs.h"

namespace deepiri {

VCCS::VCCS(double transconductance)
    : name_("G"), gm_(transconductance) {}

VCCS::VCCS(const std::string& name, double transconductance)
    : name_(name), gm_(transconductance) {}

void VCCS::setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN) {
    nodeP_ = nP;
    nodeN_ = nN;
    nodeCP_ = nCP;
    nodeCN_ = nCN;
    terminals_ = {nP, nN, nCP, nCN};
}

std::vector<size_t> VCCS::terminals() const {
    return {nodeP_, nodeN_, nodeCP_, nodeCN_};
}

void VCCS::initializeDC() {}

std::vector<double> VCCS::getCurrent() const {
    return {0.0, 0.0, 0.0, 0.0};
}

std::vector<std::vector<double>> VCCS::getConductanceMatrix() const {
    // Terminals: 0=n+, 1=n−, 2=nc+, 3=nc−
    // Matches Isrc RHS convention (stampDevice: rhs += I_into_device negated via Gv=rhs).
    // Positive gm·(Vnc+−Vnc−) flows n+→n− in SPICE → Vout = +gm·Vin·Rload.
    std::vector<std::vector<double>> G(4, std::vector<double>(4, 0.0));
    G[0][2] = -gm_;
    G[0][3] = gm_;
    G[1][2] = gm_;
    G[1][3] = -gm_;
    return G;
}

void VCCS::getInitialGuess(std::vector<double>& /*guess*/) const {}

void VCCS::updateState(const std::vector<double>& /*state*/) {}

}
