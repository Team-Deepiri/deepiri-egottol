#include "mna_solver.h"
#include "../models/device.h"
#include "matrix.h"
#include <iostream>
#include <stdexcept>

namespace deepiri {

MNASolver::MNASolver() : solverMethod_("LU") {}

size_t MNASolver::buildStampMatrix(
    std::vector<std::vector<double>>& stamp,
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t numNodes
) {
    size_t auxCount = 0;
    for (auto& device : devices) {
        if (device->type() == "Vsrc" || device->type() == "Vsrc") {
            auxCount++;
        }
    }

    size_t matrixSize = numNodes + auxCount;
    stamp = std::vector<std::vector<double>>(
        matrixSize,
        std::vector<double>(matrixSize, 0.0)
    );

    size_t vsIndex = 0;
    for (auto& device : devices) {
        auto g = device->getConductanceMatrix();
        auto current = device->getCurrent();

        size_t np = device->nodeP();
        size_t nn = device->nodeN();

        if (np > 0 && np <= numNodes) {
            for (size_t i = 0; i < g.size() && (np - 1) < stamp.size(); ++i) {
                if (nn > 0 && nn <= numNodes) {
                    stamp[np - 1][np - 1] += g[i][0];
                    stamp[nn - 1][nn - 1] += g[i][1];
                    stamp[np - 1][nn - 1] -= g[i][0];
                    stamp[nn - 1][np - 1] -= g[i][1];
                }
            }
        }

        if (device->type() == "Vsrc") {
            size_t auxRow = numNodes + vsIndex;
            if (np > 0 && np <= numNodes) {
                stamp[auxRow][np - 1] = 1.0;
                stamp[np - 1][auxRow] = 1.0;
            }
            if (nn > 0 && nn <= numNodes) {
                stamp[auxRow][nn - 1] = -1.0;
                stamp[nn - 1][auxRow] = -1.0;
            }
            vsIndex++;
        }
    }

    return auxCount;
}

void MNASolver::addDeviceStamp(
    std::vector<std::vector<double>>& stamp,
    const std::vector<std::vector<double>>& deviceG,
    const std::vector<double>& deviceRHS,
    size_t nodeP,
    size_t nodeN,
    size_t auxIndex
) {
    if (nodeP == 0 || nodeN == 0) return;
    if (nodeP >= stamp.size() || nodeN >= stamp.size()) return;

    for (size_t i = 0; i < deviceG.size(); ++i) {
        for (size_t j = 0; j < deviceG[i].size(); ++j) {
            if (i == 0 && j == 0) stamp[nodeP - 1][nodeP - 1] += deviceG[i][j];
            if (i == 0 && j == 1) stamp[nodeP - 1][nodeN - 1] += deviceG[i][j];
            if (i == 1 && j == 0) stamp[nodeN - 1][nodeP - 1] += deviceG[i][j];
            if (i == 1 && j == 1) stamp[nodeN - 1][nodeN - 1] += deviceG[i][j];
        }
    }
}

MNASolver::Solution MNASolver::solve(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    const std::vector<size_t>& voltageSourceIndices
) {
    Solution result;
    result.success = false;

    if (nodeMap.empty()) {
        result.message = "No nodes defined";
        return result;
    }

    size_t numNodes = 0;
    for (auto& pair : nodeMap) {
        if (pair.second > numNodes) numNodes = pair.second;
    }

    std::vector<std::vector<double>> stamp;
    size_t auxCount = buildStampMatrix(stamp, devices, nodeMap, numNodes);

    size_t matrixSize = numNodes + auxCount;
    std::vector<double> rhs(matrixSize, 0.0);

    for (auto& device : devices) {
        auto current = device->getCurrent();
        size_t np = device->nodeP();
        size_t nn = device->nodeN();

        if (np > 0 && np <= numNodes) {
            rhs[np - 1] += current[0];
        }
        if (nn > 0 && nn <= numNodes) {
            rhs[nn - 1] += current[1];
        }
    }

    Matrix A(matrixSize, matrixSize);
    for (size_t i = 0; i < matrixSize; ++i) {
        for (size_t j = 0; j < matrixSize; ++j) {
            A.at(i, j) = stamp[i][j];
        }
    }

    try {
        result.voltages = A.solveGaussian(rhs);
        result.success = true;
        result.message = "Solution found";

        result.currents.resize(auxCount, 0.0);
        for (size_t i = 0; i < auxCount && (numNodes + i) < result.voltages.size(); ++i) {
            result.currents[i] = result.voltages[numNodes + i];
        }
    } catch (const std::exception& e) {
        result.message = std::string("Solver error: ") + e.what();
    }

    return result;
}

}