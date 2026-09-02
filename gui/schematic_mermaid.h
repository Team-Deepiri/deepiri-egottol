#pragma once

#include <string>

namespace deepiri {

class SchematicScene;

// Emit a Mermaid flowchart from the schematic (components + wires).
std::string schematicToMermaid(const SchematicScene* scene);

}
