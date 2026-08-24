#include "spice_extra.h"

#include "spice_engine.h"
#include "mna_solver.h"
#include "ac_analysis.h"
#include "matrix.h"
#include "../models/device.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "../models/resistor.h"

#include <algorithm>
#include <cmath>
#include <complex>

namespace deepiri {

namespace {

size_t maxNode(const std::map<std::string, size_t>& nodeMap) {
    size_t n = 0;
    for (const auto& kv : nodeMap) if (kv.second > n) n = kv.second;
    return n;
}

std::string lower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::shared_ptr<Device> findNamed(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::string& name
) {
    std::string w = lower(name);
    for (const auto& d : devices) {
        if (lower(d->name()) == w) return d;
    }
    return nullptr;
}

}  // namespace

TransferFunctionResult computeTransferFunction(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t outputNode,
    const std::string& inputSourceName
) {
    TransferFunctionResult r;
    size_t numNodes = maxNode(nodeMap);
    if (numNodes == 0 || outputNode > numNodes) {
        r.message = "Invalid .tf nodes";
        return r;
    }
    auto src = findNamed(devices, inputSourceName);
    if (!src || (src->type() != "Vsrc" && src->type() != "Isrc")) {
        r.message = " .tf input source not found: " + inputSourceName;
        return r;
    }

    // Linearize at DC OP.
    auto dc = DcOperatingPoint().solve(devices, nodeMap);
    std::vector<double> state(numNodes, 0.0);
    if (dc.success) {
        for (size_t i = 0; i < numNodes && i < dc.voltages.size(); ++i) state[i] = dc.voltages[i];
    }
    for (auto& d : devices) d->updateState(state);

    // Snapshot & zero all independent sources, then drive input with unity.
    struct Snap {
        std::shared_ptr<Device> d;
        double v = 0;
        bool isV = false;
    };
    std::vector<Snap> snaps;
    for (auto& d : devices) {
        if (auto* v = dynamic_cast<Vsrc*>(d.get())) {
            snaps.push_back({d, v->dc(), true});
            v->setDC(0.0);
        } else if (auto* i = dynamic_cast<Isrc*>(d.get())) {
            snaps.push_back({d, i->dc(), false});
            i->setDC(0.0);
        }
    }

    auto restore = [&]() {
        for (auto& s : snaps) {
            if (s.isV) dynamic_cast<Vsrc*>(s.d.get())->setDC(s.v);
            else dynamic_cast<Isrc*>(s.d.get())->setDC(s.v);
        }
    };

    // --- Gain: unity input ---
    if (auto* v = dynamic_cast<Vsrc*>(src.get())) v->setDC(1.0);
    else if (auto* i = dynamic_cast<Isrc*>(src.get())) i->setDC(1.0);

    auto solGain = MNASolver().solve(devices, nodeMap, {});
    if (!solGain.success) {
        restore();
        r.message = "TF gain solve failed: " + solGain.message;
        return r;
    }
    double vout = (outputNode >= 1 && outputNode - 1 < solGain.voltages.size())
                      ? solGain.voltages[outputNode - 1]
                      : 0.0;
    r.gain = vout;  // / 1.0

    // Zin: for Vsrc, Zin = V / I_branch (V=1 → 1/I); for Isrc, Zin = V_at_nodes / 1A
    if (src->type() == "Vsrc") {
        // Find aux index of this Vsrc among aux devices
        size_t auxIdx = 0;
        bool found = false;
        for (const auto& d : devices) {
            if (d->type() != "Vsrc" && d->type() != "VCVS" && d->type() != "CCVS") continue;
            if (d.get() == src.get()) {
                found = true;
                break;
            }
            ++auxIdx;
        }
        double iBranch = 0.0;
        if (found && auxIdx < solGain.currents.size()) {
            iBranch = solGain.currents[auxIdx];
        }
        r.inputZ = (std::abs(iBranch) > 1e-18) ? (1.0 / iBranch) : 1e18;
        r.inputZ = std::abs(r.inputZ);
    } else {
        size_t np = src->nodeP();
        size_t nn = src->nodeN();
        double vp = (np >= 1 && np - 1 < solGain.voltages.size()) ? solGain.voltages[np - 1] : 0.0;
        double vn = (nn >= 1 && nn - 1 < solGain.voltages.size()) ? solGain.voltages[nn - 1] : 0.0;
        r.inputZ = std::abs(vp - vn);
    }

    // Zout: short independent sources (already 0), inject 1A into output node.
    if (auto* v = dynamic_cast<Vsrc*>(src.get())) v->setDC(0.0);
    else if (auto* i = dynamic_cast<Isrc*>(src.get())) i->setDC(0.0);

    // Temporary 1A into output → ground
    auto iProbe = std::make_shared<Isrc>("#zout", 1.0);
    iProbe->setNodes(outputNode, 0);
    auto devicesZ = devices;
    devicesZ.push_back(iProbe);
    auto solZ = MNASolver().solve(devicesZ, nodeMap, {});
    if (solZ.success && outputNode >= 1 && outputNode - 1 < solZ.voltages.size()) {
        r.outputZ = std::abs(solZ.voltages[outputNode - 1]);
    } else {
        r.outputZ = 0.0;
    }

    restore();
    r.success = true;
    r.message = "TF complete";
    return r;
}

NoiseResult computeOutputNoise(
    const std::vector<std::shared_ptr<Device>>& devices,
    const std::map<std::string, size_t>& nodeMap,
    size_t outputNode,
    double freqStartHz,
    double freqEndHz,
    int numPoints,
    double temperatureK
) {
    NoiseResult r;
    size_t numNodes = maxNode(nodeMap);
    if (numNodes == 0 || outputNode == 0 || outputNode > numNodes) {
        r.message = "Invalid .noise output node";
        return r;
    }

    // Linearize
    auto dc = DcOperatingPoint().solve(devices, nodeMap);
    std::vector<double> state(numNodes, 0.0);
    if (dc.success) {
        for (size_t i = 0; i < numNodes && i < dc.voltages.size(); ++i) state[i] = dc.voltages[i];
    }
    for (auto& d : devices) d->updateState(state);

    const double kB = 1.380649e-23;
    const double fourKT = 4.0 * kB * temperatureK;

    // Zero independent AC drives for transfer probes
    struct Snap { std::shared_ptr<Device> d; double v; bool isV; };
    std::vector<Snap> snaps;
    for (auto& d : devices) {
        if (auto* v = dynamic_cast<Vsrc*>(d.get())) {
            snaps.push_back({d, v->dc(), true});
            v->setDC(0.0);
            v->setAC(0.0);
        } else if (auto* i = dynamic_cast<Isrc*>(d.get())) {
            snaps.push_back({d, i->dc(), false});
            i->setDC(0.0);
            i->setAC(0.0);
        }
    }
    auto restore = [&]() {
        for (auto& s : snaps) {
            if (s.isV) dynamic_cast<Vsrc*>(s.d.get())->setDC(s.v);
            else dynamic_cast<Isrc*>(s.d.get())->setDC(s.v);
        }
    };

    ACAnalysis ac;
    // Use AC sweep machinery: for each resistor we need per-freq transfer.
    // Collect resistors first.
    struct RInfo { size_t np, nn; double R; };
    std::vector<RInfo> resistors;
    for (const auto& d : devices) {
        if (auto* res = dynamic_cast<Resistor*>(d.get())) {
            if (res->resistance() > 0) {
                resistors.push_back({res->nodeP(), res->nodeN(), res->resistance()});
            }
        }
    }

    int npts = std::max(2, numPoints);
    r.frequenciesHz.resize(static_cast<size_t>(npts));
    r.outputNoiseDensity.assign(static_cast<size_t>(npts), 0.0);
    double log0 = std::log10(std::max(freqStartHz, 1e-30));
    double log1 = std::log10(std::max(freqEndHz, 1e-30));

    // For each frequency, for each R: inject 1A, get Vout, S += |V|^2 * 4kT/R
    // Reuse ACAnalysis by temporarily adding Isrc — expensive but clear.
    double totalVar = 0.0;
    for (int fi = 0; fi < npts; ++fi) {
        double t = static_cast<double>(fi) / static_cast<double>(npts - 1);
        double f = std::pow(10.0, log0 + t * (log1 - log0));
        r.frequenciesHz[static_cast<size_t>(fi)] = f;

        double sOut = 0.0;  // V²/Hz
        for (const auto& ri : resistors) {
            auto probe = std::make_shared<Isrc>("#noise", 1.0);
            probe->setAC(1.0, 0.0);
            probe->setDC(0.0);
            probe->setNodes(ri.np, ri.nn);
            auto devs = devices;
            devs.push_back(probe);
            // Single-point AC via sweep of 1 point
            auto sweep = ac.sweep(devs, nodeMap, f, f, 2);
            if (!sweep.success || sweep.magnitude.empty()) continue;
            size_t idx = outputNode - 1;
            if (idx >= sweep.magnitude.size() || sweep.magnitude[idx].empty()) continue;
            double vMag = sweep.magnitude[idx][0];  // |Vout| for 1A → |Z|
            // Reference magnitude may divide — ACAnalysis divides by ref.
            // With all sources 0 and our Isrc dc=0 ac=1, ref might be from Isrc.
            sOut += (vMag * vMag) * (fourKT / ri.R);
        }
        r.outputNoiseDensity[static_cast<size_t>(fi)] = std::sqrt(std::max(sOut, 0.0));

        // Trapezoidal integrate variance over log/linear band (approx linear in f)
        if (fi > 0) {
            double f0 = r.frequenciesHz[static_cast<size_t>(fi - 1)];
            double n0 = r.outputNoiseDensity[static_cast<size_t>(fi - 1)];
            double n1 = r.outputNoiseDensity[static_cast<size_t>(fi)];
            // ∫ n(f)^2 df
            totalVar += 0.5 * (n0 * n0 + n1 * n1) * (f - f0);
        }
    }

    r.totalRms = std::sqrt(std::max(totalVar, 0.0));
    restore();
    r.success = true;
    r.message = "Noise complete";
    return r;
}

}
