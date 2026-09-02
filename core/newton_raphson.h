#ifndef DEEPIRI_NEWTON_RAPHSON_H
#define DEEPIRI_NEWTON_RAPHSON_H

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <cmath>

namespace deepiri {

class NewtonRaphson {
public:
    NewtonRaphson(
        double tolerance = 1e-6,
        int maxIterations = 100,
        double damping = 1.0
    );

    struct Result {
        std::vector<double> solution;
        int iterations;
        double residual;
        bool converged;
        std::string errorMessage;
    };

    Result solve(
        const std::vector<double>& initialGuess,
        std::function<std::vector<double>(const std::vector<double>&)> f,
        std::function<std::vector<std::vector<double>>(const std::vector<double>&)> J
    );

    void setTolerance(double tol) { tolerance_ = tol; }
    void setMaxIterations(int maxIter) { maxIterations_ = maxIter; }
    void setDamping(double damping) { damping_ = damping; }
    void setVerbose(bool verbose) { verbose_ = verbose; }

    // Convergence aids (SPICE-style). When the plain Newton solve fails,
    // callers can retry with gmin stepping (diagonal conductance) and/or
    // source stepping (scale independent sources from 0 → 1).
    void setGmin(double gmin) { gmin_ = gmin; }
    double gmin() const { return gmin_; }
    void setSourceStepFactors(int factors) { sourceStepFactors_ = factors; }

    Result solveWithStepping(
        const std::vector<double>& initialGuess,
        std::function<std::vector<double>(const std::vector<double>&, double sourceScale, double gmin)> f,
        std::function<std::vector<std::vector<double>>(const std::vector<double>&, double sourceScale, double gmin)> J
    );

private:
    double tolerance_;
    int maxIterations_;
    double damping_;
    bool verbose_;
    double gmin_ = 1e-12;
    int sourceStepFactors_ = 5;

    std::vector<double> computeCorrection(
        const std::vector<double>& x,
        const std::vector<double>& fval,
        const std::vector<std::vector<double>>& jacobian
    );
};

}

#endif