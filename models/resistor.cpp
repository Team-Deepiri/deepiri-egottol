#include "resistor.h"

namespace deepiri {

Resistor::Resistor(double resistance, double temp)
    : name_("R"), R_(resistance), R0_(resistance), temperature_(temp), Tnom_(300.0), tc1_(0.0), tc2_(0.0), G_(1.0 / resistance) {
    setTemperature(temp);
}

Resistor::Resistor(const std::string& name, double resistance, double temp)
    : name_(name), R_(resistance), R0_(resistance), temperature_(temp), Tnom_(300.0), tc1_(0.0), tc2_(0.0), G_(1.0 / resistance) {
    setTemperature(temp);
}

void Resistor::setTemperature(double temp) {
    temperature_ = temp;
    double deltaT = temp - Tnom_;
    R_ = R0_ * (1.0 + tc1_ * deltaT + tc2_ * deltaT * deltaT);
    if (R_ > 0) {
        G_ = 1.0 / R_;
    }
}

void Resistor::initializeDC() {
    setTemperature(temperature_);
}

std::vector<double> Resistor::getCurrent() const {
    return {0.0, 0.0};
}

std::vector<std::vector<double>> Resistor::getConductanceMatrix() const {
    // Note: this local 2x2 is stamped by MNASolver even when one terminal is
    // ground (node 0) — a grounded resistor still needs its self-conductance
    // term applied at the other terminal, so this must not gate on both
    // terminals being non-ground.
    std::vector<std::vector<double>> G(2, std::vector<double>(2, 0.0));
    G[0][0] = G_;
    G[0][1] = -G_;
    G[1][0] = -G_;
    G[1][1] = G_;
    return G;
}

void Resistor::getInitialGuess(std::vector<double>& guess) const {
}

void Resistor::updateState(const std::vector<double>& state) {
}

}