#include "spice_engine.h"

#include "matrix.h"
#include "../models/device.h"
#include "../models/vsrc.h"
#include "../models/capacitor.h"
#include "../models/inductor.h"

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

double nodeVoltage(const std::vector<double>& v, size_t node) {
    if (node == 0) return 0.0;
    if (node - 1 < v.size()) return v[node - 1];
    return 0.0;
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
        // KCL: sum currents leaving node = 0. Device reports current into device,
        // so contribution to node residual is -I. RHS for Gv = rhs uses +injected.
        // Our convention matches existing MNA: rhs += current[terminal].
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

void stampVoltageSources(
    std::vector<std::vector<double>>& G,
    std::vector<double>& rhs,
    const std::vector<std::shared_ptr<Device>>& devices,
    size_t numNodes,
    double sourceScale,
    double timeSec = 0.0
) {
    size_t vsIndex = 0;
    for (const auto& device : devices) {
        if (device->type() != "Vsrc") continue;
        size_t aux = numNodes + vsIndex;
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
        }
        ++vsIndex;
    }
}

size_t countVsrc(const std::vector<std::shared_ptr<Device>>& devices) {
    size_t n = 0;
    for (const auto& d : devices) {
        if (d->type() == "Vsrc") ++n;
    }
    return n;
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

    for (int s = 1; s <= srcSteps; ++s) {
        double scale = static_cast<double>(s) / static_cast<double>(srcSteps);
        for (int g = 0; g <= gminSteps; ++g) {
            double gmin = (g == gminSteps)
                ? opts_.gminEnd
                : opts_.gminStart * std::pow(
                      opts_.gminEnd / opts_.gminStart,
                      static_cast<double>(g) / static_cast<double>(gminSteps));

            bool stepOk = false;
            for (int iter = 0; iter < opts_.maxNewton; ++iter) {
                for (auto& d : devices) {
                    d->updateState(x);
                }

                size_t nV = countVsrc(devices);
                size_t N = numNodes + nV;
                std::vector<std::vector<double>> G(N, std::vector<double>(N, 0.0));
                std::vector<double> rhs(N, 0.0);

                for (size_t i = 0; i < numNodes; ++i) {
                    G[i][i] += gmin;  // Gmin to ground
                }

                for (const auto& d : devices) {
                    if (d->type() == "Vsrc") continue;
                    // Open capacitors/inductors at DC: skip C, short L via large G.
                    if (d->type() == "Capacitor") continue;
                    if (d->type() == "Inductor") {
                        // DC short: large conductance between terminals.
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
                    stampDevice(G, rhs, *d, numNodes);
                }
                stampVoltageSources(G, rhs, devices, numNodes, scale);

                Matrix A(N, N);
                for (size_t i = 0; i < N; ++i)
                    for (size_t j = 0; j < N; ++j)
                        A.at(i, j) = G[i][j];

                std::vector<double> sol;
                try {
                    sol = A.solveGaussian(rhs);
                } catch (const std::exception& e) {
                    best.message = std::string("DC matrix singular: ") + e.what();
                    break;
                }

                double maxDx = 0.0;
                for (size_t i = 0; i < numNodes && i < sol.size(); ++i) {
                    maxDx = std::max(maxDx, std::abs(sol[i] - x[i]));
                    x[i] = sol[i];
                }

                if (opts_.verbose) {
                    std::cout << "DC NR iter " << iter << " dx=" << maxDx
                              << " gmin=" << gmin << " scale=" << scale << "\n";
                }

                if (maxDx < opts_.tolerance) {
                    stepOk = true;
                    best.success = true;
                    best.voltages.assign(sol.begin(), sol.begin() + static_cast<std::ptrdiff_t>(numNodes));
                    best.currents.clear();
                    for (size_t i = numNodes; i < sol.size(); ++i) {
                        best.currents.push_back(sol[i]);
                    }
                    best.message = "DC operating point converged";
                    break;
                }
            }
            if (!stepOk) {
                // Try next gmin / abort source step
                if (g == gminSteps) {
                    return best;
                }
            }
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
    size_t nV = countVsrc(devices);
    size_t N = numNodes + nV;
    std::vector<std::vector<double>> G(N, std::vector<double>(N, 0.0));
    std::vector<double> rhs(N, 0.0);

    for (size_t i = 0; i < numNodes; ++i) G[i][i] += gmin;

    for (const auto& d : devices) {
        if (d->type() == "Vsrc") continue;
        stampDevice(G, rhs, *d, numNodes);
    }
    stampVoltageSources(G, rhs, devices, numNodes, 1.0, timeSec);

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
        d->initializeDC();
        d->updateState(v);
        d->acceptTransientStep(v);
    }

    double t = tStart;
    double h = stepSize;
    result.timePoints.push_back(t);
    result.nodeVoltages.push_back(v);
    result.converged = true;
    result.message = "Transient completed";

    while (t < tEnd - 1e-18) {
        if (t + h > tEnd) h = tEnd - t;
        std::vector<double> vPrev = v;

        for (auto& d : devices) {
            d->prepareTransientStep(h, vPrev);
        }

        bool stepOk = false;
        for (int iter = 0; iter < opts_.maxNewtonPerStep; ++iter) {
            for (auto& d : devices) {
                d->updateState(v);
                // Re-apply companion using previous accepted voltages (not iterating v).
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
            // Cut timestep and retry once.
            h *= 0.5;
            v = vPrev;
            if (h < 1e-15) {
                result.converged = false;
                result.message = "Transient failed to converge";
                return result;
            }
            continue;
        }

        for (auto& d : devices) {
            d->acceptTransientStep(v);
        }

        // Time-varying sources: update Vsrc pulse at new time.
        for (auto& d : devices) {
            if (auto* vs = dynamic_cast<Vsrc*>(d.get())) {
                (void)vs;
            }
        }

        t += h;
        result.timePoints.push_back(t);
        result.nodeVoltages.push_back(v);

        // Mild adaptive growth after success.
        if (h < stepSize) h = std::min(stepSize, h * 1.5);
    }

    return result;
}

}  // namespace
