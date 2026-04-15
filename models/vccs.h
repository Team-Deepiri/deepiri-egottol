#ifndef DEEPIRI_VCCS_H
#define DEEPIRI_VCCS_H

#include "device.h"
#include <string>

namespace deepiri {

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

    std::string name() const override { return name_; }
    std::string type() const override { return "VCCS"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double gm_;
    size_t nodeCP_, nodeCN_;
};

}

#endif