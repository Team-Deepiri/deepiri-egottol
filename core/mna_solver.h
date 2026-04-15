#ifndef DEEPIRI_MNA_SOLVER_H
#define DEEPIRI_MNA_SOLVER_H

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace deepiri {

class Device;

class MNASolver {
public:
    MNASolver();

    struct Solution {
        std::vector<double> voltages;
        std::vector<double> currents;
        bool success;
        std::string message;
    };

    Solution solve(
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap,
        const std::vector<size_t>& voltageSourceIndices
    );

    void setMatrixSolver(const std::string& method) { solverMethod_ = method; }

private:
    std::string solverMethod_;

    size_t buildStampMatrix(
        std::vector<std::vector<double>>& stamp,
        const std::vector<std::shared_ptr<Device>>& devices,
        const std::map<std::string, size_t>& nodeMap,
        size_t numNodes
    );

    void addDeviceStamp(
        std::vector<std::vector<double>>& stamp,
        const std::vector<std::vector<double>>& deviceG,
        const std::vector<double>& deviceRHS,
        size_t nodeP,
        size_t nodeN,
        size_t auxIndex
    );
};

}

#endif