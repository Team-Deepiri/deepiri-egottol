#ifndef DEEPIRI_MOSFET_H
#define DEEPIRI_MOSFET_H

#include "device.h"
#include <string>
#include <cmath>

namespace deepiri {

enum class MOSFETType { NMOS, PMOS };

class MOSFETModel {
public:
    MOSFETModel() :
        vt0_(0.7), gamma_(0.0), phi_(0.6), lambda_(0.0),
        rsh_(0.0), cgs0_(0.0), cgd0_(0.0), cbs_(0.0), cbd_(0.0),
        m_(0.5), pbfactor_(1.0), ef_(0.0) {}

    double vt0_, gamma_, phi_, lambda_;
    double rsh_, cgs0_, cgd0_, cbs_, cbd_;
    double m_, pbfactor_, ef_;
};

class MOSFET : public Device {
public:
    MOSFET(MOSFETType type = MOSFETType::NMOS, double w = 10e-6, double l = 1e-5);
    MOSFET(const std::string& name, MOSFETType type = MOSFETType::NMOS, double w = 10e-6, double l = 1e-5);

    void setType(MOSFETType type) { type_ = type; }
    MOSFETType getMOSFETType() const { return type_; }
    void setWidth(double w) { W_ = w; }
    double width() const { return W_; }
    void setLength(double l) { L_ = l; }
    double length() const { return L_; }

    void setModel(const MOSFETModel& model) { model_ = model; }
    MOSFETModel& model() { return model_; }

    void initializeDC() override;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "MOSFET"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

    std::vector<size_t> terminals() const override;
    void setTerminals(const std::vector<size_t>& nodes) override;

private:
    MOSFETType type_;
    std::string name_;
    double W_, L_;
    MOSFETModel model_;

    double vgs_, vds_, vbs_, vth_, ids_, gds_, gm_, gmb_;
    double vt_, eps_ox_, eps_si_, q_, ni_, na_;

    void calculateIds(double vgs, double vds, double vbs);
    double calculateVth(double vbs);
    double calculateBeta(double vds, double vgs, double vth);
    double mobility(double vgs, double vth);
};

}

#endif