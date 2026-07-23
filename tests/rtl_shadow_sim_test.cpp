// Regression test for logic/rtl_shadow_sim.h: a native port of the Python
// CycleAccurateFallback event simulator (egottol/engines/rtl_shadow.py).
#include "../logic/rtl_shadow_sim.h"

#include <cstdio>
#include <cstdlib>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

// Mirrors `assign y = a & b;` as produced by CycleAccurateFallback.from_verilog.
void test_and_gate_truth_table() {
    const bool inputs_a[4] = {false, false, true, true};
    const bool inputs_b[4] = {false, true, false, true};
    const bool expected[4] = {false, false, false, true};

    for (int i = 0; i < 4; ++i) {
        RtlShadowFallbackSim sim(
            {GateSpec{GateType::AND, "a", "b", "y"}},
            {},
            {"y"});
        sim.set_input("a", inputs_a[i]);
        sim.set_input("b", inputs_b[i]);

        // One full clock cycle in the Python fallback is two tick() calls:
        // the posedge half (DFF latch, a no-op here) then the negedge half
        // (combinational settle).
        sim.tick();
        sim.tick();

        auto out = sim.outputs();
        expect(out.at("y") == expected[i], "AND truth-table row mismatch");
    }
}

void test_and_gate_holds_until_negedge() {
    RtlShadowFallbackSim sim(
        {GateSpec{GateType::AND, "a", "b", "y"}},
        {},
        {"y"});
    sim.set_input("a", true);
    sim.set_input("b", true);

    sim.tick();  // posedge: no DFFs, y must still be unset/false
    expect(sim.get_net("y") == false, "y must not settle before the negedge tick");

    sim.tick();  // negedge: combinational eval runs
    expect(sim.get_net("y") == true, "y must settle to 1 after the negedge tick");
}

}  // namespace

int main() {
    test_and_gate_truth_table();
    test_and_gate_holds_until_negedge();

    if (failures == 0) {
        std::printf("All rtl_shadow_sim tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
