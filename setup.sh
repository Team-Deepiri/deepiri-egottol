#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$REPO_DIR"

echo "==> Installing system dependencies (requires sudo)..."
sudo apt-get update -qq
sudo apt-get install -y \
    qt6-base-dev \
    cmake \
    build-essential \
    python3 \
    python3-pip \
    pipx \
    libxcb-cursor0

echo "==> Ensuring Poetry is installed..."
if ! command -v poetry &>/dev/null; then
    pipx install poetry
    export PATH="$HOME/.local/bin:$PATH"
fi

echo "==> Installing Python dependencies..."
poetry install --no-root

echo "==> Creating missing stub source files..."
# models/device.cpp is declared in CMakeLists but not present in the repo
if [ ! -f models/device.cpp ]; then
cat > models/device.cpp << 'EOF'
#include "device.h"
namespace deepiri {}
EOF
fi

# tests/CMakeLists.txt must exist for the build to configure
if [ ! -f tests/CMakeLists.txt ]; then
    echo "# Tests placeholder" > tests/CMakeLists.txt
fi

echo "==> Configuring C++ build..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "==> Building C++ core..."
cmake --build build --parallel "$(nproc)"

echo "==> Launching GUI..."
DISPLAY="${DISPLAY:-:1}" poetry run python -m egottol.ui.main
