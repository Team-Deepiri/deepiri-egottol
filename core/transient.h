#ifndef DEEPIRI_TRANSIENT_H
#define DEEPIRI_TRANSIENT_H

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "integrator.h"

namespace deepiri {

class Device;

struct TransientResult {
    std::vector<double> timePoints;
    std::vector<std::vector<double>> nodeVoltages;
    bool converged;
    std::string message;
};

class Transient {
public:
    Transient();

    TransientResult simulate(
        double tStart,
        double tEnd,
        double stepSize,
        const std::vector<std::shared_ptr<Device>>& devices,
        size_t numUnknowns,
        const std::vector<double>& ic = {}
    );

    void setIntegratorType(IntegratorType type);
    void setMaxStep(double maxStep) { maxStep_ = maxStep; }
    void setTolerance(double tol) { tolerance_ = tol; }
    void setVerbose(bool verbose) { verbose_ = verbose; }

private:
    std::unique_ptr<Integrator> integrator_;
    double maxStep_;
    double tolerance_;
    bool verbose_;

    std::vector<double> computeDerivatives(
        const std::vector<double>& state,
        const std::vector<std::shared_ptr<Device>>& devices,
        double t
    );

    void initializeState(
        std::vector<double>& state,
        const std::vector<double>& ic,
        size_t numUnknowns
    );
};

}

#endif