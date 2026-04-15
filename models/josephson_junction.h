#pragma once

#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class JosephsonJunction {
public:
    JosephsonJunction();
    ~JosephsonJunction();

    void setParameters(double ic, double rn, double tc = 0.0);
    double getCriticalCurrent() const;
    double getNormalResistance() const;
    double getCriticalTemperature() const;

    std::string getName() const;
    void setName(const std::string& name);

    std::vector<int> getNodes() const;
    void setNodes(int n1, int n2);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}