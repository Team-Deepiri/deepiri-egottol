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

// Converts parsed netlist elements into Device instances ready for
// MNASolver / Transient / ACAnalysis. Ground nets: "0", "gnd", "ground".
BuiltCircuit buildCircuitFromNetlist(const NetlistParser& parser);
BuiltCircuit buildCircuitFromElements(const std::vector<NetlistElement>& elements);

}
