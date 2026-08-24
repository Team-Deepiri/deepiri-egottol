#ifndef DEEPIRI_DEVICE_H
#define DEEPIRI_DEVICE_H

#include <vector>
#include <string>
#include <memory>

namespace deepiri {

// Terminal-indexed stamps: G[i][j] couples terminals()[i] → terminals()[j];
// I[i] is the current leaving terminal i into the device (MNA RHS gets -I for KCL).
class Device {
public:
    virtual ~Device() = default;

    virtual void initializeDC() {}
    virtual void getInitialGuess(std::vector<double>& guess) const {}
    virtual void updateState(const std::vector<double>& nodeVoltages) {}

    virtual std::vector<double> getCurrent() const = 0;
    virtual std::vector<std::vector<double>> getConductanceMatrix() const = 0;

    virtual std::string name() const = 0;
    virtual std::string type() const = 0;

    virtual void setTemperature(double temp) { temperature_ = temp; }
    virtual double temperature() const { return temperature_; }

    // Multi-terminal support. Default is 2-terminal (P, N).
    virtual std::vector<size_t> terminals() const {
        return {nodeP_, nodeN_};
    }
    virtual void setTerminals(const std::vector<size_t>& nodes) {
        if (nodes.size() >= 1) nodeP_ = nodes[0];
        if (nodes.size() >= 2) nodeN_ = nodes[1];
        terminals_ = nodes;
    }

    size_t nodeP() const { return nodeP_; }
    size_t nodeN() const { return nodeN_; }
    void setNodes(size_t p, size_t n) {
        nodeP_ = p;
        nodeN_ = n;
        terminals_ = {p, n};
    }

    // Transient companion (backward Euler / trapezoidal). Called each time step
    // BEFORE getConductanceMatrix/getCurrent so C and L stamp as G+Ieq.
    virtual void prepareTransientStep(double h, const std::vector<double>& prevNodeVoltages) {
        (void)h;
        (void)prevNodeVoltages;
    }
    virtual void acceptTransientStep(const std::vector<double>& nodeVoltages) {
        (void)nodeVoltages;
    }

    // Nonlinear residual contribution magnitude (for NR convergence).
    virtual double nonlinearResidual() const { return 0.0; }

protected:
    size_t nodeP_ = 0;
    size_t nodeN_ = 0;
    std::vector<size_t> terminals_;
    double temperature_ = 300.0;
};

}

#endif
