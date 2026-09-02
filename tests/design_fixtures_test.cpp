// Classic EE design fixtures — LED, RC, flyback, buck/boost, H-bridge smoke.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/spice_engine.h"
#include "../core/ac_analysis.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool c, const char* w) {
    if (!c) { std::fprintf(stderr, "FAILED: %s\n", w); ++failures; }
}

bool load(NetlistParser& p, const char* name) {
    const char* roots[] = {
        "tests/fixtures/design/",
        "../tests/fixtures/design/",
        "../../tests/fixtures/design/",
    };
    for (const char* r : roots) {
        if (p.loadFromFile(std::string(r) + name) && !p.getElements().empty()) return true;
    }
    return false;
}

bool runTran(const char* name, const char* label) {
    NetlistParser p;
    if (!load(p, name)) {
        expect(false, label);
        return false;
    }
    auto c = buildCircuitFromNetlist(p);
    expect(c.ok, label);
    if (!c.ok) return false;
    SpiceTransient::Options opts;
    opts.tolerance = 1e-4;
    double tstep = 1e-6, tstop = 1e-4;
    for (const auto& d : p.getControlDirectives()) {
        if (d.kind == "tran") {
            if (!d.numbers.empty()) tstep = d.numbers[0];
            if (d.numbers.size() > 1) tstop = d.numbers[1];
        }
    }
    auto sim = SpiceTransient(opts).simulate(0.0, tstop, tstep, c.devices, c.nodeMap);
    expect(sim.converged && !sim.timePoints.empty(), label);
    if (sim.converged) {
        std::printf("  %s: %zu steps\n", label, sim.timePoints.size());
    }
    return sim.converged;
}

}  // namespace

int main() {
    {
        NetlistParser p;
        expect(load(p, "led_series_r.cir"), "led load");
        auto c = buildCircuitFromNetlist(p);
        auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
        expect(sol.success, "led dc");
        if (sol.success && c.nodeMap.count("led")) {
            double v = sol.voltages[c.nodeMap["led"] - 1];
            expect(v > 0.4 && v < 1.2, "led Vf");
            std::printf("  LED anode=%.3f V\n", v);
        }
    }
    {
        NetlistParser p;
        expect(load(p, "rc_lowpass.cir"), "rc lpf load");
        auto c = buildCircuitFromNetlist(p);
        expect(c.ok, "rc lpf build");
        ACAnalysis ac;
        auto sweep = ac.sweep(c.devices, c.nodeMap, 10.0, 1e6, 20);
        expect(sweep.success, "rc lpf ac");
    }
    {
        NetlistParser p;
        expect(load(p, "rc_highpass.cir"), "rc hpf load");
        auto c = buildCircuitFromNetlist(p);
        ACAnalysis ac;
        auto sweep = ac.sweep(c.devices, c.nodeMap, 10.0, 1e6, 20);
        expect(sweep.success, "rc hpf ac");
    }

    runTran("flyback_diode.cir", "flyback tran");
    runTran("buck_lc.cir", "buck tran");
    runTran("boost_chopper.cir", "boost tran");
    runTran("hbridge_motor.cir", "hbridge tran");

    if (failures) {
        std::fprintf(stderr, "%d failure(s) in design_fixtures_test\n", failures);
        return 1;
    }
    std::printf("design_fixtures_test: all passed\n");
    return 0;
}
