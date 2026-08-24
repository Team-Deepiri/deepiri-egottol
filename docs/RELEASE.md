# Releasing Egottol 1.0

## What ships

| Artifact | How |
|----------|-----|
| Native desktop `egottol` | CMake + Qt6 (`ENABLE_QT=ON`) |
| Headless `egottol-cli` | Always built — no Qt required |
| Python package `egottol` 1.0.0 | Poetry + optional `_native` pybind module |
| CPack `.deb` / `.tar.gz` | `cpack` from the CMake build tree |

## Cut a release

1. Merge to `main` (version already `1.0.0` in `pyproject.toml` / CLI).
2. Tag and push:

   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. Watch [Release workflow](https://github.com/Team-Deepiri/deepiri-egottol/actions/workflows/release.yml) for desktop bundles.

## Local verify before tag

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/egottol-cli version
./build/egottol-cli sim tests/fixtures/divider.cir --op
poetry run pytest -q
```

## Notes

- CI builds the C++ core and optional `egottol._native` (`-DBUILD_PYTHON_BINDINGS=ON`).
- Desktop freeze still packages the PyQt UI for multi-OS installers; the primary engineer workflow is native `egottol` + `egottol-cli`.
- v1 builds may be **unsigned** (Gatekeeper / SmartScreen prompts).
