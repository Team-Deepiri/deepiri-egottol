#pragma once

#include <cstdint>
#include <random>
#include <utility>
#include <vector>

#include "../matrix.h"

namespace deepiri {

struct ReservoirConfig {
    size_t n_reservoir = 100;
    double spectral_radius = 0.9;
    double leak_rate = 0.3;
    double input_scaling = 1.0;
    double sparsity = 0.1;
    double ridge_alpha = 1e-6;
    uint32_t seed = 42;
};

// Echo-state reservoir with a ridge-regression readout, ported from
// egottol/engines/ai/reservoir.py (EchoStateReservoir).
class Reservoir {
public:
    using Trace = std::vector<std::vector<double>>;  // [T][D]
    using Batch = std::vector<Trace>;                // [N][T][D]

    explicit Reservoir(const ReservoirConfig& config = ReservoirConfig());

    void reset();

    // Advances internal state through a single trace and returns the full
    // [T][n_reservoir] state trajectory. Mutates the carried-over hidden
    // state, matching reservoir.py's behavior where consecutive calls (e.g.
    // one per trace in a batch fit) continue from the previous trace's state
    // rather than resetting between them.
    std::vector<std::vector<double>> collectStates(const Trace& trace);

    // One state update per trace in the batch, taking only the final state
    // of each (mirrors _collect_states' ndim==3 branch).
    std::vector<std::vector<double>> collectFinalStates(const Batch& traces);

    void fit(const Batch& traces, const std::vector<int>& labels);
    void fit(const Batch& traces, const std::vector<std::vector<double>>& targets);

    std::vector<double> predict(const Trace& trace);
    std::pair<std::vector<double>, double> infer(const Trace& trace);
    std::pair<std::vector<double>, double> inferEmbedding(const std::vector<double>& z);

    bool isTrained() const { return readoutTrained_; }

private:
    ReservoirConfig config_;
    std::mt19937 rng_;
    Matrix wRes_;
    Matrix wIn_;
    bool wInInitialized_ = false;
    std::vector<double> state_;
    Matrix readout_;
    bool readoutTrained_ = false;
    std::vector<int> classLabels_;

    void initReservoir();
    void ensureInputWeights(size_t inputDim);
    void fitOnStates(const std::vector<std::vector<double>>& states,
                      const std::vector<std::vector<double>>& targets);
};

}  // namespace deepiri
