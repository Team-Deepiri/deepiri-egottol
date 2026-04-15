#pragma once

#include <string>
#include <vector>
#include <complex>
#include <map>

namespace deepiri {

struct QuantumGate {
    std::string name;
    std::vector<std::pair<int, int>> qubits;
    std::vector<double> parameters;
};

struct QuantumCircuit {
    std::string id;
    std::vector<QuantumGate> gates;
    int num_qubits;
    std::map<std::string, std::string> metadata;
};

struct TranspilerOptions {
    int optimization_level;
    bool layout_pass;
    bool routing_pass;
    bool synthesis_pass;
    std::string coupling_map;
};

class UQEBridge {
public:
    UQEBridge();
    ~UQEBridge();

    QuantumCircuit loadCircuit(const std::string& qasm);
    std::string exportCircuit(const QuantumCircuit& circuit);

    QuantumCircuit transpile(const QuantumCircuit& input, const TranspilerOptions& options);
    std::vector<std::string> getGateDecomposition(const std::string& gate);

    std::vector<double> estimateDepth(const QuantumCircuit& circuit);
    double estimateFidelity(const QuantumCircuit& circuit);

    void setBackend(const std::string& backend_name);
    std::string getBackend() const;

private:
    class Impl;
    struct Impl* impl_;
};

}