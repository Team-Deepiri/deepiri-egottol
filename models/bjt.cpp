#include "bjt.h"

namespace deepiri {

BJT::BJT(BJTType type, double is, double bf)
    : type_(type), name_("Q"), Is_(is), BF_(bf), BR_(1.0), VAF_(1e3), VAR_(1e3),
      Cje_(0.0), Cjc_(0.0), Tf_(1e-9), Tr_(1e-6),
      vbe_(0.7), vbc_(0.0), ie_(0.0), ib_(0.0), ic_(0.0),
      gmu_(0.0), gpi_(0.0), gm_(0.0), vt_(0.02585), area_(1.0) {}

BJT::BJT(const std::string& name, BJTType type, double is, double bf)
    : type_(type), name_(name), Is_(is), BF_(bf), BR_(1.0), VAF_(1e3), VAR_(1e3),
      Cje_(0.0), Cjc_(0.0), Tf_(1e-9), Tr_(1e-6),
      vbe_(0.7), vbc_(0.0), ie_(0.0), ib_(0.0), ic_(0.0),
      gmu_(0.0), gpi_(0.0), gm_(0.0), vt_(0.02585), area_(1.0) {}

void BJT::initializeDC() {
    vt_ = (1.380649e-23 * temperature_) / (1.602176634e-19);
    vbe_ = 0.7;
    vbc_ = 0.0;

    double expF = std::exp(vbe_ / vt_);
    ib_ = Is_ * (expF - 1.0) / BF_;
    ic_ = Is_ * (expF - 1.0) * (1.0 + vbc_ / VAF_);

    gm_ = (ic_ + Is_) / vt_;
    gpi_ = (ib_ + Is_ / BF_) / vt_;
    gmu_ = (Is_ * expF) / (BF_ * VAF_);
}

std::vector<double> BJT::getCurrent() const {
    return {ib_ + ic_, -ic_, -ib_};
}

std::vector<std::vector<double>> BJT::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(3, std::vector<double>(3, 0.0));
    if (nodeP_ > 0 && nodeN_ > 0) {
        G[0][0] = gpi_ + gmu_;
        G[0][1] = -gmu_;
        G[0][2] = -gpi_;
        G[1][0] = -gm_ - gmu_;
        G[1][1] = gm_ + gmu_;
        G[1][2] = 0.0;
        G[2][0] = -gpi_ + gm_;
        G[2][1] = -gm_;
        G[2][2] = gpi_;
    }
    return G;
}

void BJT::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = 0.7;
    }
}

void BJT::updateState(const std::vector<double>& state) {
    if (nodeP_ > 0 && nodeN_ > 0 && nodeP_ - 1 < state.size() && nodeN_ - 1 < state.size()) {
        vbe_ = state[nodeP_ - 1] - state[nodeN_ - 1];
        vbc_ = vbe_ - (state[nodeP_ - 1] - 0.0);

        double expF = (vbe_ > -5.0 * vt_) ? std::exp(vbe_ / vt_) : 0.0;
        double expR = (vbc_ > -5.0 * vt_) ? std::exp(vbc_ / vt_) : 0.0;

        double sign = (type_ == BJTType::NPN) ? 1.0 : -1.0;
        ie_ = sign * Is_ * (expF - 1.0);
        ib_ = sign * Is_ * (expF - 1.0) / BF_ + sign * Is_ * (expR - 1.0) / BR_;
        ic_ = sign * Is_ * (expF - 1.0) * (1.0 + vbc_ / VAF_) - sign * Is_ * (expR - 1.0) / BR_;

        if (vbe_ > -5.0 * vt_) {
            gm_ = sign * (Is_ * expF) / vt_;
            gpi_ = sign * (Is_ * expF) / (BF_ * vt_);
        } else {
            gm_ = sign * Is_ / vt_;
            gpi_ = sign * Is_ / (BF_ * vt_);
        }

        if (vbc_ > -5.0 * vt_) {
            gmu_ = sign * (Is_ * expR) / (BR_ * vt_);
        } else {
            gmu_ = sign * Is_ / (BR_ * vt_);
        }
    }
}

}