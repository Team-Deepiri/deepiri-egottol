#ifndef DEEPIRI_VCVS_H
#define DEEPIRI_VCVS_H

#include "device.h"
#include <string>

namespace deepiri {

// Exxx n+ n- nc+ nc- gain — voltage-controlled voltage source (needs MNA aux branch).
class VCVS : public Device {
public:
    VCVS(double gain = 1.0);
    VCVS(const std::string& name, double gain = 1.0);

    void setGain(double gain) { gain_ = gain; }
    double gain() const { return gain_; }

    void setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;
    std::vector<size_t> terminals() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "VCVS"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

    size_t nodeCP() const { return nodeCP_; }
    size_t nodeCN() const { return nodeCN_; }

private:
    std::string name_;
    double gain_;
    size_t nodeCP_ = 0, nodeCN_ = 0;
};

}

#endif
