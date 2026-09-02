#ifndef DEEPIRI_VSWITCH_H
#define DEEPIRI_VSWITCH_H

#include "device.h"
#include <string>

namespace deepiri {

// Sxxx n+ n- nc+ nc- — voltage-controlled switch (Ron/Roff vs Vctrl threshold)
class VSwitch : public Device {
public:
    VSwitch(const std::string& name,
            double vt = 0.0,
            double ron = 1.0,
            double roff = 1e12);

    void setThreshold(double vt) { vt_ = vt; }
    void setRon(double r) { ron_ = (r > 0) ? r : 1e-6; }
    void setRoff(double r) { roff_ = (r > 0) ? r : 1e12; }
    void setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;
    std::vector<size_t> terminals() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "VSwitch"; }

    void getInitialGuess(std::vector<double>&) const override {}
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double vt_, ron_, roff_;
    size_t nodeCP_ = 0, nodeCN_ = 0;
    double g_ = 0.0;
};

}

#endif
