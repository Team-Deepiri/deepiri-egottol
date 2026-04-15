#ifndef DEEPIRI_INTEGRATOR_H
#define DEEPIRI_INTEGRATOR_H

#include <vector>
#include <functional>
#include <string>
#include <memory>

namespace deepiri {

enum class IntegratorType {
    Trapezoidal,
    Gear,
    Euler
};

class Integrator {
public:
    Integrator(IntegratorType type = IntegratorType::Trapezoidal);
    virtual ~Integrator() = default;

    virtual std::vector<double> step(
        const std::vector<double>& yCurrent,
        const std::vector<double>& dyCurrent,
        const std::vector<double>& dydt,
        double t,
        double h
    ) = 0;

    virtual void setOrder(int order) { order_ = order; }
    virtual int order() const { return order_; }
    IntegratorType type() const { return type_; }
    virtual void reset() {}
    virtual std::string name() const = 0;

protected:
    IntegratorType type_;
    int order_;
};

class TrapezoidalIntegrator : public Integrator {
public:
    TrapezoidalIntegrator();

    std::vector<double> step(
        const std::vector<double>& yCurrent,
        const std::vector<double>& dyCurrent,
        const std::vector<double>& dydt,
        double t,
        double h
    ) override;

    std::string name() const override { return "Trapezoidal"; }
};

class GearIntegrator : public Integrator {
public:
    GearIntegrator(int order = 2);

    std::vector<double> step(
        const std::vector<double>& yCurrent,
        const std::vector<double>& dyCurrent,
        const std::vector<double>& dydt,
        double t,
        double h
    ) override;

    void setOrder(int order) override;
    void reset() override;
    std::string name() const override { return "Gear"; }

private:
    std::vector<std::vector<double>> history_;
    int maxHistory_;
};

class EulerIntegrator : public Integrator {
public:
    EulerIntegrator();

    std::vector<double> step(
        const std::vector<double>& yCurrent,
        const std::vector<double>& dyCurrent,
        const std::vector<double>& dydt,
        double t,
        double h
    ) override;

    std::string name() const override { return "Euler"; }
};

std::unique_ptr<Integrator> createIntegrator(IntegratorType type);

}

#endif