#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../io/waveform_writer.h"
#include "../core/mna_solver.h"
#include "../core/spice_engine.h"
#include "../core/ac_analysis.h"
#include "../core/spice_extra.h"
#include "../models/vsrc.h"
#include "../models/isrc.h"
#include "ee_lookup.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
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
        "  %s sim <file.cir> [--op|--tran|--ac|--dcsweep|--tf|--noise] [--trap] [--lte] [-o out.csv]\n"
        "  %s ee <query...>          EE design / symptom lookup\n"
        "  %s version\n"
        "  %s help\n"
        "\n"
        "Analyses: .op .tran .ac .dc .tf .noise .measure; Sources: DC PULSE SIN EXP PWL.\n"
        "Devices: R C L V I D M Q E G F H S K X(.subckt).  .param expressions supported.\n"
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
    enum class Mode { Auto, Op, Tran, Ac, DcSweep, Tf, Noise };
    Mode mode = Mode::Auto;
    std::string outPath;
    bool useTrap = false;
    bool useLte = false;

    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--op") mode = Mode::Op;
        else if (a == "--dc" || a == "--dcsweep") mode = Mode::DcSweep;
        else if (a == "--tran") mode = Mode::Tran;
        else if (a == "--ac") mode = Mode::Ac;
        else if (a == "--tf") mode = Mode::Tf;
        else if (a == "--noise") mode = Mode::Noise;
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
        bool hasTran = false, hasAc = false, hasDcSweep = false, hasTf = false, hasNoise = false;
        for (const auto& d : parser.getControlDirectives()) {
            if (d.kind == "tran") hasTran = true;
            else if (d.kind == "ac") hasAc = true;
            else if (d.kind == "dc" && d.numbers.size() >= 3) hasDcSweep = true;
            else if (d.kind == "tf") hasTf = true;
            else if (d.kind == "noise") hasNoise = true;
        }
        if (hasTran) mode = Mode::Tran;
        else if (hasNoise) mode = Mode::Noise;
        else if (hasAc) mode = Mode::Ac;
        else if (hasTf) mode = Mode::Tf;
        else if (hasDcSweep) mode = Mode::DcSweep;
        else mode = Mode::Op;
    }

    auto parseOutNode = [&](const std::string& tok) -> size_t {
        std::string net = tok;
        if (net.size() >= 4 && (net[0] == 'V' || net[0] == 'v') && net[1] == '(' && net.back() == ')') {
            net = net.substr(2, net.size() - 3);
        }
        auto it = circuit.nodeMap.find(net);
        return (it == circuit.nodeMap.end()) ? 0 : it->second;
    };

    if (mode == Mode::Tf) {
        std::string outTok = "out";
        std::string srcName = "V1";
        for (const auto& d : parser.getControlDirectives()) {
            if (d.kind != "tf") continue;
            if (d.tokens.size() >= 1) outTok = d.tokens[0];
            if (d.tokens.size() >= 2) srcName = d.tokens[1];
            break;
        }
        size_t outNode = parseOutNode(outTok);
        if (outNode == 0) outNode = pickProbe(circuit);
        auto tf = computeTransferFunction(circuit.devices, circuit.nodeMap, outNode, srcName);
        if (!tf.success) {
            std::fprintf(stderr, "TF failed: %s\n", tf.message.c_str());
            return 2;
        }
        std::printf("Transfer function V(%s) / %s:\n", nodeName(circuit, outNode).c_str(),
                    srcName.c_str());
        std::printf("  transfer = %.6g\n", tf.gain);
        std::printf("  input_impedance = %.6g ohm\n", tf.inputZ);
        std::printf("  output_impedance = %.6g ohm\n", tf.outputZ);
        return 0;
    }

    if (mode == Mode::Noise) {
        std::string outTok = "out";
        double fstart = 1.0, fstop = 1e6;
        int npoints = 20;
        for (const auto& d : parser.getControlDirectives()) {
            if (d.kind != "noise") continue;
            if (!d.tokens.empty()) outTok = d.tokens[0];
            // .noise V(out) V1 dec np fstart fstop — numbers often np fstart fstop
            if (d.numbers.size() >= 3) {
                npoints = std::max(2, static_cast<int>(d.numbers[0]));
                fstart = d.numbers[1];
                fstop = d.numbers[2];
            } else if (d.numbers.size() == 2) {
                fstart = d.numbers[0];
                fstop = d.numbers[1];
            }
            break;
        }
        size_t outNode = parseOutNode(outTok);
        if (outNode == 0) outNode = pickProbe(circuit);
        auto nr = computeOutputNoise(circuit.devices, circuit.nodeMap, outNode, fstart, fstop, npoints);
        if (!nr.success) {
            std::fprintf(stderr, "Noise failed: %s\n", nr.message.c_str());
            return 2;
        }
        std::printf("Output noise V(%s): %zu points, %.6g .. %.6g Hz\n",
                    nodeName(circuit, outNode).c_str(), nr.frequenciesHz.size(), fstart, fstop);
        if (!nr.outputNoiseDensity.empty()) {
            std::printf("  density@fstart = %.6g V/sqrtHz\n", nr.outputNoiseDensity.front());
            std::printf("  density@fstop  = %.6g V/sqrtHz\n", nr.outputNoiseDensity.back());
        }
        std::printf("  total_rms ≈ %.6g V\n", nr.totalRms);
        if (!outPath.empty()) {
            WaveformData wd;
            wd.name = "onoise_V(" + nodeName(circuit, outNode) + ")";
            wd.time_points = nr.frequenciesHz;
            wd.values = nr.outputNoiseDensity;
            WaveformWriter writer;
            if (!writer.writeCSV(outPath, {wd})) {
                std::fprintf(stderr, "Failed to write %s\n", outPath.c_str());
                return 1;
            }
            std::printf("Wrote %s\n", outPath.c_str());
        }
        return 0;
    }

    if (mode == Mode::DcSweep) {
        // .dc srcName start stop step
        std::string srcName;
        double start = 0, stop = 1, step = 0.1;
        for (const auto& d : parser.getControlDirectives()) {
            if (d.kind != "dc") continue;
            if (!d.tokens.empty()) srcName = d.tokens[0];
            if (d.numbers.size() >= 3) {
                start = d.numbers[0];
                stop = d.numbers[1];
                step = d.numbers[2];
            } else if (d.numbers.size() >= 3) {
                // already handled
            }
            // tokens may be: V1 0 5 0.5 → first token name, numbers from parse
            if (srcName.empty() && !d.tokens.empty()) srcName = d.tokens[0];
            break;
        }
        if (srcName.empty()) {
            std::fprintf(stderr, "DC sweep requires .dc <src> <start> <stop> <step>\n");
            return 1;
        }
        if (step == 0.0) step = (stop >= start) ? 0.1 : -0.1;
        auto src = findSourceByName(circuit, srcName);
        if (!src) {
            std::fprintf(stderr, "DC sweep source not found: %s\n", srcName.c_str());
            return 1;
        }

        size_t probe = pickProbe(circuit);
        WaveformData wd;
        wd.name = "V(" + nodeName(circuit, probe) + ")";
        std::printf("DC sweep %s from %g to %g step %g, probe %s\n",
                    srcName.c_str(), start, stop, step, wd.name.c_str());

        auto setSrc = [&](double val) {
            if (auto* v = dynamic_cast<Vsrc*>(src.get())) v->setDC(val);
            else if (auto* i = dynamic_cast<Isrc*>(src.get())) i->setDC(val);
        };

        const bool up = step > 0;
        for (double x = start; up ? (x <= stop + 1e-15 * std::abs(step))
                                  : (x >= stop - 1e-15 * std::abs(step));
             x += step) {
            setSrc(x);
            auto sol = DcOperatingPoint().solve(circuit.devices, circuit.nodeMap);
            if (!sol.success) sol = MNASolver().solve(circuit.devices, circuit.nodeMap, {});
            if (!sol.success) {
                std::fprintf(stderr, "DC sweep failed at %s=%g: %s\n",
                             srcName.c_str(), x, sol.message.c_str());
                return 2;
            }
            double v = 0.0;
            if (probe >= 1 && probe - 1 < sol.voltages.size()) v = sol.voltages[probe - 1];
            wd.time_points.push_back(x);
            wd.values.push_back(v);
            std::printf("  %s=%g  %s=%g\n", srcName.c_str(), x, wd.name.c_str(), v);
            if ((up && x + step > stop && x < stop) || (!up && x + step < stop && x > stop)) {
                // ensure final point
            }
        }
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

        auto icNamed = parseNodeVoltagesFromControls(parser.getControlDirectives(), "ic");
        if (icNamed.empty()) {
            icNamed = parseNodeVoltagesFromControls(parser.getControlDirectives(), "nodeset");
        }
        std::vector<double> ic;
        if (!icNamed.empty()) {
            ic = initialConditionVector(circuit, icNamed);
        }
        auto sim = transient.simulate(0.0, tstop, tstep, circuit.devices, circuit.nodeMap, ic);
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
        auto measures = evaluateMeasures(
            parser.getControlDirectives(), circuit, sim.timePoints, sim.nodeVoltages);
        for (const auto& m : measures) {
            if (m.ok) {
                std::printf("  .measure %s = %.6g\n", m.name.c_str(), m.value);
            } else {
                std::printf("  .measure %s failed: %s\n", m.name.c_str(), m.message.c_str());
            }
        }
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
