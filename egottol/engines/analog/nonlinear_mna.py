"""Newton-Raphson modified nodal analysis with nonlinear device stamps."""

from __future__ import annotations

from typing import Dict, List, Optional, Tuple

import numpy as np

from egottol.models.base import Circuit, ComponentType

try:
    from egottol.engines.analog_compute.crossbar import CrossbarEngine

    _CROSSBAR_AVAILABLE = True
except ImportError:
    CrossbarEngine = None  # type: ignore[misc, assignment]
    _CROSSBAR_AVAILABLE = False

VT = 0.02585
MAX_NR_ITER = 50
NR_TOL = 1e-9
G_MIN = 1e-12


class NonlinearMNASolver:
    """Nonlinear MNA with Shockley diodes, memristors, op-amp VCVS, and crossbar coupling."""

    def __init__(self, circuit: Circuit, crossbar: Optional["CrossbarEngine"] = None):
        self.circuit = circuit
        self.node_map: Dict[str, int] = {}
        self.dim = 0
        self.crossbar = crossbar
        self._crossbars: Dict[str, "CrossbarEngine"] = {}
        self._memristor_state: Dict[str, float] = {}
        self._cap_state: Dict[str, float] = {}
        self._nr_voltages: Optional[np.ndarray] = None

    def _build_node_map(self) -> None:
        nodes = set()
        for wire in self.circuit.wires:
            nodes.add(wire.from_component + ":" + wire.from_port)
            nodes.add(wire.to_component + ":" + wire.to_port)
        sorted_nodes = sorted(nodes)
        gnd_node = next(
            (n for n in sorted_nodes if "GND" in n or ":G" in n),
            sorted_nodes[0] if sorted_nodes else None,
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
        return self.node_map.get(comp_id + ":" + port, -1)

    def _is_crossbar(self, comp) -> bool:
        return comp.name in ("Memristor Crossbar", "CROSSBAR") or (
            comp.type == ComponentType.ANALOG_COMPUTE and "Crossbar" in comp.name
        )

    def _init_crossbars(self) -> None:
        if not _CROSSBAR_AVAILABLE or CrossbarEngine is None:
            return
        for comp in self.circuit.components:
            if not self._is_crossbar(comp):
                continue
            rows = int(comp.parameters.get("rows", 4))
            cols = int(comp.parameters.get("cols", 4))
            if self.crossbar is not None and comp.id == getattr(self.crossbar, "comp_id", None):
                self._crossbars[comp.id] = self.crossbar
            elif comp.id not in self._crossbars:
                from egottol.engines.analog_compute.crossbar import CrossbarConfig

                cfg = CrossbarConfig(
                    rows=rows,
                    cols=cols,
                    ir_drop=float(comp.parameters.get("IR_drop", 0.01)),
                    read_noise_std=float(comp.parameters.get("read_noise", 0.0)),
                )
                self._crossbars[comp.id] = CrossbarEngine(rows=rows, cols=cols, config=cfg)

    def _memristor_conductance(self, comp) -> float:
        params = comp.parameters
        r_on = float(params.get("R_on", 100.0))
        r_off = float(params.get("R_off", 10000.0))
        w = self._memristor_state.get(comp.id, float(params.get("w", 0.5)))
        w = np.clip(w, 0.0, 1.0)
        r = r_on * w + r_off * (1.0 - w)
        return 1.0 / max(r, 1e-9)

    def _update_memristor_state(self, comp, v1: float, v2: float, dt: float) -> None:
        """Linear drift memristor: dw/dt = mu_v * R_on * i / D."""
        params = comp.parameters
        d = float(params.get("D", 10e-9))
        mu_v = float(params.get("mu_v", 1e-14))
        r_on = float(params.get("R_on", 100.0))
        w = self._memristor_state.get(comp.id, float(params.get("w", 0.5)))
        g = self._memristor_conductance(comp)
        i_m = g * (v1 - v2)
        dw = mu_v * r_on * i_m / max(d, 1e-18) * dt
        self._memristor_state[comp.id] = float(np.clip(w + dw, 0.0, 1.0))

    @staticmethod
    def _shockley_diode(v_d: float, i_s: float, n: float, vt: float = VT) -> Tuple[float, float]:
        """Return diode current and small-signal conductance for Newton linearization."""
        nv = n * vt
        if v_d < -5.0 * nv:
            i_d = -i_s
            g_d = i_s / nv
        else:
            exp_term = np.exp(v_d / nv)
            i_d = i_s * (exp_term - 1.0)
            g_d = (i_d + i_s) / nv
        return float(i_d), float(max(g_d, G_MIN))

    def _stamp_conductance(self, g: np.ndarray, n1: int, n2: int, conductance: float) -> None:
        for ni in (n1, n2):
            if ni >= 0:
                g[ni, ni] += conductance
        if n1 >= 0 and n2 >= 0:
            g[n1, n2] -= conductance
            g[n2, n1] -= conductance

    def _stamp_current(self, i_vec: np.ndarray, n1: int, n2: int, current: float) -> None:
        if n1 >= 0:
            i_vec[n1] -= current
        if n2 >= 0:
            i_vec[n2] += current

    def _count_aux(self) -> Tuple[int, int, int, List, List, List, List]:
        """Count voltage sources, VCVS (op-amps), and collect device metadata."""
        vsrc_list: List[Tuple[int, int, float]] = []
        vcvs_list: List[Tuple[int, int, int, int, float]] = []
        diode_list: List[Tuple[str, int, int, float, float]] = []
        mem_list: List[Tuple[str, int, int]] = []

        for comp in self.circuit.components:
            if comp.type == ComponentType.SOURCE or comp.name in ("Voltage Source", "VSource"):
                np_node = self._node(comp.id, "+")
                nn_node = self._node(comp.id, "−")
                if np_node < 0:
                    np_node = self._node(comp.id, "P")
                if nn_node < 0:
                    nn_node = self._node(comp.id, "N")
                v_dc = float(comp.parameters.get("v_dc", comp.parameters.get("V", 5.0)))
                vsrc_list.append((np_node, nn_node, v_dc))

            elif comp.name in ("Diode", "Shockley Diode", "Zener Diode", "Schottky Diode"):
                n_a = self._node(comp.id, "A")
                n_k = self._node(comp.id, "K")
                if n_a < 0:
                    n_a = self._node(comp.id, "1")
                if n_k < 0:
                    n_k = self._node(comp.id, "2")
                i_s = float(comp.parameters.get("Is", comp.parameters.get("I_s", 1e-14)))
                n = float(comp.parameters.get("n", 1.0))
                diode_list.append((comp.id, n_a, n_k, i_s, n))

            elif comp.name == "Memristor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                mem_list.append((comp.id, n1, n2))
                if comp.id not in self._memristor_state:
                    self._memristor_state[comp.id] = float(comp.parameters.get("w", 0.5))

            elif "Op-Amp" in comp.name or comp.name.startswith("OpAmp") or "OPAMP" in comp.name.upper():
                n_out = self._node(comp.id, "OUT")
                n_p = self._node(comp.id, "+")
                n_n = self._node(comp.id, "−")
                gain = float(comp.parameters.get("gain", comp.parameters.get("A", 1e5)))
                vcvs_list.append((n_out, 0, n_p, n_n, gain))

        return len(vsrc_list), len(vcvs_list), len(diode_list), vsrc_list, vcvs_list, diode_list, mem_list

    def _assemble_linear(
        self,
        v_guess: np.ndarray,
        dt: Optional[float] = None,
        cap_prev: Optional[Dict[str, float]] = None,
    ) -> Tuple[np.ndarray, np.ndarray, int]:
        """Build MNA matrix for current Newton iterate (linear + linearized nonlinear)."""
        n = self.dim
        nv, n_vcvs, _, vsrc_list, vcvs_list, diode_list, mem_list = self._count_aux()
        total = n + nv + n_vcvs
        g = np.zeros((total, total))
        i_vec = np.zeros(total)

        for comp in self.circuit.components:
            if comp.type == ComponentType.PASSIVE and comp.name == "Resistor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                r = float(comp.parameters.get("R", 1000.0))
                if r == 0:
                    r = 1e-9
                self._stamp_conductance(g[:n, :n], n1, n2, 1.0 / r)

            elif comp.type == ComponentType.PASSIVE and comp.name == "Capacitor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                c = float(comp.parameters.get("C", 1e-9))
                if dt is not None and dt > 0:
                    geq = c / dt
                    v_prev = 0.0
                    if cap_prev is not None:
                        v_prev = cap_prev.get(comp.id, self._cap_state.get(comp.id, 0.0))
                    self._stamp_conductance(g[:n, :n], n1, n2, geq)
                    i_eq = geq * v_prev
                    self._stamp_current(i_vec[:n], n1, n2, i_eq)
                else:
                    pass

            elif comp.type == ComponentType.PASSIVE and comp.name == "Inductor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                self._stamp_conductance(g[:n, :n], n1, n2, 1.0 / 1e-6)

        for comp_id, n_a, n_k, i_s, n_ideal in diode_list:
            v_a = v_guess[n_a] if n_a >= 0 else 0.0
            v_k = v_guess[n_k] if n_k >= 0 else 0.0
            v_d = v_a - v_k
            i_d, g_d = self._shockley_diode(v_d, i_s, n_ideal)
            i_eq = i_d - g_d * v_d
            self._stamp_conductance(g[:n, :n], n_a, n_k, g_d)
            self._stamp_current(i_vec[:n], n_a, n_k, i_eq)

        for comp_id, n1, n2 in mem_list:
            g_m = self._memristor_conductance(
                next(c for c in self.circuit.components if c.id == comp_id)
            )
            self._stamp_conductance(g[:n, :n], n1, n2, g_m)

        self._init_crossbars()
        for comp_id, cb in self._crossbars.items():
            g_stamp, i_stamp = cb.stamp_for_component(self.node_map, comp_id, n)
            g[:n, :n] += g_stamp
            i_vec[:n] += i_stamp

        for k, (np_node, nn_node, v_dc) in enumerate(vsrc_list):
            row = n + k
            if np_node >= 0:
                g[row, np_node] = 1.0
                g[np_node, row] = 1.0
            if nn_node >= 0:
                g[row, nn_node] = -1.0
                g[nn_node, row] = -1.0
            i_vec[row] = v_dc

        for k, (n_pos, n_neg, n_cp, n_cn, gain) in enumerate(vcvs_list):
            row = n + nv + k
            if n_pos >= 0:
                g[row, n_pos] = 1.0
                g[n_pos, row] = 1.0
            if n_neg >= 0:
                g[row, n_neg] = -1.0
                g[n_neg, row] = -1.0
            if n_cp >= 0:
                g[row, n_cp] -= gain
            if n_cn >= 0:
                g[row, n_cn] += gain
            i_vec[row] = 0.0

        g[0, :] = 0.0
        g[:, 0] = 0.0
        g[0, 0] = 1.0
        i_vec[0] = 0.0
        g += np.eye(total) * G_MIN

        return g, i_vec, total

    def _newton_solve(self, dt: Optional[float] = None, cap_prev: Optional[Dict[str, float]] = None) -> np.ndarray:
        n = self.dim
        if n == 0:
            return np.zeros(0)

        if self._nr_voltages is not None and self._nr_voltages.size >= n:
            v = self._nr_voltages[:n].copy()
        else:
            v = np.zeros(n)

        for comp in self.circuit.components:
            if comp.name in ("Diode", "Shockley Diode"):
                na = self._node(comp.id, "A")
                if na < 0:
                    na = self._node(comp.id, "1")
                if na >= 0:
                    v[na] = 0.7

        for _ in range(MAX_NR_ITER):
            g, i_vec, total = self._assemble_linear(v, dt=dt, cap_prev=cap_prev)
            x = np.linalg.solve(g, i_vec)
            v_new = x[:n]
            if np.max(np.abs(v_new - v)) < NR_TOL:
                v = v_new
                break
            v = v_new

        self._nr_voltages = x
        return v

    def solve_dc(self) -> Dict[str, float]:
        self._build_node_map()
        v = self._newton_solve()
        inv_map = {idx: name for name, idx in self.node_map.items()}
        return {inv_map[i]: float(v[i]) for i in range(len(v))}

    def solve_transient(self, t_stop: float, dt: float) -> List[Dict]:
        self._build_node_map()
        results: List[Dict] = []
        cap_prev: Dict[str, float] = {}

        self._newton_solve()
        for comp in self.circuit.components:
            if comp.name == "Capacitor":
                n1 = self._node(comp.id, "1")
                n2 = self._node(comp.id, "2")
                v = self._nr_voltages[: self.dim] if self._nr_voltages is not None else np.zeros(self.dim)
                v1 = v[n1] if n1 >= 0 else 0.0
                v2 = v[n2] if n2 >= 0 else 0.0
                cap_prev[comp.id] = v1 - v2
                self._cap_state[comp.id] = cap_prev[comp.id]

        t = 0.0
        while t < t_stop - 1e-15:
            v = self._newton_solve(dt=dt, cap_prev=cap_prev)
            inv_map = {idx: name for name, idx in self.node_map.items()}
            snapshot = {inv_map[i]: float(v[i]) for i in range(len(v))}

            for comp in self.circuit.components:
                if comp.name == "Capacitor":
                    n1 = self._node(comp.id, "1")
                    n2 = self._node(comp.id, "2")
                    v1 = v[n1] if n1 >= 0 else 0.0
                    v2 = v[n2] if n2 >= 0 else 0.0
                    cap_prev[comp.id] = v1 - v2
                    self._cap_state[comp.id] = cap_prev[comp.id]

                elif comp.name == "Memristor":
                    n1 = self._node(comp.id, "1")
                    n2 = self._node(comp.id, "2")
                    v1 = v[n1] if n1 >= 0 else 0.0
                    v2 = v[n2] if n2 >= 0 else 0.0
                    self._update_memristor_state(comp, v1, v2, dt)

            if _CROSSBAR_AVAILABLE and self._crossbars:
                for cb in self._crossbars.values():
                    cb.consume_stdp_delta()

            results.append({"t": float(t), "v": snapshot})
            t += dt

        return results

    def sync_crossbar_conductance(self, comp_id: str, crossbar: "CrossbarEngine") -> None:
        """Attach external crossbar engine and use its conductance matrix in MNA stamps."""
        self._crossbars[comp_id] = crossbar
