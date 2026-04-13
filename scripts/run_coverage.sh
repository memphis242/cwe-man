#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-coverage}"
REPORT_DIR="${2:-coverage}"
FETCH_TEST_DEPS="${CWE_MAN_FETCH_TEST_DEPS:-OFF}"

if ! command -v gcovr >/dev/null 2>&1; then
  echo "gcovr is required. Install it first (see docs/DEVELOPMENT-TOOLING.md)." >&2
  exit 1
fi

cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCWE_MAN_ENABLE_TESTS=ON \
  -DCWE_MAN_ENABLE_COVERAGE=ON \
  -DCWE_MAN_FETCH_TEST_DEPS="$FETCH_TEST_DEPS"

cmake --build "$BUILD_DIR" -j
if ! ctest --test-dir "$BUILD_DIR" -N | grep -q "Total Tests: [1-9]"; then
  echo "No tests discovered. Install GTest or set CWE_MAN_FETCH_TEST_DEPS=ON." >&2
  exit 1
fi
ctest --test-dir "$BUILD_DIR" --output-on-failure

mkdir -p "$REPORT_DIR"

gcovr \
  --root . \
  --filter '^src/' \
  --exclude '.*_deps/.*' \
  --exclude 'src/main.cpp' \
  --fail-under-line 95 \
  --fail-under-branch 90 \
  --txt "$REPORT_DIR/coverage.txt" \
  --html-details "$REPORT_DIR/coverage.html" \
  --xml "$REPORT_DIR/coverage.xml" \
  --json "$REPORT_DIR/coverage.json" \
  --print-summary

echo "Coverage reports written to $REPORT_DIR"
