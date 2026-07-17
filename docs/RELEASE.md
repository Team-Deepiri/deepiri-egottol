# Releasing Egottol Desktop

GitHub Actions packages the PyQt6 Egottol UI (`python -m egottol.ui.main`) for Linux, macOS, and Windows when you push a version tag. Filenames match the [Deepiri landing site](https://github.com/Team-Deepiri/deepiri-landing).

## Cut a release

1. Merge changes to `main`.
2. Bump `version` in `pyproject.toml` if needed (current: `0.1.0`).
3. Tag and push:

   ```bash
   git tag v0.1.0
   git push origin v0.1.0
   ```

4. Watch [Release workflow](https://github.com/Team-Deepiri/deepiri-egottol/actions/workflows/release.yml).

## Test CI without tagging

**Actions → Release → Run workflow** on a branch. Publish is skipped unless the ref is a `v*` tag.

## Local packaging (optional)

```bash
poetry install --no-root
pip install pyinstaller
bash scripts/ci/build-release.sh
ls release/
```

## Release assets

| Platform | Filename |
|----------|----------|
| macOS | `Egottol-latest.dmg` |
| Linux | `Egottol-latest.AppImage` |
| Windows | `Egottol-latest-setup.exe` |

## Verify download URLs

```bash
BASE=https://github.com/Team-Deepiri/deepiri-egottol/releases/latest/download

curl -I "$BASE/Egottol-latest.dmg"
curl -I "$BASE/Egottol-latest.AppImage"
curl -I "$BASE/Egottol-latest-setup.exe"
```

## Notes

- CI builds the C++ core and **`egottol._native`** pybind11 extension (`-DBUILD_PYTHON_BINDINGS=ON`) before PyInstaller runs. DC MNA solves use `deepiri::Matrix` from the native module when bundled.
- Dev install: `cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON && cmake --build build --target _native`
- The frozen app bundles VHDL assets and `io/eii_weights.schema.json` for RTL/EII features.
- Optional deps (`onnxruntime`, `cupy`) are excluded from the frozen bundle to reduce size.
- v1 builds are **unsigned**; expect Gatekeeper / SmartScreen prompts.
