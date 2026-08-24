#ifndef DEEPIRI_CAPACITOR_H
#define DEEPIRI_CAPACITOR_H

#include "device.h"
#include <string>

namespace deepiri {

class Capacitor : public Device {
public:
    Capacitor(double capacitance = 1e-6, double v = 0.0);
    Capacitor(const std::string& name, double capacitance, double initialVoltage = 0.0);

    void setCapacitance(double c) { C_ = c; C0_ = c; }
    double capacitance() const { return C_; }
    void setInitialVoltage(double v) { vInitial_ = v; v_ = v; }
    double initialVoltage() const { return vInitial_; }
    void setVC1(double vc) { vc1_ = vc; }
    void setVC2(double vc) { vc2_ = vc; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Capacitor"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;
    void prepareTransientStep(double h, const std::vector<double>& prevNodeVoltages) override;
    void acceptTransientStep(const std::vector<double>& nodeVoltages) override;

    double charge() const { return q_; }
    double voltage() const { return v_; }

private:
    std::string name_;
    double C_, C0_;
    double v_, vInitial_;
    double q_;
    double vc1_, vc2_;
    // Companion model (backward Euler): Geq = C/h, Ieq = Geq * v_prev
    bool transientActive_ = false;
    double geq_ = 0.0;
    double ieq_ = 0.0;  // current into + terminal from companion source
};

}

#endif
