#pragma once

#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class TransmissionLine {
public:
    TransmissionLine();
    ~TransmissionLine();

    void setParameters(double z0, double delay, double loss = 0.0);
    double getZ0() const;
    double getDelay() const;
    double getLoss() const;

    std::string getName() const;
    void setName(const std::string& name);

    std::vector<int> getNodes() const;
    void setNodes(int n1, int n2);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}