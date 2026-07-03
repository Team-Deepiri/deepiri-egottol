#!/usr/bin/env bash
# Ensure CMake can configure (matches setup.sh stubs).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [[ ! -f models/device.cpp ]]; then
  cat > models/device.cpp <<'EOF'
#include "device.h"
namespace deepiri {}
EOF
fi

if [[ ! -s tests/CMakeLists.txt ]]; then
  echo "# Tests placeholder" > tests/CMakeLists.txt
fi
