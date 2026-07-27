// Regression tests for core/analog/ising.h: simulated annealing should find
// (or closely approach) the known ground state of a small, hand-picked
// coupling, and the reported energy trace should trend downward.
#include "../core/analog/ising.h"

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

void test_two_spin_ferromagnetic_ground_state() {
    // H = -J12 * s1 * s2 with J12 > 0: ground state has s1 == s2 (aligned).
    size_t n = 2;
    IsingMachine machine(n, /*rngSeed=*/42);
    std::vector<double> J{0.0, 1.0,
                           1.0, 0.0};
    machine.setCoupling(J);

    IsingResult result = machine.solve(/*nSteps=*/5000, /*tStart=*/5.0, /*tEnd=*/0.01);

    expect(result.spins[0] == result.spins[1],
           "two ferromagnetically coupled spins should align at the ground state");
    // energy() sums J_ij over both (0,1) and (1,0), so the symmetric coupling
    // of 1.0 in each direction contributes -2.0 at the aligned ground state.
    expect(std::abs(result.energy - (-2.0)) < 1e-9,
           "aligned two-spin ground state energy should be -2 * J12 = -2.0");
}

void test_three_spin_known_ground_state_reached_or_approached() {
    // s1 wants to align with s2 (J12=1), s2 wants to align with s3 (J23=1),
    // and a strong field h3 pins s3 = +1: ground state s = (+1,+1,+1).
    // energy() double-counts each symmetric coupling (both J_ij and J_ji),
    // so pair energy = -(2*J12 + 2*J23) = -4, field energy = -h3 = -5,
    // total ground-state energy = -9.
    size_t n = 3;
    IsingMachine machine(n, /*rngSeed=*/7);
    std::vector<double> J(n * n, 0.0);
    J[0 * n + 1] = 1.0;
    J[1 * n + 0] = 1.0;
    J[1 * n + 2] = 1.0;
    J[2 * n + 1] = 1.0;
    machine.setCoupling(J);
    machine.setField({0.0, 0.0, 5.0});

    IsingResult result = machine.solve(/*nSteps=*/8000, /*tStart=*/8.0, /*tEnd=*/0.005);

    // Generous tolerance: annealing is stochastic, so assert the machine gets
    // close to the known ground-state energy rather than requiring an exact hit.
    expect(result.energy <= -8.0, "annealing should approach the known ground-state energy of -9.0");
    expect(result.spins[2] == 1.0, "the strongly-fielded spin should settle at +1");
}

void test_energy_after_annealing_beats_energy_before() {
    size_t n = 3;
    IsingMachine machine(n, /*rngSeed=*/123);
    std::vector<double> J(n * n, 0.0);
    J[0 * n + 1] = 1.0;
    J[1 * n + 0] = 1.0;
    J[1 * n + 2] = 1.0;
    J[2 * n + 1] = 1.0;
    machine.setCoupling(J);

    std::vector<double> initial{1.0, -1.0, 1.0};
    double initialEnergy = machine.energy(initial);

    IsingResult result = machine.solve(6000, 8.0, 0.01, &initial);

    expect(result.energy <= initialEnergy,
           "annealed energy should not be worse than the deliberately frustrated starting state");
}

}  // namespace

int main() {
    test_two_spin_ferromagnetic_ground_state();
    test_three_spin_known_ground_state_reached_or_approached();
    test_energy_after_annealing_beats_energy_before();

    if (failures == 0) {
        std::printf("All analog Ising tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
