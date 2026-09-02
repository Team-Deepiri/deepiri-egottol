#ifndef DEEPIRI_ISRC_H
#define DEEPIRI_ISRC_H

#include "device.h"
#include "source_signal.h"
#include <string>

namespace deepiri {

class Isrc : public Device {
public:
    Isrc(double dc = 1e-3);
    Isrc(const std::string& name, double dc = 1e-3);

    void setDC(double dc) { signal_.dc = dc; signal_.kind = SourceWaveform::DC; time_ = 0.0; }
    double dc() const { return signal_.dc; }
    void setAC(double ac) { signal_.acMag = ac; signal_.acPhaseDeg = 0.0; }
    void setAC(double ac, double phase) { signal_.acMag = ac; signal_.acPhaseDeg = phase; }
    void setPulse(double i1, double i2, double td, double tr, double tf, double pw, double period);
    void setSin(double io, double ia, double freq, double td = 0, double theta = 0, double phase = 0);
    void setExp(double i1, double i2, double td1, double tau1, double td2, double tau2);
    void setPwl(const std::vector<double>& times, const std::vector<double>& values);
    void setSignal(const SourceSignal& s) { signal_ = s; }
    const SourceSignal& signal() const { return signal_; }
    void setType(SourceWaveform type) { signal_.kind = type; }

    void initializeDC() override;
    double getCurrentValue(double t) const;
    std::vector<double> getCurrent() const override;
    std::vector<std::vector<double>> getConductanceMatrix() const override;

    std::string name() const override { return name_; }
    std::string type() const override { return "Isrc"; }

    void getInitialGuess(std::vector<double>& guess) const override;
    void updateState(const std::vector<double>& state) override;
    void setAnalysisTime(double tSec) override { time_ = tSec; }

private:
    std::string name_;
    SourceSignal signal_;
    double time_ = 0.0;
};

}

#endif
