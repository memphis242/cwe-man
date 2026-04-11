# QA Implementation Map

This file maps `SOFTWARE-QA-DIRECTIVES.md` to concrete implementation in the current repo state.

## Current Implementations
- Assertions and invariants:
  - Existing runtime checks plus targeted parser/state hardening updates.
  - Additional assertion expansion remains ongoing in module internals.
- C++20 usage:
  - Project stays on C++20 (`CMAKE_CXX_STANDARD 20`).
- Compiler warnings:
  - Strict warning flags enabled by CMake option (`CWE_MAN_ENABLE_WARNINGS`).
- Multiple compilers:
  - GCC and Clang are supported in build/test scripts.
- Static analyzers:
  - `clang-tidy` and `cppcheck` are integrated through CMake options.
- Linting:
  - `clang-tidy` is the default lint/static-quality path.
- Unit tests + sanitizers:
  - GoogleTest + CTest harness added.
  - ASan/UBSan and TSan execution scripts added.
- Integration tests:
  - Deterministic event-driven TUI interaction tests added at layout/event level.
- Multi-platform posture:
  - Linux is the active gate; CMake/options are structured for portable expansion.
- Fuzzing layers:
  - Strategy captured in `SECURITY-PLAN.md`; harness implementation is the next phase.
- Manual/agentic scans:
  - Security and quality review guidance documented; workflow scripts included.
- Encapsulation/orthogonality:
  - Module boundaries remain `app/ui/api/data/logging`; tests target seams per module.

## Coverage Policy (Current)
- Hard gates (core modules):
  - line coverage >= 95%
  - branch coverage >= 90%
- MC/DC:
  - report-only in this phase (proxy metrics through coverage tooling).

## Commands
```bash
./scripts/run_tests.sh
./scripts/run_coverage.sh
./scripts/run_sanitizers.sh
./scripts/run_static_analysis.sh
./scripts/run_quality.sh
```
