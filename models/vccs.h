#ifndef DEEPIRI_VCCS_H
#define DEEPIRI_VCCS_H

#include "device.h"
#include <string>

namespace deepiri {

// Gxxx n+ n- nc+ nc- gm  — voltage-controlled current source: I(n+,n-) = gm·(Vnc+ − Vnc−)
class VCCS : public Device {
public:
    VCCS(double transconductance = 1.0);
    VCCS(const std::string& name, double transconductance = 1.0);

    void setTransconductance(double gm) { gm_ = gm; }
    double transconductance() const { return gm_; }

    void setNodes(size_t nP, size_t nN, size_t nCP, size_t nCN);

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;
    std::vector<size_t> terminals() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "VCCS"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

    size_t nodeCP() const { return nodeCP_; }
    size_t nodeCN() const { return nodeCN_; }

private:
    std::string name_;
    double gm_;
    size_t nodeCP_ = 0, nodeCN_ = 0;
};

}

#endif
