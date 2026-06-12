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
        I = np.zeros(n)
        vsrc_list = []

        for comp in self.circuit.components:
            if comp.type == ComponentType.PASSIVE and comp.name == "Resistor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                R = float(comp.parameters.get("R", 1000.0))
                if R == 0:
                    R = 1e-9
                g = 1.0 / R
                for ni in (n1, n2):
                    if ni >= 0:
                        G[ni, ni] += g
                if n1 >= 0 and n2 >= 0:
                    G[n1, n2] -= g
                    G[n2, n1] -= g

            elif comp.type == ComponentType.PASSIVE and comp.name == "Capacitor":
                pass

            elif comp.type == ComponentType.PASSIVE and comp.name == "Inductor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                g = 1.0 / 1e-6
                for ni in (n1, n2):
                    if ni >= 0:
                        G[ni, ni] += g
                if n1 >= 0 and n2 >= 0:
                    G[n1, n2] -= g
                    G[n2, n1] -= g

            elif comp.type == ComponentType.SOURCE or comp.name in ("Voltage Source", "VSource"):
                np_node = self._node(comp.id, "+")
                nn_node = self._node(comp.id, "−")
                v_dc = float(comp.parameters.get("v_dc", comp.parameters.get("V", 5.0)))
                vsrc_list.append((np_node, nn_node, v_dc))

        nv = len(vsrc_list)
        total = n + nv
        A = np.zeros((total, total))
        b = np.zeros(total)
        A[:n, :n] = G
        b[:n] = I
        for k, (np_node, nn_node, v_dc) in enumerate(vsrc_list):
            row = n + k
            if np_node >= 0:
                A[row, np_node] = 1
                A[np_node, row] = 1
            if nn_node >= 0:
                A[row, nn_node] = -1
                A[nn_node, row] = -1
            b[row] = v_dc

        A[0, :] = 0
        A[:, 0] = 0
        A[0, 0] = 1
        b[0] = 0
        A += np.eye(total) * 1e-12

        try:
            x = np.linalg.solve(A, b)
        except np.linalg.LinAlgError:
            x = np.zeros(total)

        inv_map = {v: k for k, v in self.node_map.items()}
        return {inv_map[i]: float(x[i]) for i in range(n)}

    def solve_transient(self, t_stop: float, dt: float):
        results = []
        base = self.solve_dc()
        for t in np.arange(0, t_stop, dt):
            results.append({"t": float(t), "v": base})
        return results
