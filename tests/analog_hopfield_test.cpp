// Regression tests for core/analog/hopfield.h: Hebbian pattern storage and
// asynchronous energy-descent recall from a corrupted cue.
#include "../core/analog/hopfield.h"

#include <cmath>
#include <cstdio>
#include <vector>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

void test_recall_from_single_bit_flip() {
    // 8-bit bipolar pattern (+1/-1, the network's native representation) with
    // a distinctive structure so a single stored pattern has a clear,
    // unambiguous basin of attraction.
    std::vector<double> pattern{1, -1, 1, -1, 1, 1, -1, -1};

    HopfieldNetwork net(pattern.size());
    net.storePattern(pattern);

    std::vector<double> cue = pattern;
    cue[3] *= -1.0;

    std::vector<double> recalled = net.recall(cue, 100);

    bool matches = true;
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (recalled[i] != pattern[i]) {
            matches = false;
            break;
        }
    }
    expect(matches, "recall from a single-bit-corrupted cue should converge to the stored pattern");
}

void test_energy_of_stored_pattern_is_local_minimum() {
    std::vector<double> pattern{1, 1, -1, -1};
    HopfieldNetwork net(pattern.size());
    net.storePattern(pattern);

    const std::vector<double>& bp = pattern;
    double storedEnergy = net.energy(bp);

    for (size_t i = 0; i < bp.size(); ++i) {
        std::vector<double> flipped = bp;
        flipped[i] *= -1.0;
        double flippedEnergy = net.energy(flipped);
        expect(flippedEnergy >= storedEnergy - 1e-9,
               "flipping any single bit of a stored pattern should not lower its energy");
    }
}

}  // namespace

int main() {
    test_recall_from_single_bit_flip();
    test_energy_of_stored_pattern_is_local_minimum();

    if (failures == 0) {
        std::printf("All analog Hopfield tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
