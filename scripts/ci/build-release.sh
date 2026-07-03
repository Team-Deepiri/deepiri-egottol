#!/usr/bin/env bash
# Build Egottol desktop release artifact for the current OS.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/release"

cd "$ROOT"
mkdir -p "$OUT"
rm -rf "$ROOT/dist"

bash scripts/ci/prepare-native-build.sh

echo "==> Installing Poetry dependencies"
pip install poetry pyinstaller
poetry config virtualenvs.in-project true
poetry install --no-root --no-interaction

echo "==> Building native C++ core + Python extension"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON
cmake --build build --parallel --target _native

echo "==> Running PyInstaller"
poetry run pyinstaller packaging/egottol.spec --noconfirm --clean

case "$(uname -s)" in
  Linux)
  echo "==> Building AppImage"
  APPDIR="$ROOT/build/AppDir"
  rm -rf "$APPDIR"
  mkdir -p "$APPDIR/usr/bin"
  cp -a "$ROOT/dist/Egottol/." "$APPDIR/usr/bin/"
  cat > "$APPDIR/egottol.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Egottol
Exec=Egottol
Icon=egottol
Categories=Science;Engineering;
EOF
  curl -fsSL -o /tmp/appimagetool.AppImage \
    https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
  chmod +x /tmp/appimagetool.AppImage
  ARCH=x86_64 /tmp/appimagetool.AppImage "$APPDIR" "$OUT/Egottol-latest.AppImage"
  ;;
  Darwin)
  echo "==> Building DMG"
  hdiutil create -volname "Egottol" -srcfolder "$ROOT/dist/Egottol.app" -ov -format UDZO "$OUT/Egottol-latest.dmg"
  ;;
  MINGW*|MSYS*|CYGWIN*)
  echo "==> Packaging Windows executable"
  cp "$ROOT/dist/Egottol/Egottol.exe" "$OUT/Egottol-latest-setup.exe"
  ;;
  *)
  echo "Unsupported OS: $(uname -s)" >&2
  exit 1
  ;;
esac

echo "==> Release artifacts"
ls -la "$OUT"
