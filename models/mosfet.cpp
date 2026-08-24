#include "mosfet.h"
#include <algorithm>
#include <cmath>

namespace deepiri {

MOSFET::MOSFET(MOSFETType type, double w, double l)
    : type_(type), name_("M"), W_(w), L_(l),
      vgs_(0.0), vds_(0.0), vbs_(0.0), vth_(0.7), ids_(0.0),
      gds_(0.0), gm_(0.0), gmb_(0.0),
      vt_(0.02585), eps_ox_(3.9 * 8.854e-12), eps_si_(11.7 * 8.854e-12),
      q_(1.602e-19), ni_(1.45e16), na_(0.0) {
    terminals_ = {0, 0, 0, 0};
}

MOSFET::MOSFET(const std::string& name, MOSFETType type, double w, double l)
    : type_(type), name_(name), W_(w), L_(l),
      vgs_(0.0), vds_(0.0), vbs_(0.0), vth_(0.7), ids_(0.0),
      gds_(0.0), gm_(0.0), gmb_(0.0),
      vt_(0.02585), eps_ox_(3.9 * 8.854e-12), eps_si_(11.7 * 8.854e-12),
      q_(1.602e-19), ni_(1.45e16), na_(0.0) {
    terminals_ = {0, 0, 0, 0};
}

double MOSFET::mobility(double /*vgs*/, double /*vth*/) {
    return (type_ == MOSFETType::NMOS) ? 0.06 : 0.025;  // rough U0
}

double MOSFET::calculateVth(double vbs) {
    double phis = model_.phi_ - vbs;
    if (phis < 0.1) phis = 0.1;
    vth_ = model_.vt0_ + model_.gamma_ * (std::sqrt(phis) - std::sqrt(model_.phi_));
    return vth_;
}

double MOSFET::calculateBeta(double /*vds*/, double /*vgs*/, double /*vth*/) {
    double u0 = mobility(0, 0);
    double cox = eps_ox_ / 10e-9;  // tox ~ 10nm default
    return u0 * cox * W_ / std::max(L_, 1e-9);
}

void MOSFET::calculateIds(double vgs, double vds, double vbs) {
    // Work in NMOS polarity; flip for PMOS at the end.
    double s = (type_ == MOSFETType::NMOS) ? 1.0 : -1.0;
    double vgss = s * vgs;
    double vdss = s * vds;
    double vbss = s * vbs;

    vgs_ = vgs;
    vds_ = vds;
    vbs_ = vbs;
    vth_ = calculateVth(vbss);

    double beta = calculateBeta(vdss, vgss, vth_);
    double von = vgss - vth_;

    if (von <= 0.0) {
        // Subthreshold soft floor
        ids_ = 0.0;
        gm_ = 1e-12;
        gds_ = 1e-12;
        gmb_ = 0.0;
        return;
    }

    double vdsat = von;
    if (vdss < vdsat) {
        // Linear
        ids_ = beta * (von * vdss - 0.5 * vdss * vdss) * (1.0 + model_.lambda_ * vdss);
        gm_ = beta * vdss * (1.0 + model_.lambda_ * vdss);
        gds_ = beta * (von - vdss) * (1.0 + model_.lambda_ * vdss)
             + beta * (von * vdss - 0.5 * vdss * vdss) * model_.lambda_;
    } else {
        // Saturation
        ids_ = 0.5 * beta * von * von * (1.0 + model_.lambda_ * vdss);
        gm_ = beta * von * (1.0 + model_.lambda_ * vdss);
        gds_ = 0.5 * beta * von * von * model_.lambda_;
    }
    gmb_ = gm_ * model_.gamma_ / (2.0 * std::sqrt(std::max(model_.phi_ - vbss, 0.1)));

    ids_ *= s;
    gm_ *= s;
    gds_ = std::abs(gds_);
    gmb_ *= s;
}

void MOSFET::initializeDC() {
    calculateIds(0.0, 0.0, 0.0);
}

std::vector<size_t> MOSFET::terminals() const {
    if (terminals_.size() >= 4) return terminals_;
    // Fallback: treat nodeP=drain, nodeN=source, gate=drain (diode-wired) — bad but safe.
    return {nodeP_, nodeP_, nodeN_, nodeN_};
}

void MOSFET::setTerminals(const std::vector<size_t>& nodes) {
    terminals_ = nodes;
    if (nodes.size() >= 1) nodeP_ = nodes[0];  // drain
    if (nodes.size() >= 3) nodeN_ = nodes[2];  // source
}

std::vector<double> MOSFET::getCurrent() const {
    // Terminals: [D, G, S, B]
    // Channel current Ids from D→S; gate/bulk ideal DC = 0.
    // Newton: iD ≈ ids0 + gm*vgs + gds*vds + gmb*vbs
    //       = gm*(vg-vs) + gds*(vd-vs) + gmb*(vb-vs) + (ids0 - gm*vgs0 - gds*vds0 - gmb*vbs0)
    double ieqD = ids_ - gm_ * vgs_ - gds_ * vds_ - gmb_ * vbs_;
    // iD into drain, -iD into source; G/B zero eq current for now.
    return {ieqD, 0.0, -ieqD, 0.0};
}

std::vector<std::vector<double>> MOSFET::getConductanceMatrix() const {
    // 4x4 stamp on [D,G,S,B]
    std::vector<std::vector<double>> G(4, std::vector<double>(4, 0.0));
    // ∂iD/∂vd = gds, ∂iD/∂vg = gm, ∂iD/∂vs = -(gm+gds+gmb), ∂iD/∂vb = gmb
    // iS = -iD
    G[0][0] = gds_;
    G[0][1] = gm_;
    G[0][2] = -(gm_ + gds_ + gmb_);
    G[0][3] = gmb_;

    G[2][0] = -gds_;
    G[2][1] = -gm_;
    G[2][2] = (gm_ + gds_ + gmb_);
    G[2][3] = -gmb_;
    return G;
}

void MOSFET::getInitialGuess(std::vector<double>& guess) const {
    auto t = terminals();
    if (t.size() >= 1 && t[0] > 0 && t[0] - 1 < guess.size()) guess[t[0] - 1] = 5.0;
    if (t.size() >= 2 && t[1] > 0 && t[1] - 1 < guess.size()) guess[t[1] - 1] = 5.0;
}

void MOSFET::updateState(const std::vector<double>& state) {
    auto t = terminals();
    auto volt = [&](size_t idx) -> double {
        if (idx >= t.size()) return 0.0;
        size_t n = t[idx];
        if (n == 0) return 0.0;
        if (n - 1 < state.size()) return state[n - 1];
        return 0.0;
    };
    double vd = volt(0), vg = volt(1), vs = volt(2), vb = volt(3);
    calculateIds(vg - vs, vd - vs, vb - vs);
}

}
