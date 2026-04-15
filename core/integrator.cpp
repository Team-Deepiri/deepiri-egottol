#include "integrator.h"

namespace deepiri {

Integrator::Integrator(IntegratorType type) : type_(type), order_(2) {}

TrapezoidalIntegrator::TrapezoidalIntegrator() : Integrator(IntegratorType::Trapezoidal) {}

std::vector<double> TrapezoidalIntegrator::step(
    const std::vector<double>& yCurrent,
    const std::vector<double>& dyCurrent,
    const std::vector<double>& dydt,
    double t,
    double h
) {
    std::vector<double> yNext(yCurrent.size());
    for (size_t i = 0; i < yCurrent.size(); ++i) {
        yNext[i] = yCurrent[i] + 0.5 * h * (dyCurrent[i] + dydt[i]);
    }
    return yNext;
}

GearIntegrator::GearIntegrator(int order)
    : Integrator(IntegratorType::Gear), maxHistory_(6) {
    setOrder(order);
}

void GearIntegrator::setOrder(int order) {
    order_ = std::max(1, std::min(order, maxHistory_));
    history_.clear();
}

void GearIntegrator::reset() {
    history_.clear();
}

std::vector<double> GearIntegrator::step(
    const std::vector<double>& yCurrent,
    const std::vector<double>& dyCurrent,
    const std::vector<double>& dydt,
    double t,
    double h
) {
    if (history_.empty()) {
        history_.push_back(yCurrent);
        history_.push_back(dyCurrent);
        return yCurrent;
    }

    size_t n = yCurrent.size();
    std::vector<double> yNext(n);

    if (history_.size() < static_cast<size_t>(order_)) {
        for (size_t i = 0; i < n; ++i) {
            yNext[i] = yCurrent[i] + h * dydt[i];
        }
        history_.push_back(yCurrent);
        return yNext;
    }

    static const double gearCoeffs[7][7] = {
        {1.0},
        {1.5, -0.5},
        {23.0/12.0, -4.0/3.0, 5.0/12.0},
        {55.0/24.0, -59.0/24.0, 37.0/24.0, -3.0/8.0},
        {323.0/120.0, -451.0/120.0, 281.0/120.0, -109.0/120.0, 13.0/60.0},
        {137.0/48.0, -59.0/12.0, 239.0/72.0, -53.0/72.0, 11.0/72.0, -1.0/48.0}
    };

    int m = order_;
    if (m > 6) m = 6;

    for (size_t i = 0; i < n; ++i) {
        double sum = gearCoeffs[m-1][0] * yCurrent[i];
        for (int k = 1; k < m; ++k) {
            if (k < static_cast<int>(history_.size())) {
                sum += gearCoeffs[m-1][k] * history_[history_.size() - k][i];
            }
        }
        yNext[i] = sum + h * dydt[i];
    }

    history_.push_back(yCurrent);
    if (static_cast<int>(history_.size()) > m) {
        history_.erase(history_.begin());
    }

    return yNext;
}

EulerIntegrator::EulerIntegrator() : Integrator(IntegratorType::Euler) {
    order_ = 1;
}

std::vector<double> EulerIntegrator::step(
    const std::vector<double>& yCurrent,
    const std::vector<double>& dyCurrent,
    const std::vector<double>& dydt,
    double t,
    double h
) {
    std::vector<double> yNext(yCurrent.size());
    for (size_t i = 0; i < yCurrent.size(); ++i) {
        yNext[i] = yCurrent[i] + h * dydt[i];
    }
    return yNext;
}

std::unique_ptr<Integrator> createIntegrator(IntegratorType type) {
    switch (type) {
        case IntegratorType::Trapezoidal:
            return std::make_unique<TrapezoidalIntegrator>();
        case IntegratorType::Gear:
            return std::make_unique<GearIntegrator>();
        case IntegratorType::Euler:
            return std::make_unique<EulerIntegrator>();
        default:
            return std::make_unique<TrapezoidalIntegrator>();
    }
}

}