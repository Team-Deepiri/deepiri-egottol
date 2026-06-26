"""Small-signal AC analysis via complex modified nodal analysis."""

from __future__ import annotations

from typing import Any, Dict, List, Optional, Tuple

import numpy as np

from egottol.models.base import Circuit, ComponentType

try:
    from egottol.engines.solver import AdvancedMNASolver
except ImportError:
    AdvancedMNASolver = None  # type: ignore[misc, assignment]

try:
    from egottol.engines.analog.nonlinear_mna import NonlinearMNASolver, VT, G_MIN
except ImportError:
    NonlinearMNASolver = None  # type: ignore[misc, assignment]
    VT = 0.02585
    G_MIN = 1e-12


class ACAnalysisEngine:
    """Linearize at the DC operating point and sweep frequency for |H(jω)| and phase."""

    def __init__(self, circuit: Circuit, solver: Optional[Any] = None):
        self.circuit = circuit
        self.solver = solver if solver is not None else self._default_solver()

    def _default_solver(self) -> Any:
        if self._needs_nonlinear() and NonlinearMNASolver is not None:
            return NonlinearMNASolver(self.circuit)
        if AdvancedMNASolver is None:
            raise RuntimeError("No MNA solver available for AC analysis")
        return AdvancedMNASolver(self.circuit)

    def _needs_nonlinear(self) -> bool:
        nonlinear_names = {
            "Diode",
            "Shockley Diode",
            "Zener Diode",
            "Schottky Diode",
            "Memristor",
            "Memristor Crossbar",
            "CROSSBAR",
        }
        for comp in self.circuit.components:
            if comp.name in nonlinear_names:
                return True
            if "Op-Amp" in comp.name or comp.name.startswith("OpAmp") or "OPAMP" in comp.name.upper():
                return True
            if comp.type == ComponentType.ANALOG_COMPUTE and "Crossbar" in comp.name:
                return True
        return False

    def solve_ac(
        self,
        freq_start: float,
        freq_stop: float,
        points: int,
    ) -> Dict[str, Any]:
        """Sweep frequency and return Bode magnitude/phase for each node."""
        self._install_merged_node_map()
        dc_op = self.solver.solve_dc()
        dc_v = self._dc_voltage_vector(dc_op)
        freqs = self._frequency_sweep(freq_start, freq_stop, points)
        node_names = self._node_names()

        magnitude: Dict[str, np.ndarray] = {name: np.zeros(len(freqs)) for name in node_names}
        phase_deg: Dict[str, np.ndarray] = {name: np.zeros(len(freqs)) for name in node_names}
        ref_ac = self._reference_ac_magnitude()

        for fi, freq in enumerate(freqs):
            s = 1j * 2.0 * np.pi * freq
            v_ac = self._solve_at_frequency(s, dc_v)
            for ni, name in enumerate(node_names):
                h = v_ac[ni] / ref_ac if ref_ac != 0 else v_ac[ni]
                magnitude[name][fi] = float(np.abs(h))
                phase_deg[name][fi] = float(np.degrees(np.angle(h)))

        return {
            "frequencies": freqs,
            "dc_op": dc_op,
            "reference_ac_v": float(np.abs(ref_ac)),
            "nodes": {
                name: {"magnitude": magnitude[name], "phase_deg": phase_deg[name]}
                for name in node_names
            },
        }

    def _dc_voltage_vector(self, dc_op: Dict[str, float]) -> np.ndarray:
        n = self.solver.dim
        if hasattr(self.solver, "_nr_voltages") and self.solver._nr_voltages is not None:
            nr = self.solver._nr_voltages
            if nr.size >= n:
                return nr[:n].copy()

        inv_map = {idx: name for name, idx in self.solver.node_map.items()}
        v = np.zeros(n)
        for i in range(n):
            v[i] = float(dc_op.get(inv_map.get(i, ""), 0.0))
        return v

    def _node_names(self) -> List[str]:
        by_idx: Dict[int, List[str]] = {}
        for name, idx in self.solver.node_map.items():
            by_idx.setdefault(idx, []).append(name)
        return [sorted(by_idx.get(i, [f"node{i}"]))[0] for i in range(self.solver.dim)]

    def _node(self, comp_id: str, port: str) -> int:
        return self.solver._node(comp_id, port)

    @staticmethod
    def _frequency_sweep(freq_start: float, freq_stop: float, points_per_decade: int) -> np.ndarray:
        if freq_start <= 0 or freq_stop <= 0:
            raise ValueError("AC frequency endpoints must be positive")
        f0, f1 = (freq_start, freq_stop) if freq_stop >= freq_start else (freq_stop, freq_start)
        if np.isclose(f0, f1):
            return np.array([f0], dtype=float)
        decades = np.log10(f1 / f0)
        n = max(int(np.ceil(decades * max(points_per_decade, 1))) + 1, 2)
        return np.logspace(np.log10(f0), np.log10(f1), n)

    def _reference_ac_magnitude(self) -> complex:
        for comp in self.circuit.components:
            if comp.type == ComponentType.SOURCE or comp.name in ("Voltage Source", "VSource"):
                v_ac = comp.parameters.get(
                    "v_ac",
                    comp.parameters.get("V_ac", comp.parameters.get("ac", 1.0)),
                )
                return complex(float(v_ac), 0.0)
            if comp.name in ("Current Source", "ISRC"):
                i_ac = comp.parameters.get(
                    "i_ac",
                    comp.parameters.get("I_ac", comp.parameters.get("ac", 1.0)),
                )
                return complex(float(i_ac), 0.0)
        return complex(1.0, 0.0)

    def _install_merged_node_map(self) -> None:
        """Merge electrically connected ports (wires) into one node index per net."""
        node_map, dim = self._build_merged_node_map()

        def _build_node_map() -> None:
            self.solver.node_map = dict(node_map)
            self.solver.dim = dim

        self.solver._build_node_map = _build_node_map
        self.solver._build_node_map()

    def _build_merged_node_map(self) -> Tuple[Dict[str, int], int]:
        parent: Dict[str, str] = {}

        def find(node: str) -> str:
            parent.setdefault(node, node)
            if parent[node] != node:
                parent[node] = find(parent[node])
            return parent[node]

        def union(a: str, b: str) -> None:
            root_a, root_b = find(a), find(b)
            if root_a != root_b:
                parent[root_b] = root_a

        nodes: set[str] = set()
        for wire in self.circuit.wires:
            a = f"{wire.from_component}:{wire.from_port}"
            b = f"{wire.to_component}:{wire.to_port}"
            nodes.add(a)
            nodes.add(b)
            union(a, b)

        for comp in self.circuit.components:
            for port in comp.ports:
                key = f"{comp.id}:{port.name}"
                nodes.add(key)
                parent.setdefault(key, key)

        groups: Dict[str, List[str]] = {}
        for node in nodes:
            groups.setdefault(find(node), []).append(node)

        gnd_root = next(
            (
                root
                for root, members in groups.items()
                if any("GND" in member or ":G" in member for member in members)
            ),
            None,
        )

        node_map: Dict[str, int] = {}
        idx = 0
        if gnd_root is not None:
            for member in groups[gnd_root]:
                node_map[member] = 0
            idx = 1

        for root in sorted(groups):
            if root == gnd_root:
                continue
            for member in groups[root]:
                node_map[member] = idx
            idx += 1

        return node_map, idx

    def _collect_aux(self) -> Tuple[List, List, List, List]:
        vsrc_list: List[Tuple[int, int, complex]] = []
        isrc_list: List[Tuple[int, int, complex]] = []
        vcvs_list: List[Tuple[int, int, int, int, float]] = []
        diode_list: List[Tuple[int, int, float, float]] = []

        for comp in self.circuit.components:
            if comp.type == ComponentType.SOURCE or comp.name in ("Voltage Source", "VSource"):
                np_node = self._node(comp.id, "+")
                nn_node = self._node(comp.id, "−")
                if np_node < 0:
                    np_node = self._node(comp.id, "P")
                if nn_node < 0:
                    nn_node = self._node(comp.id, "N")
                v_ac = comp.parameters.get(
                    "v_ac",
                    comp.parameters.get("V_ac", comp.parameters.get("ac", 1.0)),
                )
                vsrc_list.append((np_node, nn_node, complex(float(v_ac), 0.0)))

            elif comp.name in ("Current Source", "ISRC"):
                np_node = self._node(comp.id, "+")
                nn_node = self._node(comp.id, "−")
                if np_node < 0:
                    np_node = self._node(comp.id, "P")
                if nn_node < 0:
                    nn_node = self._node(comp.id, "N")
                i_ac = comp.parameters.get(
                    "i_ac",
                    comp.parameters.get("I_ac", comp.parameters.get("ac", 0.0)),
                )
                isrc_list.append((np_node, nn_node, complex(float(i_ac), 0.0)))

            elif comp.name in ("Diode", "Shockley Diode", "Zener Diode", "Schottky Diode"):
                n_a = self._node(comp.id, "A")
                n_k = self._node(comp.id, "K")
                if n_a < 0:
                    n_a = self._node(comp.id, "1")
                if n_k < 0:
                    n_k = self._node(comp.id, "2")
                i_s = float(comp.parameters.get("Is", comp.parameters.get("I_s", 1e-14)))
                n_ideal = float(comp.parameters.get("n", 1.0))
                diode_list.append((n_a, n_k, i_s, n_ideal))

            elif "Op-Amp" in comp.name or comp.name.startswith("OpAmp") or "OPAMP" in comp.name.upper():
                n_out = self._node(comp.id, "OUT")
                n_p = self._node(comp.id, "+")
                n_n = self._node(comp.id, "−")
                gain = float(comp.parameters.get("gain", comp.parameters.get("A", 1e5)))
                vcvs_list.append((n_out, 0, n_p, n_n, gain))

        return vsrc_list, isrc_list, vcvs_list, diode_list

    @staticmethod
    def _shockley_conductance(v_d: float, i_s: float, n: float, vt: float = VT) -> float:
        nv = n * vt
        if v_d < -5.0 * nv:
            g_d = i_s / nv
        else:
            exp_term = np.exp(v_d / nv)
            i_d = i_s * (exp_term - 1.0)
            g_d = (i_d + i_s) / nv
        return float(max(g_d, G_MIN))

    def _memristor_conductance(self, comp) -> float:
        params = comp.parameters
        r_on = float(params.get("R_on", 100.0))
        r_off = float(params.get("R_off", 10000.0))
        w = float(params.get("w", 0.5))
        if hasattr(self.solver, "_memristor_state"):
            w = float(self.solver._memristor_state.get(comp.id, w))
        w = float(np.clip(w, 0.0, 1.0))
        r = r_on * w + r_off * (1.0 - w)
        return 1.0 / max(r, 1e-9)

    @staticmethod
    def _stamp_admittance(y: np.ndarray, n1: int, n2: int, admittance: complex) -> None:
        for ni in (n1, n2):
            if ni >= 0:
                y[ni, ni] += admittance
        if n1 >= 0 and n2 >= 0:
            y[n1, n2] -= admittance
            y[n2, n1] -= admittance

    @staticmethod
    def _stamp_current(i_vec: np.ndarray, n1: int, n2: int, current: complex) -> None:
        if n1 >= 0:
            i_vec[n1] -= current
        if n2 >= 0:
            i_vec[n2] += current

    def _assemble_ac(self, s: complex, dc_v: np.ndarray) -> Tuple[np.ndarray, np.ndarray, int]:
        n = self.solver.dim
        vsrc_list, isrc_list, vcvs_list, diode_list = self._collect_aux()
        nv = len(vsrc_list)
        n_vcvs = len(vcvs_list)
        total = n + nv + n_vcvs
        y = np.zeros((total, total), dtype=complex)
        i_vec = np.zeros(total, dtype=complex)

        for comp in self.circuit.components:
            if comp.type == ComponentType.PASSIVE and comp.name == "Resistor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                r = float(comp.parameters.get("R", 1000.0))
                if r == 0:
                    r = 1e-9
                self._stamp_admittance(y[:n, :n], n1, n2, 1.0 / r)

            elif comp.type == ComponentType.PASSIVE and comp.name == "Capacitor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                c = float(comp.parameters.get("C", 1e-9))
                if abs(s) > 0:
                    self._stamp_admittance(y[:n, :n], n1, n2, s * c)

            elif comp.type == ComponentType.PASSIVE and comp.name == "Inductor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                inductance = float(comp.parameters.get("L", 1e-6))
                if abs(s) > 0 and inductance > 0:
                    self._stamp_admittance(y[:n, :n], n1, n2, 1.0 / (s * inductance))

            elif comp.name == "Memristor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                self._stamp_admittance(y[:n, :n], n1, n2, self._memristor_conductance(comp))

        for n_a, n_k, i_s, n_ideal in diode_list:
            v_a = dc_v[n_a] if n_a >= 0 else 0.0
            v_k = dc_v[n_k] if n_k >= 0 else 0.0
            g_d = self._shockley_conductance(v_a - v_k, i_s, n_ideal)
            self._stamp_admittance(y[:n, :n], n_a, n_k, g_d)

        if hasattr(self.solver, "_crossbars") and getattr(self.solver, "_crossbars", None):
            self.solver._init_crossbars()
            for comp_id, cb in self.solver._crossbars.items():
                g_stamp, i_stamp = cb.stamp_for_component(self.solver.node_map, comp_id, n)
                y[:n, :n] += g_stamp
                i_vec[:n] += i_stamp

        for np_node, nn_node, i_ac in isrc_list:
            self._stamp_current(i_vec[:n], np_node, nn_node, i_ac)

        for k, (np_node, nn_node, v_ac) in enumerate(vsrc_list):
            row = n + k
            if np_node >= 0:
                y[row, np_node] = 1.0
                y[np_node, row] = 1.0
            if nn_node >= 0:
                y[row, nn_node] = -1.0
                y[nn_node, row] = -1.0
            i_vec[row] = v_ac

        for k, (n_pos, n_neg, n_cp, n_cn, gain) in enumerate(vcvs_list):
            row = n + nv + k
            if n_pos >= 0:
                y[row, n_pos] = 1.0
                y[n_pos, row] = 1.0
            if n_neg >= 0:
                y[row, n_neg] = -1.0
                y[n_neg, row] = -1.0
            if n_cp >= 0:
                y[row, n_cp] -= gain
            if n_cn >= 0:
                y[row, n_cn] += gain
            i_vec[row] = 0.0

        y[0, :] = 0.0
        y[:, 0] = 0.0
        y[0, 0] = 1.0
        i_vec[0] = 0.0
        y += np.eye(total, dtype=complex) * G_MIN

        return y, i_vec, total

    def _solve_at_frequency(self, s: complex, dc_v: np.ndarray) -> np.ndarray:
        n = self.solver.dim
        if n == 0:
            return np.zeros(0, dtype=complex)

        y, i_vec, total = self._assemble_ac(s, dc_v)
        try:
            x = np.linalg.solve(y, i_vec)
        except np.linalg.LinAlgError:
            x = np.zeros(total, dtype=complex)
        return x[:n]
