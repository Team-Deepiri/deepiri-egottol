#include "bjt.h"
#include <cmath>

namespace deepiri {

BJT::BJT(BJTType type, double is, double bf)
    : type_(type), name_("Q"), Is_(is), BF_(bf), BR_(1.0), VAF_(100.0), VAR_(100.0),
      Cje_(0.0), Cjc_(0.0), Tf_(1e-9), Tr_(1e-6),
      vbe_(0.7), vbc_(0.0), ie_(0.0), ib_(0.0), ic_(0.0),
      gmu_(0.0), gpi_(0.0), gm_(0.0), vt_(0.02585), area_(1.0) {
    terminals_ = {0, 0, 0};
}

BJT::BJT(const std::string& name, BJTType type, double is, double bf)
    : type_(type), name_(name), Is_(is), BF_(bf), BR_(1.0), VAF_(100.0), VAR_(100.0),
      Cje_(0.0), Cjc_(0.0), Tf_(1e-9), Tr_(1e-6),
      vbe_(0.7), vbc_(0.0), ie_(0.0), ib_(0.0), ic_(0.0),
      gmu_(0.0), gpi_(0.0), gm_(0.0), vt_(0.02585), area_(1.0) {
    terminals_ = {0, 0, 0};
}

void BJT::initializeDC() {
    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    vbe_ = 0.7;
    vbc_ = 0.0;
    double expF = std::exp(vbe_ / vt_);
    ib_ = Is_ * (expF - 1.0) / BF_;
    ic_ = Is_ * (expF - 1.0);
    gm_ = (ic_ + Is_) / vt_;
    gpi_ = (ib_ + Is_ / BF_) / vt_;
    gmu_ = 0.0;
}

std::vector<size_t> BJT::terminals() const {
    if (terminals_.size() >= 3) return terminals_;
    return {nodeP_, nodeP_, nodeN_};
}

void BJT::setTerminals(const std::vector<size_t>& nodes) {
    terminals_ = nodes;
    if (nodes.size() >= 1) nodeP_ = nodes[0];  // collector
    if (nodes.size() >= 3) nodeN_ = nodes[2];  // emitter
}

std::vector<double> BJT::getCurrent() const {
    // Terminals [C, B, E]. Newton Ieq for transport model.
    // iC ≈ ic0 + gm*vbe + go*vce ... simplified: use eq currents
    // iC = gm*vbe + gmu*(vbc) + ... ; use:
    // ieqC = ic - gm*vbe - gmu*vbc (approx)
    double ieqC = ic_ - gm_ * vbe_ - gmu_ * vbc_;
    double ieqB = ib_ - gpi_ * vbe_ - gmu_ * vbc_;
    double ieqE = -(ieqC + ieqB);
    return {ieqC, ieqB, ieqE};
}

std::vector<std::vector<double>> BJT::getConductanceMatrix() const {
    // Hybrid-pi on [C, B, E]
    std::vector<std::vector<double>> G(3, std::vector<double>(3, 0.0));
    // iC: gm(vb-ve) + gmu(vb-vc) + go(vc-ve) ≈ (gmu)*vc + (gm+gmu)*vb + (-gm)*ve  with go~0
    // Using gds≈0: ∂iC/∂vc = gmu? Actually classic:
    //   iC = gm vbe + go vce ; go small
    //   iB = vbe/rπ + vbc/rμ
    double go = (VAF_ > 0) ? std::abs(ic_) / VAF_ : 0.0;

    // C row
    G[0][0] = go + gmu_;
    G[0][1] = gm_ - gmu_;
    G[0][2] = -(gm_ + go);

    // B row
    G[1][0] = -gmu_;
    G[1][1] = gpi_ + gmu_;
    G[1][2] = -gpi_;

    // E row (KCL)
    G[2][0] = -(go);
    G[2][1] = -(gm_ + gpi_);
    G[2][2] = gm_ + gpi_ + go;
    return G;
}

void BJT::getInitialGuess(std::vector<double>& guess) const {
    auto t = terminals();
    if (t.size() >= 2 && t[1] > 0 && t[1] - 1 < guess.size()) guess[t[1] - 1] = 0.7;
    if (t.size() >= 1 && t[0] > 0 && t[0] - 1 < guess.size()) guess[t[0] - 1] = 5.0;
}

void BJT::updateState(const std::vector<double>& state) {
    auto t = terminals();
    auto volt = [&](size_t idx) -> double {
        if (idx >= t.size()) return 0.0;
        size_t n = t[idx];
        if (n == 0) return 0.0;
        if (n - 1 < state.size()) return state[n - 1];
        return 0.0;
    };
    double vc = volt(0), vb = volt(1), ve = volt(2);
    vbe_ = vb - ve;
    vbc_ = vb - vc;

    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    double sign = (type_ == BJTType::NPN) ? 1.0 : -1.0;

    // Clamp exponentials for NR stability
    double vbe_c = std::max(std::min(sign * vbe_, 0.9), -1.0);
    double vbc_c = std::max(std::min(sign * vbc_, 0.9), -1.0);

    double expF = std::exp(vbe_c / vt_);
    double expR = std::exp(vbc_c / vt_);

    ib_ = sign * Is_ * ((expF - 1.0) / BF_ + (expR - 1.0) / BR_);
    ic_ = sign * Is_ * ((expF - 1.0) - (expR - 1.0) / BR_);
    // Early effect
    if (VAF_ > 0) {
        ic_ *= (1.0 - sign * vbc_ / VAF_);
    }
    ie_ = -(ib_ + ic_);

    gm_ = sign * Is_ * expF / vt_;
    gpi_ = sign * Is_ * expF / (BF_ * vt_);
    gmu_ = sign * Is_ * expR / (BR_ * vt_);
}

}
