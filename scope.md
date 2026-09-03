# deepiri-egottol — C++ Native GUI & Engine Scope

This document defines the staged plan to replace the Python/PyQt6 application layer with a
100% C++ Qt6 desktop app, while connecting the existing simulation core (`core/`,
`models/`, `logic/`, `avionics/`, `infra/`).

Each stage lists **goals**, **scope** (what is in / out), and **deliverables**.
Stages are ordered; later stages depend on earlier ones.

---

## Stage 0 — Project bootstrap & build target

### Goals

- Ship a launchable C++ binary (`egottol_gui`) independent of Python.
- Establish the GUI module layout and CMake wiring.

### Scope

- Add `gui/main.cpp` entry point.
- Add `add_executable(egottol_gui …)` to CMake when `ENABLE_QT=ON`.
- Link `deepiri_gui`, Qt6 Widgets, and (later) simulation libraries.
- Document run command: `cmake --build build && ./build/egottol_gui`.

### Deliverables

- Empty window appears on launch.
- CI still builds the static `deepiri_gui` library and the new executable.

### Out of scope

- Schematic interaction, simulation, save/load.

---

## Stage 1 — Replicate Python Qt GUI shell

**Reference implementation:** `egottol/ui/main.py` (`EgottolApp`, `SchematicScene`, `SchematicView`).

### Goals

- Match the Python app's **window chrome**: central canvas, docks, toolbar, status bar, dark theme.
- Wire existing C++ classes (`SchematicScene`, `SchematicView`, tools) into one cohesive shell.
- Introduce scaffolding files for data model and rendering (stubs OK; must compile).

### Scope

#### 1.1 Main window (`gui/main_window.h/cpp`)

- `MainWindow : QMainWindow` replacing the non-Qt stub class.
- Central widget: `SchematicView` hosting `SchematicScene`.
- **Left dock — Components:** palette list populated from `SymbolLibrary` (names/categories).
- **Right dock — Services:** placeholder status indicators (zepGPU, UQE).
- **Bottom dock — Waveform / Console:** horizontal splitter with plot placeholder + read-only console.
- **Toolbar:** actions mirroring Python (DC, Transient, Config, quick-place R/C/L/V/I/GND/VCC, gates, clear, zoom).
- **Status bar:** mode label (`SELECT`, `PLACE: Resistor`, …) and shortcut hints.
- **Dark Fusion-style palette** via `gui/egottol_theme.h` (colors aligned with Python `COLORS` dict).

#### 1.2 Scene & view behavior

- `SchematicScene::drawBackground()` — dot grid on `#1a1b2e` (Python `GRID = 20`).
- Place mode: palette/toolbar sets a registry key; click canvas to drop component (stub symbol OK).
- Pan: middle-mouse drag (`SchematicView`).
- Zoom: scroll wheel (match Python: scroll zoom, not only Ctrl+wheel).
- Keys: Esc cancels wire/place; Del deletes selection; Space fit-to-view.

#### 1.3 Scaffolding (stubs + comments)

- `gui/schematic_document.h/cpp` — logical circuit graph (components, wires, params); no solver yet.
- `gui/component_factory.h/cpp` — registry key → `ComponentItem` + document entry.
- `gui/component_palette.h/cpp` — dock widget emitting `componentRequested(key)`.
- `gui/symbol_renderer.h/cpp` — port Python `SYMBOLS` draw commands (stub returns false until filled).
- `gui/port_layout.h/cpp` — port coordinates per symbol key (port Python `PORT_OFFSETS`).
- `gui/console_panel.h/cpp` — thin wrapper over `QTextEdit` for sim logs.
- `gui/waveform_panel.h/cpp` — placeholder plot area (replace with Qt Charts / QCustomPlot later).

#### 1.4 Tools (wire up, minimal behavior)

- Instantiate `WireTool` and `SelectionTool`; attach to `SchematicScene`.
- Default mode: port-click wiring (Python style) **or** explicit wire tool toggle — document choice in code comments.
- Wire preview orthogonal routing (stub path OK).

### Deliverables

- Visual layout indistinguishable from Python at a glance (docks, colors, toolbar).
- User can place stub components from palette/toolbar on a gridded canvas.
- Console accepts log lines from toolbar actions (e.g. "DC not implemented yet").

### Out of scope

- Full symbol geometry for all 150+ components (start with toolbar subset).
- Working DC solver, net merging, param sync.
- Save/load, service health polling, real waveform data.

---

