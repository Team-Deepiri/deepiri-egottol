#include "vsrc.h"
#include <cmath>

namespace deepiri {

Vsrc::Vsrc(double dc)
    : name_("V"), type_(SourceType::DC), dcValue_(dc), acValue_(0.0), acPhase_(0.0),
      pulseV1_(0.0), pulseV2_(dc), pulseTd_(0.0), pulseTr_(1e-9), pulseTf_(1e-9),
      pulsePw_(1e-3), pulsePeriod_(1e-3), current_(0.0) {}

Vsrc::Vsrc(const std::string& name, double dc)
    : name_(name), type_(SourceType::DC), dcValue_(dc), acValue_(0.0), acPhase_(0.0),
      pulseV1_(0.0), pulseV2_(dc), pulseTd_(0.0), pulseTr_(1e-9), pulseTf_(1e-9),
      pulsePw_(1e-3), pulsePeriod_(1e-3), current_(0.0) {}

void Vsrc::setPulse(double v1, double v2, double td, double tr, double tf, double pw, double period) {
    type_ = SourceType::PULSE;
    pulseV1_ = v1;
    pulseV2_ = v2;
    pulseTd_ = td;
    pulseTr_ = tr;
    pulseTf_ = tf;
    pulsePw_ = pw;
    pulsePeriod_ = period;
}

void Vsrc::initializeDC() {
    current_ = 0.0;
}

double Vsrc::getVoltage(double t) const {
    if (type_ == SourceType::DC) {
        return dcValue_;
    } else if (type_ == SourceType::AC) {
        return acValue_ * std::cos(2.0 * M_PI * t * 1e9 + acPhase_ * M_PI / 180.0);
    } else if (type_ == SourceType::PULSE) {
        if (t < pulseTd_) return pulseV1_;
        double t_local = t - pulseTd_;
        if (pulsePeriod_ > 0) {
            t_local = std::fmod(t_local, pulsePeriod_);
        }
        if (t_local < pulseTr_) {
            return pulseV1_ + (pulseV2_ - pulseV1_) * t_local / pulseTr_;
        } else if (t_local < pulseTr_ + pulsePw_) {
            return pulseV2_;
        } else if (t_local < pulseTr_ + pulsePw_ + pulseTf_) {
            return pulseV2_ + (pulseV1_ - pulseV2_) * (t_local - pulseTr_ - pulsePw_) / pulseTf_;
        }
        return pulseV1_;
    }
    return dcValue_;
}

std::vector<double> Vsrc::getCurrent() const {
    return {current_, -current_};
}

std::vector<std::vector<double>> Vsrc::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (nodeP_ > 0 && nodeN_ > 0) {
        G[0][0] = 0.0;
        G[0][1] = 0.0;
        G[1][0] = 0.0;
        G[1][1] = 0.0;
    }
    return G;
}

void Vsrc::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = dcValue_;
    }
}

void Vsrc::updateState(const std::vector<double>& state) {
}

}