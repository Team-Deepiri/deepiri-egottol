#include "newton_raphson.h"
#include "matrix.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <algorithm>

namespace deepiri {

NewtonRaphson::NewtonRaphson(double tolerance, int maxIterations, double damping)
    : tolerance_(tolerance), maxIterations_(maxIterations), damping_(damping),
      verbose_(false), gmin_(1e-12), sourceStepFactors_(5) {}

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

NewtonRaphson::Result NewtonRaphson::solveWithStepping(
    const std::vector<double>& initialGuess,
    std::function<std::vector<double>(const std::vector<double>&, double, double)> f,
    std::function<std::vector<std::vector<double>>(const std::vector<double>&, double, double)> J
) {
    // Source stepping: ramp independent sources 0 → 1, then gmin continuation
    // from large shunt conductance down to the configured gmin_.
    std::vector<double> x = initialGuess;
    Result last;
    last.converged = false;

    const int factors = std::max(1, sourceStepFactors_);
    for (int s = 1; s <= factors; ++s) {
        double scale = static_cast<double>(s) / static_cast<double>(factors);
        auto fScaled = [&](const std::vector<double>& v) { return f(v, scale, gmin_); };
        auto jScaled = [&](const std::vector<double>& v) { return J(v, scale, gmin_); };
        last = solve(x, fScaled, jScaled);
        if (last.converged) {
            x = last.solution;
        } else {
            // Try with a larger gmin for this source factor, then continue.
            double boost = 1e-6;
            auto fBoost = [&](const std::vector<double>& v) { return f(v, scale, boost); };
            auto jBoost = [&](const std::vector<double>& v) { return J(v, scale, boost); };
            last = solve(x, fBoost, jBoost);
            if (last.converged) x = last.solution;
            else return last;
        }
    }

    // Gmin stepping: 1e-3 → configured gmin_
    const double gminStart = 1e-3;
    const int gminSteps = 4;
    for (int g = 0; g <= gminSteps; ++g) {
        double gminNow = (g == gminSteps)
            ? gmin_
            : gminStart * std::pow(gmin_ / gminStart, static_cast<double>(g) / gminSteps);
        auto fG = [&](const std::vector<double>& v) { return f(v, 1.0, gminNow); };
        auto jG = [&](const std::vector<double>& v) { return J(v, 1.0, gminNow); };
        last = solve(x, fG, jG);
        if (last.converged) x = last.solution;
        else return last;
    }
    return last;
}

}