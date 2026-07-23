#pragma once

#include "../../models/device.h"
#include <string>

namespace deepiri {

class OTA : public Device {
public:
    explicit OTA(double gm = 1e-3, double vLimit = 1.0);
    OTA(const std::string& name, double gm = 1e-3, double vLimit = 1.0);

    void setTransconductance(double gm) { gm_ = gm; }
    double transconductance() const { return gm_; }
    void setVLimit(double vLimit) { vLimit_ = vLimit; }
    double vLimit() const { return vLimit_; }

    void setNodes(size_t nPlus, size_t nMinus, size_t nOut);
    size_t nodeOut() const { return nodeOut_; }

    double outputCurrent(double vPlus, double vMinus) const;

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "OTA"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    double gm_;
    double vLimit_;
    size_t nodeOut_;
};

}
