---
phase: 25-prepare-model-mesh-rendering-and-camera-interaction
plan: 01
subsystem: rendering
tags: [qrhi, prepare, mesh, source-mapping, qml-boundary, tdd]

requires:
  - phase: 24-prepare-scene-data-and-plate-rendering
    provides: PrepareSceneData bed/plate cache and dirty flags
provides:
  - Source-index-aware Prepare model batch scene data
  - Renderer-facing mesh batch source-object metadata
  - Selected source object state binding for QRhi/GL/Software viewport parity
affects: [phase-25, phase-26, rhi-viewport, prepare-scene-data]

tech-stack:
  added: []
  patterns:
    - Pure C++ renderer scene contracts before QRhi resource ownership
    - QML binding-only metadata forwarding
    - Explicit source-object mapping instead of render-ID inference

key-files:
  created: []
  modified:
    - tests/PrepareSceneDataTests.cpp
    - tests/QmlUiAuditTests.cpp
    - src/qml_gui/Renderer/PrepareSceneData.h
    - src/qml_gui/Renderer/PrepareSceneData.cpp
    - src/core/services/ProjectServiceMock.h
    - src/core/services/ProjectServiceMock.cpp
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/GLViewport.h
    - src/qml_gui/Renderer/GLViewport.cpp
    - src/qml_gui/Renderer/SoftwareViewport.h
    - src/qml_gui/Renderer/SoftwareViewport.cpp
    - src/qml_gui/pages/PreparePage.qml

key-decisions:
  - "Keep packed render object IDs renderer-local; selection identity uses explicit source object indices."
  - "Expose mesh batch source indices through C++ viewmodel and viewport properties, with QML limited to binding."
  - "Selection and hover dirty state in PrepareSceneData does not mark full model mesh or GPU buffer dirty."

patterns-established:
  - "Model scene parsing validates packed mesh byte lengths before allocation."
  - "Fallback viewport property parity is required for any new PreparePage viewport binding."

requirements-completed: [PREP-02, PREP-04, PREP-07]

duration: 54min
completed: 2026-06-27
---

# Phase 25 Plan 01: Model Batch Contract And Source Mapping Summary

**Source-indexed Prepare mesh scene data with validated packed-mesh parsing and viewport metadata bindings**

## Performance

- **Duration:** 54 min
- **Started:** 2026-06-27T10:37:00Z
- **Completed:** 2026-06-27T11:31:00Z
- **Tasks:** 3
- **Files modified:** 15

## Accomplishments

- Added RED tests for packed model batch parsing, malformed payload rejection, active-plate filtering, and selection/hover dirty separation.
- Extended `PrepareSceneData` with model vertices, model batches, aggregate bounds, selected/hovered source indices, `DirtySelection`, and `DirtyCamera`.
- Added an empty-active-plate regression so invalid or empty plates do not fall back to rendering all model batches.
- Added renderer-facing mesh batch source-object metadata from `ProjectServiceMock` through `EditorViewModel`.
- Bound mesh batch source indices and selected source index into `PreparePage.qml` while keeping QML free of filtering or mapping logic.
- Preserved fallback property parity across `RhiViewport`, `GLViewport`, and `SoftwareViewport`.

## Task Commits

1. **Task 25-01-T1: Add model batch parsing tests first** - `66cdf89` (test)
2. **Task 25-01-T2: Implement source-index-aware model scene data** - `61bb918` (feat)
3. **Task 25-01-T3: Expose renderer batch source metadata through viewmodels and viewport parity** - `8bdffb1` (test), `2949e81` (feat)
4. **Regression: Empty active plate mesh filtering** - `88c3baf` (test), `fd76ba2` (fix)

## Files Created/Modified

- `tests/PrepareSceneDataTests.cpp` - RED/GREEN coverage for model batch parsing, invalid payloads, active filtering, and selection/hover dirty behavior.
- `tests/QmlUiAuditTests.cpp` - Static guards for new renderer metadata bindings and fallback viewport property parity.
- `src/qml_gui/Renderer/PrepareSceneData.h/.cpp` - Source-index-aware model scene contract and packed mesh parser.
- `src/core/services/ProjectServiceMock.h/.cpp` - Same-order mesh batch source object metadata.
- `src/core/viewmodels/EditorViewModel.h/.cpp` - QML-facing metadata and selected source object properties.
- `src/qml_gui/Renderer/RhiViewport.*`, `GLViewport.*`, `SoftwareViewport.*` - Compatible metadata property storage.
- `src/qml_gui/pages/PreparePage.qml` - Binding-only metadata forwarding.

