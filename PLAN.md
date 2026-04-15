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
- [ ] Logic Gate engine (Event-driven).
- [ ] Poetry project setup + Pydantic models for Components.
- [ ] VHDL Full Adder implementation.

### Phase 2: The UI (Schematic Editor)
- [ ] PyQt6 Canvas for component dragging.
- [ ] Wire routing engine.
- [ ] Component library palette.

### Phase 3: The Solver
- [ ] Basic MNA (Modified Nodal Analysis) solver.
- [ ] zepGPU bridge for matrix operations.
- [ ] ADS-B packet decoder node.

### Phase 4: The Bridge
- [ ] `to_uqe()` export logic.
- [ ] Automated schematic to Mermaid export (vizult-style).

---

## 6. Commands

```bash
# Start the UI
poetry run python -m egottol.ui.main

# Run a simulation headless
poetry run python -m egottol.simulate --circuit my_circuit.egt

# Export to UQE
poetry run python -m egottol.export --format uqe --input my_circuit.egt
```
