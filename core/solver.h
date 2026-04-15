#pragma once

#include <string>
#include <vector>
#include <memory>

namespace deepiri {

class Circuit;
class SimulationData;

class Solver {
public:
    Solver();
    ~Solver();

    void setCircuit(std::shared_ptr<Circuit> circuit);
    bool solve(double tStart, double tEnd, double step);
    std::shared_ptr<SimulationData> getResults() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}