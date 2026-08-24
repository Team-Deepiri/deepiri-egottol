# deepiri-egottol — Modern Binary Electrical Systems & Avionics Lab

**Copyright 2026 Deepiri — Apache 2.0**

> A "beyond-LTspice" simulation platform: Analog + Digital (VHDL) + GPU (zepGPU) + Airspace (ADS-B) + Quantum (uqe) bridge.

---

## 1. Core Architecture: The Hybrid Engine

Unlike standard LTspice which is analog-first, **egottol** uses a **Multi-Domain Solver**:

1.  **SPICE Engine (MNA)**: Modified Nodal Analysis for nonlinear analog components (Diodes, MOSFETs, Op-Amps).
2.  **Logic Engine (Event-Driven)**: High-speed digital simulation for basic gates, Flip-Flops, and VHDL-defined blocks.
3.  **GPU Solver (zepGPU)**: Massive parallelization for Matrix Stamping, FFT analysis, and SDR (ADS-B) decoding.
4.  **UQE Bridge**: Transpiles classical logic circuits into Quantum Gate sequences (e.g., XOR → CNOT, AND → Toffoli).

---

## 2. Component Ecosystem

### 🧱 Standard Library
- **Passive**: R, L, C, Transformers.
- **Active**: BJTs, MOSFETs, JFETs, IGBTs.
- **Logic**: AND, OR, NOT, XOR, NAND, Flip-Flops (SR, D, JK).
- **IC Blocks**: Op-Amps, Buck/Boost Controllers, DAC/ADC.

### 🚀 Experimental / Next-Gen
- **Neural Processor (NSP)**: AI-driven signal cleanup and classification.
- **ADS-B Node**: Real-time virtual transponder for airspace simulation.
- **SDR Node**: GPU-accelerated software-defined radio for RF-to-Digital simulation.
- **Synthetic Airspace**: Inject aircraft trajectories into the circuit.

---

## 3. UI/UX: Native Desktop (LTspice Style)

- **Framework**: PyQt6 (Native Python bindings for Qt) for high-performance canvas rendering.
- **Schematic Capture**: Drag-and-drop nodes, auto-routing wires, hierarchical blocks.
- **Waveform Viewer**: Built-in Oscilloscope + Spectrum Analyzer (FFT) with multi-trace math.
- **Command Terminal**: Embedded SPICE-like console (`.tran`, `.ac`, `.step`).

---

## 4. Integrations

### ⚡ zepGPU Integration
- Offloads heavy `.tran` (Transient Analysis) matrices to GPU workers.
- Parallelizes Parametric Sweeps (`.step`) across the cluster.
- Real-time ADS-B decoding using GPU-based FFT/Viterbi filters.

### 🌌 UQE (Universal Quantum Engine) Bridge
- **Output Mechanism**: `egottol` exports circuits as `QuantumCircuit` objects.
- **Mapping**:
  - `NOT` → `X` gate.
  - `CNOT` → Reversible `XOR`.
  - `Toffoli` → Reversible `AND`.
- Purpose: Test classical pre-processing logic for quantum algorithms.

---

## 5. Implementation Roadmap

### Phase 1: The Foundation
- [x] Logic Gate engine (Event-driven) — `logic/event_queue.*`, `logic/logic_gate.*`.
- [x] Poetry project setup + Pydantic models for Components.
- [x] VHDL Full Adder implementation — `vhdl/full_adder.vhd`, native `logic/vhdl_parser.*`.

### Phase 2: The UI (Schematic Editor)
- [x] PyQt6 Canvas for component dragging — `egottol/ui/main.py` (original Python app).
- [x] Wire routing engine — implemented natively in `gui/wire_tool.cpp`/`wire_item.cpp` too.
- [x] Component library palette — 220+ `ComponentType` entries in `gui/component_item.h`.

### Phase 3: The Solver
- [x] Basic MNA (Modified Nodal Analysis) solver — `core/mna_solver.*` (DC), `core/transient.*`.
- [ ] AC/Bode small-signal solver — in progress natively (`core/ac_analysis.*`), was Python-only.
- [ ] zepGPU bridge for matrix operations — Python-only (`egottol/engines/gpu_mesh.py`); kept as a networking sidecar, not ported to C++ (aiohttp/cloudpickle have no clean native equivalent).
- [x] ADS-B packet decoder node — `avionics/adsb_decoder.cpp`.

### Phase 4: The Bridge
- [ ] `to_uqe()` export logic — not yet started.
- [ ] Automated schematic to Mermaid export (vizult-style) — not yet started.

### Phase 5: Native Desktop App (added — see plan history for detail)
- [x] `gui/` compiles to a real, installable executable (`egottol_app`, CPack `.deb`/`.tar.gz`), not just a static library launched from Python.
- [x] Native `MainWindow`, `PropertyEditor`, `WaveformPlotter` (previously empty stubs).
- [x] Demo circuits run through the native `MNASolver`/`Transient` directly from the GUI — no Python in that loop.
- [ ] Full schematic-to-netlist extraction (Run button currently drives hardcoded demo circuits, not the drawn schematic) — not yet started.
- [ ] Native ports of the Python-only analog/AI/EII engines (`core/analog/`, `core/ai/`, `core/eii/`) — in progress.

---

## 6. Commands

```bash
# Start the native desktop app (C++/Qt6)
./build/egottol

# Start the legacy Python UI
poetry run python -m egottol.ui.main
```

> Note: the previously documented `python -m egottol.simulate`, `python -m egottol.export`,
> and `python -m egottol.benchmarks.eii.run` entry points do **not** exist yet — see the
> audit below. They are scheduled in Phase 7.

---

## 7. Product-Readiness Audit (2026-08-24, branch `feat/native-cpp-desktop-app`)

