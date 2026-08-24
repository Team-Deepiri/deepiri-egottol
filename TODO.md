# TODO — What Needs Done to Ship 1.0

> Audit 2026-08-24. Full context: [PLAN.md §7–8](PLAN.md).

## Current state

**Shipped on this branch:** product loop (parse → build → draw/sim → save `.egt` →
CSV), `egottol-cli`, Python `egottol.simulate` / `.export`, EII thermistor
benchmark runner, analytical golden waveforms, Newton gmin/source stepping,
`to_uqe()` + Mermaid export. **16/16 ctest**.

## Phase 6 — Close the loop ✅
## Phase 7 — mostly done
- [x] `egottol-cli`
- [x] `python -m egottol.simulate` / `.export`
- [x] `python -m egottol.benchmarks.eii.run`
- [x] mypy duplicate `eii` unblocked
- [x] version 1.0.0
- [ ] ruff autofix + CI gate when green

## Phase 8 — in progress
- [x] Analytical golden waveforms (divider / RC DC / RC AC corner / UQE map)
- [x] Newton gmin + source stepping API (`solveWithStepping`)
- [ ] Broader ngspice/LTspice corpus (~20 circuits)
- [ ] MOSFET/BJT curve validation
- [ ] Perf baseline in CI

## Phase 9 — started
- [x] `to_uqe()` NOT→X, XOR→CNOT, AND→Toffoli + test
- [x] Schematic → Mermaid export
- [ ] zepGPU bridge documentation
- [ ] ADS-B/SDR end-to-end GUI demo

## Phase 10
- [x] README verified commands
- [ ] Packaging audit / tag v1.0.0
