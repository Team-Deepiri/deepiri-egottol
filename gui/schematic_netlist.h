#pragma once

#include <string>

namespace deepiri {

class SchematicScene;

struct SchematicNetlistResult {
    std::string netlist;
    std::string error;
    bool ok = false;
    int componentCount = 0;
    int netCount = 0;
};

// Walk the schematic scene (components + wires), assign nets by connectivity,
// and emit a SPICE netlist string suitable for NetlistParser.
SchematicNetlistResult extractNetlistFromScene(const SchematicScene* scene);

}
