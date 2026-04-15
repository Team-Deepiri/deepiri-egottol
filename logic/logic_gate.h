#pragma once

#include <functional>
#include <vector>
#include <string>
#include <unordered_map>

namespace deepiri {

enum class GateType {
    AND,
    OR,
    NOT,
    NAND,
    XOR,
    XNOR,
    NOR
};

struct LogicGate {
    GateType type;
    std::vector<size_t> inputs;
    size_t output;
    std::function<bool(const std::vector<bool>&)> func;

    bool evaluate() const;
    static std::function<bool(const std::vector<bool>&)> get_function(GateType type);
    static std::string gate_name(GateType type);
};

class GateLibrary {
public:
    static GateLibrary& instance();
    void register_gate(GateType type, std::function<bool(const std::vector<bool>&)> func);
    LogicGate create_gate(GateType type, const std::vector<size_t>& inputs, size_t output);

private:
    GateLibrary();
    std::unordered_map<GateType, std::function<bool(const std::vector<bool>&)>> functions_;
};

}