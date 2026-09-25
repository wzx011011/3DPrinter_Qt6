---
plan: 29-02
phase: 29
status: complete
requirements: [ARRANGE-01]
---

# Plan 29-02 Summary: tests/PartPlateTests.cpp + CMake Registration

## What was built

Created `tests/PartPlateTests.cpp` (NEW file — RESEARCH finding #1: it did NOT exist) and registered it in `CMakeLists.txt` following the `ViewModelSmokeTests` pattern (links `owzx_app_core` + Qt6::Test, QT_TESTCASE_SOURCEDIR for fixture paths). Added the `PartPlateTests` target to `auto_verify_with_vcvars.ps1` (build + run block).

ARRANGE-01 unit tests (41 assertions, all pass):
- **`computeColumCount` data-driven parity table** — all 36 rows (count 1..36 → cols), covering every perfect square (1,4,9,16,25,36) and every +1 transition (2,5,10,17,26).
- **`computeOriginGridMath`** — verifies the sign-flip: plate 0=(0,0,0), plate 1=(+stride,0,0), plate 3=(0,-stride,0), plate 4=(+stride,-stride,0).
- **`computePlateIndexRoundTrip`** — verifies the `-world_y/stride_y` decode + round() boundary characterization (x=120 → col 1 via round-half-away-from-zero).
- **`updatePlateOriginsWritesToPlates`** — verifies D-29-13 origin realization: createPlate×4 writes real origins to each plate.

## Key decisions / deviations

- **The test caught a real decode bug** in Plan 01's `computePlateIndex` (the `(stride_y - t_y)/stride_y` form gave row 1 for world (0,0)). Plan 01 was corrected to `-world_y/stride_y`. This is the deterministic-test-replaces-transport-confidence pattern working as intended.

## Verification

- `PartPlateTests.exe` → **41 passed, 0 failed** (geometry unit tests).
- Full canonical verify green.
- `rg -c "QTest::newRow" tests/PartPlateTests.cpp` → ≥ 36.

## Files changed

- `tests/PartPlateTests.cpp` — NEW file (ARRANGE-01 geometry unit tests).
- `CMakeLists.txt` — PartPlateTests target registration.
- `scripts/auto_verify_with_vcvars.ps1` — PartPlateTests build target + run block.
