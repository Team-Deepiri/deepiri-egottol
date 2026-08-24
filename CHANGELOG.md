# Changelog

All notable changes to **deepiri-egottol** are documented here.

## [1.0.0] — 2026-08-24

### Added
- Production SPICE engine: nonlinear DC OP (Newton + gmin/source stepping) and companion-model transient (backward Euler + trapezoidal, optional LTE)
- `.model` cards for diodes, MOSFETs, and BJTs; diode `Rs` expands to an explicit series resistor
- `.subckt` / `X` instance expansion with **nested** subcircuits and `.include` / `.inc` libraries
- Headless `egottol-cli` (`sim --op|--tran|--ac`, `--trap`, `--lte`) and Python `egottol.simulate` / `egottol.export`
- Native Qt schematic → netlist simulation, `.egt` save/load, CSV waveform export, Mermaid export, UQE logic bridge
- 22-circuit golden corpus + ngspice cross-check + perf baseline in `ctest`

### Verified
- MOSFET Level-1 Id matches analytical `½·KP·(W/L)·(Vgs−Vt)²·(1+λVds)`
- RC step ≈ 0.993 at 5τ; diode forward OP; subckt/include dividers = 2.5 V
