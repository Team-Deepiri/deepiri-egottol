#include "../core/analog/ota.h"

#include <cmath>
#include <cstdio>

using namespace deepiri;

namespace {

int failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) {
        std::fprintf(stderr, "FAILED: %s\n", what);
        ++failures;
    }
}

void test_output_current_linear_region() {
    OTA ota("G1", 2e-3, 1.0);
    double i = ota.outputCurrent(0.3, 0.1);
    expect(std::abs(i - 2e-3 * 0.2) < 1e-12, "OTA output current should be gm*(V+ - V-) inside the linear range");
}

void test_output_current_soft_clip() {
    OTA ota("G1", 1e-3, 0.5);
    double i = ota.outputCurrent(2.0, 0.0);
    expect(std::abs(i - 1e-3 * 0.5) < 1e-12, "OTA output current should clamp differential voltage to +vLimit");

    double iNeg = ota.outputCurrent(-2.0, 0.0);
    expect(std::abs(iNeg - (-1e-3 * 0.5)) < 1e-12, "OTA output current should clamp differential voltage to -vLimit");
}

void test_conductance_matrix_stamp() {
    OTA ota("G1", 5e-3, 1.0);
    ota.setNodes(1, 2, 3);
    auto G = ota.getConductanceMatrix();
    expect(G.size() == 3 && G[0].size() == 3, "OTA conductance matrix should be 3x3 (plus, minus, out)");
    expect(std::abs(G[2][0] - 5e-3) < 1e-12, "row 'out' should carry +gm in the 'plus' column");
    expect(std::abs(G[2][1] - (-5e-3)) < 1e-12, "row 'out' should carry -gm in the 'minus' column");
    expect(G[0][0] == 0.0 && G[1][1] == 0.0, "OTA conductance matrix should not stamp the input rows");
}

void test_current_vector_is_zero() {
    OTA ota;
    auto I = ota.getCurrent();
    expect(I.size() == 3, "OTA current vector should have 3 entries");
    for (double v : I) {
        expect(v == 0.0, "OTA is a pure Norton conductance stamp with zero current-source term");
    }
}

}  // namespace

int main() {
    test_output_current_linear_region();
    test_output_current_soft_clip();
    test_conductance_matrix_stamp();
    test_current_vector_is_zero();

    if (failures == 0) {
        std::printf("All OTA tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
