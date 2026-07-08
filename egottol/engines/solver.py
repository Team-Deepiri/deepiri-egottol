import numpy as np
from typing import Dict
from egottol.models.base import Circuit, ComponentType
from egottol.native_bridge import native_available, solve_linear


class AdvancedMNASolver:
    def __init__(self, circuit: Circuit):
        self.circuit = circuit
        self.node_map: Dict[str, int] = {}
        self.dim = 0
        self.uses_native_core = native_available()

    def _build_node_map(self):
        nodes = set()
        for wire in self.circuit.wires:
            nodes.add(wire.from_component + ":" + wire.from_port)
            nodes.add(wire.to_component + ":" + wire.to_port)

        # Merge pins connected by wires into electrical nets.
        parent = {node: node for node in nodes}

        def find(node: str) -> str:
            while parent[node] != node:
                parent[node] = parent[parent[node]]
                node = parent[node]
            return node

        def union(a: str, b: str):
            ra = find(a)
            rb = find(b)
            if ra != rb:
                parent[rb] = ra

        for wire in self.circuit.wires:
            a = wire.from_component + ":" + wire.from_port
            b = wire.to_component + ":" + wire.to_port
            union(a, b)

        groups = {}
        for node in sorted(nodes):
            root = find(node)
            groups.setdefault(root, []).append(node)

        # Any net containing a ground-like pin is assigned index 0.
        ground_root = None
        for root, members in groups.items():
            if any("GND" in member or ":G" in member for member in members):
                ground_root = root
                break

        root_to_index = {}
        idx = 0
        if ground_root is not None:
            root_to_index[ground_root] = 0
            idx = 1

        for root in sorted(groups.keys()):
            if root not in root_to_index:
                root_to_index[root] = idx
                idx += 1

        self.node_map = {}
        for root, members in groups.items():
            node_idx = root_to_index[root]
            for member in members:
                self.node_map[member] = node_idx

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

        if self.uses_native_core:
            try:
                x = solve_linear(A, b)
            except Exception:
                x = np.linalg.solve(A, b)
        else:
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
