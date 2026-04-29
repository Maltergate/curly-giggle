#!/usr/bin/env bash
# build.sh — configure, build, and test fastscope
#
# Usage:
#   ./build.sh              — incremental Debug build + tests
#   ./build.sh --clean      — wipe build/ then full rebuild
#   ./build.sh --release    — Release build
#   ./build.sh --no-tests   — skip ctest

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_TYPE="Debug"
RUN_TESTS=true

# ── Parse arguments ────────────────────────────────────────────────────────────
for arg in "$@"; do
    case "$arg" in
        --clean)    rm -rf "${BUILD_DIR}" ; echo "Cleaned ${BUILD_DIR}" ;;
        --release)  BUILD_TYPE="Release" ;;
        --no-tests) RUN_TESTS=false ;;
    esac
done

# ── Configure ──────────────────────────────────────────────────────────────────
mkdir -p "${BUILD_DIR}"
cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# ── Build ──────────────────────────────────────────────────────────────────────
NCPUS=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)
cmake --build "${BUILD_DIR}" --parallel "${NCPUS}"

# ── Test ───────────────────────────────────────────────────────────────────────
if $RUN_TESTS; then
    echo ""
    echo "═══════════════════════════════════════"
    echo " Running tests"
    echo "═══════════════════════════════════════"
    ctest --test-dir "${BUILD_DIR}" --output-on-failure --parallel "${NCPUS}"
fi

echo ""
echo "✓ Build complete (${BUILD_TYPE})"
echo "  App bundle: ${BUILD_DIR}/bin/fastscope.app"
echo "  Run:        open ${BUILD_DIR}/bin/fastscope.app"
