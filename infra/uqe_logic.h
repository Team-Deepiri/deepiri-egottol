#pragma once

#include "uqe_bridge.h"

#include <string>
#include <vector>

namespace deepiri {

enum class LogicGateKind {
    NOT,
    AND,
    OR,
    XOR,
    NAND,
    NOR,
    XNOR,
    BUF
};

struct LogicNet {
    std::string name;
    LogicGateKind kind;
    std::vector<std::string> inputs;  // net / wire names
    std::string output;
};

// Classical → quantum mapping used by the UQE bridge:
//   NOT → X, XOR → CNOT, AND → Toffoli (with ancilla), BUF → identity wire.
QuantumCircuit toUQE(const std::vector<LogicNet>& nets);
QuantumCircuit toUQEFromNetlist(const std::string& classicalDescription);

// Round-trip helper used by tests: emit OpenQASM 2.0 and parse back.
std::string toOpenQasm(const QuantumCircuit& circuit);

}
