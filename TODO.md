# TODO — Ship 1.0

## Done this push (production SPICE engine)
- [x] Companion-model transient (backward Euler C/L) — RC step hits 0.993 @ 5τ
- [x] Nonlinear DC OP with Newton + gmin/source stepping — diode Vf ≈ 0.57 V
- [x] Multi-terminal MOSFET (D/G/S/B) + BJT (C/B/E) stamps
- [x] PULSE() sources, W=/L= params, keyed SPICE tokens
- [x] `spice_production_test` + ngspice presence check
- [x] CLI uses SpiceTransient / DcOperatingPoint
- [x] `.model` card parse + apply (D/M/Q: IS/N/VTO/KP/BF/…)
- [x] Trapezoidal C/L companions + optional LTE timestep control
- [x] 12-circuit golden corpus (`golden_spice_test` + `tests/fixtures/goldens/`)

## Still open for “beyond LTspice” credibility
- [ ] Expand goldens toward ~20 + tighter ngspice numeric parse compare
- [ ] Subcircuit expansion (X instances)
- [ ] Diode Rs consistent Thevenin stamp
- [ ] MOSFET/BJT curve family validation plots
- [ ] Perf baseline vs node count in CI
- [ ] DC source-stepping must only accept full-scale solution as final