#include "cccs.h"

namespace deepiri {

CCCS::CCCS(const std::string& name, double gain)
    : name_(name), gain_(gain) {}

std::vector<std::vector<double>> CCCS::getConductanceMatrix() const {
    // Coupling to sense-branch current is stamped in spice_engine / mna_solver.
    return std::vector<std::vector<double>>(2, std::vector<double>(2, 0.0));
}

}
