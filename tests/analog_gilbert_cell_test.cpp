#include "../core/analog/gilbert_cell.h"

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

void test_four_quadrant_multiply() {
    GilbertCell cell(1e-3, 0.5);

    expect(std::abs(cell.multiply(0.2, 0.3) - 1e-3 * 0.2 * 0.3) < 1e-12, "quadrant I: +Vx * +Vy");
    expect(std::abs(cell.multiply(-0.2, 0.3) - 1e-3 * -0.2 * 0.3) < 1e-12, "quadrant II: -Vx * +Vy");
    expect(std::abs(cell.multiply(-0.2, -0.3) - 1e-3 * -0.2 * -0.3) < 1e-12, "quadrant III: -Vx * -Vy");
    expect(std::abs(cell.multiply(0.2, -0.3) - 1e-3 * 0.2 * -0.3) < 1e-12, "quadrant IV: +Vx * -Vy");
}

void test_input_range_limiting() {
    GilbertCell cell(1e-3, 0.5);
    double out = cell.multiply(5.0, 5.0);
    expect(std::abs(out - 1e-3 * 0.5 * 0.5) < 1e-12, "inputs beyond vLimit must be clamped before multiplying");
}

void test_multiply_array() {
    GilbertCell cell(2e-3, 1.0);
    std::vector<double> vx{0.1, 0.2, 0.3};
    std::vector<double> vy{0.4, 0.5, 0.6};
    auto out = cell.multiplyArray(vx, vy);
    expect(out.size() == 3, "multiplyArray should preserve element count");
    for (size_t i = 0; i < out.size(); ++i) {
        expect(std::abs(out[i] - 2e-3 * vx[i] * vy[i]) < 1e-12, "multiplyArray should match element-wise multiply()");
    }
}

void test_small_signal_gain() {
    GilbertCell cell(1e-3, 0.5);
    auto [dIdVx, dIdVy] = cell.smallSignalGain(0.2, 0.3);
    expect(std::abs(dIdVx - 1e-3 * 0.3) < 1e-12, "dI/dVx should equal k*Vy at the operating point");
    expect(std::abs(dIdVy - 1e-3 * 0.2) < 1e-12, "dI/dVy should equal k*Vx at the operating point");
}

}  // namespace

int main() {
    test_four_quadrant_multiply();
    test_input_range_limiting();
    test_multiply_array();
    test_small_signal_gain();

    if (failures == 0) {
        std::printf("All Gilbert cell tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
