#include "vsrc.h"

namespace deepiri {

Vsrc::Vsrc(double dc)
    : name_("V"), current_(0.0) {
    signal_.kind = SourceWaveform::DC;
    signal_.dc = dc;
}

Vsrc::Vsrc(const std::string& name, double dc)
    : name_(name), current_(0.0) {
    signal_.kind = SourceWaveform::DC;
    signal_.dc = dc;
}

void Vsrc::setPulse(double v1, double v2, double td, double tr, double tf, double pw, double period) {
    signal_.kind = SourceWaveform::PULSE;
    signal_.pulseV1 = v1;
    signal_.pulseV2 = v2;
    signal_.pulseTd = td;
    signal_.pulseTr = tr;
    signal_.pulseTf = tf;
    signal_.pulsePw = pw;
    signal_.pulsePer = period;
    signal_.dc = v1;
}

void Vsrc::setSin(double vo, double va, double freq, double td, double theta, double phase) {
    signal_.kind = SourceWaveform::SIN;
    signal_.sinVo = vo;
    signal_.sinVa = va;
    signal_.sinFreq = freq;
    signal_.sinTd = td;
    signal_.sinTheta = theta;
    signal_.sinPhase = phase;
    signal_.dc = vo;
}

void Vsrc::setExp(double v1, double v2, double td1, double tau1, double td2, double tau2) {
    signal_.kind = SourceWaveform::EXP;
    signal_.expV1 = v1;
    signal_.expV2 = v2;
    signal_.expTd1 = td1;
    signal_.expTau1 = tau1;
    signal_.expTd2 = td2;
    signal_.expTau2 = tau2;
    signal_.dc = v1;
}

void Vsrc::setPwl(const std::vector<double>& times, const std::vector<double>& values) {
    signal_.kind = SourceWaveform::PWL;
    signal_.pwlT = times;
    signal_.pwlV = values;
    if (!values.empty()) signal_.dc = values.front();
}

void Vsrc::initializeDC() {
    current_ = 0.0;
}

double Vsrc::getVoltage(double t) const {
    return signal_.eval(t);
}

std::vector<double> Vsrc::getCurrent() const {
    return {current_, -current_};
}

std::vector<std::vector<double>> Vsrc::getConductanceMatrix() const {
    return std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0));
}

void Vsrc::getInitialGuess(std::vector<double>& guess) const {
    if (nodeP_ > 0 && nodeP_ - 1 < guess.size()) {
        guess[nodeP_ - 1] = signal_.dc;
    }
}

void Vsrc::updateState(const std::vector<double>& /*state*/) {}

}
