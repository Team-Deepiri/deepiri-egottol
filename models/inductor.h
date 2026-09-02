#ifndef DEEPIRI_INDUCTOR_H
#define DEEPIRI_INDUCTOR_H

#include "device.h"
#include <string>
#include <map>

namespace deepiri {

class Inductor : public Device {
public:
    Inductor(double inductance = 1e-3, double i = 0.0);
    Inductor(const std::string& name, double inductance, double initialCurrent = 0.0);

    void setInductance(double l) { L_ = l; L0_ = l; }
    double inductance() const { return L_; }
    void setInitialCurrent(double i) { iInitial_ = i; i_ = i; }
    double initialCurrent() const { return iInitial_; }
    void setCoupling(size_t otherInductor, double k);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Inductor"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;
    void setTrapezoidal(bool trap) override { useTrap_ = trap; }
    void prepareTransientStep(double h, const std::vector<double>& prevNodeVoltages) override;
    void acceptTransientStep(const std::vector<double>& nodeVoltages) override;

    double flux() const { return flux_; }
    double current() const { return i_; }

private:
    std::string name_;
    double L_, L0_;
    double i_, iInitial_;
    double flux_;
    std::map<size_t, double> couplingMap_;
    bool transientActive_ = false;
    bool useTrap_ = false;
    double geq_ = 0.0;
    double ieq_ = 0.0;
    double vPrev_ = 0.0;
};

}

#endif
