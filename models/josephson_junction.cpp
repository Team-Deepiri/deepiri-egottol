#include "josephson_junction.h"

namespace deepiri {

class JosephsonJunction::Impl {
public:
    std::string name;
    double ic = 0.0;
    double rn = 0.0;
    double tc = 0.0;
    std::vector<int> nodes;
};

JosephsonJunction::JosephsonJunction() : pImpl(std::make_unique<Impl>()) {}
JosephsonJunction::~JosephsonJunction() = default;

void JosephsonJunction::setParameters(double ic, double rn, double tc) {
    pImpl->ic = ic;
    pImpl->rn = rn;
    pImpl->tc = tc;
}

double JosephsonJunction::getCriticalCurrent() const { return pImpl->ic; }
double JosephsonJunction::getNormalResistance() const { return pImpl->rn; }
double JosephsonJunction::getCriticalTemperature() const { return pImpl->tc; }

std::string JosephsonJunction::getName() const { return pImpl->name; }
void JosephsonJunction::setName(const std::string& name) { pImpl->name = name; }

std::vector<int> JosephsonJunction::getNodes() const { return pImpl->nodes; }
void JosephsonJunction::setNodes(int n1, int n2) { pImpl->nodes = {n1, n2}; }

}