#ifndef DEEPIRI_ISWITCH_H
#define DEEPIRI_ISWITCH_H

#include "device.h"
#include <string>

namespace deepiri {

// Wxxx n+ n- Vsense model — current-controlled switch (Ron/Roff vs |I(Vsense)| threshold)
class ISwitch : public Device {
public:
    ISwitch(const std::string& name,
            double it = 0.0,
            double ron = 1.0,
            double roff = 1e12);

    void setThresholdCurrent(double it) { it_ = it; }
    void setRon(double r) { ron_ = (r > 0) ? r : 1e-6; }
    void setRoff(double r) { roff_ = (r > 0) ? r : 1e12; }
    void setSenseName(const std::string& n) { senseName_ = n; }
    const std::string& senseName() const { return senseName_; }

    void setNodes(size_t nP, size_t nN);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;
    std::vector<size_t> terminals() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "ISwitch"; }

    void getInitialGuess(std::vector<double>&) const override {}
    void updateState(const std::vector<double>&) override {}
    void updateFromSenseCurrent(double senseCurrent);

private:
    std::string name_;
    std::string senseName_;
    double it_, ron_, roff_;
    double g_ = 0.0;
};

}

#endif
