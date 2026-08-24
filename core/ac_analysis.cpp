#include "ac_analysis.h"

#include "mna_solver.h"
#include "../models/device.h"
#include "../models/capacitor.h"
#include "../models/inductor.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "../models/vcvs.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace deepiri {

std::vector<double> ACAnalysis::frequencySweep(double freqStartHz, double freqEndHz, int numPoints) {
    if (freqStartHz <= 0.0 || freqEndHz <= 0.0) {
        throw std::invalid_argument("AC frequency endpoints must be positive");
    }
    double f0 = std::min(freqStartHz, freqEndHz);
    double f1 = std::max(freqStartHz, freqEndHz);
    if (std::abs(f1 - f0) < 1e-9 * std::max(f0, 1.0)) {
        return {f0};
    }
    int n = std::max(numPoints, 2);
    std::vector<double> freqs(n);
    double logF0 = std::log10(f0);
    double logF1 = std::log10(f1);
    for (int i = 0; i < n; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(n - 1);
        freqs[i] = std::pow(10.0, logF0 + t * (logF1 - logF0));
    }
    return freqs;
}

void ACAnalysis::stampAdmittance(
    std::vector<std::complex<double>>& y,
    size_t width,
    size_t numNodes,
    size_t np,
    size_t nn,
    std::complex<double> admittance
) {
    bool npValid = np > 0 && np <= numNodes;
    bool nnValid = nn > 0 && nn <= numNodes;
    if (npValid) y[(np - 1) * width + (np - 1)] += admittance;
    if (nnValid) y[(nn - 1) * width + (nn - 1)] += admittance;
    if (npValid && nnValid) {
        y[(np - 1) * width + (nn - 1)] -= admittance;
        y[(nn - 1) * width + (np - 1)] -= admittance;
    }
}

// Complex analogue of Matrix::solveGaussian (matrix.cpp): same partial-pivoting,
// forward-elimination, back-substitution structure, but over std::complex<double>
// since the AC/Bode system's admittances (jwC, 1/jwL) are inherently complex.
std::vector<std::complex<double>> ACAnalysis::solveComplexGaussian(
    std::vector<std::complex<double>> a,
    std::vector<std::complex<double>> b,
    size_t n
) {
    if (a.size() != n * n || b.size() != n) {
        throw std::invalid_argument("solveComplexGaussian: dimension mismatch");
    }

    for (size_t i = 0; i < n; ++i) {
        size_t maxRow = i;
        double maxVal = std::abs(a[i * n + i]);
        for (size_t k = i + 1; k < n; ++k) {
            double v = std::abs(a[k * n + i]);
            if (v > maxVal) {
                maxRow = k;
                maxVal = v;
            }
        }

        if (maxVal < 1e-12) {
            throw std::runtime_error("AC MNA matrix is singular or nearly singular");
        }

        if (maxRow != i) {
            for (size_t j = 0; j < n; ++j) {
                std::swap(a[i * n + j], a[maxRow * n + j]);
            }
            std::swap(b[i], b[maxRow]);
        }

        for (size_t k = i + 1; k < n; ++k) {
            std::complex<double> factor = a[k * n + i] / a[i * n + i];
            b[k] -= factor * b[i];
            for (size_t j = i; j < n; ++j) {
                a[k * n + j] -= factor * a[i * n + j];
            }
        }
    }

    std::vector<std::complex<double>> x(n, std::complex<double>(0.0, 0.0));
    for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
        size_t row = static_cast<size_t>(i);
        std::complex<double> sum = b[row];
        for (size_t j = row + 1; j < n; ++j) {
            sum -= a[row * n + j] * x[j];
        }
        x[row] = sum / a[row * n + row];
    }
    return x;
}

