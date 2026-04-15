#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace deepiri {

class Subcircuit {
public:
    Subcircuit(const std::string& name);
    ~Subcircuit();

    std::string getName() const;
    void setName(const std::string& name);

    void addPort(const std::string& name);
    std::vector<std::string> getPorts() const;

    void addParameter(const std::string& name, double value);
    double getParameter(const std::string& name) const;
    std::vector<std::string> getParameterNames() const;

    void addInstance(const std::string& name, const std::string& type);
    std::vector<std::string> getInstanceNames() const;
    std::string getInstanceType(const std::string& name) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}