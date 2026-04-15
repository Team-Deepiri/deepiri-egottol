#pragma once

#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class SimulationData;

class WaveformPlotter {
public:
    WaveformPlotter();
    ~WaveformPlotter();

    void setData(std::shared_ptr<SimulationData> data);
    void addSignal(const std::string& name);
    void removeSignal(const std::string& name);

    void render();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}