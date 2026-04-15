#ifndef DEEPIRI_ISRC_H
#define DEEPIRI_ISRC_H

#include "device.h"
#include "vsrc.h"
#include <string>

namespace deepiri {

class Isrc : public Device {
public:
    Isrc(double dc = 1e-3);
    Isrc(const std::string& name, double dc = 1e-3);

    void setDC(double dc) { dcValue_ = dc; }
    double dc() const { return dcValue_; }
    void setAC(double ac) { acValue_ = ac; acPhase_ = 0.0; }
    void setAC(double ac, double phase) { acValue_ = ac; acPhase_ = phase; }
    void setPulse(double i1, double i2, double td, double tr, double tf, double pw, double period);
    void setType(SourceType type) { type_ = type; }

    void initializeDC() override;
    double getCurrentValue(double t) const;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Isrc"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    SourceType type_;
    double dcValue_;
    double acValue_, acPhase_;
    double pulseI1_, pulseI2_, pulseTd_, pulseTr_, pulseTf_, pulsePw_, pulsePeriod_;
};

}

#endif