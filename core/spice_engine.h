#pragma once

#include "mna_solver.h"
#include "newton_raphson.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class Device;

// Production-style DC operating point: Newton–Raphson on the MNA residual with
// optional gmin stepping and source stepping for hard nonlinear circuits.
class DcOperatingPoint {
public:
    struct Options {
        double tolerance = 1e-6;
        int maxNewton = 50;
        double gminStart = 1e-3;
        double gminEnd = 1e-12;
        int gminSteps = 5;
        int sourceSteps = 5;
        bool verbose = false;
    };

    DcOperatingPoint() = default;
    explicit DcOperatingPoint(Options opts);

    MNASolver::Solution solve(
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap
    );

private:
    Options opts_;
};

// Transient analysis via companion-model MNA (backward Euler). Capacitors and
// inductors become G+Ieq each step; nonlinear devices are re-linearized with
// an inner Newton loop — the same structure production SPICE uses.
class SpiceTransient {
public:
    struct Options {
        double tolerance = 1e-6;
        int maxNewtonPerStep = 20;
        double gmin = 1e-12;
        bool useTrapezoidal = false;  // false = backward Euler (more stable)
        bool verbose = false;
    };

    struct Result {
        std::vector<double> timePoints;
        std::vector<std::vector<double>> nodeVoltages;  // [time][nodeIndex 0=node1]
        bool converged = false;
        std::string message;
    };

    SpiceTransient() = default;
    explicit SpiceTransient(Options opts);

    Result simulate(
        double tStart,
        double tEnd,
        double stepSize,
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap,
        const std::vector<double>& ic = {}
    );

private:
    Options opts_;

    MNASolver::Solution solveLinearized(
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap,
        size_t numNodes,
        double gmin,
        double timeSec
    ) const;
};

}
