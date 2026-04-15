#ifndef DEEPIRI_OPAMP_H
#define DEEPIRI_OPAMP_H

#include "device.h"
#include <string>

namespace deepiri {

class OpAmp : public Device {
public:
    OpAmp(double gain = 1e5);
    OpAmp(const std::string& name, double gain = 1e5);

    void setGain(double gain) { gain_ = gain; }
    double gain() const { return gain_; }
    void setInputResistance(double rin) { rin_ = rin; }
    void setOutputResistance(double rout) { rout_ = rout; }
    void setSlewRate(double sr) { slewRate_ = sr; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "OpAmp"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double gain_, rin_, rout_, slewRate_;
    double vOut_, vIn_;
};

}

#endif