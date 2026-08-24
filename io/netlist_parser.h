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
    int index = -1;
};

struct NetlistElement {
    NetlistElementType type = NetlistElementType::Instance;
    std::string name;
    std::vector<NetlistNode> nodes;
    std::vector<double> parameters;
    std::map<std::string, double> named_parameters;  // W=10u, VTO=0.7, …
    std::string model_name;
    std::string subckt_name;
};

// Parsed analysis / option directive (`.tran`, `.ac`, `.op`, …).
struct NetlistControl {
    std::string kind;   // lower-case: tran, ac, dc, op, step, end, …
    std::string raw;
    std::vector<std::string> tokens;  // tokens after the leading `.kind`
    std::vector<double> numbers;      // numeric args with unit suffixes applied
};

// `.model name type (PARAM=val …)`
struct SpiceModel {
    std::string name;
    std::string type;  // d, npn, pnp, nmos, pmos, …
    std::map<std::string, double> params;
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
    std::vector<NetlistControl> getControlDirectives() const;
    std::map<std::string, SpiceModel> getModels() const;

    std::string toNetlist() const;

    // SPICE engineering notation → double (`1k` → 1000, `4.7u` → 4.7e-6).
    static bool parseValue(const std::string& token, double& out);
    static int expectedNodeCount(NetlistElementType type);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}
