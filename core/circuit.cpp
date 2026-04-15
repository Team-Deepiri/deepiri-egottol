#include "circuit.h"

namespace deepiri {

class Device {
public:
    virtual ~Device() = default;
    virtual std::string getName() const = 0;
    virtual std::string getType() const = 0;
};

class Circuit::Impl {
public:
    std::vector<Node> nodes;
    std::map<std::string, int> nodeMap;
    std::vector<std::shared_ptr<Device>> devices;
};

Circuit::Circuit() : pImpl(std::make_unique<Impl>()) {}
Circuit::~Circuit() = default;

int Circuit::addNode(const std::string& name) {
    if (pImpl->nodeMap.count(name)) return pImpl->nodeMap[name];
    int id = static_cast<int>(pImpl->nodes.size());
    pImpl->nodes.emplace_back(id, name);
    pImpl->nodeMap[name] = id;
    return id;
}

const Node* Circuit::getNode(int id) const {
    if (id >= 0 && id < static_cast<int>(pImpl->nodes.size()))
        return &pImpl->nodes[id];
    return nullptr;
}

const Node* Circuit::getNode(const std::string& name) const {
    auto it = pImpl->nodeMap.find(name);
    if (it != pImpl->nodeMap.end()) return &pImpl->nodes[it->second];
    return nullptr;
}

void Circuit::addDevice(std::shared_ptr<Device> device) {
    pImpl->devices.push_back(std::move(device));
}

std::vector<std::shared_ptr<Device>> Circuit::getDevices() const {
    return pImpl->devices;
}

int Circuit::getNodeCount() const { return static_cast<int>(pImpl->nodes.size()); }
int Circuit::getDeviceCount() const { return static_cast<int>(pImpl->devices.size()); }

}