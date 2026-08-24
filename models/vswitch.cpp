#include "vswitch.h"
#include <cmath>

namespace deepiri {

VSwitch::VSwitch(const std::string& name, double vt, double ron, double roff)
    : name_(name), vt_(vt), ron_(ron > 0 ? ron : 1.0), roff_(roff > 0 ? roff : 1e12) {
    g_ = 1.0 / roff_;
}

void VSwitch::setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN) {
    nodeP_ = nP;
    nodeN_ = nN;
    nodeCP_ = nCP;
    nodeCN_ = nCN;
    terminals_ = {nP, nN, nCP, nCN};
}

std::vector<size_t> VSwitch::terminals() const {
    return {nodeP_, nodeN_, nodeCP_, nodeCN_};
}

void VSwitch::initializeDC() {
    g_ = 1.0 / roff_;
}

std::vector<double> VSwitch::getCurrent() const {
    return {0.0, 0.0, 0.0, 0.0};
}

std::vector<std::vector<double>> VSwitch::getConductanceMatrix() const {
    // Channel between n+/n−; control ports draw no current.
    std::vector<std::vector<double>> G(4, std::vector<double>(4, 0.0));
    G[0][0] = g_;
    G[0][1] = -g_;
    G[1][0] = -g_;
    G[1][1] = g_;
    return G;
}

void VSwitch::updateState(const std::vector<double>& state) {
    auto vAt = [&](size_t n) -> double {
        if (n == 0) return 0.0;
        if (n - 1 < state.size()) return state[n - 1];
        return 0.0;
    };
    double vc = vAt(nodeCP_) - vAt(nodeCN_);
    g_ = (vc >= vt_) ? (1.0 / ron_) : (1.0 / roff_);
}

}