## Stage 2 — Symbol rendering & port layout

### Goals

- Replace rectangle placeholders with schematic symbols matching Python `SYMBOLS`.
- Pin positions match Python `PORT_OFFSETS` for correct wiring visuals.

### Scope

- Implement `SymbolRenderer` draw commands: `line`, `rect`, `circle`, `arc`, `path`, `text`, `bubble`.
- Implement `PortLayout` for all symbol keys used in Stage 1 toolbar + palette.
- Update `ComponentItem::paint()` to call `SymbolRenderer` instead of `drawRect()`.
- Update `ComponentItem::update_pins()` to load pins from `PortLayout` by symbol key.
- Category colors on palette items (Python `cat_colors` map).
- Component ID label under symbol (Python shows `comp_id`).

### Deliverables

- R, C, L, diode, VSRC, ISRC, GND, VCC, BJT, gates, DFF, op-amp draw correctly.
- Port hit-testing aligns with visible lead dots.

### Out of scope

- Every registry entry in `SymbolLibrary` (expand in batches).

---

## Stage 3 — Wiring & schematic document

### Goals

- Port-to-port wiring with snap, preview, and document updates.
- Wires stay attached when components move.

### Scope

- Extend `WireItem` with `from_comp_id`, `from_pin`, `to_comp_id`, `to_pin`.
- `SchematicScene::start_wire / finish_wire / cancel_wire` (mirror Python).
- Port snap in preview via nearest-port search (C++ `WireTool::find_connection_point` logic).
- Orthogonal `_ortho_path` routing in `WireItem::update_path()`.
- On `ComponentItem::itemChange(ItemPositionHasChanged)`, refresh connected wires.
- Delete component → remove attached wires from scene + document.
- `SchematicDocument` owns authoritative component/wire lists; scene mirrors for graphics.

### Deliverables

- User can wire VSRC → R → GND with wires visually glued to ports.
- Document JSON-serializable struct ready for save/load.

### Out of scope

- Junction dots, net labels, bus syntax.

---

## Stage 4 — Property editor & parameter sync

### Goals

- Double-click component → edit parameters (Python `ParamDialog`).
- Edited values flow into `SchematicDocument` and simulation input.

### Scope

- Replace `PropertyEditor` stub with Qt widget (`QDialog` + `QFormLayout` or docked panel).
- Map parameter names/types from `SymbolLibrary` / component metadata.
- Sync on accept: update document **and** any live annotations.

### Deliverables

- Changing resistor `R` or source `V` persists in document and affects next sim run.

### Out of scope

- Unit validation, SPICE-style expression params.

---

## Stage 5 — DC simulation in C++

### Goals

- **Run DC** uses native `MNASolver` + device models, not Python.
- Fix net merging: wired ports share one electrical node.

### Scope

- `gui/schematic_to_circuit.h/cpp` — document → `deepiri::Circuit` with union-find net merge.
- Ground node from `GND` component (not alphabetical first port).
- Stamp R, L (DC), VSRC at minimum; expand to C (open), diode (Newton) incrementally.
- `MainWindow::runDcAnalysis()` — invoke solver, log to console, annotate scene.
- Voltage labels on ports (Python `_annotate_results`).
- DC bar chart in `WaveformPanel` (replace placeholder).

### Deliverables

- VSRC → R → GND divider shows correct voltages (e.g. 5 V / 0 V / current consistent).
- Python `AdvancedMNASolver` can be deprecated for GUI path.

### Out of scope

- Transient, AC, `.step` sweeps.

---

## Stage 6 — Transient, AC, and simulation config dialog

### Goals

- Match Python sim config tabs and non-placeholder transient plots.

### Scope

- `SimConfigDialog` (Qt): DC sweep, transient time step, AC freq range, display limits.
- Transient: `core/transient.cpp` + integrator driving time steps.
- AC: small-signal linearization or stub with roadmap comment.
- `WaveformPanel`: multi-trace time plots; probe/add trace from port click.
- FFT / spectrum view (Python README promises this).

### Deliverables

- Transient shows real waveforms, not placeholder sine.
- Config dialog persists settings in session.

### Out of scope

- GPU offload for transient matrices.

---

## Stage 7 — Digital logic & VHDL in the GUI

### Goals

- Event-driven logic simulation visible in the schematic editor.

### Scope

