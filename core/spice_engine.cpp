#include "spice_engine.h"

#include "matrix.h"
#include "../models/device.h"
#include "../models/vsrc.h"
#include "../models/vcvs.h"
#include "../models/capacitor.h"
#include "../models/inductor.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace deepiri {

namespace {

size_t maxNodeIndex(const std::map<std::string, size_t>& nodeMap) {
    size_t n = 0;
    for (const auto& kv : nodeMap) {
        if (kv.second > n) n = kv.second;
    }
    return n;
}

bool needsAuxBranch(const Device& d) {
    return d.type() == "Vsrc" || d.type() == "VCVS";
}

size_t countAuxBranches(const std::vector<std::shared_ptr<Device>>& devices) {
    size_t n = 0;
    for (const auto& d : devices) {
        if (needsAuxBranch(*d)) ++n;
    }
    return n;
}

// Stamp a device's G and I onto the reduced MNA system (nodes 1..N, no ground row).
void stampDevice(
    std::vector<std::vector<double>>& G,
    std::vector<double>& rhs,
    const Device& device,
    size_t numNodes
) {
    auto terminals = device.terminals();
    auto g = device.getConductanceMatrix();
    auto i = device.getCurrent();

    const size_t nt = terminals.size();
    for (size_t a = 0; a < nt && a < g.size(); ++a) {
        size_t na = terminals[a];
        if (na == 0 || na > numNodes) continue;
        if (a < i.size()) {
            rhs[na - 1] += i[a];
        }
        for (size_t b = 0; b < nt && b < g[a].size(); ++b) {
            size_t nb = terminals[b];
            if (nb == 0 || nb > numNodes) continue;
            G[na - 1][nb - 1] += g[a][b];
        }
    }
}

void stampAuxBranches(
    std::vector<std::vector<double>>& G,
    std::vector<double>& rhs,
    const std::vector<std::shared_ptr<Device>>& devices,
    size_t numNodes,
    double sourceScale,
    double timeSec = 0.0
) {
    size_t auxIndex = 0;
    for (const auto& device : devices) {
        if (!needsAuxBranch(*device)) continue;
        size_t aux = numNodes + auxIndex;
        size_t np = device->nodeP();
        size_t nn = device->nodeN();
        if (np > 0 && np <= numNodes) {
            G[aux][np - 1] = 1.0;
            G[np - 1][aux] = 1.0;
        }
        if (nn > 0 && nn <= numNodes) {
            G[aux][nn - 1] = -1.0;
            G[nn - 1][aux] = -1.0;
        }

        if (auto* v = dynamic_cast<Vsrc*>(device.get())) {
            rhs[aux] = sourceScale * v->getVoltage(timeSec);
        } else if (auto* e = dynamic_cast<VCVS*>(device.get())) {
            // Vp − Vn − gain·(Vcp − Vcn) = 0
            size_t ncp = e->nodeCP();
            size_t ncn = e->nodeCN();
            double gain = e->gain();
            if (ncp > 0 && ncp <= numNodes) G[aux][ncp - 1] -= gain;
            if (ncn > 0 && ncn <= numNodes) G[aux][ncn - 1] += gain;
            rhs[aux] = 0.0;
        }
        ++auxIndex;
    }
}

}  // namespace

DcOperatingPoint::DcOperatingPoint(Options opts) : opts_(std::move(opts)) {}

