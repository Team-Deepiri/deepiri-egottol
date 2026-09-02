#include "isrc.h"

namespace deepiri {

Isrc::Isrc(double dc) : name_("I") {
    signal_.kind = SourceWaveform::DC;
    signal_.dc = dc;
}

Isrc::Isrc(const std::string& name, double dc) : name_(name) {
    signal_.kind = SourceWaveform::DC;
    signal_.dc = dc;
}

void Isrc::setPulse(double i1, double i2, double td, double tr, double tf, double pw, double period) {
    signal_.kind = SourceWaveform::PULSE;
    signal_.pulseV1 = i1;
    signal_.pulseV2 = i2;
    signal_.pulseTd = td;
    signal_.pulseTr = tr;
    signal_.pulseTf = tf;
    signal_.pulsePw = pw;
    signal_.pulsePer = period;
    signal_.dc = i1;
}

void Isrc::setSin(double io, double ia, double freq, double td, double theta, double phase) {
    signal_.kind = SourceWaveform::SIN;
    signal_.sinVo = io;
    signal_.sinVa = ia;
    signal_.sinFreq = freq;
    signal_.sinTd = td;
    signal_.sinTheta = theta;
    signal_.sinPhase = phase;
    signal_.dc = io;
}

void Isrc::setExp(double i1, double i2, double td1, double tau1, double td2, double tau2) {
    signal_.kind = SourceWaveform::EXP;
    signal_.expV1 = i1;
    signal_.expV2 = i2;
    signal_.expTd1 = td1;
    signal_.expTau1 = tau1;
    signal_.expTd2 = td2;
    signal_.expTau2 = tau2;
    signal_.dc = i1;
}

void Isrc::setPwl(const std::vector<double>& times, const std::vector<double>& values) {
    signal_.kind = SourceWaveform::PWL;
    signal_.pwlT = times;
    signal_.pwlV = values;
    if (!values.empty()) signal_.dc = values.front();
}

void Isrc::initializeDC() { time_ = 0.0; }

double Isrc::getCurrentValue(double t) const {
    return signal_.eval(t);
}

std::vector<double> Isrc::getCurrent() const {
    double i = signal_.eval(time_);
    // Injected into circuit: out of + node into device convention used by RHS += current
    // Historical Isrc stamp: rhs[np] += -I, rhs[nn] += +I  via getCurrent {-I, +I}
    return {-i, i};
}

std::vector<std::vector<double>> Isrc::getConductanceMatrix() const {
    return std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0));
}

void Isrc::getInitialGuess(std::vector<double>& /*guess*/) const {}

void Isrc::updateState(const std::vector<double>& /*state*/) {}

}
