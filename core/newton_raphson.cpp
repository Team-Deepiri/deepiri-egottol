#include "newton_raphson.h"
#include "matrix.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace deepiri {

NewtonRaphson::NewtonRaphson(double tolerance, int maxIterations, double damping)
    : tolerance_(tolerance), maxIterations_(maxIterations), damping_(damping), verbose_(false) {}

std::vector<double> NewtonRaphson::computeCorrection(
    const std::vector<double>& x,
    const std::vector<double>& fval,
    const std::vector<std::vector<double>>& jacobian
) {
    size_t n = x.size();
    Matrix J(n, n);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
            J.at(i, j) = jacobian[i][j];
        }
    }

    std::vector<double> rhs(n);
    for (size_t i = 0; i < n; ++i) {
        rhs[i] = -fval[i];
    }

    return J.solveGaussian(rhs);
}

NewtonRaphson::Result NewtonRaphson::solve(
    const std::vector<double>& initialGuess,
    std::function<std::vector<double>(const std::vector<double>&)> f,
    std::function<std::vector<std::vector<double>>(const std::vector<double>&)> J_func
) {
    Result result;
    result.iterations = 0;
    result.converged = false;
    result.residual = 0.0;

    std::vector<double> x = initialGuess;
    std::vector<double> x_new;

    for (int iter = 0; iter < maxIterations_; ++iter) {
        std::vector<double> fval = f(x);
        std::vector<std::vector<double>> jacobian = J_func(x);

        double maxResidual = 0.0;
        for (size_t i = 0; i < fval.size(); ++i) {
            double absVal = std::abs(fval[i]);
            if (absVal > maxResidual) maxResidual = absVal;
        }

        if (verbose_) {
            std::cout << "Iteration " << iter << ", residual = " << maxResidual << std::endl;
        }

        if (maxResidual < tolerance_) {
            result.solution = x;
            result.iterations = iter;
            result.residual = maxResidual;
            result.converged = true;
            return result;
        }

        std::vector<double> delta;
        try {
            delta = computeCorrection(x, fval, jacobian);
        } catch (const std::exception& e) {
            result.solution = x;
            result.residual = maxResidual;
            result.converged = false;
            result.errorMessage = std::string("Jacobian solve failed: ") + e.what();
            return result;
        }

        double stepSize = damping_;
        x_new.resize(x.size());
        for (size_t i = 0; i < x.size(); ++i) {
            x_new[i] = x[i] + stepSize * delta[i];
        }

        x = x_new;
        result.iterations = iter + 1;
    }

    std::vector<double> fval = f(x);
    double maxResidual = 0.0;
    for (size_t i = 0; i < fval.size(); ++i) {
        double absVal = std::abs(fval[i]);
        if (absVal > maxResidual) maxResidual = absVal;
    }

    result.solution = x;
    result.residual = maxResidual;
    result.converged = false;
    result.errorMessage = "Max iterations reached";
    return result;
}

}