#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace deepiri {

class Subckt {
public:
    Subckt(const std::string& name);
    ~Subckt();

    std::string getName() const;

    void addPort(const std::string& name);
    std::vector<std::string> getPorts() const;

    void addParameter(const std::string& name, double value);
    double getParameter(const std::string& name) const;
    std::vector<std::string> getParameterNames() const;

    bool hasParameter(const std::string& name) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}