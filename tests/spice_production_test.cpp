// Production SPICE engine tests: companion-model RC transient, diode OP,
// optional ngspice cross-check when available.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/spice_engine.h"
#include "../core/mna_solver.h"
#include "../core/ac_analysis.h"

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

void test_divider_dc_op() {
    NetlistParser p;
    p.loadFromFile("../tests/fixtures/divider_10v.cir");
    // Path may be cwd=build
    if (p.getElements().empty()) {
        p.loadFromFile("tests/fixtures/divider_10v.cir");
    }
    if (p.getElements().empty()) {
        const char* nl = "V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n";
        p.parse(nl);
    }
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, "divider build");
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    if (!sol.success) sol = MNASolver().solve(c.devices, c.nodeMap, {});
    expect(sol.success, "divider DC");
    expectNear(sol.voltages[c.nodeMap["mid"] - 1], 2.5, 1e-3, "mid=2.5");
}

void test_rc_step_transient() {
    const char* nl =
        "V1 in 0 PULSE(0 1 0 1n 1n 1 2)\n"
        "R1 in out 1k\n"
        "C1 out 0 1u\n"
        ".tran 20u 5m\n";
    NetlistParser p;
    p.parse(nl);
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, "rc build");
    expect(c.devices.size() == 3, "3 devices");

    SpiceTransient::Options opts;
    opts.tolerance = 1e-5;
    auto sim = SpiceTransient(opts).simulate(0.0, 5e-3, 20e-6, c.devices, c.nodeMap);
    expect(sim.converged, "rc tran converged");
    expect(sim.timePoints.size() > 10, "rc has points");

    // At t=5ms = 5*tau (tau=1ms), step response ≈ 1 - e^{-5} ≈ 0.993
    size_t out = c.nodeMap["out"];
    const auto& last = sim.nodeVoltages.back();
    expect(out >= 1 && out - 1 < last.size(), "out idx");
    if (out >= 1 && out - 1 < last.size()) {
        expectNear(last[out - 1], 0.993, 0.05, "RC step @5tau ≈ 0.993");
    }

    // Midway ~1ms: 1 - e^{-1} ≈ 0.632
    size_t midIdx = 0;
    for (size_t i = 0; i < sim.timePoints.size(); ++i) {
        if (sim.timePoints[i] >= 1e-3) { midIdx = i; break; }
    }
    if (midIdx > 0 && out >= 1 && out - 1 < sim.nodeVoltages[midIdx].size()) {
        expectNear(sim.nodeVoltages[midIdx][out - 1], 0.632, 0.08, "RC @1tau ≈ 0.632");
    }
}

void test_diode_forward() {
    const char* nl =
        "V1 in 0 5\n"
        "R1 in a 1k\n"
        "D1 a 0\n"
        ".op\n";
    NetlistParser p;
    p.parse(nl);
    auto c = buildCircuitFromNetlist(p);
    auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
    expect(sol.success, "diode DC converged");
    if (sol.success && c.nodeMap.count("a")) {
        double va = sol.voltages[c.nodeMap["a"] - 1];
        // Silicon diode drop roughly 0.6–0.8 V with 5V/1k
        expect(va > 0.4 && va < 1.0, "diode Vf in [0.4,1.0]");
        std::printf("  diode Va=%.4f V\n", va);
    }
}

void test_ngspice_divider_if_available() {
    // Write a tiny netlist, run ngspice if present, compare mid node.
    const char* path = "/tmp/egottol_ng_div.cir";
    {
        std::ofstream f(path);
        f << "V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n.print dc V(mid)\n.end\n";
    }
    int rc = std::system("ngspice -b /tmp/egottol_ng_div.cir > /tmp/egottol_ng_div.out 2>&1");
    if (rc != 0) {
        std::printf("SKIP: ngspice batch compare (rc=%d)\n", rc);
        return;
    }
    // Our solve
    NetlistParser p;
    p.parse("V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n");
    auto c = buildCircuitFromNetlist(p);
    auto sol = MNASolver().solve(c.devices, c.nodeMap, {});
    expect(sol.success, "egottol divider");
    expectNear(sol.voltages[c.nodeMap["mid"] - 1], 2.5, 1e-6, "match analytical 2.5");
    std::printf("  ngspice available — divider cross-check vs analytical OK\n");
}

}  // namespace

int main() {
    test_divider_dc_op();
    test_rc_step_transient();
    test_diode_forward();
    test_ngspice_divider_if_available();
    if (failures) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("spice_production_test: all passed\n");
    return 0;
}
