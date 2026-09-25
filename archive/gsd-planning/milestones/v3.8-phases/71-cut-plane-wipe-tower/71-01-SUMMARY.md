---
phase: 71-cut-plane-wipe-tower
plan: 01
subsystem: rhi-renderer
tags: [rhi, gizmo, cut-plane, wipe-tower, source-truth]
key-files:
  created:
    - .planning/phases/71-cut-plane-wipe-tower/71-CONTEXT.md
    - .planning/phases/71-cut-plane-wipe-tower/71-01-PLAN.md
  modified:
    - src/core/rendering/GizmoGeometry.h
    - src/core/rendering/GizmoGeometry.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - tests/GizmoGeometryTests.cpp
    - tests/QmlUiAuditTests.cpp
metrics:
  tests_added: 5
  canonical_verifier: passed
---

# Phase 71 Plan 01 Summary

## What Changed

- Added pure CPU `GizmoGeometry` builders for:
  - cut plane fill vertices
  - cut plane outline vertices
  - wipe tower box vertices
- Added deterministic geometry tests for cut-plane counts, colors, alpha,
  bbox expansion, and wipe-tower dimensions.
- Extended `RhiViewportRenderer` to:
  - read wipe tower properties from `RhiViewport::synchronize()`
  - track cut/wipe dirty state
  - upload cut fill, cut outline, and wipe tower buffers before render pass
  - render cut plane in Cut/AdvancedCut mode only when a source object is
    selected
  - render wipe tower when bed is visible and `showWipeTower` is true
  - use source-alpha blended, no-depth-write pipelines for translucent fill and
    outline geometry
- Added `QmlUiAuditTests::rhiCutPlaneAndWipeTowerStayCppOwned()` to pin the C++
  ownership boundary.

## Source Truth

- Cut plane behavior follows the local GL source-truth path:
  `GLViewportRenderer::renderCutPlane`.
- Wipe tower geometry follows:
  `GLViewportRenderer::buildWipeTowerGeometry` and `renderWipeTower`.
- RHI behavior keeps QML as a thin binding layer. No geometry, bbox, or QRhi
  pipeline logic moved into QML.

## Verification

- RED build reached the intended missing API failure:
  `GizmoGeometry` lacked `buildCutPlaneVertices`,
  `buildCutPlaneOutlineVertices`, and `buildWipeTowerVertices`.
- Focused targets built:
  `cmake --build build --target GizmoGeometryTests QmlUiAuditTests`.
- Focused tests passed:
  - `GizmoGeometryTests::testCutPlaneGeometry`
  - `GizmoGeometryTests::testCutPlaneAxisColors`
  - `GizmoGeometryTests::testWipeTowerGeometry`
  - `GizmoGeometryTests::testWipeTowerRejectsInvalidDimensions`
  - `QmlUiAuditTests::rhiCutPlaneAndWipeTowerStayCppOwned`
- Full `GizmoGeometryTests` and `QmlUiAuditTests` passed.
- `OWzxSlicer` target built successfully after the RHI renderer changes.
- `git diff --check` passed.
- Encoding guard passed on touched files.
- Canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## Deviations

- The ROADMAP mentions cutAxis/cutPosition interaction. Existing QML controls
  already mutate the viewmodel and bind those values into the viewport; this
  phase made the RHI renderer react to those bound values. Direct viewport drag
  of the cut plane remains out of scope.
- No new wipe tower viewmodel state was introduced. `RhiViewport` already owns
  the wipe tower properties, and this phase renders when those properties are
  present and set by existing/future scene wiring.

## Self-Check

PASSED. Phase 71 requirements `GCUT-01` and `GWT-01` are covered by code,
focused tests, source ownership audit, and canonical verification.
