#include "diode.h"
#include <cmath>
#include <algorithm>

namespace deepiri {

Diode::Diode(double is, double n)
    : name_("D"), Is_(is), n_(n), Rs_(0.0), tt_(0.0), BV_(1e3), Ibv_(1e-3),
      Cj0_(0.0), Vj_(0.7), m_(0.5), vD_(0.0), iD_(0.0), gd_(0.0),
      qVdc_(0.0), qid_(0.0), vt_(0.02585) {}

Diode::Diode(const std::string& name, double is, double n)
    : name_(name), Is_(is), n_(n), Rs_(0.0), tt_(0.0), BV_(1e3), Ibv_(1e-3),
      Cj0_(0.0), Vj_(0.7), m_(0.5), vD_(0.0), iD_(0.0), gd_(0.0),
      qVdc_(0.0), qid_(0.0), vt_(0.02585) {}

void Diode::initializeDC() {
    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    vD_ = 0.7;
    iD_ = Is_ * (std::exp(vD_ / (n_ * vt_)) - 1.0);
    gd_ = (iD_ + Is_) / (n_ * vt_);
}

double Diode::gdEff() const {
    if (Rs_ > 0.0 && gd_ > 0.0) {
        return gd_ / (1.0 + Rs_ * gd_);
    }
    return gd_;
}

std::vector<double> Diode::getCurrent() const {
    // Consistent series-Rs Thevenin: i ≈ i0 + gd_eff*(v_ext - v0)
    double ge = gdEff();
    double ieq = ge * vD_ - iD_;
    return {ieq, -ieq};
}

std::vector<std::vector<double>> Diode::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    double ge = gdEff();
    G[0][0] = ge;
    G[0][1] = -ge;
    G[1][0] = -ge;
    G[1][1] = ge;
    return G;
}

void Diode::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = 0.7;
    }
}

void Diode::updateState(const std::vector<double>& state) {
    double vp = (nodeP_ > 0 && nodeP_ - 1 < state.size()) ? state[nodeP_ - 1] : 0.0;
    double vn = (nodeN_ > 0 && nodeN_ - 1 < state.size()) ? state[nodeN_ - 1] : 0.0;
    double vext = vp - vn;
    // Junction ≈ external; series Rs is folded into gd_eff in the stamp (SPICE-style).
    double vj = vext;
    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    double vdLim = std::max(std::min(vj, 0.9), -1.5 * BV_);
    if (vdLim < -0.1 * BV_) {
        iD_ = -Ibv_ * std::exp(-(vdLim + BV_) / vt_);
        gd_ = Ibv_ / vt_;
    } else if (vdLim < -5.0 * n_ * vt_) {
        iD_ = -Is_;
        gd_ = Is_ / (n_ * vt_);
    } else {
        iD_ = Is_ * (std::exp(vdLim / (n_ * vt_)) - 1.0);
        gd_ = (iD_ + Is_) / (n_ * vt_);
    }
    vD_ = vext;
}

}
