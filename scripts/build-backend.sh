#!/usr/bin/env bash
# Configure, build and unit-test the C++ backend. Works on macOS (Apple Silicon) and Linux.
#   BUILD_TYPE=Debug scripts/build-backend.sh      # default: Release
#   SKIP_TESTS=1 scripts/build-backend.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/backend/build"

GEN=(); EXTRA=()
command -v ninja >/dev/null 2>&1 && GEN=(-G Ninja)
if command -v brew >/dev/null 2>&1; then EXTRA=(-DCMAKE_PREFIX_PATH="$(brew --prefix)"); fi
if [ "$(uname -s)" = "Darwin" ] && [ "$(uname -m)" = "arm64" ]; then EXTRA+=(-DCMAKE_OSX_ARCHITECTURES=arm64); fi

# ${arr[@]+"${arr[@]}"} keeps macOS's bash 3.2 happy when an array is empty.
cmake -S "$ROOT/backend" -B "$BUILD_DIR" ${GEN[@]+"${GEN[@]}"} ${EXTRA[@]+"${EXTRA[@]}"} \
  -DCMAKE_BUILD_TYPE="${BUILD_TYPE:-Release}" -Wno-dev
cmake --build "$BUILD_DIR" --parallel
if [ -z "${SKIP_TESTS:-}" ]; then ctest --test-dir "$BUILD_DIR" --output-on-failure; fi
echo "Built: $BUILD_DIR/dlx_server"
