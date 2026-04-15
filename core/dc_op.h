#ifndef DEEPIRI_DCOP_H
#define DEEPIRI_DCOP_H

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <memory>

namespace deepiri {

struct DCOpResult {
    std::vector<double> nodeVoltages;
    std::vector<double> branchCurrents;
    bool converged;
    int iterations;
    double error;
    std::string message;
};

class Device;

class DCOperatingPoint {
public:
    DCOperatingPoint(double tolerance = 1e-6, int maxIterations = 100);

    DCOpResult solve(
        size_t numUnknowns,
        const std::vector<std::shared_ptr<Device>>& devices
    );

    void setTolerance(double tol) { tolerance_ = tol; }
    void setMaxIterations(int maxIter) { maxIterations_ = maxIter; }
    void setVerbose(bool verbose) { verbose_ = verbose; }

private:
    double tolerance_;
    int maxIterations_;
    bool verbose_;

    std::vector<double> buildRHS(const std::vector<std::shared_ptr<Device>>& devices);
    std::vector<std::vector<double>> buildJacobian(
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::vector<double>& state
    );
    bool checkConvergence(const std::vector<double>& state, const std::vector<double>& prevState);
};

}

#endif