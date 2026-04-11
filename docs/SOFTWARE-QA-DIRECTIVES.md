# Software Quality Assurance Principles

## Code Quality & Robustness
- `assert()` throughout the code at any point assumption are made prior to line executions
- C++20 constructs prioritized
- thorough set of compiler warnings
- multiple compilers (e.g., gcc, clang, msvc)
- multiple static analyzers (e.g., cppcheck, clang-analyzer, gcc static analyzer)
- linting against Google's C++ coding guidelines (e.g., clang-tidy)
- unit tests + multiple sanitizers, striving for good _behavioral_ coverage _in addition to_ max possible [MC/DC](https://en.wikipedia.org/wiki/Modified_condition/decision_coverage) coverage
- integration tests (nominal/chaos monkey) /w test frameworks (e.g., `TUI Test` for TUI apps)
- builds for multiple platforms (Linux, Windows, MacOS) - no platform-specific behavior (e.g., `_GNU_SOURCE` or GNU variants of compilers)
- internal function fuzzing
- inter-module fuzzing
- app-level fuzz harness testing
- manual + multi-agent scans of codebase against CWEs
- manual + multi-agent scans of codebase against CVEs of dependencies
- manual + agent scan of codebase against for best-practice idiomatic constructs in language + library usage

## Architecture-Level Robustness
- Maximize encapsulation of data/functionality internal to a module
- Modules and the APIs they expose shall be internally orthogonal
