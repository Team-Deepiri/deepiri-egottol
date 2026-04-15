#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace deepiri {

enum class NetlistElementType {
    Resistor,
    Capacitor,
    Inductor,
    VoltageSource,
    CurrentSource,
    MOSFET,
    BJT,
    Diode,
    Subckt,
    Instance
};

struct NetlistNode {
    std::string name;
    std::string net;
    int index;
};

struct NetlistElement {
    NetlistElementType type;
    std::string name;
    std::vector<NetlistNode> nodes;
    std::vector<double> parameters;
    std::string model_name;
    std::string subckt_name;
};

class NetlistParser {
public:
    NetlistParser();
    ~NetlistParser();

    bool parse(const std::string& netlist_content);
    bool loadFromFile(const std::string& filename);

    std::vector<NetlistElement> getElements() const;
    std::map<std::string, std::vector<std::string>> getNets() const;
    std::vector<std::string> getControls() const;

    std::string toNetlist() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}