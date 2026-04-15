#include "transmission_line.h"

namespace deepiri {

class TransmissionLine::Impl {
public:
    std::string name;
    double z0 = 50.0;
    double delay = 0.0;
    double loss = 0.0;
    std::vector<int> nodes;
};

TransmissionLine::TransmissionLine() : pImpl(std::make_unique<Impl>()) {}
TransmissionLine::~TransmissionLine() = default;

void TransmissionLine::setParameters(double z0, double delay, double loss) {
    pImpl->z0 = z0;
    pImpl->delay = delay;
    pImpl->loss = loss;
}

double TransmissionLine::getZ0() const { return pImpl->z0; }
double TransmissionLine::getDelay() const { return pImpl->delay; }
double TransmissionLine::getLoss() const { return pImpl->loss; }

std::string TransmissionLine::getName() const { return pImpl->name; }
void TransmissionLine::setName(const std::string& name) { pImpl->name = name; }

std::vector<int> TransmissionLine::getNodes() const { return pImpl->nodes; }
void TransmissionLine::setNodes(int n1, int n2) { pImpl->nodes = {n1, n2}; }

}