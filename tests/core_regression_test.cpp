// Regression tests for bugs fixed in core/: Matrix::at() bounds checking,
// Transient::computeDerivatives zeroing near-zero sums, NewtonRaphson exception
// safety on a singular Jacobian, and MNASolver's conductance-matrix bounds guard.
#include "../core/matrix.h"
#include "../core/transient.h"
#include "../core/newton_raphson.h"
#include "../core/mna_solver.h"
#include "../models/isrc.h"
#include "../models/vsrc.h"
#include "../models/resistor.h"

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

void test_matrix_bounds() {
    Matrix m(2, 2);
    bool threw = false;
    try {
        m.at(5, 0) = 1.0;
    } catch (const std::out_of_range&) {
        threw = true;
    }
    expect(threw, "Matrix::at() should throw std::out_of_range on OOB index");
}

void test_transient_derivative_not_zeroed() {
    // A single current source between node 1 and node 2 with zero conductance:
    // sum is always 0, so rhs - sum must equal rhs, not be clamped to 0.
    auto isrc = std::make_shared<Isrc>("I1", 1.0);
    isrc->setNodes(1, 2);

    std::vector<std::shared_ptr<Device>> devices{isrc};

    Transient transient;
    TransientResult result = transient.simulate(0.0, 1e-3, 1e-3, devices, 2);

    expect(!result.nodeVoltages.empty(), "transient simulation should produce at least one point");
    if (!result.nodeVoltages.empty()) {
        const auto& last = result.nodeVoltages.back();
        expect(last.size() == 2, "state vector should have 2 unknowns");
        expect(last[0] != 0.0 || last[1] != 0.0,
               "state must move under a nonzero RHS even when the conductance-weighted sum is ~0");
    }
}

void test_newton_raphson_singular_jacobian_does_not_throw() {
    NewtonRaphson nr(1e-6, 5, 1.0);

    auto f = [](const std::vector<double>& x) -> std::vector<double> {
        return {1.0, 1.0};
    };
    auto J = [](const std::vector<double>&) -> std::vector<std::vector<double>> {
        return {{0.0, 0.0}, {0.0, 0.0}};
    };

    bool threw = false;
    NewtonRaphson::Result result;
    try {
        result = nr.solve({0.0, 0.0}, f, J);
    } catch (...) {
        threw = true;
    }

    expect(!threw, "NewtonRaphson::solve() must not throw on a singular Jacobian");
    expect(!result.converged, "a singular Jacobian should be reported as non-convergence, not a crash");
}

void test_mna_solver_short_conductance_row_is_safe() {
    // A malformed device returning a conductance row with fewer than 2 columns
    // must not cause an out-of-bounds access in buildStampMatrix.
    class ShortRowDevice : public Device {
    public:
        std::vector<double> getCurrent() const override { return {0.0, 0.0}; }
        std::vector<std::vector<double>> getConductanceMatrix() const override {
            return {{1.0}};  // only one column
        }
        std::string name() const override { return "short"; }
        std::string type() const override { return "ShortRow"; }
    };

    auto dev = std::make_shared<ShortRowDevice>();
    dev->setNodes(1, 2);

    std::vector<std::shared_ptr<Device>> devices{dev};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    MNASolver solver;
    bool threw = false;
    try {
        solver.solve(devices, nodeMap, {});
    } catch (...) {
        threw = true;
    }
    expect(!threw, "MNASolver::solve() must not crash on a short conductance row");
}

void test_mna_solver_voltage_source_divider() {
    // 5V source into a 1k/1k divider: node 1 (source node) must read ~5V and
    // node 2 (midpoint) must read ~2.5V. This exercises the Vsrc branch-equation
    // RHS, which used to be left at 0 (the source's voltage never reached the
    // solve), collapsing every Vsrc-driven circuit to the trivial 0V solution.
    auto vsrc = std::make_shared<Vsrc>("V1", 5.0);
    vsrc->setNodes(1, 0);
    auto r1 = std::make_shared<Resistor>("R1", 1000.0);
    r1->setNodes(1, 2);
    auto r2 = std::make_shared<Resistor>("R2", 1000.0);
    r2->setNodes(2, 0);

    std::vector<std::shared_ptr<Device>> devices{vsrc, r1, r2};
    std::map<std::string, size_t> nodeMap{{"1", 1}, {"2", 2}};

    MNASolver solver;
    auto sol = solver.solve(devices, nodeMap, {});

    expect(sol.success, "voltage-divider DC solve should succeed");
    if (sol.success && sol.voltages.size() >= 2) {
        expect(std::abs(sol.voltages[0] - 5.0) < 1e-6, "node 1 should read ~5V");
        expect(std::abs(sol.voltages[1] - 2.5) < 1e-6, "node 2 (midpoint) should read ~2.5V");
    }
}

}  // namespace

int main() {
    test_matrix_bounds();
    test_transient_derivative_not_zeroed();
    test_newton_raphson_singular_jacobian_does_not_throw();
    test_mna_solver_short_conductance_row_is_safe();
    test_mna_solver_voltage_source_divider();

    if (failures == 0) {
        std::printf("All core regression tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