MNASolver::Solution DcOperatingPoint::solve(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap
) {
    MNASolver::Solution best;
    best.success = false;
    best.message = "DC OP failed";

    size_t numNodes = maxNodeIndex(nodeMap);
    if (numNodes == 0) {
        best.message = "No nodes";
        return best;
    }

    std::vector<double> x(numNodes, 0.0);
    for (const auto& d : devices) {
        d->initializeDC();
        d->getInitialGuess(x);
    }

    const int srcSteps = std::max(1, opts_.sourceSteps);
    const int gminSteps = std::max(1, opts_.gminSteps);

    auto runNewton = [&](double scale, double gmin, std::vector<double>& state) -> bool {
        for (int iter = 0; iter < opts_.maxNewton; ++iter) {
            for (auto& d : devices) {
                d->updateState(state);
            }

            size_t nV = countAuxBranches(devices);
            size_t N = numNodes + nV;
            std::vector<std::vector<double>> G(N, std::vector<double>(N, 0.0));
            std::vector<double> rhs(N, 0.0);

            for (size_t i = 0; i < numNodes; ++i) {
                G[i][i] += gmin;
            }

            for (const auto& d : devices) {
                if (needsAuxBranch(*d)) continue;
                if (d->type() == "Capacitor") continue;
                if (d->type() == "Inductor") {
                    auto terms = d->terminals();
                    if (terms.size() >= 2) {
                        size_t a = terms[0], b = terms[1];
                        const double gshort = 1e6;
                        if (a > 0 && a <= numNodes) G[a - 1][a - 1] += gshort;
                        if (b > 0 && b <= numNodes) G[b - 1][b - 1] += gshort;
                        if (a > 0 && b > 0 && a <= numNodes && b <= numNodes) {
                            G[a - 1][b - 1] -= gshort;
                            G[b - 1][a - 1] -= gshort;
                        }
                    }
                    continue;
                }
                d->setAnalysisTime(0.0);
                stampDevice(G, rhs, *d, numNodes);
            }
            stampAuxBranches(G, rhs, devices, numNodes, scale);

            Matrix A(N, N);
            for (size_t i = 0; i < N; ++i)
                for (size_t j = 0; j < N; ++j)
                    A.at(i, j) = G[i][j];

            std::vector<double> sol;
            try {
                sol = A.solveGaussian(rhs);
            } catch (const std::exception& e) {
                best.message = std::string("DC matrix singular: ") + e.what();
                return false;
            }

            double maxDx = 0.0;
            for (size_t i = 0; i < numNodes && i < sol.size(); ++i) {
                double dx = sol[i] - state[i];
                // Damped Newton — kills period-2 oscillation on hard nonlinearities.
                constexpr double damp = 0.7;
                constexpr double vLim = 0.8;
                if (dx > vLim) dx = vLim;
                if (dx < -vLim) dx = -vLim;
                double next = state[i] + damp * dx;
                maxDx = std::max(maxDx, std::abs(next - state[i]));
                state[i] = next;
            }

            if (opts_.verbose) {
                std::cout << "DC NR iter " << iter << " dx=" << maxDx
                          << " gmin=" << gmin << " scale=" << scale << "\n";
            }

            if (maxDx < opts_.tolerance) {
                best.currents.clear();
                for (size_t i = numNodes; i < sol.size(); ++i) {
                    best.currents.push_back(sol[i]);
                }
                return true;
            }
        }
        return false;
    };

    for (int s = 1; s <= srcSteps; ++s) {
        double scale = static_cast<double>(s) / static_cast<double>(srcSteps);
        bool scaleOk = false;
        std::vector<double> lastGood = x;

        for (int g = 0; g <= gminSteps; ++g) {
            double gmin = (g == gminSteps)
                ? opts_.gminEnd
                : opts_.gminStart * std::pow(
                      opts_.gminEnd / opts_.gminStart,
                      static_cast<double>(g) / static_cast<double>(gminSteps));

            std::vector<double> trial = x;
            if (runNewton(scale, gmin, trial)) {
                x = trial;
                lastGood = trial;
                scaleOk = true;
                // Continue to tighter gmin; keep lastGood if tighter fails.
            } else if (scaleOk) {
                // Revert to last converged gmin for this scale and stop tightening.
                x = lastGood;
                break;
            } else {
                break;
            }
        }

        // Only accept a solution from the full source scale as the OP result.
        if (s == srcSteps) {
            if (scaleOk) {
                best.success = true;
                best.voltages = x;
                best.message = "DC operating point converged";
            } else {
                best.success = false;
                if (best.message == "DC OP failed" || best.message.empty()) {
                    best.message = "DC OP failed at full source scale";
                }
            }
            return best;
        }

        if (!scaleOk) {
            // Mid-scale failure: continue ramping with whatever state we have.
            best.message = "DC OP struggling at source scale " + std::to_string(scale);
        }
    }

    return best;
}

SpiceTransient::SpiceTransient(Options opts) : opts_(std::move(opts)) {}

MNASolver::Solution SpiceTransient::solveLinearized(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t numNodes,
    double gmin,
    double timeSec
) const {
    size_t nV = countAuxBranches(devices);
    size_t N = numNodes + nV;
    std::vector<std::vector<double>> G(N, std::vector<double>(N, 0.0));
    std::vector<double> rhs(N, 0.0);

    for (size_t i = 0; i < numNodes; ++i) G[i][i] += gmin;

    for (const auto& d : devices) {
        if (needsAuxBranch(*d)) continue;
        d->setAnalysisTime(timeSec);
        stampDevice(G, rhs, *d, numNodes);
    }
    stampAuxBranches(G, rhs, devices, numNodes, 1.0, timeSec);

    MNASolver::Solution out;
    out.success = false;
    Matrix A(N, N);
    for (size_t i = 0; i < N; ++i)
        for (size_t j = 0; j < N; ++j)
            A.at(i, j) = G[i][j];
    try {
        auto sol = A.solveGaussian(rhs);
        out.success = true;
        out.voltages.assign(sol.begin(), sol.begin() + static_cast<std::ptrdiff_t>(numNodes));
        for (size_t i = numNodes; i < sol.size(); ++i) out.currents.push_back(sol[i]);
        out.message = "ok";
    } catch (const std::exception& e) {
        out.message = e.what();
    }
    return out;
}

