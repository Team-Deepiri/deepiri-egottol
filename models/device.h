#ifndef DEEPIRI_DEVICE_H
#define DEEPIRI_DEVICE_H

#include <vector>
#include <string>
#include <memory>

namespace deepiri {

class Device {
public:
    virtual ~Device() = default;

    virtual void initializeDC() {}
    virtual void getInitialGuess(std::vector<double>& guess) const {}
    virtual void updateState(const std::vector<double>& state) {}

    virtual std::vector<double> getCurrent() const = 0;
    virtual std::vector<std::vector<double>> getConductanceMatrix() const = 0;

    virtual std::string name() const = 0;
    virtual std::string type() const = 0;

    virtual void setTemperature(double temp) { temperature_ = temp; }
    virtual double temperature() const { return temperature_; }

    size_t nodeP() const { return nodeP_; }
    size_t nodeN() const { return nodeN_; }
    void setNodes(size_t p, size_t n) { nodeP_ = p; nodeN_ = n; }

protected:
    size_t nodeP_ = 0;
    size_t nodeN_ = 0;
    double temperature_ = 300.0;
};

}

#endif