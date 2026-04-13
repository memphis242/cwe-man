#!/usr/bin/env bash
set -euo pipefail

ASAN_BUILD="${1:-build-asan-ubsan}"
TSAN_BUILD="${2:-build-tsan}"
FETCH_TEST_DEPS="${CWE_MAN_FETCH_TEST_DEPS:-OFF}"

cmake -S . -B "$ASAN_BUILD" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCWE_MAN_ENABLE_TESTS=ON \
  -DCWE_MAN_ENABLE_ASAN=ON \
  -DCWE_MAN_ENABLE_UBSAN=ON \
  -DCWE_MAN_FETCH_TEST_DEPS="$FETCH_TEST_DEPS"
cmake --build "$ASAN_BUILD" -j
if ! ctest --test-dir "$ASAN_BUILD" -N | grep -q "Total Tests: [1-9]"; then
  echo "No tests discovered. Install GTest or set CWE_MAN_FETCH_TEST_DEPS=ON." >&2
  exit 1
fi
ctest --test-dir "$ASAN_BUILD" --output-on-failure

cmake -S . -B "$TSAN_BUILD" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCWE_MAN_ENABLE_TESTS=ON \
  -DCWE_MAN_ENABLE_TSAN=ON \
  -DCWE_MAN_FETCH_TEST_DEPS="$FETCH_TEST_DEPS"
cmake --build "$TSAN_BUILD" -j
if ! ctest --test-dir "$TSAN_BUILD" -N | grep -q "Total Tests: [1-9]"; then
  echo "No tests discovered. Install GTest or set CWE_MAN_FETCH_TEST_DEPS=ON." >&2
  exit 1
fi
ctest --test-dir "$TSAN_BUILD" --output-on-failure