### ✅ What works (verified by deep test pass)

| Area | Status |
|------|--------|
| Native C++ build (CMake ≥3.16, C++20, Qt6) | Builds 100% clean, incl. CPack TGZ + DEB |
| Native test suite (`ctest`) | **12/12 pass** — core MNA/transient regression, AC, OTA, Gilbert cell, opamp neuron, noise, Hopfield, Ising, NSP, reservoir, EII pipeline, RTL shadow |
| Python test suite (`pytest`) | **59 passed, 1 skipped** (skip = optional onnx extra not installed) |
| GUI binary launches; Python UI imports | OK (verified offscreen) |
| Core solvers | DC operating point, transient, AC/Bode — all with passing regression tests |

### ❌ What's broken or missing (the actual gap to "finished product")

1. **No headless CLI at all.** The only `main()` is the GUI. No way to run a simulation from a terminal or script.
2. **The product loop is severed: schematic → simulation does not exist.** The GUI Run button executes four hardcoded demo circuits (`gui/simulation_controller.cpp`); nothing reads what the user drew.
3. **`io/netlist_parser.cpp` is broken.** It expects `R name n1 n2 value`; real SPICE is `R1 n1 n2 1k`. Every real netlist line parses as generic `Instance`. Unit suffixes (`1k`, `1u`, `100m`) are never converted to numeric values. `.tran` / `.ac` / `.dc` control cards are not recognized (only `.end/.include/.lib/.param/.option`).
4. **No NetlistElement → Device bridge.** Even a correct parse cannot feed `MNASolver` — no builder converts parsed elements into `models/` device objects.
5. **The entire IO layer is dead code** (0 consumers): `netlist_parser`, `project_loader`, `waveform_writer`, `symbol_library`. The GUI has **no save, no load, no export** — a drawn schematic cannot be persisted.
6. **Documented commands don't exist**: `egottol.simulate`, `egottol.export`, `egottol.benchmarks.eii.run`.
7. **mypy cannot run**: duplicate module name `eii` (`egottol/models/eii.py` vs `egottol/engines/eii/` package). Ruff reports 913 issues (543 auto-fixable).
8. **Version drift**: `pyproject.toml` says 0.1.0, CMake says 1.0.0.
9. Phase 4 items not started: `to_uqe()` quantum export, Mermaid export.

---

## 8. Path-to-1.0 Roadmap

Ordered by dependency, not by coolness. Each phase ends shippable.

### Phase 6 — Close the loop (the product only exists when this is done)
- [ ] **Fix `NetlistParser`**: accept standard SPICE (`R1 n1 n2 1k`) while keeping named-type form; recognize `.tran/.ac/.dc/.op/.step` control cards; parse unit suffixes (k/meg/M/u/n/p/G) to doubles. Add a dedicated ctest with real-world netlists.
- [ ] **Netlist → Device builder**: `NetlistParser` result → `std::vector<std::shared_ptr<Device>>` + node map, ready for `MNASolver`/`Transient`/`ACAnalysis`.
- [ ] **Schematic → netlist extraction** in `gui/`: serialize scene components/wires (via `component_item`/`wire_item`) into the parsed netlist form; wire it into `SimulationController` so Run simulates *what is drawn*.
- [ ] **Project save/load (.egt)**: JSON project format via `project_loader`; File → Save/Open in `MainWindow`. A schematic you can't save is not a product.
- [ ] **Waveform export**: CSV via `waveform_writer` hooked to `WaveformPlotter`.

### Phase 7 — Headless CLI + CI hygiene
- [ ] `egottol-cli` (native, links same libs as GUI): `egottol sim file.cir [--tran|--ac|--dc]`, `egottol export ...`. Exit codes for CI use.
- [ ] Console scripts: `python -m egottol.simulate` thin wrapper over pybind `_native` (or drop from docs until real).
- [ ] Benchmark runner: `egottol.benchmarks.eii.run` implementing the thermistor_classify benchmark from `benchmarks/eii/README.md`; wire both benchmarks into CI.
- [ ] Fix mypy duplicate-module error (rename `models/eii.py` or add `explicit-package-bases`); get mypy green.
- [ ] `ruff check --fix` the 543 auto-fixables; gate CI on ruff+mypy+clang-tidy.
- [ ] Unify version: single source of truth (CMake `project(... VERSION)` feeds `pyproject.toml` via release script).

### Phase 8 — Solver credibility ("beyond-LTspice" must first match LTspice)
- [ ] Golden-waveform regression suite vs LTspice/ngspice on ~20 reference circuits (RC/RLC, dividers, rectifiers, common-emitter, op-amp filters). Tolerance-based comparison automated in CI.
- [ ] Convergence hardening: gmin stepping / source stepping fallbacks in Newton-Raphson; document limits.
- [ ] MOSFET/BJT model validation against known device curves (levels, parameters actually honored).
- [ ] Performance baseline: benchmarks/ suite runs in CI with size tracking (matrix solve time vs node count).

### Phase 9 — Differentiators (only after Phase 8)
- [ ] UQE bridge: `to_uqe()` export (NOT→X, XOR→CNOT, AND→Toffoli) with round-trip tests.
- [ ] Schematic → Mermaid export.
- [ ] zepGPU bridge decision: port to native or formally keep as Python sidecar; document either way.
- [ ] ADS-B/SDR demo circuit end-to-end in the GUI (decoder exists and passes tests — surface it).

### Phase 10 — Release engineering
- [ ] CPack DEB dependencies audited on a clean container; AppImage or Flatpak for portable Linux; Windows/macOS story decided (CI already packages Python UI — reconcile with native app).
- [ ] README rewritten around verified commands only; screenshots of the native app.
- [ ] Tag v1.0.0 after Phases 6–8 are complete and CI is fully green.
