#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace deepiri {

class Device;

class Node {
public:
    Node() : id(0), name("") {}
    Node(int id_, const std::string& name_) : id(id_), name(name_) {}
    int id;
    std::string name;
};

class Circuit {
public:
    Circuit();
    ~Circuit();

    int addNode(const std::string& name);
    const Node* getNode(int id) const;
    const Node* getNode(const std::string& name) const;

    void addDevice(std::shared_ptr<Device> device);
    std::vector<std::shared_ptr<Device>> getDevices() const;

    int getNodeCount() const;
    int getDeviceCount() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}