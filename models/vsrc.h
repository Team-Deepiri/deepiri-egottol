#ifndef DEEPIRI_VSRC_H
#define DEEPIRI_VSRC_H

#include "device.h"
#include <string>

namespace deepiri {

enum class SourceType { DC, AC, PULSE };

class Vsrc : public Device {
public:
    Vsrc(double dc = 1.0);
    Vsrc(const std::string& name, double dc = 1.0);

    void setDC(double dc) { dcValue_ = dc; }
    double dc() const { return dcValue_; }
    void setAC(double ac) { acValue_ = ac; acPhase_ = 0.0; }
    void setAC(double ac, double phase) { acValue_ = ac; acPhase_ = phase; }
    void setPulse(double v1, double v2, double td, double tr, double tf, double pw, double period);
    void setType(SourceType type) { type_ = type; }

    void initializeDC() override;
    double getVoltage(double t) const;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Vsrc"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;

private:
    std::string name_;
    SourceType type_;
    double dcValue_;
    double acValue_, acPhase_;
    double pulseV1_, pulseV2_, pulseTd_, pulseTr_, pulseTf_, pulsePw_, pulsePeriod_;
    double current_;
};

}

#endif