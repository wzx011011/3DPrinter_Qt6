---
phase: 25-prepare-model-mesh-rendering-and-camera-interaction
plan: 02
subsystem: rendering
tags: [qrhi, prepare, model-mesh, camera, uniform, performance, tdd]

requires:
  - phase: 25
    plan: 01
    provides: Source-index-aware model batch scene data
provides:
  - QRhi active-plate model vertex buffer rendering
  - CameraController-backed QRhi orbit/pan/zoom/preset/fit interaction
  - MVP uniform update path separated from model buffer uploads
affects: [phase-25, rhi-viewport, prepare-scene-data]

tech-stack:
  added: []
  patterns:
    - Persistent QRhi model vertex buffers
    - Camera MVP uniform updates via QRhi dynamic uniform buffer
    - Separate scene/model/camera generation gates

key-files:
  created: []
  modified:
    - tests/QmlUiAuditTests.cpp
    - src/qml_gui/Renderer/PrepareSceneData.h
    - src/qml_gui/Renderer/PrepareSceneData.cpp
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/shaders/rhi_viewport.vert

key-decisions:
  - "QRhi model rendering consumes Phase 25-01 world-space model vertices directly; no shader- or QML-side coordinate conversion."
  - "Camera interaction is owned by RhiViewport through CameraController and transferred to the renderer as an MVP uniform."
  - "Bed scene changes, model changes, and camera changes use separate dirty/generation paths to avoid unnecessary model buffer uploads."

patterns-established:
  - "Renderer uploads camera uniforms before drawing but excludes DirtyCamera from model upload conditions."
  - "RhiViewport keeps QML-facing requestFitView/requestViewPreset signatures while routing to CameraController."

requirements-completed: []

duration: 44min
completed: 2026-06-27
---

# Phase 25 Plan 02: QRhi Model Mesh Rendering And Camera Uniforms Summary

**QRhi now renders active-plate model geometry through persistent buffers and moves camera changes through MVP uniforms**

## Accomplishments

- Added RED audit coverage for QRhi model buffers, camera uniform resources, CameraController input routing, and no camera-triggered full model upload.
- Upgraded the QRhi vertex format and shader path from 2D bed-only positions to 3D positions transformed by an MVP uniform.
- Added persistent QRhi model vertex buffer ownership and active-plate model draw calls.
- Routed QRhi orbit, pan, zoom, fit view, and view presets through `CameraController`.
- Split scene/model/camera generation and dirty paths so bed-only and camera-only changes do not reparse or reupload the model buffer.

## Task Commits

1. **Task 25-02-T1: Add QRhi model buffer and camera audits first** - `1bb5a64` (test)
2. **Tasks 25-02-T2/T3: Render model batches and wire camera uniforms** - `5e7a8a5` (feat)

## Verification

- RED evidence: `build/phase25_02_t1_red.log` failed at `[UI] QML UI audit tests failed` because QRhi model/camera symbols were absent.
- GREEN evidence: `build/phase25_02_green_attempt2.log` completed with:
  - `[PrepareScene] Prepare scene data tests passed`
  - `[UI] QML UI audit tests passed`
  - `[E2E] All pipeline tests passed`

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Split bed scene generation from model generation**
- **Found during:** Plan close-out review
- **Issue:** The first implementation used a single scene generation counter, so bed-only changes could re-run `setModelMeshData()` and schedule model uploads.
- **Fix:** Added separate `m_modelGeneration` and strengthened `QmlUiAuditTests` so bed setters cannot increment model generation.
- **Files modified:** `src/qml_gui/Renderer/RhiViewport.h`, `src/qml_gui/Renderer/RhiViewport.cpp`, `src/qml_gui/Renderer/RhiViewportRenderer.h`, `src/qml_gui/Renderer/RhiViewportRenderer.cpp`, `tests/QmlUiAuditTests.cpp`
- **Verification:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed in `build/phase25_02_green_attempt2.log`.
- **Committed in:** `5e7a8a5`

---

**Total deviations:** 1 auto-fixed (performance correctness guard)
**Impact on plan:** The fix tightens the intended no-full-reupload design without expanding feature scope.

## User Setup Required

None. QRhi remains behind the existing `OWZX_RHI_RENDERER` gate.

## Next Phase Readiness

Plan 25-03 can build on the rendered model buffers and selected source index to add selection, hover, and picking feedback without changing QML business logic.

## Self-Check: PASSED

- Plan task commits exist: `1bb5a64`, `5e7a8a5`.
- Canonical verification passed after final implementation.
- Camera dirty state is audited separately from model upload state.

---
*Phase: 25-prepare-model-mesh-rendering-and-camera-interaction*
*Completed: 2026-06-27*
