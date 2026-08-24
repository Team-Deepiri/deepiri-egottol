#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../io/waveform_writer.h"
#include "../core/mna_solver.h"
#include "../core/spice_engine.h"
#include "../core/ac_analysis.h"
#include "ee_lookup.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace deepiri;

namespace {

void printUsage(const char* argv0) {
    std::fprintf(stderr,
        "egottol-cli — headless Deepiri Egottol simulator\n"
        "Usage:\n"
        "  %s sim <file.cir> [--op|--tran|--ac] [--trap] [--lte] [-o out.csv]\n"
        "  %s ee <query...>          EE design / symptom lookup\n"
        "  %s version\n"
        "  %s help\n"
        "\n"
        "Exit codes: 0 success, 1 usage/parse error, 2 simulation failure\n",
        argv0, argv0, argv0, argv0);
}

double controlNumber(const NetlistParser& parser, const char* kind, size_t index, double fallback) {
    for (const auto& d : parser.getControlDirectives()) {
        if (d.kind == kind && index < d.numbers.size()) return d.numbers[index];
    }
    return fallback;
}

size_t pickProbe(const BuiltCircuit& circuit) {
    for (const char* name : {"out", "mid", "n2", "n1"}) {
        auto it = circuit.nodeMap.find(name);
        if (it != circuit.nodeMap.end() && it->second > 0) return it->second;
    }
    size_t best = 0;
    for (const auto& kv : circuit.nodeMap) {
        if (kv.second > best) best = kv.second;
    }
    return best;
}

std::string nodeName(const BuiltCircuit& circuit, size_t id) {
    for (const auto& kv : circuit.nodeMap) {
        if (kv.second == id) return kv.first;
    }
    return "n" + std::to_string(id);
}

int cmdSim(int argc, char** argv) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    std::string path = argv[2];
    enum class Mode { Auto, Op, Tran, Ac };
    Mode mode = Mode::Auto;
    std::string outPath;
    bool useTrap = false;
    bool useLte = false;

    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--op" || a == "--dc") mode = Mode::Op;
        else if (a == "--tran") mode = Mode::Tran;
        else if (a == "--ac") mode = Mode::Ac;
        else if (a == "--trap") useTrap = true;
        else if (a == "--lte") useLte = true;
        else if ((a == "-o" || a == "--output") && i + 1 < argc) {
            outPath = argv[++i];
        } else if (a == "-h" || a == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n", a.c_str());
            return 1;
        }
    }

    NetlistParser parser;
    if (!parser.loadFromFile(path)) {
        std::fprintf(stderr, "Failed to read netlist: %s\n", path.c_str());
        return 1;
    }

    BuiltCircuit circuit = buildCircuitFromNetlist(parser);
    if (!circuit.ok) {
        std::fprintf(stderr, "Netlist build failed: %s\n", circuit.error.c_str());
        return 1;
    }

    // Auto-select analysis from control cards if not overridden.
    if (mode == Mode::Auto) {
        bool hasTran = false, hasAc = false, hasOp = false;
        for (const auto& d : parser.getControlDirectives()) {
            if (d.kind == "tran") hasTran = true;
            else if (d.kind == "ac") hasAc = true;
            else if (d.kind == "op" || d.kind == "dc") hasOp = true;
        }
        if (hasTran) mode = Mode::Tran;
        else if (hasAc) mode = Mode::Ac;
        else mode = Mode::Op;
        (void)hasOp;
    }

    if (mode == Mode::Op) {
        DcOperatingPoint dc;
        auto sol = dc.solve(circuit.devices, circuit.nodeMap);
        if (!sol.success) {
            // Fallback to linear MNA for purely linear netlists.
            sol = MNASolver().solve(circuit.devices, circuit.nodeMap, {});
        }
        if (!sol.success) {
            std::fprintf(stderr, "DC solve failed: %s\n", sol.message.c_str());
            return 2;
        }
        std::printf("DC operating point (%zu devices):\n", circuit.devices.size());
        for (const auto& kv : circuit.nodeMap) {
            if (kv.second == 0) continue;
            size_t idx = kv.second - 1;
            if (idx < sol.voltages.size()) {
                std::printf("  V(%s) = %.6g V\n", kv.first.c_str(), sol.voltages[idx]);
            }
        }
        if (!outPath.empty()) {
            WaveformData wd;
            wd.time_points = {0.0};
            size_t probe = pickProbe(circuit);
            wd.name = "V(" + nodeName(circuit, probe) + ")";
            double v = (probe >= 1 && probe - 1 < sol.voltages.size()) ? sol.voltages[probe - 1] : 0.0;
            wd.values = {v};
            WaveformWriter writer;
            if (!writer.writeCSV(outPath, {wd})) {
                std::fprintf(stderr, "Failed to write %s\n", outPath.c_str());
                return 1;
            }
            std::printf("Wrote %s\n", outPath.c_str());
        }
        return 0;
    }

    if (mode == Mode::Tran) {
        double tstep = controlNumber(parser, "tran", 0, 1e-5);
        double tstop = controlNumber(parser, "tran", 1, 1e-3);
        if (tstep <= 0) tstep = 1e-5;
        if (tstop <= tstep) tstop = tstep * 100;

        size_t probe = pickProbe(circuit);

        SpiceTransient::Options opts;
        opts.useTrapezoidal = useTrap;
        opts.adaptiveLte = useLte;
        SpiceTransient transient(opts);
        auto sim = transient.simulate(0.0, tstop, tstep, circuit.devices, circuit.nodeMap);
        if (!sim.converged) {
            std::fprintf(stderr, "Transient failed: %s\n", sim.message.c_str());
            return 2;
        }

        WaveformData wd;
        wd.name = "V(" + nodeName(circuit, probe) + ")";
        wd.time_points = sim.timePoints;
        wd.values.reserve(sim.nodeVoltages.size());
        for (const auto& state : sim.nodeVoltages) {
            double v = 0.0;
            if (probe >= 1 && probe - 1 < state.size()) v = state[probe - 1];
            else if (!state.empty()) v = state[0];
            wd.values.push_back(v);
        }

        std::printf("Transient (companion MNA): %zu points, probe %s\n",
                    wd.time_points.size(), wd.name.c_str());
        if (!outPath.empty()) {
            WaveformWriter writer;
            if (!writer.writeCSV(outPath, {wd})) {
                std::fprintf(stderr, "Failed to write %s\n", outPath.c_str());
                return 1;
            }
            std::printf("Wrote %s\n", outPath.c_str());
        } else if (!wd.values.empty()) {
            std::printf("  t_end=%.6g  V=%.6g\n", wd.time_points.back(), wd.values.back());
        }
        return 0;
    }

    // AC
    double fstart = 1.0, fstop = 1e6;
    int npoints = 60;
    for (const auto& d : parser.getControlDirectives()) {
        if (d.kind != "ac") continue;
        if (d.numbers.size() >= 3) {
            npoints = std::max(2, static_cast<int>(d.numbers[0]));
            fstart = d.numbers[1];
            fstop = d.numbers[2];
        } else if (d.numbers.size() == 2) {
            fstart = d.numbers[0];
            fstop = d.numbers[1];
        }
    }

    ACAnalysis ac;
    auto sweep = ac.sweep(circuit.devices, circuit.nodeMap, fstart, fstop, npoints);
    if (!sweep.success) {
        std::fprintf(stderr, "AC failed: %s\n", sweep.message.c_str());
        return 2;
    }

    size_t probe = pickProbe(circuit);
    WaveformData wd;
    wd.name = "|V(" + nodeName(circuit, probe) + ")|";
    wd.time_points = sweep.frequenciesHz;
    if (probe >= 1 && probe - 1 < sweep.magnitude.size()) {
        wd.values = sweep.magnitude[probe - 1];
    } else if (!sweep.magnitude.empty()) {
        wd.values = sweep.magnitude.back();
    }

    std::printf("AC sweep: %zu points, %g .. %g Hz, probe %s\n",
                wd.time_points.size(), fstart, fstop, wd.name.c_str());
    if (!outPath.empty()) {
        WaveformWriter writer;
        if (!writer.writeCSV(outPath, {wd})) {
            std::fprintf(stderr, "Failed to write %s\n", outPath.c_str());
            return 1;
        }
        std::printf("Wrote %s\n", outPath.c_str());
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        printUsage(argv[0]);
        return 0;
    }
    if (cmd == "version" || cmd == "--version") {
        std::printf("egottol-cli 1.0.0\n");
        return 0;
    }
    if (cmd == "sim") {
        return cmdSim(argc, argv);
    }
    if (cmd == "ee") {
        if (argc < 3) {
            std::fprintf(stderr, "Usage: %s ee <query words...>\n", argv[0]);
            return 1;
        }
        std::string q;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) q += ' ';
            q += argv[i];
        }
        std::fputs(deepiri::formatEeLookup(q).c_str(), stdout);
        return 0;
    }

    std::fprintf(stderr, "Unknown command: %s\n", cmd.c_str());
    printUsage(argv[0]);
    return 1;
}
