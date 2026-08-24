// Analytical golden waveforms vs native solvers (Phase 8 foundation).
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/mna_solver.h"
#include "../core/ac_analysis.h"
#include "../core/transient.h"
#include "../infra/uqe_logic.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool c, const char* w) {
    if (!c) { std::fprintf(stderr, "FAILED: %s\n", w); ++failures; }
}

void expectNear(double got, double want, double tol, const char* w) {
    if (std::fabs(got - want) > tol) {
        std::fprintf(stderr, "FAILED: %s (got %g want %g ±%g)\n", w, got, want, tol);
        ++failures;
    }
}

void golden_divider() {
    NetlistParser p;
    p.parse("V1 in 0 10\nR1 in mid 3k\nR2 mid 0 1k\n.op\n");
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, "divider build");
    auto sol = MNASolver().solve(c.devices, c.nodeMap, {});
    expect(sol.success, "divider solve");
    // 10 * 1/(3+1) = 2.5 V
    expectNear(sol.voltages[c.nodeMap["mid"] - 1], 2.5, 1e-3, "mid=2.5");
}

void golden_rc_dc() {
    // At DC capacitors open → V(out) = V(in) through R only to open C → floating.
    // Use resistive divider with C to gnd: DC out = mid of divider.
    NetlistParser p;
    p.parse("V1 in 0 5\nR1 in out 1k\nR2 out 0 1k\nC1 out 0 1u\n.op\n");
    auto c = buildCircuitFromNetlist(p);
    auto sol = MNASolver().solve(c.devices, c.nodeMap, {});
    expect(sol.success, "rc dc");
    expectNear(sol.voltages[c.nodeMap["out"] - 1], 2.5, 0.05, "rc dc out~2.5");
}

void golden_rc_ac_corner() {
    // RC low-pass R=1k C=1u → fc = 1/(2πRC) ≈ 159.15 Hz; |H(fc)| ≈ 1/√2
    NetlistParser p;
    p.parse("V1 in 0 1 1\nR1 in out 1k\nC1 out 0 1u\n.ac lin 3 159.15 159.15\n");
    auto c = buildCircuitFromNetlist(p);
    auto sweep = ACAnalysis().sweep(c.devices, c.nodeMap, 159.15, 159.15, 3);
    expect(sweep.success, "ac sweep");
    size_t out = c.nodeMap["out"];
    expect(out >= 1 && out - 1 < sweep.magnitude.size(), "out mag row");
    if (out >= 1 && out - 1 < sweep.magnitude.size() && !sweep.magnitude[out - 1].empty()) {
        double mag = sweep.magnitude[out - 1][0];
        // AC engine returns node voltage magnitude for 1V AC source.
        expectNear(mag, 0.7071, 0.08, "|H(fc)|≈0.707");
    }
}

void golden_uqe_mapping() {
    auto circ = toUQEFromNetlist("NOT a y\nXOR a b z\nAND a b w\n");
    expect(circ.num_qubits >= 3, "qubits allocated");
    expect(!circ.gates.empty(), "gates emitted");
    bool hasX = false, hasCx = false, hasCcx = false;
    for (const auto& g : circ.gates) {
        if (g.name == "x") hasX = true;
        if (g.name == "cx") hasCx = true;
        if (g.name == "ccx") hasCcx = true;
    }
    expect(hasX && hasCx && hasCcx, "NOT→X XOR→CX AND→CCX");
    auto qasm = toOpenQasm(circ);
    expect(qasm.find("OPENQASM") != std::string::npos, "qasm header");
    expect(qasm.find("x q[") != std::string::npos, "qasm x");
}

}  // namespace

int main() {
    golden_divider();
    golden_rc_dc();
    golden_rc_ac_corner();
    golden_uqe_mapping();
    if (failures) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("golden_waveform_test: all passed\n");
    return 0;
}
