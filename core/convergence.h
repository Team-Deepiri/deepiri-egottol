#ifndef DEEPIRI_CONVERGENCE_H
#define DEEPIRI_CONVERGENCE_H

#include <algorithm>
#include <cmath>
#include <vector>

namespace deepiri {

inline bool checkConvergence(
    const std::vector<double>& state,
    const std::vector<double>& prevState,
    double tolerance
) {
    double maxDiff = 0.0;
    size_t n = std::min(state.size(), prevState.size());
    for (size_t i = 0; i < n; ++i) {
        double diff = std::abs(state[i] - prevState[i]);
        if (diff > maxDiff) maxDiff = diff;
    }
    return maxDiff < tolerance;
}

}  // namespace deepiri

#endif
