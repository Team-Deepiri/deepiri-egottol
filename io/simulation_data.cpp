#include "simulation_data.h"

namespace deepiri {

class SimulationData::Impl {
public:
  double tStart = 0.0;
  double tEnd = 0.0;
  double step = 0.0;
  std::vector<double> timePoints;
  std::map<std::string, std::vector<double>> signals;
};

SimulationData::SimulationData() : pImpl(std::make_unique<Impl>()) {}
SimulationData::~SimulationData() = default;

void SimulationData::setTimeRange(double start, double end, double step) {
  pImpl->tStart = start;
  pImpl->tEnd = end;
  pImpl->step = step;
}

void SimulationData::addTimePoint(double time) {
  pImpl->timePoints.push_back(time);
}

void SimulationData::addSignal(const std::string &name) {
  pImpl->signals[name] = std::vector<double>();
}

void SimulationData::addSignalValue(const std::string &name, double value) {
  if (pImpl->signals.count(name)) {
    pImpl->signals[name].push_back(value);
  }
}

double SimulationData::getStartTime() const { return pImpl->tStart; }
double SimulationData::getEndTime() const { return pImpl->tEnd; }
double SimulationData::getStep() const { return pImpl->step; }

const std::vector<double> &SimulationData::getTimePoints() const {
  return pImpl->timePoints;
}

bool SimulationData::hasSignal(const std::string &name) const {
  return pImpl->signals.count(name) > 0;
}

const std::vector<double> *
SimulationData::getSignal(const std::string &name) const {
  auto it = pImpl->signals.find(name);
  if (it != pImpl->signals.end())
    return &it->second;
  return nullptr;
}

std::vector<std::string> SimulationData::getSignalNames() const {
  std::vector<std::string> names;
  for (const auto &s : pImpl->signals)
    names.push_back(s.first);
  return names;
}

} // namespace deepiri