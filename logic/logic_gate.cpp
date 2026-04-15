#include "logic_gate.h"

namespace deepiri {

bool LogicGate::evaluate() const {
    std::vector<bool> input_values;
    input_values.reserve(inputs.size());
    for (size_t id : inputs) {
        input_values.push_back(false);
    }
    return func(input_values);
}

std::function<bool(const std::vector<bool>&)> LogicGate::get_function(GateType type) {
    switch (type) {
        case GateType::AND:
            return [](const std::vector<bool>& in) {
                if (in.empty()) return false;
                for (bool v : in) if (!v) return false;
                return true;
            };
        case GateType::OR:
            return [](const std::vector<bool>& in) {
                for (bool v : in) if (v) return true;
                return false;
            };
        case GateType::NOT:
            return [](const std::vector<bool>& in) {
                return in.empty() || !in[0];
            };
        case GateType::NAND:
            return [](const std::vector<bool>& in) {
                if (in.empty()) return true;
                for (bool v : in) if (!v) return true;
                return false;
            };
        case GateType::XOR:
            return [](const std::vector<bool>& in) {
                int count = 0;
                for (bool v : in) if (v) count++;
                return count % 2 == 1;
            };
        case GateType::XNOR:
            return [](const std::vector<bool>& in) {
                int count = 0;
                for (bool v : in) if (v) count++;
                return count % 2 == 0;
            };
        case GateType::NOR:
            return [](const std::vector<bool>& in) {
                for (bool v : in) if (v) return false;
                return true;
            };
    }
    return [](const std::vector<bool>&) { return false; };
}

std::string LogicGate::gate_name(GateType type) {
    switch (type) {
        case GateType::AND: return "AND";
        case GateType::OR: return "OR";
        case GateType::NOT: return "NOT";
        case GateType::NAND: return "NAND";
        case GateType::XOR: return "XOR";
        case GateType::XNOR: return "XNOR";
        case GateType::NOR: return "NOR";
    }
    return "UNKNOWN";
}

GateLibrary& GateLibrary::instance() {
    static GateLibrary instance;
    return instance;
}

void GateLibrary::register_gate(GateType type, std::function<bool(const std::vector<bool>&)> func) {
    functions_[type] = func;
}

LogicGate GateLibrary::create_gate(GateType type, const std::vector<size_t>& inputs, size_t output) {
    LogicGate gate;
    gate.type = type;
    gate.inputs = inputs;
    gate.output = output;
    auto it = functions_.find(type);
    if (it != functions_.end()) {
        gate.func = it->second;
    } else {
        gate.func = LogicGate::get_function(type);
    }
    return gate;
}

GateLibrary::GateLibrary() {
    register_gate(GateType::AND, LogicGate::get_function(GateType::AND));
    register_gate(GateType::OR, LogicGate::get_function(GateType::OR));
    register_gate(GateType::NOT, LogicGate::get_function(GateType::NOT));
    register_gate(GateType::NAND, LogicGate::get_function(GateType::NAND));
    register_gate(GateType::XOR, LogicGate::get_function(GateType::XOR));
    register_gate(GateType::XNOR, LogicGate::get_function(GateType::XNOR));
    register_gate(GateType::NOR, LogicGate::get_function(GateType::NOR));
}

}