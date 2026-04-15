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

private:
    double tolerance_;
    int maxIterations_;
    double damping_;
    bool verbose_;

    std::vector<double> computeCorrection(
        const std::vector<double>& x,
        const std::vector<double>& fval,
        const std::vector<std::vector<double>>& jacobian
    );
};

}

#endif