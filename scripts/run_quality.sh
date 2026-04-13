#!/usr/bin/env bash
set -euo pipefail

./scripts/run_tests.sh build-tests
./scripts/run_coverage.sh build-coverage coverage
./scripts/run_sanitizers.sh build-asan-ubsan build-tsan
./scripts/run_static_analysis.sh build-analysis
