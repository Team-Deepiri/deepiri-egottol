#pragma once

#include <memory>
#include <string>

namespace deepiri {

class SimulationData;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void show();
    void setSimulationData(std::shared_ptr<SimulationData> data);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}