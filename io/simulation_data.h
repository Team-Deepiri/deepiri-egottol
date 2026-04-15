#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace deepiri {

class SimulationData {
public:
    SimulationData();
    ~SimulationData();

    void setTimeRange(double start, double end, double step);
    void addTimePoint(double time);

    void addSignal(const std::string& name);
    void addSignalValue(const std::string& name, double value);

    double getStartTime() const;
    double getEndTime() const;
    double getStep() const;
    const std::vector<double>& getTimePoints() const;

    bool hasSignal(const std::string& name) const;
    const std::vector<double>* getSignal(const std::string& name) const;

    std::vector<std::string> getSignalNames() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}