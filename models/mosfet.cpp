#include "mosfet.h"

namespace deepiri {

MOSFET::MOSFET(MOSFETType type, double w, double l)
    : type_(type), name_("M"), W_(w), L_(l),
      vgs_(0.0), vds_(0.0), vbs_(0.0), vth_(0.7), ids_(0.0),
      gds_(0.0), gm_(0.0), gmb_(0.0),
      vt_(0.02585), eps_ox_(3.9 * 8.854e-12), eps_si_(11.7 * 8.854e-12),
      q_(1.602e-19), ni_(1.45e16), na_(0.0) {}

MOSFET::MOSFET(const std::string& name, MOSFETType type, double w, double l)
    : type_(type), name_(name), W_(w), L_(l),
      vgs_(0.0), vds_(0.0), vbs_(0.0), vth_(0.7), ids_(0.0),
      gds_(0.0), gm_(0.0), gmb_(0.0),
      vt_(0.02585), eps_ox_(3.9 * 8.854e-12), eps_si_(11.7 * 8.854e-12),
      q_(1.602e-19), ni_(1.45e16), na_(0.0) {}

double MOSFET::mobility(double vgs, double vth) {
    return 0.05;
}

double MOSFET::calculateVth(double vbs) {
    double sign = (type_ == MOSFETType::NMOS) ? 1.0 : -1.0;
    double vbs_eff = sign * std::abs(vbs);
    if (vbs_eff > 0.0) vbs_eff = 0.0;
    double phis = model_.phi_ - vbs_eff;
    if (phis < 0.1) phis = 0.1;
    vth_ = model_.vt0_ + model_.gamma_ * (std::sqrt(phis) - std::sqrt(model_.phi_));
    return vth_;
}

double MOSFET::calculateBeta(double vds, double vgs, double vth) {
    double u0 = mobility(vgs, vth);
    double vds_eff = vds;
    if (vds_eff > (vgs - vth)) vds_eff = vgs - vth;
    return u0 * W_ / L_ * (1.0 + model_.lambda_ * vds_eff);
}

void MOSFET::calculateIds(double vgs, double vds, double vbs) {
    vgs_ = vgs;
    vds_ = vds;
    vbs_ = vbs;

    double sign = (type_ == MOSFETType::NMOS) ? 1.0 : -1.0;
    vgs_ *= sign;
    vds_ *= sign;
    vbs_ *= sign;

    vth_ = calculateVth(vbs_);

    if (vgs_ < vth_) {
        ids_ = 0.0;
        gm_ = gds_ = gmb_ = 0.0;
        return;
    }

    double vds_eff = vds_;
    if (vds_ > (vgs_ - vth_)) vds_eff = vgs_ - vth_;

    if (vds_ <= 0.0) {
        ids_ = 0.0;
    } else {
        double beta = calculateBeta(vds_, vgs_, vth_);
        ids_ = beta * (vds_eff - vds_eff * vds_eff / (2.0 * (vgs_ - vth_ + 0.001)));
    }

    if (vds_ > 0.0) {
        double vds_sat = vgs_ - vth_;
        if (vds_ < vds_sat) {
            gm_ = calculateBeta(vds_, vgs_, vth_) * vds_eff;
            gds_ = model_.lambda_ * (vgs_ - vth_) * vds_eff / L_ + (vgs_ - vth_) * calculateBeta(vds_, vgs_, vth_) * (-0.5);
            gmb_ = model_.gamma_ / (2.0 * std::sqrt(model_.phi_ - vbs_)) * gm_ * 0.5;
        } else {
            gm_ = calculateBeta(vds_sat, vgs_, vth_) * vds_sat;
            gds_ = model_.lambda_ * (vgs_ - vth_) * vds_sat / L_;
            gmb_ = model_.gamma_ / (2.0 * std::sqrt(model_.phi_ - vbs_)) * gm_ * 0.5;
        }
    }

    ids_ *= sign;
    gm_ *= sign;
    gds_ *= sign;
    gmb_ *= sign;
}

void MOSFET::initializeDC() {
    vgs_ = 5.0;
    vds_ = 2.5;
    vbs_ = 0.0;
    calculateIds(vgs_, vds_, vbs_);
}

std::vector<double> MOSFET::getCurrent() const {
    return {0.0, -ids_, ids_};
}

std::vector<std::vector<double>> MOSFET::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(3, std::vector<double>(3, 0.0));
    if (nodeP_ > 0) {
        G[0][0] = gds_ + gm_ + gmb_;
        G[0][1] = -gds_;
        G[0][2] = -gm_ - gmb_;
        G[1][0] = -gds_ - gm_ - gmb_;
        G[1][1] = gds_ + gm_ + gmb_;
        G[1][2] = 0.0;
    }
    return G;
}

void MOSFET::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = 5.0;
    }
}

void MOSFET::updateState(const std::vector<double>& state) {
    if (nodeP_ > 0 && nodeN_ > 0 && nodeP_ - 1 < state.size() && nodeN_ - 1 < state.size()) {
        double vg = state[nodeP_ - 1];
        double vd = state[nodeN_ - 1];
        double vb = 0.0;
        vgs_ = vg - (nodeN_ > 0 ? state[nodeN_ - 1] : 0.0);
        vds_ = vg - vd;
        vbs_ = vb - (nodeN_ > 0 ? state[nodeN_ - 1] : 0.0);
        calculateIds(vgs_, vds_, vbs_);
    }
}

}