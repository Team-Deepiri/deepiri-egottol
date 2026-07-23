#include "../core/analog/opamp_neuron.h"

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

void test_forward_known_weights_tanh() {
    std::vector<double> weights{1.0, 1.0};
    std::vector<double> bias{0.0};
    OpAmpNeuronLayer layer(2, 1, weights, bias, NeuronActivation::Tanh, 1e-3, 1.0);

    auto out = layer.forward({0.2, 0.3});
    double expected = std::tanh(0.5);
    expect(out.size() == 1, "single-output layer should return a vector of size 1");
    expect(std::abs(out[0] - expected) < 1e-9,
           "forward() with unit weights/gm should reduce to tanh(sum of inputs)");
}

void test_forward_known_weights_sigmoid() {
    std::vector<double> weights{1.0, -1.0};
    std::vector<double> bias{0.0};
    OpAmpNeuronLayer layer(2, 1, weights, bias, NeuronActivation::Sigmoid, 1e-3, 1.0);

    auto out = layer.forward({0.4, 0.1});
    double expected = 1.0 / (1.0 + std::exp(-0.3));
    expect(std::abs(out[0] - expected) < 1e-9,
           "sigmoid activation should match 1/(1+exp(-v_net)) for the weighted sum");
}

void test_forward_pads_short_input() {
    std::vector<double> weights{1.0, 1.0, 1.0};
    std::vector<double> bias{0.0};
    OpAmpNeuronLayer layer(3, 1, weights, bias, NeuronActivation::Tanh, 1e-3, 1.0);

    auto out = layer.forward({0.5});
    double expected = std::tanh(0.5);
    expect(std::abs(out[0] - expected) < 1e-9, "short input vectors should be zero-padded up to n_in");
}

void test_bias_shifts_output() {
    std::vector<double> weights{0.0};
    std::vector<double> bias{0.25};
    OpAmpNeuronLayer layer(1, 1, weights, bias, NeuronActivation::Tanh, 1e-3, 1.0);

    auto out = layer.forward({10.0});
    double expected = std::tanh(0.25);
    expect(std::abs(out[0] - expected) < 1e-9, "with zero weight the output should equal tanh(bias)");
}

}  // namespace

int main() {
    test_forward_known_weights_tanh();
    test_forward_known_weights_sigmoid();
    test_forward_pads_short_input();
    test_bias_shifts_output();

    if (failures == 0) {
        std::printf("All OpAmpNeuronLayer tests passed.\n");
        return 0;
    }
    std::fprintf(stderr, "%d test(s) failed.\n", failures);
    return 1;
}
