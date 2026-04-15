#include "transient.h"
#include "../models/device.h"
#include <cmath>
#include <iostream>

namespace deepiri {

Transient::Transient()
    : integrator_(createIntegrator(IntegratorType::Trapezoidal))
    , maxStep_(1e-3)
    , tolerance_(1e-6)
    , verbose_(false) {}

void Transient::setIntegratorType(IntegratorType type) {
    integrator_ = createIntegrator(type);
}

void Transient::initializeState(
    std::vector<double>& state,
    const std::vector<double>& ic,
    size_t numUnknowns
) {
    if (!ic.empty()) {
        state = ic;
        if (state.size() < numUnknowns) {
            state.resize(numUnknowns, 0.0);
        }
    } else {
        state.resize(numUnknowns, 0.0);
    }
}

std::vector<double> Transient::computeDerivatives(
    const std::vector<double>& state,
    const std::vector<std::shared_ptr<Device>>& devices,
    double t
) {
    std::vector<double> deriv(state.size(), 0.0);

    for (auto& device : devices) {
        device->updateState(state);
    }

    std::vector<double> rhs(state.size(), 0.0);
    for (auto& device : devices) {
        auto current = device->getCurrent();
        if (rhs.size() < current.size()) {
            rhs.resize(current.size(), 0.0);
        }
        for (size_t i = 0; i < current.size(); ++i) {
            rhs[i] += current[i];
        }
    }

    std::vector<std::vector<double>> G(state.size(), std::vector<double>(state.size(), 0.0));
    for (auto& device : devices) {
        auto g = device->getConductanceMatrix();
        for (size_t i = 0; i < g.size(); ++i) {
            for (size_t j = 0; j < g[i].size(); ++j) {
                if (i < G.size() && j < G.size()) {
                    G[i][j] += g[i][j];
                }
            }
        }
    }

    for (size_t i = 0; i < deriv.size(); ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < state.size(); ++j) {
            if (i < G.size() && j < G[i].size()) {
                sum += G[i][j] * state[j];
            }
        }
        if (std::abs(sum) > 1e-12) {
            deriv[i] = rhs[i] - sum;
        }
    }

    return deriv;
}

bool Transient::checkConvergence(
    const std::vector<double>& state,
    const std::vector<double>& prevState
) {
    double maxDiff = 0.0;
    for (size_t i = 0; i < state.size(); ++i) {
        double diff = std::abs(state[i] - prevState[i]);
        if (diff > maxDiff) maxDiff = diff;
    }
    return maxDiff < tolerance_;
}

TransientResult Transient::simulate(
    double tStart,
    double tEnd,
    double stepSize,
    const std::vector<std::shared_ptr<Device>>& devices,
    size_t numUnknowns,
    const std::vector<double>& ic
) {
    TransientResult result;
    result.converged = true;
    result.message = "Simulation completed";

    std::vector<double> state;
    initializeState(state, ic, numUnknowns);

    double t = tStart;
    double h = stepSize;

    result.timePoints.push_back(t);
    result.nodeVoltages.push_back(state);

    std::vector<double> prevState = state;
    std::vector<double> deriv(state.size(), 0.0);

    while (t < tEnd) {
        if (t + h > tEnd) {
            h = tEnd - t;
        }

        deriv = computeDerivatives(state, devices, t);

        std::vector<double> dyCurrent = deriv;
        std::vector<double> yNext = integrator_->step(state, dyCurrent, dyCurrent, t, h);

        if (!checkConvergence(yNext, state)) {
            h *= 0.5;
            if (h < 1e-12) {
                result.converged = false;
                result.message = "Convergence failed";
                break;
            }
            continue;
        }

        state = yNext;
        t += h;

        result.timePoints.push_back(t);
        result.nodeVoltages.push_back(state);

        prevState = yNext;

        if (verbose_ && static_cast<int>(t / stepSize) % 100 == 0) {
            std::cout << "t = " << t << ", step = " << h << std::endl;
        }
    }

    return result;
}

}