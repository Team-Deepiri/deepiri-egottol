#include "uqe_bridge.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace deepiri {

class UQEBridge::Impl {
public:
    std::string backend_;
    bool initialized_;

    Impl() : backend_("statevector"), initialized_(true) {}

    QuantumGate parseGate(const std::string& gate_str) {
        QuantumGate gate;
        gate.name = gate_str;
        return gate;
    }

    std::string gateToQASM(const QuantumGate& gate) {
        std::ostringstream oss;
        oss << gate.name << " ";
        for (size_t i = 0; i < gate.qubits.size(); ++i) {
            if (i > 0) oss << ",";
            if (gate.qubits[i].second >= 0) {
                oss << "q[" << gate.qubits[i].first << "],q[" << gate.qubits[i].second << "]";
            } else {
                oss << "q[" << gate.qubits[i].first << "]";
            }
        }
        return oss.str();
    }
};

UQEBridge::UQEBridge() : impl_(new Impl()) {}

UQEBridge::~UQEBridge() {
    delete impl_;
}

QuantumCircuit UQEBridge::loadCircuit(const std::string& qasm) {
    QuantumCircuit circuit;
    circuit.id = "loaded";
    circuit.num_qubits = 1;

    std::istringstream iss(qasm);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("qreg") != std::string::npos) {
            size_t pos = line.find('[');
            if (pos != std::string::npos) {
                circuit.num_qubits = std::stoi(line.substr(pos + 1));
            }
        } else if (line.find("gate") != std::string::npos || line.find("cx") != std::string::npos) {
            QuantumGate gate = impl_->parseGate(line);
            circuit.gates.push_back(gate);
        }
    }

    return circuit;
}

std::string UQEBridge::exportCircuit(const QuantumCircuit& circuit) {
    std::ostringstream oss;
    oss << "OPENQASM 2.0\n";
    oss << "qreg q[" << circuit.num_qubits << "]\n";

    for (const auto& gate : circuit.gates) {
        oss << impl_->gateToQASM(gate) << "\n";
    }

    return oss.str();
}

QuantumCircuit UQEBridge::transpile(const QuantumCircuit& input, const TranspilerOptions& options) {
    QuantumCircuit output = input;

    if (options.optimization_level > 0) {
        std::vector<QuantumGate> optimized_gates;
        for (const auto& gate : input.gates) {
            auto decomposed = getGateDecomposition(gate.name);
            for (const auto& decomp_gate : decomposed) {
                QuantumGate g = impl_->parseGate(decomp_gate);
                optimized_gates.push_back(g);
            }
        }
        output.gates = optimized_gates;
    }

    return output;
}

std::vector<std::string> UQEBridge::getGateDecomposition(const std::string& gate) {
    if (gate == "cz") {
        return {"h q[1]", "cx q[0],q[1]", "h q[1]"};
    } else if (gate == "swap") {
        return {"cx q[0],q[1]", "cx q[1],q[0]", "cx q[0],q[1]"};
    } else if (gate == "toffoli") {
        return {"h q[2]", "cx q[1],q[2]", "tdg q[2]", "cx q[0],q[2]", "t q[2]", "cx q[1],q[2]", "tdg q[2]", "cx q[0],q[2]", "t q[2]", "h q[2]"};
    }
    return {gate};
}

std::vector<double> UQEBridge::estimateDepth(const QuantumCircuit& circuit) {
    std::vector<double> depths;
    for (const auto& gate : circuit.gates) {
        depths.push_back(1.0);
    }
    return depths;
}

double UQEBridge::estimateFidelity(const QuantumCircuit& circuit) {
    double base_fidelity = 0.99;
    return std::pow(base_fidelity, circuit.gates.size());
}

void UQEBridge::setBackend(const std::string& backend_name) {
    impl_->backend_ = backend_name;
}

std::string UQEBridge::getBackend() const {
    return impl_->backend_;
}

}