#include "schematic_to_circuit.h"
#include "schematic_document.h"
#include "../core/circuit.h"

#include <QtGlobal>

namespace deepiri {

std::unique_ptr<Circuit> buildCircuitFromSchematic(const SchematicDocument& document) {
    Q_UNUSED(document);
    // TODO Stage 5: full implementation
    return std::make_unique<Circuit>();
}

} // namespace deepiri
