# Performance Measurement Plan (Measure-Only)

Quality and correctness remain the primary gates. Performance work in this phase is measurement-first.

## What Is Implemented
- Benchmark scaffold target: `cwe-man-bench`
- Initial benchmark focus:
  - repository search path (`search_cwes`)
  - category-to-CWE query path (`get_cwes_for_category`)
- Output reports wall-clock timing in milliseconds for repeated operations.

## Build and Run
```bash
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DCWE_MAN_ENABLE_BENCHMARKS=ON
cmake --build build-bench -j
./build-bench/cwe-man-bench
```

## Interpretation Guidance
- Compare runs only on stable hardware/load conditions.
- Use repeated runs and median values for tracking.
- Do not apply fail gates in this phase; use output to prioritize optimization candidates.
