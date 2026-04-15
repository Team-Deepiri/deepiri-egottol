#ifndef DEEPIRI_DIODE_H
#define DEEPIRI_DIODE_H

#include "device.h"
#include <string>
#include <cmath>

namespace deepiri {

class Diode : public Device {
public:
    Diode(double is = 1e-12, double n = 1.0);
    Diode(const std::string& name, double is = 1e-12, double n = 1.0);

    void setSaturationCurrent(double is) { Is_ = is; }
    double saturationCurrent() const { return Is_; }
    void setEmissionCoefficient(double n) { n_ = n; }
    double emissionCoefficient() const { return n_; }
    void setResistance(double rs) { Rs_ = rs; }
    void setTransitTime(double tt) { tt_ = tt; }
    void setBreakdownVoltage(double bv) { BV_ = bv; }
    void setBreakdownCurrent(double ibv) { Ibv_ = ibv; }
    void setJunctionCap(double cj0, double vj, double m) { Cj0_ = cj0; Vj_ = vj; m_ = m; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Diode"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double Is_, n_, Rs_, tt_, BV_, Ibv_;
    double Cj0_, Vj_, m_;
    double vD_, iD_, gd_;
    double qVdc_, qid_;
    double vt_;
};

}

#endif