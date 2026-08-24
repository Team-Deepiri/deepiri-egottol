// Golden SPICE corpus: analytical DC/tran checks + optional ngspice OP compare.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/spice_engine.h"
#include "../core/mna_solver.h"
#include "../models/vsrc.h"

#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool c, const char* w) {
    if (!c) { std::fprintf(stderr, "FAILED: %s\n", w); ++failures; }
}

void expectNear(double got, double want, double tol, const char* w) {
    if (!(std::isfinite(got)) || std::fabs(got - want) > tol) {
        std::fprintf(stderr, "FAILED: %s (got %g want %g ±%g)\n", w, got, want, tol);
        ++failures;
    }
}

bool loadCir(NetlistParser& p, const std::string& name) {
    const char* roots[] = {
        "tests/fixtures/goldens/",
        "../tests/fixtures/goldens/",
        "../../tests/fixtures/goldens/",
    };
    for (const char* r : roots) {
        if (p.loadFromFile(std::string(r) + name) && !p.getElements().empty()) return true;
    }
    return false;
}

double nodeV(const MNASolver::Solution& sol, const BuiltCircuit& c, const char* net) {
    auto it = c.nodeMap.find(net);
    if (it == c.nodeMap.end() || it->second == 0) return 0.0;
    size_t i = it->second - 1;
    if (i >= sol.voltages.size()) return NAN;
    return sol.voltages[i];
}

MNASolver::Solution dcSolve(BuiltCircuit& c) {
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    if (!sol.success) sol = MNASolver().solve(c.devices, c.nodeMap, {});
    return sol;
}

void checkOp(const char* file, const char* node, double want, double tol) {
    NetlistParser p;
    expect(loadCir(p, file), file);
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, (std::string(file) + " build").c_str());
    auto sol = dcSolve(c);
    expect(sol.success, (std::string(file) + " dc").c_str());
    if (sol.success) expectNear(nodeV(sol, c, node), want, tol, file);
}

void test_model_card_parse_apply() {
    const char* nl =
        ".model D1N4148 D (IS=2.52n N=1.752)\n"
        "V1 in 0 5\n"
        "R1 in a 1k\n"
        "D1 a 0 D1N4148\n"
        ".op\n";
    NetlistParser p;
    expect(p.parse(nl), "model parse");
    auto models = p.getModels();
    expect(models.count("d1n4148") == 1, "model stored");
    if (models.count("d1n4148")) {
        expectNear(models["d1n4148"].params["is"], 2.52e-9, 1e-12, "IS=2.52n");
        expectNear(models["d1n4148"].params["n"], 1.752, 1e-6, "N");
    }
    auto c = buildCircuitFromNetlist(p);
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    expect(sol.success, "model diode dc");
    if (sol.success && c.nodeMap.count("a")) {
        double va = sol.voltages[c.nodeMap["a"] - 1];
        expect(va > 0.4 && va < 1.2, "model diode Vf");
        std::printf("  .model diode Va=%.4f V\n", va);
    }
}

void test_trap_rc() {
    const char* nl =
        "V1 in 0 PULSE(0 1 0 1n 1n 1 2)\n"
        "R1 in out 1k\n"
        "C1 out 0 1u\n"
        ".tran 20u 5m\n";
    NetlistParser p;
    p.parse(nl);
    auto c = buildCircuitFromNetlist(p);
    SpiceTransient::Options opts;
    opts.useTrapezoidal = true;
    opts.adaptiveLte = false;
    opts.tolerance = 1e-5;
    auto sim = SpiceTransient(opts).simulate(0.0, 5e-3, 20e-6, c.devices, c.nodeMap);
    expect(sim.converged, "trap rc converged");
    size_t out = c.nodeMap["out"];
    if (out >= 1 && !sim.nodeVoltages.empty()) {
        expectNear(sim.nodeVoltages.back()[out - 1], 0.993, 0.06, "trap RC @5tau");
    }
}

void test_rl_step() {
    NetlistParser p;
    expect(loadCir(p, "g07_rl_step.cir"), "load rl");
    auto c = buildCircuitFromNetlist(p);
    SpiceTransient::Options opts;
    opts.tolerance = 1e-5;
    auto sim = SpiceTransient(opts).simulate(0.0, 50e-6, 0.5e-6, c.devices, c.nodeMap);
    expect(sim.converged, "rl converged");
    size_t out = c.nodeMap["out"];
    if (out >= 1 && !sim.nodeVoltages.empty()) {
        // tau=L/R=1us; at 50us v_L ≈ 0
        expectNear(sim.nodeVoltages.back()[out - 1], 0.0, 0.05, "RL settled ~0");
    }
}

