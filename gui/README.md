# Stage 1 — C++ GUI implementation guide

This folder implements the native Qt6 schematic editor. **Start here** after reading
[`../scope.md`](../scope.md) Stage 1.

## Run the app

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/egottol_gui
```

Requires Qt6 (`qt6-base-dev` on Linux, `brew install qt@6` on macOS). Set `CMAKE_PREFIX_PATH`
if CMake does not find Qt6.

## Python → C++ file map

| Python (`egottol/ui/main.py`) | C++                                                            |
| ----------------------------- | -------------------------------------------------------------- |
| `EgottolApp`                  | `main_window.h/cpp`                                            |
| `COLORS`, `GRID`              | `egottol_theme.h`                                              |
| `SchematicScene`              | `scene.h/cpp`                                                  |
| `SchematicView`               | `schematic_view.h/cpp`                                         |
| `ComponentItem`               | `component_item.h/cpp`                                         |
| `SYMBOLS`                     | `symbol_renderer.h/cpp` (**Stage 2**)                          |
| `PORT_OFFSETS`, `SYMBOL_KEY`  | `port_layout.h/cpp` (**Stage 2**)                              |
| `_drop_component`             | `component_factory.h/cpp`                                      |
| Left palette                  | `component_palette.h/cpp`                                      |
| `Circuit` / `Wire` models     | `schematic_document.h/cpp`                                     |
| `_console`                    | `console_panel.h/cpp`                                          |
| PyQtGraph plot                | `waveform_panel.h/cpp`                                         |
| `AdvancedMNASolver`           | `schematic_to_circuit.h/cpp` + `core/mna_solver` (**Stage 5**) |
| `main()`                      | `main.cpp`                                                     |

## What is already wired (Stage 1 scaffold)

- [x] `MainWindow` with docks, toolbar, status bar, dark theme
- [x] Dot grid background on `SchematicScene`
- [x] Place mode from palette + toolbar → `ComponentFactory::placeComponent`
- [x] Placeholder symbol drawing until `SymbolRenderer::draw` is filled in
- [x] Basic port layout for toolbar components in `port_layout.cpp`
- [x] Console logging for toolbar actions
- [x] Pan (middle mouse) and scroll zoom on `SchematicView`

## What you implement next (in order)

### 1. Verify build and layout

Touch: `main.cpp`, `main_window.cpp` only if layout tweaks needed.

### 2. Symbol drawing (Stage 2 preview)

Touch: `symbol_renderer.cpp` — port draw commands from Python `SYMBOLS` for `R`, `C`, `VSRC`, `GND`.

Touch: `port_layout.cpp` — full `PORT_OFFSETS` for those keys.

Touch: `component_item.cpp` — ensure `paint()` uses `SymbolRenderer::draw` when it returns true.

### 3. Port-click wiring (Stage 3 preview)

Touch: `component_item.cpp` — `mousePressEvent`: hit-test pins, call scene wire API.

Touch: `scene.h/cpp` — add `start_wire`, `finish_wire`, `update_wire_preview`, `cancel_wire`
(mirror Python `SchematicScene` methods).

Touch: `wire_item.cpp` — store endpoint component ids + pin names; orthogonal path.

Touch: `schematic_document.cpp` — `addWire` when wire completes.

### 4. Registry key alignment

Touch: `component_palette.cpp` — map `SymbolLibrary` names (`R`) to Python registry keys (`RES`).

Touch: `io/symbol_library.cpp` — optional: add `category` and `registry_key` fields to `SymbolDefinition`.

## Class ownership diagram

```
main.cpp
  └── MainWindow
        ├── SchematicView → SchematicScene
        │                     ├── ComponentItem[]  (via factory)
        │                     ├── WireItem[]       (Stage 3)
        │                     ├── WireTool
        │                     └── SelectionTool
        ├── SchematicDocument  (logical model)
        ├── ComponentPalette
        ├── ConsolePanel
        └── WaveformPanel
```

`MainWindow` owns `SchematicDocument*`. Scene holds a non-owning pointer via `set_document()`.

## Common pitfalls

1. **Do not call `update_pins()` on move** — factory loads pins from `PortLayout`; see comment in `component_item.cpp`.
2. **MOC**: classes with `Q_OBJECT` must stay in `GUI_SOURCES`; CMake `AUTOMOC` handles moc — do not `#include *.moc` manually.
3. **Two mains**: `gui/main.cpp` is only linked into `egottol_gui` executable, not the static library.
4. **Net merging** belongs in `schematic_to_circuit.cpp` (Stage 5), not in the scene.
