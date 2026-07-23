#pragma once

#include <vector>
#include <utility>

namespace deepiri {

class GilbertCell {
public:
    explicit GilbertCell(double k = 1e-3, double vLimit = 0.5);

    double multiply(double vx, double vy) const;
    std::vector<double> multiplyArray(const std::vector<double>& vx, const std::vector<double>& vy) const;
    std::pair<double, double> smallSignalGain(double vx, double vy) const;

    void setK(double k) { k_ = k; }
    double k() const { return k_; }
    void setVLimit(double vLimit) { vLimit_ = vLimit; }
    double vLimit() const { return vLimit_; }

private:
    double k_;
    double vLimit_;
};

}