void test_ngspice_batch_compare() {
    struct Case { const char* cir; const char* node; double want; };
    const Case cases[] = {
        {"V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n", "mid", 2.5},
        {"V1 in 0 5\nR1 in a 1k\nR2 a 0 1k\n.op\n", "a", 2.5},
        {"V1 in 0 5\nR1 in mid 500\nRload mid 0 500\n.op\n", "mid", 2.5},
    };
    int compared = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        std::string path = "/tmp/egottol_ng_" + std::to_string(i) + ".cir";
        std::string outp = "/tmp/egottol_ng_" + std::to_string(i) + ".out";
        {
            std::ofstream f(path);
            f << cases[i].cir << ".print dc V(" << cases[i].node << ")\n.end\n";
        }
        std::string cmd = "ngspice -b " + path + " > " + outp + " 2>&1";
        if (std::system(cmd.c_str()) != 0) {
            std::printf("SKIP: ngspice case %zu\n", i);
            continue;
        }
        double ng = NAN;
        {
            std::ifstream in(outp);
            std::string line;
            std::string key = std::string("v(") + cases[i].node + ")";
            while (std::getline(in, line)) {
                std::string low = line;
                for (char& c : low) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                auto pos = low.find(key);
                if (pos == std::string::npos) continue;
                auto eq = line.find('=', pos);
                if (eq == std::string::npos) continue;
                try { ng = std::stod(line.substr(eq + 1)); break; } catch (...) {}
            }
        }
        NetlistParser p;
        p.parse(cases[i].cir);
        auto c = buildCircuitFromNetlist(p);
        auto sol = dcSolve(c);
        double ours = nodeV(sol, c, cases[i].node);
        expectNear(ours, cases[i].want, 1e-6, "egottol analytical");
        if (std::isfinite(ng)) {
            expectNear(ours, ng, 1e-3, "egottol vs ngspice");
            ++compared;
        }
    }
    std::printf("  ngspice cross-check: %d circuit(s)\n", compared);
}

void test_nmos_ids_level1() {
    // Level-1 saturation: Id = 0.5*KP*(W/L)*(Vgs-Vt)^2*(1+λVds)
    const double KP = 120e-6, W = 10e-6, L = 1e-6, Vt = 0.7, lam = 0.05;
    const double Vgs = 2.0, Vds = 3.0;
    const double von = Vgs - Vt;
    const double idWant = 0.5 * KP * (W / L) * von * von * (1.0 + lam * Vds);

    const char* nl =
        "Vds d 0 3\n"
        "Vgs g 0 2\n"
        "M1 d g 0 0 NMOS W=10u L=1u\n"
        ".model NMOS NMOS (VTO=0.7 KP=120u LAMBDA=0.05)\n"
        ".op\n";
    NetlistParser p;
    p.parse(nl);
    auto c = buildCircuitFromNetlist(p);
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    expect(sol.success, "ids dc");
    // Current through Vds ≈ Id (second auxiliary unknown after voltages)
    expect(!sol.currents.empty(), "has source currents");
    if (!sol.currents.empty()) {
        // Vds is first Vsrc → current[0] into + of Vds (drain side convention)
        double id = std::fabs(sol.currents[0]);
        expectNear(id, idWant, idWant * 0.15 + 1e-8, "NMOS Id Level-1");
        std::printf("  NMOS Id=%.6g want≈%.6g\n", id, idWant);
    }
}

void test_dc_full_scale_rails() {
    NetlistParser p;
    expect(loadCir(p, "g11_nmos_model.cir"), "g11 fullscale load");
    auto c = buildCircuitFromNetlist(p);
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    expect(sol.success, "g11 fullscale dc");
    if (sol.success && c.nodeMap.count("vdd")) {
        expectNear(nodeV(sol, c, "vdd"), 5.0, 1e-3, "Vdd full scale 5V");
    }
}

}  // namespace