ACResult ACAnalysis::sweep(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    double freqStartHz,
    double freqEndHz,
    int numPoints
) {
    ACResult result;

    if (nodeMap.empty()) {
        result.message = "No nodes defined";
        return result;
    }

    size_t numNodes = 0;
    for (auto& pair : nodeMap) {
        if (pair.second > numNodes) numNodes = pair.second;
    }
    if (numNodes == 0) {
        result.message = "No non-ground nodes to analyze";
        return result;
    }

    // Linearize at the DC operating point, same as ac_analysis.py's
    // solver.solve_dc() + dc_v step, but using MNASolver's per-device
    // nodeP()/nodeN() convention (ground excluded) rather than dc_op.cpp's
    // solver (whose Jacobian assembly indexes conductance rows globally and
    // is not device-node-aware in the same way).
    MNASolver dcSolver;
    auto dcSolution = dcSolver.solve(devices, nodeMap, {});
    std::vector<double> dcNodeVoltages(numNodes, 0.0);
    if (dcSolution.success) {
        for (size_t i = 0; i < numNodes && i < dcSolution.voltages.size(); ++i) {
            dcNodeVoltages[i] = dcSolution.voltages[i];
        }
    }
    for (auto& device : devices) {
        device->updateState(dcNodeVoltages);
    }

    size_t auxCount = 0;
    for (auto& device : devices) {
        if (device->type() == "Vsrc" || device->type() == "VCVS") auxCount++;
    }
    size_t total = numNodes + auxCount;

    // Prefer AC magnitude from source signal when set; else fall back to DC value.
    std::complex<double> refMagnitude(1.0, 0.0);
    for (auto& device : devices) {
        if (device->type() == "Vsrc") {
            if (auto* v = dynamic_cast<Vsrc*>(device.get())) {
                double mag = (v->signal().acMag != 0.0) ? v->signal().acMag : v->dc();
                double ph = v->signal().acPhaseDeg * M_PI / 180.0;
                refMagnitude = std::polar(mag, ph);
                break;
            }
        }
        if (device->type() == "Isrc") {
            if (auto* i = dynamic_cast<Isrc*>(device.get())) {
                double mag = (i->signal().acMag != 0.0) ? i->signal().acMag : i->dc();
                double ph = i->signal().acPhaseDeg * M_PI / 180.0;
                refMagnitude = std::polar(mag, ph);
            }
        }
    }

    std::vector<double> freqs = frequencySweep(freqStartHz, freqEndHz, numPoints);
    result.frequenciesHz = freqs;
    result.referenceMagnitude = std::abs(refMagnitude);
    result.magnitude.assign(numNodes, std::vector<double>(freqs.size(), 0.0));
    result.phaseDeg.assign(numNodes, std::vector<double>(freqs.size(), 0.0));

    for (size_t fi = 0; fi < freqs.size(); ++fi) {
        // s-domain substitution s = j*2*pi*f: capacitor admittance Y_C = sC,
        // inductor admittance Y_L = 1/(sL).
        std::complex<double> s(0.0, 2.0 * M_PI * freqs[fi]);

        std::vector<std::complex<double>> y(total * total, std::complex<double>(0.0, 0.0));
        std::vector<std::complex<double>> iVec(total, std::complex<double>(0.0, 0.0));

        size_t vsIndex = 0;
        for (auto& device : devices) {
            size_t np = device->nodeP();
            size_t nn = device->nodeN();
            bool npValid = np > 0 && np <= numNodes;
            bool nnValid = nn > 0 && nn <= numNodes;

            if (device->type() == "Capacitor") {
                if (auto* cap = dynamic_cast<Capacitor*>(device.get())) {
                    stampAdmittance(y, total, numNodes, np, nn, s * cap->capacitance());
                }
            } else if (device->type() == "Inductor") {
                if (auto* ind = dynamic_cast<Inductor*>(device.get())) {
                    double l = ind->inductance();
                    if (l > 0.0) {
                        stampAdmittance(y, total, numNodes, np, nn, std::complex<double>(1.0, 0.0) / (s * l));
                    }
                }
            } else if (device->type() == "Isrc") {
                if (auto* isrc = dynamic_cast<Isrc*>(device.get())) {
                    double mag = (isrc->signal().acMag != 0.0) ? isrc->signal().acMag : isrc->dc();
                    double ph = isrc->signal().acPhaseDeg * M_PI / 180.0;
                    std::complex<double> iAc = std::polar(mag, ph);
                    if (npValid) iVec[np - 1] -= iAc;
                    if (nnValid) iVec[nn - 1] += iAc;
                }
            } else if (device->type() == "Vsrc" || device->type() == "VCVS") {
                size_t auxRow = numNodes + vsIndex;
                if (npValid) {
                    y[auxRow * total + (np - 1)] = std::complex<double>(1.0, 0.0);
                    y[(np - 1) * total + auxRow] = std::complex<double>(1.0, 0.0);
                }
                if (nnValid) {
                    y[auxRow * total + (nn - 1)] = std::complex<double>(-1.0, 0.0);
                    y[(nn - 1) * total + auxRow] = std::complex<double>(-1.0, 0.0);
                }
                if (auto* v = dynamic_cast<Vsrc*>(device.get())) {
                    double mag = (v->signal().acMag != 0.0) ? v->signal().acMag : v->dc();
                    double ph = v->signal().acPhaseDeg * M_PI / 180.0;
                    iVec[auxRow] = std::polar(mag, ph);
                } else if (auto* e = dynamic_cast<VCVS*>(device.get())) {
                    size_t ncp = e->nodeCP();
                    size_t ncn = e->nodeCN();
                    double gain = e->gain();
                    if (ncp > 0 && ncp <= numNodes) {
                        y[auxRow * total + (ncp - 1)] -= std::complex<double>(gain, 0.0);
                    }
                    if (ncn > 0 && ncn <= numNodes) {
                        y[auxRow * total + (ncn - 1)] += std::complex<double>(gain, 0.0);
                    }
                    iVec[auxRow] = 0.0;
                }
                vsIndex++;
            } else {
                // Resistors, VCCS, diodes (linearized), etc. — multi-terminal G stamp.
                auto terms = device->terminals();
                auto g = device->getConductanceMatrix();
                for (size_t a = 0; a < terms.size() && a < g.size(); ++a) {
                    size_t na = terms[a];
                    if (na == 0 || na > numNodes) continue;
                    for (size_t b = 0; b < terms.size() && b < g[a].size(); ++b) {
                        size_t nb = terms[b];
                        if (nb == 0 || nb > numNodes) continue;
                        y[(na - 1) * total + (nb - 1)] += std::complex<double>(g[a][b], 0.0);
                    }
                }
            }
        }

        std::vector<std::complex<double>> x(total, std::complex<double>(0.0, 0.0));
        try {
            x = solveComplexGaussian(y, iVec, total);
        } catch (const std::exception&) {
            // Leave x as zeros for this frequency point, mirroring
            // ac_analysis.py's LinAlgError fallback, so one singular point
            // does not abort the whole sweep.
        }

        for (size_t nodeIdx = 0; nodeIdx < numNodes; ++nodeIdx) {
            std::complex<double> h = (refMagnitude != std::complex<double>(0.0, 0.0))
                ? x[nodeIdx] / refMagnitude
                : x[nodeIdx];
            result.magnitude[nodeIdx][fi] = std::abs(h);
            result.phaseDeg[nodeIdx][fi] = std::arg(h) * 180.0 / M_PI;
        }
    }

    result.success = true;
    result.message = "AC sweep complete";
    return result;
}

}
