---
phase: 24-prepare-scene-data-and-plate-rendering
plan: 01
subsystem: rendering
tags: [qt6, qrhi, prepare, scene-data, tests]

requires:
  - phase: 24-context
    provides: Prepare scene/cache and dirty upload constraints
provides:
  - Pure C++ PrepareSceneData scene/cache contract
  - Dirty flags for bed, plate, mesh, visibility, and GPU resource state
  - Deterministic bed fill, border, 10 mm grid, 50 mm grid, and origin cue geometry
  - PrepareSceneDataTests canonical coverage
affects: [phase-24, phase-25, phase-26]

tech-stack:
  added: [Qt Test target]
  patterns:
    - Scene conversion is testable C++ before QRhi resource ownership
    - Canonical script builds and runs new focused tests explicitly

key-files:
  created:
    - src/qml_gui/Renderer/PrepareSceneData.h
    - src/qml_gui/Renderer/PrepareSceneData.cpp
    - tests/PrepareSceneDataTests.cpp
  modified:
    - CMakeLists.txt
    - scripts/auto_verify_with_vcvars.ps1
    - .planning/phases/24-prepare-scene-data-and-plate-rendering/24-01-PLAN.md
    - .planning/phases/24-prepare-scene-data-and-plate-rendering/24-VALIDATION.md
    - .planning/ROADMAP.md
    - .planning/STATE.md

key-decisions:
  - "PrepareSceneData is pure C++ and uses QtCore containers only; QRhi resource ownership remains for Plan 24-03."
  - "Invalid or out-of-range plate context becomes an empty active context instead of falling back to all object indices."
  - "The canonical verification script now fails immediately on missing required build targets and runs PrepareSceneDataTests before UI/app smoke."

patterns-established:
  - "Focused renderer data tests are built and run by the canonical script."
  - "Dirty flag APIs separate `peekDirtyFlags()` from `takeDirtyFlags()` so upload code can consume state deliberately."

requirements-progress: [RHI-04, PREP-01, PREP-05]

duration: 3h
completed: 2026-06-27
---

# Phase 24 Plan 01: Prepare Scene Data Contract And Dirty Flags Summary

**Pure C++ scene/cache contract with deterministic bed geometry and dirty flag tests**

## Accomplishments

- Added `PrepareSceneData`, a renderer-facing scene data object for bed metrics, generated bed/grid/origin vertices, active plate context, mesh generation, and dirty flag tracking.
- Added `PrepareSceneDataTests` covering bed geometry intervals, dirty flag consumption, active plate isolation, and bounded invalid bed dimensions.
- Added the new source files to `owzx_app_core` and the test target to CMake.
- Updated the canonical verification script so required ninja targets fail immediately, `PrepareSceneDataTests` is built and run, and the absent `test-slice-direct` target is treated as optional with explicit output.

## TDD Evidence

- RED: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` failed at `PrepareSceneDataTests.cpp` because `qml_gui/Renderer/PrepareSceneData.h` did not exist.
- GREEN: `build/PrepareSceneDataTests.exe` exited 0 after implementation.
- FULL GREEN: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; output included `Prepare scene data tests passed`, `QML UI audit tests passed`, app smoke PID, and E2E pipeline passed.

## Files Created/Modified

- `src/qml_gui/Renderer/PrepareSceneData.h` - Scene state, vertex struct, dirty flags, and accessors.
- `src/qml_gui/Renderer/PrepareSceneData.cpp` - Bed/grid/origin geometry generation and dirty flag implementation.
- `tests/PrepareSceneDataTests.cpp` - Focused Qt Test coverage for Phase 24 scene data.
- `CMakeLists.txt` - Adds app core sources and `PrepareSceneDataTests`.
- `scripts/auto_verify_with_vcvars.ps1` - Builds/runs the new test and fixes hidden target-failure handling.

## Deviations from Plan

- The canonical script needed a verification fix: it previously built fixed targets and only checked `$LASTEXITCODE` after several ninja calls, which hid the missing `test-slice-direct` target. The script now checks each required target immediately and skips the missing slice-direct target explicitly as optional.

## Next Plan Readiness

Ready for Plan 24-02. The next step can bind `EditorViewModel` bed/plate state into `RhiViewport` and pass it to `PrepareSceneData`; no QML scene conversion is needed.
