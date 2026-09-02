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
- [x] AC/Bode small-signal solver — `core/ac_analysis.*` + production `core/spice_engine.*`.
- [ ] zepGPU bridge for matrix operations — Python sidecar (`egottol/engines/gpu_mesh.py`); optional post-1.0.
- [x] ADS-B packet decoder node — `avionics/adsb_decoder.cpp`.

### Phase 4: The Bridge
- [x] `to_uqe()` export logic — `infra/uqe_logic.*`.
- [x] Automated schematic to Mermaid export — `gui/schematic_mermaid.*`.

### Phase 5: Native Desktop App (added — see plan history for detail)
- [x] `gui/` compiles to a real, installable executable (`egottol_app`, CPack `.deb`/`.tar.gz`), not just a static library launched from Python.
- [x] Native `MainWindow`, `PropertyEditor`, `WaveformPlotter` (previously empty stubs).
- [x] Demo circuits run through the native `MNASolver`/`Transient` directly from the GUI — no Python in that loop.
- [x] Full schematic-to-netlist extraction — `gui/schematic_netlist.*` + `SimulationController`.
- [x] Native ports of the Python-only analog/AI/EII engines (`core/analog/`, `core/ai/`, `core/eii/`).
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

### ✅ Closed for 1.0 (was the gap; now shipped on this branch)

1. **Headless CLI** — `egottol-cli` (`sim`, `ee`, `version`) with production DC/tran/AC.
2. **Schematic → simulation** — `extractNetlistFromScene` + `DcOperatingPoint` / `SpiceTransient`; demos only on empty canvas.
3. **SPICE parser + builder** — standard device lines, units, `.tran/.ac/.op`, `.model`, nested `.subckt` via `expandedElements()`.
4. **IO live** — save/load, CSV export, goldens, design fixtures under `tests/fixtures/design/`.
5. **Python API** — `egottol.simulate`, EE knowledge `lookup_ee_design`, Copilot tool.
6. **Version** — CMake / CLI report 1.0.0; see CHANGELOG / SHIP.md.

### ⏳ Still open (post-1.0 polish, not blockers for ship)

1. mypy / ruff hygiene on the Python tree.
2. Phase 4: `to_uqe()` quantum export (optional).
3. Richer schematic device coverage (every ComponentType → SPICE).

---

## 8. Path-to-1.0 Roadmap

Ordered by dependency, not by coolness. Each phase ends shippable.

### Phase 6 — Close the loop — **DONE on feat/spice-production-engine**
- [x] **Fix `NetlistParser`**: SPICE syntax, control cards, unit suffixes; ctest goldens.
- [x] **Netlist → Device builder**: feeds production `DcOperatingPoint` / `SpiceTransient` / AC.
- [x] **Schematic → netlist extraction** wired into `SimulationController` (no silent demo when parts exist).
- [x] **Project save/load (.egt)** and waveform CSV export.
- [x] **Headless CLI** + EE design lookup (`egottol-cli ee …`).

### Phase 7 — Headless CLI + CI hygiene — **mostly done**
- [x] `egottol-cli` (native): `sim`, `ee`, exit codes for CI.
- [x] Python `egottol.simulate` / knowledge lookup.
- [ ] Benchmark runner wiring into CI; mypy/ruff gate; single version source.

### Phase 8 — Solver credibility — **core done on this branch**
- [x] Golden corpus + optional ngspice compare; design fixtures.
- [x] Convergence: gmin/source stepping; Level-1 MOSFET/BJT validation goldens.
- [ ] Broader LTspice curve library / CI ngspice required (optional today).
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