- Connect `logic/event_queue`, `logic/logic_gate`, `logic/flip_flop` to schematic document.
- Step or clocked simulation mode; logic levels on wires (color/state).
- Load `vhdl/*.vhd` via `logic/vhdl_parser` → hierarchical block in palette.
- Optional: integrate `egottol/cpu` blocks as subcircuits.

### Deliverables

- AND → NOT chain toggles correctly when inputs change.
- At least one VHDL block (e.g. full adder) simulates from GUI.

### Out of scope

- Full VHDL-2008 coverage.

---

## Stage 8 — Project I/O & persistence

### Goals

- Save and load schematics; headless CLI without Python.

### Scope

- JSON (or `.egt`) format for `SchematicDocument` (components, wires, params, positions).
- Finish `io/project_loader.cpp` parsing/writing.
- File menu: New, Open, Save, Save As.
- CLI: `egottol_cli simulate --circuit file.egt` (new `tools/simulate_main.cpp`).

### Deliverables

- Round-trip save/load restores canvas exactly.
- Headless DC run prints node voltages to stdout.

### Out of scope

- KiCad/SPICE netlist import (keep `io/netlist_parser` as follow-up).

---

## Stage 9 — Integrations (GPU, UQE, avionics)

### Goals

- Wire optional external services and experimental nodes.

### Scope

- **Services dock:** async health probe (`infra/zepgpu_client`, UQE endpoint) — port Python `ServiceDiscovery`.
- **GPU:** parametric sweep / large matrix offload hooks from sim menu.
- **UQE export:** `infra/uqe_bridge` + schematic → OpenQASM file menu item.
- **Avionics:** ADS-B / GDL-90 nodes in palette; `avionics/adsb_decoder` demo circuit.
- **SDR buffer** visualization hook in waveform panel.

### Deliverables

- Service LEDs reflect online/offline when backends run.
- Export menu produces valid stub QASM for logic circuits.

### Out of scope

- Production SDR pipeline.

---

## Stage 10 — Polish, testing, and Python deprecation

### Goals

- remove dependency on `egottol/ui/main.py`.

### Scope

- Unit tests: net merge, symbol port count, document serialization, DC golden circuits.
- CMake `ctest` targets for GUI logic (Qt Test or minimal harness).
- Palette search/filter, category tabs, recent components.
- Implement `gui/waveform_plotter.cpp` or remove in favor of `WaveformPanel`.
- Implement `gui/property_editor` Qt UI fully; delete dead stubs.
- Update README: primary launch via `./build/egottol_gui`.
- Mark Python UI as legacy or remove from default workflow.

### Deliverables

- CI runs GUI tests + simulation files.
- README and `scope.md` reflect shipped C++ app.

---

## File ownership map (C++ GUI)


| Concern           | Primary files                                             |
| ----------------- | --------------------------------------------------------- |
| App entry         | `gui/main.cpp`                                            |
| Window shell      | `gui/main_window.h/cpp`                                   |
| Theme/colors      | `gui/egottol_theme.h`                                     |
| Canvas            | `gui/schematic_view.h/cpp`, `gui/scene.h/cpp`             |
| Components        | `gui/component_item.h/cpp`, `gui/component_factory.h/cpp` |
| Symbols/ports     | `gui/symbol_renderer.h/cpp`, `gui/port_layout.h/cpp`      |
| Wires             | `gui/wire_item.h/cpp`, `gui/wire_tool.h/cpp`              |
| Selection         | `gui/selection_tool.h/cpp`                                |
| Document model    | `gui/schematic_document.h/cpp`                            |
| Netlist bridge    | `gui/schematic_to_circuit.h/cpp` (Stage 5)                |
| Palette dock      | `gui/component_palette.h/cpp`                             |
| Console / plot    | `gui/console_panel.h/cpp`, `gui/waveform_panel.h/cpp`     |
| Registry metadata | `io/symbol_library.h/cpp`                                 |
| Simulation        | `core/mna_solver.h/cpp`, `models/`*                       |


---

## Stage 1 implementation checklist (start here)

Use this list when working through the scaffolded files:

- [ ] Build succeeds: `cmake -B build && cmake --build build`
- [ ] `./build/egottol_gui` opens 1600×950 dark window
- [ ] Left palette lists symbols from `SymbolLibrary`
- [ ] Click palette item → status bar shows `PLACE: …`
- [ ] Click canvas → stub component appears snapped to grid
- [ ] Toolbar quick-place buttons trigger same place mode
- [ ] Middle-drag pans; scroll zooms; Space fits view
- [ ] Bottom console logs toolbar clicks
- [ ] Esc clears place mode