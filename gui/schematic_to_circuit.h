#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

#include <map>
#include <memory>

#include "../core/circuit.h"

namespace deepiri {

class SchematicDocument;

struct SchematicCircuitExport {
    std::unique_ptr<Circuit> circuit;
    std::map<std::string, size_t> solverNodeMap;
    QMap<QString, size_t> portNodes;
    QStringList nodeLabels;
    QString error;

    bool isValid() const { return circuit != nullptr && error.isEmpty(); }
};

/**
 * schematic_to_circuit.h — STAGE 5: Export schematic → simulation netlist.
 *
 * Critical: implement union-find net merging so wired ports share one node
 * (fixes the Python AdvancedMNASolver bug where each port is a separate node).
 *
 * TODO:
 *   1. collectNodes(document) — merge ports connected by SchematicWire
 *   2. pick ground from GND component port
 *   3. for each component, instantiate models::Resistor, Vsrc, etc.
 *   4. return deepiri::Circuit ready for MNASolver::solve
 */
SchematicCircuitExport
buildCircuitFromSchematic(const SchematicDocument& document);

} // namespace deepiri
