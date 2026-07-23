// Regression/behavior tests for core/ac_analysis.{h,cpp}: the native AC/Bode
// small-signal sweep ported from egottol/engines/analog/ac_analysis.py.
#include "../core/ac_analysis.h"
#include "../models/vsrc.h"
#include "../models/resistor.h"
#include "../models/capacitor.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

void test_resistive_divider_ac_matches_dc_at_all_frequencies() {
    // A purely resistive 1k/1k divider has no frequency-dependent elements,
    // so its AC transfer function must equal its DC ratio (0.5) at every
    // swept frequency, with ~0 phase shift.
    auto vsrc = std::make_shared<Vsrc>("V1", 1.0);
    vsrc->setNodes(1, 0);
    auto r1 = std::make_shared<Resistor>("R1", 1000.0);
    r1->setNodes(1, 2);
    auto r2 = std::make_shared<Resistor>("R2", 1000.0);
    r2->setNodes(2, 0);

    std::vector<std::shared_ptr<Device>> devices{vsrc, r1, r2};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    ACAnalysis ac;
    ACResult result = ac.sweep(devices, nodeMap, 1.0, 1.0e6, 10);

    expect(result.success, "resistive divider AC sweep should succeed");
    if (result.success) {
        expect(result.magnitude.size() == 2, "should have magnitude series for 2 nodes");
        for (size_t fi = 0; fi < result.frequenciesHz.size(); ++fi) {
            expect(std::abs(result.magnitude[0][fi] - 1.0) < 1e-6,
                   "node 1 (source node) magnitude should be 1.0 (source/source) at every frequency");
            expect(std::abs(result.magnitude[1][fi] - 0.5) < 1e-6,
                   "node 2 (midpoint) magnitude should be 0.5 at every frequency (purely resistive)");
            expect(std::abs(result.phaseDeg[1][fi]) < 1e-6,
                   "purely resistive divider should have ~0 phase shift");
        }
    }
}

void test_rc_lowpass_corner_frequency() {
    // R + C to ground driven by a unit AC source: classic RC low-pass with
    // a -3dB (|H| = 1/sqrt(2) ~ 0.707) corner at f = 1/(2*pi*R*C), and
    // -20dB/decade rolloff above it.
    const double R = 1000.0;
    const double C = 1e-6;
    const double fCorner = 1.0 / (2.0 * M_PI * R * C);

    auto vsrc = std::make_shared<Vsrc>("V1", 1.0);
    vsrc->setNodes(1, 0);
    auto r1 = std::make_shared<Resistor>("R1", R);
    r1->setNodes(1, 2);
    auto c1 = std::make_shared<Capacitor>("C1", C);
    c1->setNodes(2, 0);

    std::vector<std::shared_ptr<Device>> devices{vsrc, r1, c1};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    ACAnalysis ac;
    ACResult atCorner = ac.sweep(devices, nodeMap, fCorner, fCorner, 1);
    expect(atCorner.success, "RC low-pass AC sweep at corner frequency should succeed");
    if (atCorner.success) {
        double magAtCorner = atCorner.magnitude[1][0];
        expect(std::abs(magAtCorner - 0.70710678) < 0.02,
               "RC low-pass magnitude at corner frequency should be ~0.707 (-3dB)");
        expect(std::abs(atCorner.phaseDeg[1][0] - (-45.0)) < 1.0,
               "RC low-pass phase at corner frequency should be ~-45 degrees");
    }

    ACResult sweep = ac.sweep(devices, nodeMap, fCorner / 100.0, fCorner * 100.0, 40);
    expect(sweep.success, "RC low-pass full sweep should succeed");
    if (sweep.success) {
        double magLow = sweep.magnitude[1].front();
        double magHigh = sweep.magnitude[1].back();
        expect(magLow > 0.99, "far below the corner, |H| should be ~1 (passband)");
        expect(magHigh < 0.02, "two decades above the corner, |H| should be small (-20dB/decade rolloff)");

        for (size_t fi = 1; fi < sweep.magnitude[1].size(); ++fi) {
            expect(sweep.magnitude[1][fi] <= sweep.magnitude[1][fi - 1] + 1e-9,
                   "RC low-pass magnitude should be monotonically non-increasing with frequency");
        }
    }
}

}  // namespace

int main() {
    test_resistive_divider_ac_matches_dc_at_all_frequencies();
    test_rc_lowpass_corner_frequency();

    if (failures == 0) {
        std::printf("All AC analysis tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
