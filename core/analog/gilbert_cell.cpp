#include "gilbert_cell.h"
#include <algorithm>

namespace deepiri {

GilbertCell::GilbertCell(double k, double vLimit) : k_(k), vLimit_(vLimit) {}

double GilbertCell::multiply(double vx, double vy) const {
    double x = std::clamp(vx, -vLimit_, vLimit_);
    double y = std::clamp(vy, -vLimit_, vLimit_);
    return k_ * x * y;
}

std::vector<double> GilbertCell::multiplyArray(const std::vector<double>& vx, const std::vector<double>& vy) const {
    size_t n = std::min(vx.size(), vy.size());
    std::vector<double> out(n);
    for (size_t i = 0; i < n; ++i) {
        out[i] = multiply(vx[i], vy[i]);
    }
    return out;
}

std::pair<double, double> GilbertCell::smallSignalGain(double vx, double vy) const {
    double x = std::clamp(vx, -vLimit_, vLimit_);
    double y = std::clamp(vy, -vLimit_, vLimit_);
    return {k_ * y, k_ * x};
}

}
