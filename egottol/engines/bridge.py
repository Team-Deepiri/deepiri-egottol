from typing import Any
from egottol.models.base import Circuit, ComponentType

class UQEBridge:
    """Transpiles classical egottol circuits into UQE QuantumCircuit objects."""
    
    def __init__(self, circuit: Circuit):
        self.circuit = circuit

    def to_quantum_circuit(self) -> str:
        """
        Converts the classical logic graph into an OpenQASM or UQE-compatible 
        sequence of gates.
        """
        gates = []
        for comp in self.circuit.components:
            if comp.type == ComponentType.LOGIC:
                if comp.name == "NOT":
                    gates.append(f"X({comp.id})")
                elif comp.name == "XOR":
                    gates.append(f"CNOT({comp.ports[0].name}, {comp.id})")
                elif comp.name == "AND":
                    gates.append(f"Toffoli(A, B, {comp.id})")
        
        return "\n".join(gates)

    def export_uqe(self, file_path: str):
        with open(file_path, "w") as f:
            f.write(self.to_quantum_circuit())
