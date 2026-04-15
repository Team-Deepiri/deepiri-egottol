#include "vhdl_parser.h"
#include "subckt.h"

namespace deepiri {

class VHDLParser::Impl {
public:
    std::shared_ptr<Subckt> subckt;
    std::string error;
};

VHDLParser::VHDLParser() : pImpl(std::make_unique<Impl>()) {}
VHDLParser::~VHDLParser() = default;

bool VHDLParser::parse(const std::string& vhdlCode) {
    pImpl->subckt = std::make_shared<Subckt>("parsed");
    pImpl->error.clear();
    return !vhdlCode.empty();
}

std::shared_ptr<Subckt> VHDLParser::getSubckt() const {
    return pImpl->subckt;
}

std::string VHDLParser::getLastError() const {
    return pImpl->error;
}

}