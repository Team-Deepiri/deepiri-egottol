import numpy as np
import pytest

from egottol.engines.solver import AdvancedMNASolver
from egottol.models.base import Circuit, Component, ComponentType, Wire
from egottol.native_bridge import native_available, solve_linear


def test_native_module_imports_when_built():
    if not native_available():
        pytest.skip("egottol._native not built (run cmake with -DBUILD_PYTHON_BINDINGS=ON)")
    import egottol._native as native

    assert native.available()
    assert native.core_version()


def test_native_solve_linear_matches_numpy():
    if not native_available():
        pytest.skip("egottol._native not built")

    rng = np.random.default_rng(0)
    n = 5
    a = rng.standard_normal((n, n))
    a = a @ a.T + np.eye(n) * 10.0
    b = rng.standard_normal(n)

    x_native = solve_linear(a, b)
    x_numpy = np.linalg.solve(a, b)
    np.testing.assert_allclose(x_native, x_numpy, rtol=1e-10, atol=1e-10)


def test_advanced_mna_solver_uses_native_when_available():
    circuit = Circuit(
        id="t1",
        name="divider",
        components=[
            Component(
                id="v1",
                name="Voltage Source",
                type=ComponentType.SOURCE,
                parameters={"v_dc": 10.0},
            ),
            Component(
                id="r1",
                name="Resistor",
                type=ComponentType.PASSIVE,
                parameters={"R": 1000.0},
            ),
            Component(
                id="r2",
                name="Resistor",
                type=ComponentType.PASSIVE,
                parameters={"R": 1000.0},
            ),
            Component(id="gnd", name="GND", type=ComponentType.POWER),
        ],
        wires=[
            Wire(id="w1", from_component="v1", from_port="+", to_component="r1", to_port="1"),
            Wire(id="w2", from_component="v1", from_port="−", to_component="gnd", to_port="G"),
            Wire(id="w3", from_component="r1", from_port="2", to_component="r2", to_port="1"),
            Wire(id="w4", from_component="r2", from_port="2", to_component="gnd", to_port="G"),
        ],
    )

    solver = AdvancedMNASolver(circuit)
    result = solver.solve_dc()

    if native_available():
        assert solver.uses_native_core is True

    assert result
    mid_node = next(k for k in result if "r1:2" in k or "r2:1" in k)
    assert abs(result[mid_node] - 5.0) < 0.5
