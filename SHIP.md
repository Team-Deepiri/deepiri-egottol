# Egottol 1.0 — Finished Product Checklist

Status: **SHIPPED** on branch `feat/spice-production-engine` (PR #39).

## Product surfaces

| Surface | Status |
|---------|--------|
| Native desktop `egottol` (Qt schematic → sim, save/load, CSV, Mermaid) | Done |
| Headless `egottol-cli` 1.0.0 (`sim` / `ee` / `--op` / `--tran` / `--ac`) | Done |
| Python `egottol.simulate` / `egottol.export` / Copilot | Done |
| Production SPICE (DC OP, BE/trap, `.model`, nested `.subckt`, `.include`) | Done |
| GUI schematic → production engine (demo only if canvas empty) | Done |
| EE design knowledge (`docs/ee/` + CLI/`lookup_ee_design`) | Done |
| Goldens (22+) + ngspice cross-check + perf baseline | Done |

## Verify before merge / tag

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
poetry run pytest -q
./build/egottol-cli version
./build/egottol-cli sim tests/fixtures/divider.cir --op
./build/egottol-cli ee flyback diode
./build/egottol-cli sim tests/fixtures/design/boost_chopper.cir --tran
poetry run python -c "from egottol.knowledge import lookup_ee_design; print(lookup_ee_design('flyback')[0]['id'])"
```

## Tag when merged to main

```bash
git tag v1.0.0
git push origin v1.0.0
```

See [docs/RELEASE.md](docs/RELEASE.md) and [CHANGELOG.md](CHANGELOG.md).
