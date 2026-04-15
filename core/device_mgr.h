#pragma once

#include <memory>
#include <string>
#include <map>
#include <vector>

namespace deepiri {

class Device;

class DeviceMgr {
public:
    DeviceMgr();
    ~DeviceMgr();

    void registerDevice(const std::string& type, std::shared_ptr<Device> (*creator)());
    std::shared_ptr<Device> createDevice(const std::string& type) const;

    bool isRegistered(const std::string& type) const;
    std::vector<std::string> getRegisteredTypes() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}