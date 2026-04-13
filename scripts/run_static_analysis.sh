#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-analysis}"
FETCH_TEST_DEPS="${CWE_MAN_FETCH_TEST_DEPS:-OFF}"

cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCWE_MAN_ENABLE_TESTS=ON \
  -DCWE_MAN_ENABLE_CLANG_TIDY=ON \
  -DCWE_MAN_ENABLE_CPPCHECK=ON \
  -DCWE_MAN_FETCH_TEST_DEPS="$FETCH_TEST_DEPS"

cmake --build "$BUILD_DIR" -j
if ! ctest --test-dir "$BUILD_DIR" -N | grep -q "Total Tests: [1-9]"; then
  echo "No tests discovered. Install GTest or set CWE_MAN_FETCH_TEST_DEPS=ON." >&2
  exit 1
fi
ctest --test-dir "$BUILD_DIR" --output-on-failure
