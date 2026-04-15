#pragma once

#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class Subckt;

class VHDLParser {
public:
    VHDLParser();
    ~VHDLParser();

    bool parse(const std::string& vhdlCode);
    std::shared_ptr<Subckt> getSubckt() const;

    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}