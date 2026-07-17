#!/usr/bin/env bash
# Build the egottol._native pybind11 extension for local development.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

bash scripts/ci/prepare-native-build.sh

cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON
cmake --build build --parallel --target _native

echo "==> Native extension:"
ls -la egottol/_native* 2>/dev/null || ls -la egottol/

echo "Verify: PYTHONPATH=. python -c \"import egottol._native; print(egottol._native.core_version())\""
