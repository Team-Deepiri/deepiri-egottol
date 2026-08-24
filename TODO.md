# TODO — Ship 1.0

## Done this push (production SPICE engine)
- [x] Companion-model transient (backward Euler C/L) — RC step hits 0.993 @ 5τ
- [x] Nonlinear DC OP with Newton + gmin/source stepping — diode Vf ≈ 0.57 V
- [x] Multi-terminal MOSFET (D/G/S/B) + BJT (C/B/E) stamps
- [x] PULSE() sources, W=/L= params, keyed SPICE tokens
- [x] `spice_production_test` + ngspice presence check
- [x] CLI uses SpiceTransient / DcOperatingPoint

## Still open for “beyond LTspice” credibility
- [ ] ~20-circuit ngspice golden corpus (automated compare)
- [ ] Full `.model` card application (VTO/KP/Is from .model lines)
- [ ] Subcircuit expansion (X instances)
- [ ] Trapezoidal integration + LTE timestep control
- [ ] MOSFET/BJT curve family validation plots
- [ ] Perf baseline vs node count in CI
