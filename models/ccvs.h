#ifndef DEEPIRI_CCVS_H
#define DEEPIRI_CCVS_H

#include "device.h"
#include <string>

namespace deepiri {

// Hxxx n+ n- Vsense gain — V(n+,n-) = gain · I(Vsense)  (needs own aux branch)
class CCVS : public Device {
public:
    CCVS(const std::string& name, double gain = 1.0);

    void setGain(double g) { gain_ = g; }
    double gain() const { return gain_; }
    void setSenseName(const std::string& n) { senseName_ = n; }
    const std::string& senseName() const { return senseName_; }

    void initializeDC() override {}
    std::vector<double> getCurrent() const override { return {0.0, 0.0}; }
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "CCVS"; }

    void getInitialGuess(std::vector<double>&) const override {}
    void updateState(const std::vector<double>&) override {}

private:
    std::string name_;
    std::string senseName_;
    double gain_;
};

}

#endif
