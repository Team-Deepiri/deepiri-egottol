# Deepiri Egottol

**Binary Electrical Systems Simulation Lab**

egottol is an advanced multi-domain circuit simulation platform built for engineers who need more than conventional SPICE tools can provide. It combines analog circuit solving, event-driven digital logic, VHDL-defined hardware blocks, GPU-accelerated computation, and real-time avionics protocol simulation in a single integrated environment.
<img width="1865" height="1050" alt="image" src="https://github.com/user-attachments/assets/fac32901-a19d-4525-bfec-783f40ccb6e9" />

---

## What It Does

### Multi-Domain Simulation Engine

egottol operates across four tightly coupled simulation domains:

| Domain | Technology | Purpose |
|--------|-----------|---------|
| Analog | Modified Nodal Analysis (MNA) | Nonlinear components — diodes, MOSFETs, BJTs, op-amps |
| Digital | Event-driven logic engine | Gates, flip-flops, subcircuits, VHDL blocks |
| GPU | zepGPU integration | Parallel transient analysis, FFT, parametric sweeps |
| Quantum | UQE bridge | Transpile classical logic to quantum gate sequences |

### VHDL Component Library

A native VHDL lexer and parser allows hardware description files to be loaded directly into the simulation graph. The included library covers:

- **Combinational**: AND, OR, NOT, NAND, NOR, XOR, XNOR, multiplexers (2:1, 4:1, 8:1), demultiplexers, decoders, encoders, comparators, priority encoders
- **Arithmetic**: half adder, full adder, ripple-carry adder, carry-lookahead adder, N-bit ALU
- **Sequential**: D flip-flop, JK flip-flop, SR flip-flop, T flip-flop, shift register, binary counter, BCD counter
- **Memory**: synchronous RAM, ROM, N-bit register
- **Processor**: CPU datapath, control unit, simple CPU top-level, BCD-to-7-segment display driver

### Avionics Simulation

- **ADS-B decoder**: Real-time virtual transponder simulation for airspace injection scenarios
- **SDR node**: GPU-accelerated software-defined radio for RF-to-digital signal chain simulation
- **GDL-90 encoder**: Aviation data link protocol encoding for MFD/EFB integration testing
- **Synthetic airspace**: Inject arbitrary aircraft trajectories into a live circuit simulation

### Schematic Editor (PyQt6)

- Native desktop canvas with drag-and-drop component placement
- Auto-routing wire tool with hierarchical block support
- Built-in oscilloscope and spectrum analyzer (FFT) with multi-trace math
- Embedded SPICE-style console supporting `.tran`, `.ac`, `.step`, and `.dc` directives
- Component property editor with live parameter validation

### EII Pipeline (Electrical Impulse Inference)

EII treats detected electrical events as information and runs a closed **Φ → Ψ → Γ** loop each simulation window:

- **Impulse detectors** — threshold, differentiator, comparator, RF envelope, memristor switch
- **Encoding manifold Φ** — rate, latency, filter, population, or continuous embeddings
- **Inference engine Ψ** — analog crossbar readout, digital linear/MLP head, or energy-based classifier
- **Feedback actuator Γ** — DAC drive, STDP weight updates, digital relay control, optical phase shift

Formal spec: [`docs/eii-math.md`](docs/eii-math.md). Example benchmark circuit: [`benchmarks/eii/thermistor_classify.json`](benchmarks/eii/thermistor_classify.json). Readout weights use the open [`.egt-weights` schema](io/eii_weights.schema.json).

```bash
# Optional GPU offload for large crossbar readouts
poetry install -E gpu

# Run Python EII unit tests
poetry run pytest tests/test_eii.py -q
```

### Analog-AI Compute Stack

Beyond classic MNA, egottol simulates **analog neural hardware** and couples it to AI inference:

| Engine | Module | Capability |
|--------|--------|------------|
| Nonlinear MNA | `engines/analog/nonlinear_mna.py` | Newton–Raphson DC/transient with diodes, memristors, op-amps |
| Op-amp neurons | `engines/analog/opamp_neuron.py` | Tanh/sigmoid analog neuron layers via OTA summation |
| Hopfield net | `engines/analog/hopfield.py` | Associative memory, energy minimization, pattern recall |
| Ising machine | `engines/analog/ising.py` | Simulated annealing on spin couplings (QUBO-style) |
| OTA / Gilbert | `engines/analog/ota.py`, `gilbert_cell.py` | Transconductance amps, four-quadrant analog multiply |
| Noise models | `engines/analog/noise.py` | Johnson thermal + 1/f flicker injection |
| Echo-state reservoir | `engines/ai/reservoir.py` | Sparse reservoir + ridge-regression readout |
| NSP | `engines/ai/nsp.py` | Denoise, FFT classify, anomaly detect (`NSP_AI` block) |
| Auto-tuner | `engines/ai/auto_tune.py` | Fit component params to measured waveforms |

**Schematic components:** `MEMRISTOR`, `CROSSBAR`, `LIF_NEURON`, `MZI_MESH`, `OTA`, `GILBERT_MULT`, `OPAMP_NEURON`, `HOPFIELD_NET`, `ISING_CELL`, `WINNER_TAKE_ALL`, `ANALOG_SAMPLE_HOLD`, plus full EII chain.