int main() {
    test_model_card_parse_apply();
    checkOp("g01_divider.cir", "mid", 2.5, 1e-3);
    checkOp("g02_series_r.cir", "a", 2.5, 1e-3);
    checkOp("g03_isrc_r.cir", "n1", 2.0, 1e-3);
    checkOp("g04_bridge.cir", "sense", 4.0, 1e-3);
    checkOp("g08_ladder.cir", "mid2", 10.0 / 3.0, 1e-2);
    checkOp("g09_parallel.cir", "mid", 5.0, 1e-3);
    checkOp("g13_subckt_div.cir", "mid", 2.5, 1e-3);
    checkOp("g15_subckt_series.cir", "mid", 5.0, 1e-3);
    checkOp("g17_star.cir", "c", 10.0 / 3.0, 1e-2);
    checkOp("g18_current_div.cir", "n1", 0.5, 1e-3);
    checkOp("g20_thevenin.cir", "mid", 2.5, 1e-3);
    checkOp("g21_include.cir", "mid", 2.5, 1e-3);
    checkOp("g22_nested_subckt.cir", "mid", 2.5, 1e-3);
    checkOp("g23_vcvs.cir", "out", 2.0, 1e-3);
    checkOp("g24_vccs.cir", "out", 2.0, 1e-3);

    {
        NetlistParser p;
        expect(loadCir(p, "g25_sin_rc.cir"), "g25 load");
        auto c = buildCircuitFromNetlist(p);
        expect(c.ok, "g25 build");
        SpiceTransient::Options opts;
        opts.tolerance = 1e-4;
        auto sim = SpiceTransient(opts).simulate(0.0, 2e-3, 10e-6, c.devices, c.nodeMap);
        expect(sim.converged && sim.timePoints.size() > 10, "g25 sin tran");
    }
    {
        // .dc sweep: mid should track Vin/2
        NetlistParser p;
        expect(loadCir(p, "g26_dc_sweep.cir"), "g26 load");
        auto c = buildCircuitFromNetlist(p);
        expect(c.ok, "g26 build");
        auto src = findSourceByName(c, "V1");
        expect(src != nullptr, "g26 find V1");
        if (auto* v = dynamic_cast<Vsrc*>(src.get())) {
            for (double vin : {0.0, 2.0, 4.0}) {
                v->setDC(vin);
                auto sol = dcSolve(c);
                expect(sol.success, "g26 dc point");
                expectNear(nodeV(sol, c, "mid"), vin * 0.5, 1e-3, "g26 mid");
            }
        }
    }

    {
        NetlistParser p;
        expect(loadCir(p, "g05_diode_model.cir"), "g05 load");
        auto c = buildCircuitFromNetlist(p);
        auto sol = dcSolve(c);
        expect(sol.success, "g05 dc");
        if (sol.success && c.nodeMap.count("a")) {
            double va = nodeV(sol, c, "a");
            expect(va > 0.4 && va < 1.2, "g05 Vf");
        }
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g14_diode_rs.cir"), "g14 load");
        auto c = buildCircuitFromNetlist(p);
        auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
        expect(sol.success, "g14 diode+Rs dc");
        if (sol.success && c.nodeMap.count("a")) {
            double va = nodeV(sol, c, "a");
            expect(std::isfinite(va) && va > 0.4 && va < 1.5, "g14 Vf with Rs");
            std::printf("  diode+Rs Va=%.4f V\n", va);
        }
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g10_diode_default.cir"), "g10 load");
        auto c = buildCircuitFromNetlist(p);
        auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
        expect(sol.success, "g10 dc");
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g06_rc_step.cir"), "g06 load");
        auto c = buildCircuitFromNetlist(p);
        auto sim = SpiceTransient().simulate(0.0, 5e-3, 20e-6, c.devices, c.nodeMap);
        expect(sim.converged, "g06 tran");
        size_t out = c.nodeMap["out"];
        if (out >= 1) expectNear(sim.nodeVoltages.back()[out - 1], 0.993, 0.05, "g06 @5tau");
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g16_rc_2k.cir"), "g16 load");
        auto c = buildCircuitFromNetlist(p);
        // tau=2ms; at 10ms=5tau → ~0.993
        auto sim = SpiceTransient().simulate(0.0, 10e-3, 50e-6, c.devices, c.nodeMap);
        expect(sim.converged, "g16 tran");
        size_t out = c.nodeMap["out"];
        if (out >= 1) expectNear(sim.nodeVoltages.back()[out - 1], 0.993, 0.06, "g16 @5tau");
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g11_nmos_model.cir"), "g11 load");
        expect(p.getModels().count("nmos") == 1, "g11 model");
        auto c = buildCircuitFromNetlist(p);
        expect(c.ok, "g11 build");
        auto sol = dcSolve(c);
        expect(sol.success, "g11 dc");
        if (sol.success && c.nodeMap.count("d")) {
            double vd = nodeV(sol, c, "d");
            double vdd = c.nodeMap.count("vdd") ? nodeV(sol, c, "vdd") : 5.0;
            expect(std::isfinite(vd) && vd < vdd + 0.1, "g11 Vd below rail");
            std::printf("  nmos Vd=%.4f V (Vdd=%.4f)\n", vd, vdd);
        }
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g12_bjt_model.cir"), "g12 load");
        expect(p.getModels().count("npn") == 1, "g12 model");
        auto c = buildCircuitFromNetlist(p);
        auto sol = dcSolve(c);
        expect(sol.success, "g12 dc");
        if (sol.success && c.nodeMap.count("c")) {
            double vc = nodeV(sol, c, "c");
            double vcc = c.nodeMap.count("vcc") ? nodeV(sol, c, "vcc") : 5.0;
            expect(vc < vcc - 0.05, "g12 Vc pulled below Vcc");
            std::printf("  bjt Vc=%.4f V (Vcc=%.4f)\n", vc, vcc);
        }
    }
    {
        NetlistParser p;
        expect(loadCir(p, "g19_pmos_model.cir"), "g19 load");
        auto c = buildCircuitFromNetlist(p);
        auto sol = dcSolve(c);
        expect(sol.success, "g19 pmos dc");
        if (sol.success && c.nodeMap.count("d")) {
            double vd = nodeV(sol, c, "d");
            expect(std::isfinite(vd), "g19 Vd finite");
            std::printf("  pmos Vd=%.4f V\n", vd);
        }
    }

    test_dc_full_scale_rails();
    test_nmos_ids_level1();
    test_rl_step();
    test_trap_rc();
    test_ngspice_batch_compare();

    if (failures) {
        std::fprintf(stderr, "%d failure(s) in golden_spice_test\n", failures);
        return 1;
    }
    std::printf("golden_spice_test: all passed (%d circuits)\n", 22);
    return 0;
}
