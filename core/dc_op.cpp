#include "dc_op.h"
#include "../models/device.h"
#include "newton_raphson.h"
#include <iostream>
#include <cmath>

namespace deepiri {

DCOperatingPoint::DCOperatingPoint(double tolerance, int maxIterations)
    : tolerance_(tolerance), maxIterations_(maxIterations), verbose_(false) {}

std::vector<double> DCOperatingPoint::buildRHS(const std::vector<std::shared_ptr<Device>>& devices) {
    std::vector<double> rhs;
    for (auto& device : devices) {
        auto current = device->getCurrent();
        if (rhs.size() < current.size()) {
            rhs.resize(current.size(), 0.0);
        }
        for (size_t i = 0; i < current.size(); ++i) {
            rhs[i] += current[i];
        }
    }
    return rhs;
}

std::vector<std::vector<double>> DCOperatingPoint::buildJacobian(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::vector<double>& state
) {
    size_t numUnknowns = state.size();
    std::vector<std::vector<double>> jacobian(
        numUnknowns,
        std::vector<double>(numUnknowns, 0.0)
    );

    for (auto& device : devices) {
        auto partials = device->getConductanceMatrix();
        for (size_t i = 0; i < partials.size(); ++i) {
            for (size_t j = 0; j < partials[i].size(); ++j) {
                if (i < numUnknowns && j < numUnknowns) {
                    jacobian[i][j] += partials[i][j];
                }
            }
        }
    }
    return jacobian;
}

DCOpResult DCOperatingPoint::solve(
    size_t numUnknowns,
    const std::vector<std::shared_ptr<Device>>& devices
) {
    DCOpResult result;
    result.converged = false;
    result.iterations = 0;
    result.error = 0.0;
    result.nodeVoltages = std::vector<double>(numUnknowns, 0.0);

    for (auto& device : devices) {
        device->initializeDC();
    }

    NewtonRaphson nrSolver(tolerance_, maxIterations_, 1.0);

    auto f = [&](const std::vector<double>& x) -> std::vector<double> {
        for (auto& device : devices) {
            device->updateState(x);
        }
        return buildRHS(devices);
    };

    auto J = [&](const std::vector<double>& x) -> std::vector<std::vector<double>> {
        return buildJacobian(devices, x);
    };

    std::vector<double> initialGuess(numUnknowns, 0.0);
    for (auto& device : devices) {
        device->getInitialGuess(initialGuess);
    }

    auto nrResult = nrSolver.solve(initialGuess, f, J);

    result.converged = nrResult.converged;
    result.iterations = nrResult.iterations;
    result.error = nrResult.residual;
    result.nodeVoltages = nrResult.solution;
    result.message = nrResult.converged ? "Converged" : nrResult.errorMessage;

    return result;
}

}