SpiceTransient::Result SpiceTransient::simulate(
    double tStart,
    double tEnd,
    double stepSize,
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    const std::vector<double>& ic
) {
    Result result;
    size_t numNodes = maxNodeIndex(nodeMap);
    if (numNodes == 0 || stepSize <= 0.0 || tEnd <= tStart) {
        result.message = "Invalid transient setup";
        return result;
    }

    // DC operating point as IC unless provided.
    std::vector<double> v(numNodes, 0.0);
    if (!ic.empty()) {
        v = ic;
        if (v.size() < numNodes) v.resize(numNodes, 0.0);
    } else {
        DcOperatingPoint::Options dopts;
        dopts.tolerance = opts_.tolerance;
        dopts.gminEnd = opts_.gmin;
        auto dc = DcOperatingPoint(dopts).solve(devices, nodeMap);
        if (dc.success && dc.voltages.size() >= numNodes) {
            v.assign(dc.voltages.begin(), dc.voltages.begin() + static_cast<std::ptrdiff_t>(numNodes));
        } else {
            for (auto& d : devices) d->getInitialGuess(v);
        }
    }

    for (auto& d : devices) {
        d->setTrapezoidal(opts_.useTrapezoidal);
        d->initializeDC();
        d->updateState(v);
        d->acceptTransientStep(v);
    }

    double t = tStart;
    double h = stepSize;
    const double hMax = stepSize * std::max(1.0, opts_.hMaxFactor);
    result.timePoints.push_back(t);
    result.nodeVoltages.push_back(v);
    result.converged = true;
    result.message = "Transient completed";

    std::vector<double> vOlder = v;

    while (t < tEnd - 1e-18) {
        if (t + h > tEnd) h = tEnd - t;
        std::vector<double> vPrev = v;

        bool stepOk = false;
        for (int iter = 0; iter < opts_.maxNewtonPerStep; ++iter) {
            for (auto& d : devices) {
                d->updateState(v);
                d->prepareTransientStep(h, vPrev);
            }

            auto sol = solveLinearized(devices, nodeMap, numNodes, opts_.gmin, t + h);
            if (!sol.success) {
                result.converged = false;
                result.message = "Transient step matrix failed: " + sol.message;
                return result;
            }

            double maxDx = 0.0;
            for (size_t i = 0; i < numNodes; ++i) {
                maxDx = std::max(maxDx, std::abs(sol.voltages[i] - v[i]));
                v[i] = sol.voltages[i];
            }
            if (maxDx < opts_.tolerance) {
                stepOk = true;
                break;
            }
        }

        if (!stepOk) {
            h *= 0.5;
            v = vPrev;
            if (h < opts_.hMin) {
                result.converged = false;
                result.message = "Transient failed to converge";
                return result;
            }
            continue;
        }

        // LTE estimate via linear extrapolation residual (predictor–corrector style).
        if (opts_.adaptiveLte && result.timePoints.size() >= 2) {
            double lte = 0.0;
            for (size_t i = 0; i < numNodes; ++i) {
                double pred = vPrev[i] + (vPrev[i] - vOlder[i]);
                double scale = std::max({1.0, std::abs(v[i]), std::abs(pred)});
                lte = std::max(lte, std::abs(v[i] - pred) / scale);
            }
            if (lte > opts_.lteRelTol) {
                h *= 0.5;
                v = vPrev;
                if (h < opts_.hMin) {
                    result.converged = false;
                    result.message = "Transient LTE control failed";
                    return result;
                }
                continue;
            }
            if (lte < opts_.lteRelTol * 0.1 && h < hMax) {
                h = std::min(hMax, h * 1.5);
            }
        }

        for (auto& d : devices) {
            d->acceptTransientStep(v);
        }

        vOlder = vPrev;
        t += h;
        result.timePoints.push_back(t);
        result.nodeVoltages.push_back(v);

        if (!opts_.adaptiveLte && h < stepSize) {
            h = std::min(stepSize, h * 1.5);
        }
    }

    return result;
}

}  // namespace
