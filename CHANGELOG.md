# Changelog

All notable changes to **deepiri-egottol** are documented here.

## [1.0.0] — 2026-08-24

### Added
- Production SPICE engine: nonlinear DC OP (Newton + gmin/source stepping) and companion-model transient (backward Euler + trapezoidal, optional LTE)
- `.model` cards for diodes, MOSFETs, and BJTs; diode `Rs` expands to an explicit series resistor
- `.subckt` / `X` instance expansion with **nested** subcircuits and `.include` / `.inc` libraries
- Controlled sources **E/G (VCVS/VCCS)** and **F/H (CCCS/CCVS)** with correct MNA stamps
- Voltage-controlled **S switch** (Ron/Roff vs VT) and **K mutual inductance** (coupled L pair)
- Independent-source waveforms: **DC, PULSE, SIN, EXP, PWL** (V and I)
- **`.dc` source sweep**, **`.ic` / `.nodeset`**, **`.param {name}`**, **`.measure`** (MAX/MIN/AVG/PP/RMS)
- Headless `egottol-cli` (`sim --op|--tran|--ac|--dcsweep`, `ee <query>`, `--trap`, `--lte`) and Python `egottol.simulate` / `egottol.export`
- Native Qt schematic → **production** SPICE (`DcOperatingPoint` / `SpiceTransient`); demos only on empty canvas
- 31-circuit golden corpus + design fixtures + ngspice cross-check + perf baseline
- **EE design knowledge base** (`docs/ee/`): series/parallel, combinations, transistors+C/L, motors, symptom→fix, PCB floorplanning; Copilot + CLI `ee` lookup

### Verified
- MOSFET Level-1 Id matches analytical `½·KP·(W/L)·(Vgs−Vt)²·(1+λVds)`
- RC step ≈ 0.993 at 5τ; diode forward OP; subckt/include dividers = 2.5 V
