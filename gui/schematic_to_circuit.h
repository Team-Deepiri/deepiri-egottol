#pragma once

#include <memory>

namespace deepiri {

class SchematicDocument;
class Circuit;

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
std::unique_ptr<Circuit> buildCircuitFromSchematic(const SchematicDocument& document);

} // namespace deepiri
