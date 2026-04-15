#include "subckt.h"

namespace deepiri {

class Subckt::Impl {
public:
    std::string name;
    std::vector<std::string> ports;
    std::map<std::string, double> parameters;
};

Subckt::Subckt(const std::string& name_) : pImpl(std::make_unique<Impl>()) {
    pImpl->name = name_;
}

Subckt::~Subckt() = default;

std::string Subckt::getName() const { return pImpl->name; }

void Subckt::addPort(const std::string& name) {
    pImpl->ports.push_back(name);
}

std::vector<std::string> Subckt::getPorts() const {
    return pImpl->ports;
}

void Subckt::addParameter(const std::string& name, double value) {
    pImpl->parameters[name] = value;
}

double Subckt::getParameter(const std::string& name) const {
    auto it = pImpl->parameters.find(name);
    if (it != pImpl->parameters.end()) return it->second;
    return 0.0;
}

std::vector<std::string> Subckt::getParameterNames() const {
    std::vector<std::string> names;
    for (const auto& p : pImpl->parameters) names.push_back(p.first);
    return names;
}

bool Subckt::hasParameter(const std::string& name) const {
    return pImpl->parameters.count(name) > 0;
}

}