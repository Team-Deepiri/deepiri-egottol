# TODO — Ship 1.0

## Shipped in 1.0.0
- [x] Production SPICE DC + transient (BE/trap + LTE)
- [x] `.model` + MOSFET/BJT Level-1 stamps (analytical Id golden)
- [x] `.subckt` / nested `X` + `.include` libraries
- [x] 22-circuit goldens, ngspice cross-check, perf baseline
- [x] Native GUI schematic→sim, CLI, Python wrappers, Mermaid + UQE export
- [x] `CHANGELOG.md` + docs aligned to 1.0

## Post-1.0 backlog
- [ ] Deeper `.lib` / PDK libraries and nested `.include` trees in CI artifacts
- [ ] MOSFET/BJT family curve plot artifacts
- [ ] zepGPU offload for large MNA (optional)
