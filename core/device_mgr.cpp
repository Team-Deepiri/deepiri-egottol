#include "device_mgr.h"
#include "circuit.h"

namespace deepiri {

class DeviceMgr::Impl {
public:
    std::map<std::string, std::shared_ptr<Device> (*)()> creators;
};

DeviceMgr::DeviceMgr() : pImpl(std::make_unique<Impl>()) {}
DeviceMgr::~DeviceMgr() = default;

void DeviceMgr::registerDevice(const std::string& type, std::shared_ptr<Device> (*creator)()) {
    pImpl->creators[type] = creator;
}

std::shared_ptr<Device> DeviceMgr::createDevice(const std::string& type) const {
    auto it = pImpl->creators.find(type);
    if (it != pImpl->creators.end()) return it->second();
    return nullptr;
}

bool DeviceMgr::isRegistered(const std::string& type) const {
    return pImpl->creators.count(type) > 0;
}

std::vector<std::string> DeviceMgr::getRegisteredTypes() const {
    std::vector<std::string> types;
    for (const auto& c : pImpl->creators) types.push_back(c.first);
    return types;
}

}