## Decisions Made

- Render IDs stay renderer-local because existing `meshData()` uses pointer-derived IDs that are unsuitable for editor selection.
- Source-object identity is carried as a parallel same-order list instead of changing the packed mesh format, preserving existing GL/Software parsers.
- Selection and hover state are represented as lightweight dirty state in `PrepareSceneData`; full mesh/GPU dirty flags remain reserved for geometry changes.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Tightened mesh batch metadata traversal**
- **Found during:** Task 25-01-T3
- **Issue:** A simple object/instance count traversal could diverge from `meshData()` when an object has no valid triangles or null instances.
- **Fix:** `meshBatchSourceObjectIndices()` now checks for at least one valid triangle and mirrors the non-null instance behavior used by `meshData()`.
- **Files modified:** `src/core/services/ProjectServiceMock.cpp`
- **Verification:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed in `build/phase25_01_t3_green_attempt2.log`.
- **Committed in:** `2949e81`

---

**2. [Rule 2 - Missing Critical] Removed empty-active-list all-object fallback**
- **Found during:** Plan close-out review
- **Issue:** `PrepareSceneData` treated an empty active source list as "render all", which would leak inactive model batches on empty or invalid plates.
- **Fix:** Added `emptyActivePlateDoesNotFallbackToAllModelBatches` and changed active filtering to require explicit source-index membership.
- **Files modified:** `tests/PrepareSceneDataTests.cpp`, `src/qml_gui/Renderer/PrepareSceneData.cpp`
- **Verification:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` failed as expected in `build/phase25_01_empty_plate_red.log` and passed in `build/phase25_01_empty_plate_green.log`.
- **Committed in:** `88c3baf`, `fd76ba2`

---

**Total deviations:** 2 auto-fixed (missing critical correctness guards)
**Impact on plan:** The fix keeps the planned source mapping contract correct and does not expand scope.

## Issues Encountered

- Canonical script exceeded the default tool timeout during early RED verification. Re-ran the same canonical script with output redirected to `build/phase25_01_red_attempt.log`, which captured the expected compile failure for missing `PrepareSceneData` APIs.

## Verification

- RED evidence: `build/phase25_01_red_attempt.log` failed at `PrepareSceneDataTests.cpp` because `setModelMeshData`, `modelBatches`, `modelVertices`, `DirtySelection`, and source selection APIs were absent.
- RED evidence: `build/phase25_01_t3_red_attempt.log` failed at `QmlUiAuditTests` because the metadata properties and QML bindings were absent.
- RED evidence: `build/phase25_01_empty_plate_red.log` failed at `PrepareSceneDataTests` because an empty active source list still rendered model batches.
- GREEN evidence: `build/phase25_01_t3_green_attempt2.log` completed with:
  - `[PrepareScene] Prepare scene data tests passed`
  - `[UI] QML UI audit tests passed`
  - `[E2E] All pipeline tests passed`
- GREEN evidence: `build/phase25_01_empty_plate_green.log` completed with:
  - `[PrepareScene] Prepare scene data tests passed`
  - `[UI] QML UI audit tests passed`
  - `[E2E] All pipeline tests passed`

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 25-02 can consume `PrepareSceneData::modelVertices()`, `modelBatches()`, model bounds, `meshBatchSourceObjectIndices`, and `selectedSourceObjectIndex` to build persistent QRhi model buffers and camera uniforms.

## Self-Check: PASSED

- Plan task commits exist: `66cdf89`, `61bb918`, `8bdffb1`, `2949e81`, `88c3baf`, `fd76ba2`.
- Key files listed above exist and were modified.
- Canonical verification passed after final Task 25-01 implementation.

---
*Phase: 25-prepare-model-mesh-rendering-and-camera-interaction*
*Completed: 2026-06-27*
