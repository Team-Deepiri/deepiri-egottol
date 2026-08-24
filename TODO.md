# TODO — Ship 1.0

## Done (production SPICE credibility)
- [x] Companion-model transient (BE + trapezoidal) — RC @ 5τ ≈ 0.993
- [x] Nonlinear DC OP — Newton + gmin/source stepping, **full-scale only**, damped NR
- [x] Multi-terminal MOSFET (D/G/S/B) + BJT (C/B/E) with correct MNA RHS stamps
- [x] MOSFET Level-1 Id matches KP·W/L analytical (golden)
- [x] `.model` cards applied to D/M/Q; diode **Rs → explicit series R**
- [x] `.subckt` / `X` instance expansion (port map + name prefix)
- [x] PULSE() sources, W=/L=, keyed tokens; CLI `--trap` / `--lte`
- [x] 20-circuit golden corpus + ngspice batch cross-check
- [x] `spice_production_test` + `golden_spice_test` (18/18 ctest)

## Still open (next polish)
- [ ] Nested subckts / `.include` libraries
- [ ] MOSFET/BJT family curve plots (docs/CI artifacts)
- [ ] Perf baseline vs node count in CI
- [ ] Stronger ngspice numeric parse for more than divider OP
