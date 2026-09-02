#ifndef DEEPIRI_RESISTOR_H
#define DEEPIRI_RESISTOR_H

#include "device.h"
#include <string>
#include <cmath>

namespace deepiri {

class Resistor : public Device {
public:
    Resistor(double resistance = 1000.0, double temp = 300.0);
    Resistor(const std::string& name, double resistance, double temp = 300.0);

    void setResistance(double r) { R_ = r; }
    double resistance() const { return R_; }
    void setKF(double kf) { kf_ = kf; }
    double kf() const { return kf_; }

    void setTemperature(double temp) override;
    void setTC1(double tc) { tc1_ = tc; }
    void setTC2(double tc) { tc2_ = tc; }
    void setTnom(double tnom) { Tnom_ = tnom; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Resistor"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double R_;
    double R0_;
    double temperature_;
    double Tnom_;
    double tc1_, tc2_;
    double kf_ = 0.0;
    double G_;
};

}

#endif