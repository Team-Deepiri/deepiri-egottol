#include "waveform_plotter.h"
#include "../io/simulation_data.h"

namespace deepiri {

class WaveformPlotter::Impl {
public:
  std::shared_ptr<SimulationData> data;
  std::vector<std::string> activeSignals;
};

WaveformPlotter::WaveformPlotter() : pImpl(std::make_unique<Impl>()) {}
WaveformPlotter::~WaveformPlotter() = default;

void WaveformPlotter::setData(std::shared_ptr<SimulationData> data) {
  pImpl->data = std::move(data);
}

void WaveformPlotter::addSignal(const std::string &name) {
  pImpl->activeSignals.push_back(name);
}

void WaveformPlotter::removeSignal(const std::string &name) {
  for (auto it = pImpl->activeSignals.begin(); it != pImpl->activeSignals.end();
       ++it) {
    if (*it == name) {
      pImpl->activeSignals.erase(it);
      break;
    }
  }
}

void WaveformPlotter::render() {}

} // namespace deepiri