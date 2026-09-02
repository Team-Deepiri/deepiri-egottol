#include "uqe_logic.h"

#include <map>
#include <sstream>

namespace deepiri {

namespace {

int allocQubit(std::map<std::string, int>& wireToQubit, int& next, const std::string& name) {
    auto it = wireToQubit.find(name);
    if (it != wireToQubit.end()) return it->second;
    int q = next++;
    wireToQubit[name] = q;
    return q;
}

QuantumGate makeGate(const std::string& name, int a, int b = -1, int c = -1) {
    QuantumGate g;
    g.name = name;
    if (b < 0) {
        g.qubits.push_back({a, -1});
    } else if (c < 0) {
        g.qubits.push_back({a, b});
    } else {
        // Toffoli: encode control0/control1 in first pair, target in second.
        g.qubits.push_back({a, b});
        g.qubits.push_back({c, -1});
    }
    return g;
}

}  // namespace

QuantumCircuit toUQE(const std::vector<LogicNet>& nets) {
    // Classical→UQE synthesis assumes:
    // - Fresh output qubits start in |0⟩ (AND/NAND Toffoli, OR via ancillas).
    // - OR decompositions allocate ancilla qubits (na, nb) that are not uncomputed;
    //   treat this as a resource estimate, not a fault-tolerant circuit.
    QuantumCircuit circuit;
    circuit.id = "classical_to_uqe";
    circuit.metadata["source"] = "to_uqe";

    std::map<std::string, int> wireToQubit;
    int nextQubit = 0;
    int ancilla = 0;

    for (const auto& net : nets) {
        switch (net.kind) {
            case LogicGateKind::NOT: {
                if (net.inputs.empty()) break;
                int inQ = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int outQ = allocQubit(wireToQubit, nextQubit, net.output);
                // Copy then X on output if distinct; if same wire, just X.
                if (inQ != outQ) {
                    circuit.gates.push_back(makeGate("cx", inQ, outQ));
                }
                circuit.gates.push_back(makeGate("x", outQ));
                break;
            }
            case LogicGateKind::XOR: {
                if (net.inputs.size() < 2) break;
                int a = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int b = allocQubit(wireToQubit, nextQubit, net.inputs[1]);
                int out = allocQubit(wireToQubit, nextQubit, net.output);
                if (out != b) {
                    circuit.gates.push_back(makeGate("cx", b, out));
                }
                circuit.gates.push_back(makeGate("cx", a, out));
                break;
            }
            case LogicGateKind::AND: {
                if (net.inputs.size() < 2) break;
                int a = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int b = allocQubit(wireToQubit, nextQubit, net.inputs[1]);
                int out = allocQubit(wireToQubit, nextQubit, net.output);
                // Toffoli a,b → out (assumes out starts |0⟩).
                circuit.gates.push_back(makeGate("ccx", a, b, out));
                break;
            }
            case LogicGateKind::NAND: {
                if (net.inputs.size() < 2) break;
                int a = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int b = allocQubit(wireToQubit, nextQubit, net.inputs[1]);
                int out = allocQubit(wireToQubit, nextQubit, net.output);
                circuit.gates.push_back(makeGate("ccx", a, b, out));
                circuit.gates.push_back(makeGate("x", out));
                break;
            }
            case LogicGateKind::OR: {
                // A∨B = ¬(¬A ∧ ¬B)
                if (net.inputs.size() < 2) break;
                int a = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int b = allocQubit(wireToQubit, nextQubit, net.inputs[1]);
                int out = allocQubit(wireToQubit, nextQubit, net.output);
                int na = nextQubit++;
                int nb = nextQubit++;
                (void)ancilla;
                circuit.gates.push_back(makeGate("cx", a, na));
                circuit.gates.push_back(makeGate("x", na));
                circuit.gates.push_back(makeGate("cx", b, nb));
                circuit.gates.push_back(makeGate("x", nb));
                circuit.gates.push_back(makeGate("ccx", na, nb, out));
                circuit.gates.push_back(makeGate("x", out));
                break;
            }
            case LogicGateKind::BUF: {
                if (net.inputs.empty()) break;
                int inQ = allocQubit(wireToQubit, nextQubit, net.inputs[0]);
                int outQ = allocQubit(wireToQubit, nextQubit, net.output);
                if (inQ != outQ) {
                    circuit.gates.push_back(makeGate("cx", inQ, outQ));
                }
                break;
            }
            default:
                break;
        }
    }

    circuit.num_qubits = nextQubit > 0 ? nextQubit : 1;
    return circuit;
}

QuantumCircuit toUQEFromNetlist(const std::string& classicalDescription) {
    // Tiny DSL: lines like "NOT a y" / "XOR a b y" / "AND a b y"
    std::vector<LogicNet> nets;
    std::istringstream iss(classicalDescription);
    std::string line;
    int idx = 0;
    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        std::string op, a, b, c;
        ls >> op;
        LogicNet net;
        net.name = "g" + std::to_string(idx++);
        if (op == "NOT" || op == "not") {
            ls >> a >> b;
            net.kind = LogicGateKind::NOT;
            net.inputs = {a};
            net.output = b;
        } else if (op == "XOR" || op == "xor") {
            ls >> a >> b >> c;
            net.kind = LogicGateKind::XOR;
            net.inputs = {a, b};
            net.output = c;
        } else if (op == "AND" || op == "and") {
            ls >> a >> b >> c;
            net.kind = LogicGateKind::AND;
            net.inputs = {a, b};
            net.output = c;
        } else if (op == "NAND" || op == "nand") {
            ls >> a >> b >> c;
            net.kind = LogicGateKind::NAND;
            net.inputs = {a, b};
            net.output = c;
        } else if (op == "OR" || op == "or") {
            ls >> a >> b >> c;
            net.kind = LogicGateKind::OR;
            net.inputs = {a, b};
            net.output = c;
        } else if (op == "BUF" || op == "buf") {
            ls >> a >> b;
            net.kind = LogicGateKind::BUF;
            net.inputs = {a};
            net.output = b;
        } else {
            continue;
        }
        nets.push_back(net);
    }
    return toUQE(nets);
}

std::string toOpenQasm(const QuantumCircuit& circuit) {
    std::ostringstream oss;
    oss << "OPENQASM 2.0;\n";
    oss << "include \"qelib1.inc\";\n";
    oss << "qreg q[" << circuit.num_qubits << "];\n";
    for (const auto& g : circuit.gates) {
        if (g.name == "x" && g.qubits.size() == 1) {
            oss << "x q[" << g.qubits[0].first << "];\n";
        } else if (g.name == "cx" && g.qubits.size() == 1 && g.qubits[0].second >= 0) {
            oss << "cx q[" << g.qubits[0].first << "],q[" << g.qubits[0].second << "];\n";
        } else if (g.name == "ccx" && g.qubits.size() == 2) {
            oss << "ccx q[" << g.qubits[0].first << "],q[" << g.qubits[0].second
                << "],q[" << g.qubits[1].first << "];\n";
        } else {
            oss << g.name << ";\n";
        }
    }
    return oss.str();
}

}
