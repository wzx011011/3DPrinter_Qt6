---
phase: 21-review-fixes
plan: 01
subsystem: bugfix
tags: [partplate, clone, config, review-driven, regression-guard]

requires:
  - phase: 17
    provides: "clonePlate (buggy in mock mode)"
  - phase: 19
    provides: "setPlateScopedOptionValue (silent on unknown key)"
provides:
  - "clonePlate targets dst in mock mode (BUG-1 fixed)"
  - "setPlateScopedOptionValue warns on unknown key (BUG-2 fixed)"
  - "clone regression guard test"
  - "config-merge direction confirmed test (D-15 assumption verified)"
affects: []

key-files:
  created: []
  modified:
    - src/core/services/ProjectServiceMock.cpp
    - tests/ViewModelSmokeTests.cpp

requirements-affected: [PLATE-03, PLATE-10]

duration: 25min
completed: 2026-06-26
---

# Plan 21-01: Review-Driven Bug Fixes Summary

**Fixed 2 functional bugs found in the v3.0 code review + added regression guards. The merge-direction test also CONFIRMED the D-15 apply() assumption is correct (plate wins over preset).**

## Performance
- **Duration:** ~25 min (1 real build-fix: ConfigOption has no setFloat; use .value + double compare)
- **Files modified:** 2

## Bugs Fixed

### BUG-1: clonePlate mock-mode targets wrong plate (PLATE-03)
- **Root cause:** clonePlate reused duplicateObject, whose mock branch adds the new object to `currentPlate()`. clonePlate never set current=dst, so clones landed on whatever current was.
- **Fix:** `ProjectServiceMock.cpp` clonePlate now saves current, sets `current=dst->plateIndex()` for the duplicate loop, restores after. The std::set membership dedupes the (now-redundant) explicit `dst->addInstance`, so no double-counting. HAS mode was already correct (its duplicateObject doesn't set membership).
- **DESIGN-1 (comment):** rewrote the inaccurate "inserts at sourceIndex+1" comment to honestly describe both branches (HAS appends; mock inserts+shifts).
- **Regression guard:** strengthened `projectServiceClonePlateDeepCopiesObjects` to assert source-plate count unchanged AND current-plate count unchanged after clone (would have caught BUG-1).

### BUG-2: setPlateScopedOptionValue silently drops unknown keys (PLATE-10)
- **Root cause:** `cfg.option(k, true)` returns null for keys not in the config schema; the code returned false with no log. Per-plate overrides for typo'd/unregistered keys were silently lost.
- **Fix:** `qWarning("[PartPlate] unknown config key '%s' for plate %d; override dropped")` before returning false. Now diagnosable.

## Verification Added

### Merge-direction test (TEST-2 confirmed, not a bug)
- Added `sliceServiceConfigMergeDirectionPlateWins`: constructs base (layer_height=0.2) + plate (0.4), `base.apply(plate)`, asserts merged=0.4.
- **Result: PASS** — `DynamicPrintConfig::apply(other)` makes `other` win. This CONFIRMS SliceService.cpp:393's D-15 assumption is correct. The comment-asserted direction is now test-asserted.

## Task Commits
- **All tasks:** `59c01bf` (fix) — single commit; the 2 fixes + 2 tests are cohesive.

## Decisions Made
- **double vs float compare:** the merge test initially failed on `0.4f` (float literal) vs `0.4` (double from getFloat) precision. Fixed by casting the compared value to double and using `0.4` (no suffix). The failure was a test-harness precision artifact, not a real direction bug.

## Issues Encountered
- **ConfigOption base has no setFloat:** the test tried `o->setFloat(0.2)` on a `ConfigOption*` base pointer. Fixed via `dynamic_cast<ConfigOptionFloat*>` + `.value` member write (same pattern as the Phase 19 setPlateScopedOptionValue fix).
- **ninja cached test binary:** the standard build reused a cached test .obj the first time; forcing the target rebuild surfaced the real error (setFloat). Same cache issue as Phase 18.

## Next Phase Readiness
- BUG-1 and BUG-2 are fixed and regression-guarded. The remaining review notes (DESIGN-2 canSlice dead field, DESIGN-3 dual-source, TEST-1 fixture gap) are non-blocking and recorded for v3.1.
- v3.0 is now review-clean for the P0 items. Ready for `/gsd-complete-milestone v3.0`.

---
*Phase: 21-review-fixes*
*Completed: 2026-06-26*