**Toolbar:** Analog/AI section (MEM, XBAR, LIF, MZI, EII, NSP, HOP, ISING). Transient sim uses `NonlinearMNASolver` when nonlinear devices are present.

**Inference backends** (EII Ψ): `analog`, `digital`, `energy_based`, `reservoir`, `hopfield`.

```bash
poetry run pytest tests/test_analog_ai_integration.py tests/test_ai_engines.py -q
```

### Third Wave: AC, RTL Shadow, ONNX Import, GPU Mesh

| Feature | Module | What it does |
|---------|--------|--------------|
| **AC / Bode analysis** | `engines/analog/ac_analysis.py` | Small-signal sweep, \|H(jω)\| and phase per node; toolbar **∿ AC** |
| **RTL shadow** | `engines/rtl_shadow.py`, `vhdl/pim_peripheral.vhd` | VHDL index, Verilog netlist export, Verilator runner + Python cycle fallback, AXI4-Lite PIM bus |
| **ONNX / weight import** | `engines/ai/onnx_import.py`, `ui/weight_import_dialog.py` | Import `.onnx`, `.egt-weights`, `.json`, `.npy` → apply to `INFERENCE_ENGINE` / `EII_PIPELINE` |
| **Community GPU mesh** | `engines/gpu_mesh.py`, `ui/gpu_mesh_panel.py` | zepGPU discovery, `analog_matmul` / `crossbar_solve` / `mzi_mesh_fft` / `spike_batch` tasks, CuPy→NumPy fallback |

```bash
# Optional ONNX import
poetry install -E ai

# Optional GPU offload
poetry install -E gpu

poetry run pytest tests/test_ac_analysis.py tests/test_rtl_shadow.py tests/test_onnx_import.py tests/test_gpu_mesh.py -q
```

### Egottol Copilot

Schematic-aware AI assistant in the PyQt6 UI (View → Copilot). Copilot reads circuit context, runs simulation tools, and can insert EII pipelines from natural language.

**Copilot tools:** place components, run DC/transient/logic sim, explain waveforms, insert EII stack, run EII sim, tune analog-AI params, analyze spike rasters, optimize crossbar weights, run NSP denoise/classify, auto-tune circuit params to target waveforms.

**API key setup** (pick one or more providers):

1. **In-app** — open Copilot → gear icon → enter keys per provider → Save.
2. **Config file** — `~/.config/egottol/copilot.json` (written by the settings dialog).
3. **Environment variables** (fallback when a key is not stored):

| Provider | Variable |
|----------|----------|
| OpenAI | `OPENAI_API_KEY` |
| Anthropic | `ANTHROPIC_API_KEY` |
| Gemini | `GEMINI_API_KEY` |
| OpenRouter | `OPENROUTER_API_KEY` |
| Ollama (local) | `OLLAMA_API_KEY` (optional) |

```bash
export OPENAI_API_KEY="sk-..."
poetry run python -m egottol.ui.main
```

Local inference without cloud keys: select **Ollama** and run `ollama serve` with your model pulled. Copilot falls back to a rule-based backend when no LLM is configured.

```bash
poetry run pytest tests/test_copilot_settings.py -q
```

### UQE Quantum Bridge

Exports circuits as `QuantumCircuit` objects for use with the Universal Quantum Engine:

| Classical Gate | Quantum Equivalent |
|----------------|-------------------|
| NOT | X gate |
| XOR | CNOT (reversible XOR) |
| AND | Toffoli (reversible AND) |

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Application | Python 3.12, PyQt6 |
| Simulation core | C++ (MNA solver, logic engine, avionics stack) |
| Hardware description | VHDL (native lexer/parser in C++) |
| Data modeling | Pydantic v2 |
| Numerical | NumPy, SciPy |
| Visualization | PyQtGraph |
| Build | Poetry, CMake |

---

## Project Structure

```
egottol/
├── core/           # C++ MNA solver, Newton-Raphson, transient integrator
├── logic/          # C++ event-driven digital engine, VHDL lexer/parser
├── avionics/       # C++ ADS-B decoder, GDL-90 encoder, SDR buffer
├── gui/            # C++ schematic canvas, waveform plotter, wire routing
├── vhdl/           # VHDL component library (gates, adders, CPU, memory)
├── egottol/        # Python application layer
│   ├── engines/    # Simulator, solver bridge, GPU offload, ASM engine
│   ├── models/     # Pydantic component models and registry
│   └── ui/         # PyQt6 main window entry point
├── models/         # Shared data models
├── infra/          # Infrastructure and build configuration
└── tests/          # Test suite
```

---

## Setup

### Prerequisites

- Python 3.12 or later
- Poetry 2.0 or later
- CMake 3.20 or later (for the C++ simulation core)
- A C++17-capable compiler (GCC 11+, Clang 13+, or MSVC 2022)

### Install Python Dependencies

```bash
poetry install
```

### Build the C++ Core

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Run the Schematic Editor

```bash
poetry run python -m egottol.ui.main
```

### Run a Headless Simulation

```bash
poetry run python -m egottol.simulate --circuit my_circuit.egt
```

### Export to UQE (Quantum)

```bash
poetry run python -m egottol.export --format uqe --input my_circuit.egt
```

---

## License

Apache License 2.0. See [LICENSE](LICENSE) for the full terms.

Copyright 2026 Deepiri.
