// Perf baseline: DC OP must stay fast as node count grows.
#include "../io/netlist_parser.h"
#include "../io/netlist_builder.h"
#include "../core/spice_engine.h"

#include <chrono>
#include <cstdio>
#include <sstream>
#include <string>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool c, const char* w) {
    if (!c) { std::fprintf(stderr, "FAILED: %s\n", w); ++failures; }
}

std::string ladderNetlist(int stages) {
    std::ostringstream oss;
    oss << "V1 n0 0 10\n";
    for (int i = 0; i < stages; ++i) {
        oss << "R" << i << " n" << i << " n" << (i + 1) << " 1k\n";
    }
    oss << "Rterm n" << stages << " 0 1k\n.op\n";
    return oss.str();
}

double timeDcMs(const std::string& nl, int reps) {
    NetlistParser p;
    p.parse(nl);
    auto c = buildCircuitFromNetlist(p);
    if (!c.ok) return -1.0;
    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < reps; ++i) {
        auto sol = DcOperatingPoint().solve(c.devices, c.nodeMap);
        if (!sol.success) return -1.0;
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

}  // namespace

int main() {
    // Warm small circuit
    double t10 = timeDcMs(ladderNetlist(10), 50);
    expect(t10 >= 0.0, "ladder10 solves");
    double t50 = timeDcMs(ladderNetlist(50), 20);
    expect(t50 >= 0.0, "ladder50 solves");

    // Soft CI budget: 50-stage OP ×20 under 2s on typical runners.
    expect(t50 < 2000.0, "ladder50 perf budget <2s/20");
    std::printf("perf_baseline: ladder10×50=%.2f ms  ladder50×20=%.2f ms\n", t10, t50);

    if (failures) {
        std::fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    std::printf("perf_baseline_test: all passed\n");
    return 0;
}
