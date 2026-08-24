#pragma once

#include "netlist_parser.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace deepiri {

class Device;

struct BuiltCircuit {
    std::vector<std::shared_ptr<Device>> devices;
    std::map<std::string, size_t> nodeMap;  // net name → 1-based index (0 = ground)
    size_t numNodes = 0;                    // highest node index
    std::string error;
    bool ok = false;
};

BuiltCircuit buildCircuitFromNetlist(const NetlistParser& parser);
BuiltCircuit buildCircuitFromElements(
    const std::vector<NetlistElement>& elements,
    const std::map<std::string, SpiceModel>& models = {}
);

// Parse `.ic` / `.nodeset` cards → net name → voltage.
std::map<std::string, double> parseNodeVoltagesFromControls(
    const std::vector<NetlistControl>& controls,
    const char* kind  // "ic" or "nodeset"
);

// Map named IC voltages onto the circuit's 0-based voltage vector (size = numNodes).
std::vector<double> initialConditionVector(
    const BuiltCircuit& circuit,
    const std::map<std::string, double>& named
);

// Find a Vsrc/Isrc by element name (case-insensitive).
std::shared_ptr<Device> findSourceByName(
    const BuiltCircuit& circuit,
    const std::string& name
);

}
