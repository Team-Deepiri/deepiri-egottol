#include "diode.h"
#include <cmath>

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

std::vector<double> Diode::getCurrent() const {
    // Newton companion: i ≈ i0 + gd*(v - v0) = gd*v + (i0 - gd*v0)
    // Stamp gd in G; RHS at anode gets (gd*vD - iD).
    double ieq = gd_ * vD_ - iD_;
    return {ieq, -ieq};
}

std::vector<std::vector<double>> Diode::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    double totalG = gd_;
    if (Rs_ > 0 && gd_ > 0) totalG = 1.0 / (Rs_ + 1.0 / gd_);
    G[0][0] = totalG;
    G[0][1] = -totalG;
    G[1][0] = -totalG;
    G[1][1] = totalG;
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
    vD_ = vp - vn - Rs_ * iD_;
    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    // Limit for NR stability
    double vdLim = std::max(std::min(vD_, 0.9), -1.5 * BV_);
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
}

}