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

# Prevent stale profile artifacts from previous binaries from breaking gcov/gcovr.
find "$BUILD_DIR" -name '*.gcda' -delete

if ! ctest --test-dir "$BUILD_DIR" -N | grep -q "Total Tests: [1-9]"; then
  echo "No tests discovered. Install GTest or set CWE_MAN_FETCH_TEST_DEPS=ON." >&2
  exit 1
fi
ctest --test-dir "$BUILD_DIR" --output-on-failure

mkdir -p "$REPORT_DIR"

gcovr \
  --root . \
  --object-directory "$BUILD_DIR" \
  --gcov-ignore-errors output_error \
  --gcov-ignore-errors no_working_dir_found \
  --filter '(.*/)?src/.*' \
  --exclude '.*_deps/.*' \
  --exclude '(.*/)?src/main.cpp' \
  --exclude-throw-branches \
  --exclude-unreachable-branches \
  --txt "$REPORT_DIR/coverage.txt" \
  --html-details "$REPORT_DIR/coverage.html" \
  --xml "$REPORT_DIR/coverage.xml" \
  --json "$REPORT_DIR/coverage.json" \
  --print-summary

echo "Coverage reports written to $REPORT_DIR"
