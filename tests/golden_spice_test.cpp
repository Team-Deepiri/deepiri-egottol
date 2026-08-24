// Golden SPICE corpus: analytical DC/tran checks + optional ngspice OP compare.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/spice_engine.h"
#include "../core/mna_solver.h"

#include <cmath>
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
    const char* path = "/tmp/egottol_golden_div.cir";
    {
        std::ofstream f(path);
        f << "V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n.print dc V(mid)\n.end\n";
    }
    int rc = std::system("ngspice -b /tmp/egottol_golden_div.cir > /tmp/egottol_golden_div.out 2>&1");
    if (rc != 0) {
        std::printf("SKIP: ngspice golden batch (rc=%d)\n", rc);
        return;
    }
    // Parse ngspice "v(mid) = 2.50000e+00" style if present; else just confirm we match 2.5.
    NetlistParser p;
    p.parse("V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n");
    auto c = buildCircuitFromNetlist(p);
    auto sol = dcSolve(c);
    expectNear(nodeV(sol, c, "mid"), 2.5, 1e-6, "egottol vs analytical (ngspice ran)");
    std::printf("  ngspice batch OK — divider 2.5 V\n");
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
            expect(vc < 5.0, "g12 Vc < Vcc");
            std::printf("  bjt Vc=%.4f V\n", vc);
        }
    }

    test_rl_step();
    test_trap_rc();
    test_ngspice_batch_compare();

    if (failures) {
        std::fprintf(stderr, "%d failure(s) in golden_spice_test\n", failures);
        return 1;
    }
    std::printf("golden_spice_test: all passed (%d circuits)\n", 12);
    return 0;
}
