# Egottol 1.0 — Finished Product Checklist

Status: **SHIPPED** on branch `feat/spice-production-engine` (PR #39).

## Product surfaces

| Surface | Status |
|---------|--------|
| Native desktop `egottol` (Qt schematic → sim, save/load, CSV, Mermaid) | Done |
| Headless `egottol-cli` 1.0.0 (`--op` / `--tran` / `--ac` / `--trap` / `--lte`) | Done |
| Python `egottol.simulate` / `egottol.export` / Copilot | Done |
| Production SPICE (DC OP, BE/trap, `.model`, nested `.subckt`, `.include`) | Done |
| EE design knowledge (`docs/ee/` + `lookup_ee_design`) | Done |
| Goldens (22+) + ngspice cross-check + perf baseline | Done |

## Verify before merge / tag

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
ctest --test-dir build --output-on-failure
poetry run pytest -q
./build/egottol-cli version
./build/egottol-cli sim tests/fixtures/divider.cir --op
./build/egottol-cli sim tests/fixtures/design/led_series_r.cir --op
poetry run python -c "from egottol.knowledge import lookup_ee_design; print(lookup_ee_design('flyback')[0]['id'])"
```

## Tag when merged to main

```bash
git tag v1.0.0
git push origin v1.0.0
```

See [docs/RELEASE.md](docs/RELEASE.md) and [CHANGELOG.md](CHANGELOG.md).
