# Development Tooling

This repository uses a layered quality workflow for testing, coverage, sanitizers, static analysis, and benchmarking.

## Required Base Toolchain
- CMake 3.20+
- C++20 compiler (`g++` and/or `clang++`)
- `sqlite3` development headers
- `libcurl` development headers
- `nlohmann_json` development headers

## Recommended Developer Tools
- `gcovr` for coverage reports
- `clang-tidy` for linting/static checks
- `cppcheck` for static analysis
- `valgrind` for memory diagnostics
- `afl++` and LLVM libFuzzer-capable toolchain for fuzzing workflows

## Example Fedora Install
```bash
sudo dnf install -y \
  cmake gcc gcc-c++ clang clang-tools-extra \
  sqlite-devel libcurl-devel nlohmann-json-devel \
  cppcheck python3-gcovr valgrind afl++
```

## Common Commands
```bash
./scripts/run_tests.sh
./scripts/run_coverage.sh
./scripts/run_sanitizers.sh
./scripts/run_static_analysis.sh
./scripts/run_quality.sh
```

If `GTest` is not installed system-wide and network access is available, set:
```bash
export CWE_MAN_FETCH_TEST_DEPS=ON
```

## Coverage Notes
- Current hard coverage targets (core modules): line >= 95%, branch >= 90%.
- MC/DC is currently report-only via proxy decision/condition metrics from coverage tooling.
- True MC/DC gate integration is staged for a later hardening phase.
