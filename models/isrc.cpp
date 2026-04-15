#include "isrc.h"
#include <cmath>

namespace deepiri {

Isrc::Isrc(double dc)
    : name_("I"), type_(SourceType::DC), dcValue_(dc), acValue_(0.0), acPhase_(0.0),
      pulseI1_(0.0), pulseI2_(dc), pulseTd_(0.0), pulseTr_(1e-9), pulseTf_(1e-9),
      pulsePw_(1e-3), pulsePeriod_(1e-3) {}

Isrc::Isrc(const std::string& name, double dc)
    : name_(name), type_(SourceType::DC), dcValue_(dc), acValue_(0.0), acPhase_(0.0),
      pulseI1_(0.0), pulseI2_(dc), pulseTd_(0.0), pulseTr_(1e-9), pulseTf_(1e-9),
      pulsePw_(1e-3), pulsePeriod_(1e-3) {}

void Isrc::setPulse(double i1, double i2, double td, double tr, double tf, double pw, double period) {
    type_ = SourceType::PULSE;
    pulseI1_ = i1;
    pulseI2_ = i2;
    pulseTd_ = td;
    pulseTr_ = tr;
    pulseTf_ = tf;
    pulsePw_ = pw;
    pulsePeriod_ = period;
}

void Isrc::initializeDC() {}

double Isrc::getCurrentValue(double t) const {
    if (type_ == SourceType::DC) {
        return dcValue_;
    } else if (type_ == SourceType::AC) {
        return acValue_ * std::cos(2.0 * M_PI * t * 1e9 + acPhase_ * M_PI / 180.0);
    } else if (type_ == SourceType::PULSE) {
        if (t < pulseTd_) return pulseI1_;
        double t_local = t - pulseTd_;
        if (pulsePeriod_ > 0) {
            t_local = std::fmod(t_local, pulsePeriod_);
        }
        if (t_local < pulseTr_) {
            return pulseI1_ + (pulseI2_ - pulseI1_) * t_local / pulseTr_;
        } else if (t_local < pulseTr_ + pulsePw_) {
            return pulseI2_;
        } else if (t_local < pulseTr_ + pulsePw_ + pulseTf_) {
            return pulseI2_ + (pulseI1_ - pulseI2_) * (t_local - pulseTr_ - pulsePw_) / pulseTf_;
        }
        return pulseI1_;
    }
    return dcValue_;
}

std::vector<double> Isrc::getCurrent() const {
    return {-dcValue_, dcValue_};
}

std::vector<std::vector<double>> Isrc::getConductanceMatrix() const {
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    if (nodeP_ > 0 && nodeN_ > 0) {
        G[0][0] = 0.0;
        G[0][1] = 0.0;
        G[1][0] = 0.0;
        G[1][1] = 0.0;
    }
    return G;
}

void Isrc::getInitialGuess(std::vector<double>& guess) const {}

void Isrc::updateState(const std::vector<double>& state) {}

}