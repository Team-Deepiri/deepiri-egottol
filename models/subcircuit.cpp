#include "subcircuit.h"

namespace deepiri {

class Subcircuit::Impl {
public:
    std::string name;
    std::vector<std::string> ports;
    std::map<std::string, double> parameters;
    std::map<std::string, std::string> instances;
};

Subcircuit::Subcircuit(const std::string& name_) : pImpl(std::make_unique<Impl>()) {
    pImpl->name = name_;
}

Subcircuit::~Subcircuit() = default;

std::string Subcircuit::getName() const { return pImpl->name; }
void Subcircuit::setName(const std::string& name) { pImpl->name = name; }

void Subcircuit::addPort(const std::string& name) { pImpl->ports.push_back(name); }
std::vector<std::string> Subcircuit::getPorts() const { return pImpl->ports; }

void Subcircuit::addParameter(const std::string& name, double value) {
    pImpl->parameters[name] = value;
}

double Subcircuit::getParameter(const std::string& name) const {
    auto it = pImpl->parameters.find(name);
    if (it != pImpl->parameters.end()) return it->second;
    return 0.0;
}

std::vector<std::string> Subcircuit::getParameterNames() const {
    std::vector<std::string> names;
    for (const auto& p : pImpl->parameters) names.push_back(p.first);
    return names;
}

void Subcircuit::addInstance(const std::string& name, const std::string& type) {
    pImpl->instances[name] = type;
}

std::vector<std::string> Subcircuit::getInstanceNames() const {
    std::vector<std::string> names;
    for (const auto& i : pImpl->instances) names.push_back(i.first);
    return names;
}

std::string Subcircuit::getInstanceType(const std::string& name) const {
    auto it = pImpl->instances.find(name);
    if (it != pImpl->instances.end()) return it->second;
    return "";
}

}