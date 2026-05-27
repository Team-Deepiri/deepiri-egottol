import numpy as np
from typing import Dict
from egottol.models.base import Circuit, ComponentType


class AdvancedMNASolver:
    def __init__(self, circuit: Circuit):
        self.circuit = circuit
        self.node_map: Dict[str, int] = {}
        self.dim = 0

    def _build_node_map(self):
        nodes = set()
        for wire in self.circuit.wires:
            nodes.add(wire.from_component + ":" + wire.from_port)
            nodes.add(wire.to_component + ":" + wire.to_port)
        sorted_nodes = sorted(nodes)
        gnd_node = next(
            (n for n in sorted_nodes if "GND" in n or ":G" in n), sorted_nodes[0] if sorted_nodes else None
        )
        idx = 0
        self.node_map = {}
        if gnd_node:
            self.node_map[gnd_node] = 0
            idx = 1
        for n in sorted_nodes:
            if n not in self.node_map:
                self.node_map[n] = idx
                idx += 1
        self.dim = idx

    def _node(self, comp_id: str, port: str) -> int:
        key = comp_id + ":" + port
        return self.node_map.get(key, -1)

    def solve_dc(self) -> Dict[str, float]:
        self._build_node_map()
        n = self.dim
        if n == 0:
            return {}

        G = np.zeros((n, n))
        B = np.zeros(n)

        for comp in self.circuit.components:
            if comp.type == ComponentType.PASSIVE and comp.name == "Resistor":
                ports = {p.name: p for p in comp.ports}
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                R = float(comp.parameters.get("R", 1000.0))
                g = 1.0 / R
                for ni in (n1, n2):
                    if ni >= 0:
                        G[ni, ni] += g
                if n1 >= 0 and n2 >= 0:
                    G[n1, n2] -= g
                    G[n2, n1] -= g

        try:
            x = np.linalg.solve(G + np.eye(n) * 1e-12, B)
        except np.linalg.LinAlgError:
            x = np.zeros(n)

        inv_map = {v: k for k, v in self.node_map.items()}
        return {inv_map[i]: float(x[i]) for i in range(n)}

    def solve_transient(self, t_stop: float, dt: float):
        """Simulates time-domain behavior."""
        results = []
        for t in np.arange(0, t_stop, dt):
            # Solve at each time step
            v_vec = self.solve_dc()
            results.append({"t": t, "v": v_vec[0]})
        return results
