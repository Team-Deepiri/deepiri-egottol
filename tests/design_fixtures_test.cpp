// Classic EE design fixtures — LED limit R, RC filters smoke.
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

    if (failures) {
        std::fprintf(stderr, "%d failure(s) in design_fixtures_test\n", failures);
        return 1;
    }
    std::printf("design_fixtures_test: all passed\n");
    return 0;
}
