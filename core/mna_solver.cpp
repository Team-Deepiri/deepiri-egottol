#include "mna_solver.h"
#include "../models/device.h"
#include "../models/vsrc.h"
#include "../models/vcvs.h"
#include "matrix.h"
#include <iostream>
#include <stdexcept>

namespace deepiri {

namespace {

bool needsAux(const Device& d) {
    return d.type() == "Vsrc" || d.type() == "VCVS";
}

}  // namespace

MNASolver::MNASolver() : solverMethod_("LU") {}

size_t MNASolver::buildStampMatrix(
    std::vector<std::vector<double>>& stamp,
    std::vector<double>& auxRHS,
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& /*nodeMap*/,
    size_t numNodes
) {
    size_t auxCount = 0;
    for (auto& device : devices) {
        if (needsAux(*device)) auxCount++;
    }

    size_t matrixSize = numNodes + auxCount;
    stamp = std::vector<std::vector<double>>(
        matrixSize,
        std::vector<double>(matrixSize, 0.0)
    );
    auxRHS.assign(auxCount, 0.0);

    size_t vsIndex = 0;
    for (auto& device : devices) {
        if (needsAux(*device)) {
            size_t np = device->nodeP();
            size_t nn = device->nodeN();
            size_t auxRow = numNodes + vsIndex;
            if (np > 0 && np <= numNodes) {
                stamp[auxRow][np - 1] = 1.0;
                stamp[np - 1][auxRow] = 1.0;
            }
            if (nn > 0 && nn <= numNodes) {
                stamp[auxRow][nn - 1] = -1.0;
                stamp[nn - 1][auxRow] = -1.0;
            }
            if (auto* vsrc = dynamic_cast<Vsrc*>(device.get())) {
                auxRHS[vsIndex] = vsrc->getVoltage(0.0);
            } else if (auto* e = dynamic_cast<VCVS*>(device.get())) {
                size_t ncp = e->nodeCP();
                size_t ncn = e->nodeCN();
                double gain = e->gain();
                if (ncp > 0 && ncp <= numNodes) stamp[auxRow][ncp - 1] -= gain;
                if (ncn > 0 && ncn <= numNodes) stamp[auxRow][ncn - 1] += gain;
                auxRHS[vsIndex] = 0.0;
            }
            vsIndex++;
            continue;
        }

        auto terms = device->terminals();
        auto g = device->getConductanceMatrix();
        auto current = device->getCurrent();
        for (size_t a = 0; a < terms.size() && a < g.size(); ++a) {
            size_t na = terms[a];
            if (na == 0 || na > numNodes) continue;
            // RHS filled in solve()
            for (size_t b = 0; b < terms.size() && b < g[a].size(); ++b) {
                size_t nb = terms[b];
                if (nb == 0 || nb > numNodes) continue;
                stamp[na - 1][nb - 1] += g[a][b];
            }
        }
        (void)current;
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
    (void)deviceRHS;
    (void)auxIndex;
}

MNASolver::Solution MNASolver::solve(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    const std::vector<size_t>& /*voltageSourceIndices*/
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
    std::vector<double> auxRHS;
    size_t auxCount = buildStampMatrix(stamp, auxRHS, devices, nodeMap, numNodes);

    size_t matrixSize = numNodes + auxCount;
    std::vector<double> rhs(matrixSize, 0.0);

    for (auto& device : devices) {
        if (needsAux(*device)) continue;
        auto terms = device->terminals();
        auto current = device->getCurrent();
        for (size_t a = 0; a < terms.size() && a < current.size(); ++a) {
            size_t na = terms[a];
            if (na > 0 && na <= numNodes) {
                rhs[na - 1] += current[a];
            }
        }
    }

    for (size_t i = 0; i < auxCount; ++i) {
        rhs[numNodes + i] = auxRHS[i];
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
