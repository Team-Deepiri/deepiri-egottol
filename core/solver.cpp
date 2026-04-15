#include "solver.h"
#include "circuit.h"
#include "../io/simulation_data.h"

namespace deepiri {

class Solver::Impl {
public:
    std::shared_ptr<Circuit> circuit;
    std::shared_ptr<SimulationData> results;
    double currentTime;
    bool converged;
};

Solver::Solver() : pImpl(std::make_unique<Impl>()) {
    pImpl->currentTime = 0.0;
    pImpl->converged = true;
}

Solver::~Solver() = default;

void Solver::setCircuit(std::shared_ptr<Circuit> circuit) {
    pImpl->circuit = std::move(circuit);
}

bool Solver::solve(double tStart, double tEnd, double step) {
    if (!pImpl->circuit) return false;
    
    pImpl->results = std::make_shared<SimulationData>();
    pImpl->results->setTimeRange(tStart, tEnd, step);
    
    pImpl->currentTime = tStart;
    pImpl->converged = true;
    
    while (pImpl->currentTime < tEnd) {
        pImpl->results->addTimePoint(pImpl->currentTime);
        pImpl->currentTime += step;
    }
    
    return pImpl->converged;
}

std::shared_ptr<SimulationData> Solver::getResults() const {
    return pImpl->results;
}

}