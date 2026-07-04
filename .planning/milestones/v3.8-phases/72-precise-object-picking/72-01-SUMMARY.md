---
phase: 72-precise-object-picking
plan: 01
subsystem: rhi-renderer
tags: [rhi, picking, gizmo, source-truth]
key-files:
  created:
    - src/core/rendering/ObjectPicking.h
    - src/core/rendering/ObjectPicking.cpp
    - tests/ObjectPickingTests.cpp
  modified:
    - CMakeLists.txt
    - src/core/rendering/GizmoMath.h
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - tests/QmlUiAuditTests.cpp
metrics:
  tests_added: 4
  canonical_verifier: passed
---

# Phase 72 Plan 01 Summary

## What Changed

- Added pure CPU `ObjectPicking` for default-path source-object picking.
- Implemented ray-AABB prefiltering plus Moller-Trumbore ray-triangle
  intersection against `PrepareSceneData` scene-space vertices and model
  batches.
- Replaced `RhiViewport::pickSourceObjectAt` projected screen-rectangle
  selection with:
  - `GizmoMath::computeRay`
  - `ObjectPicking::pickSourceObject`
  - `PrepareSceneData::modelVertices`
  - `PrepareSceneData::modelBatches`
- Removed the obsolete `projectBoundsToScreenRect` helper from `RhiViewport`.
- Added `ObjectPickingTests` for triangle miss inside an AABB, nearest hit
  selection, invalid/degenerate batch rejection, and screen-ray integration
  through `GizmoMath`.
- Strengthened `QmlUiAuditTests::rhiViewportSelectionPickingBridgeStaysCppOwned`
  so RHI selection remains C++-owned and cannot fall back to projected AABB
  picking.

## Source Truth

- This phase ports the precision behavior needed by the GL path: object
  selection is determined by the ray intersecting actual mesh triangles, not by
  the cursor landing inside a projected bounding rectangle.
- QML remains a signal bridge. Ray construction, scene access, batch iteration,
  AABB filtering, and triangle tests all stay in C++.

## Verification

- RED build reached the intended missing-helper failure: `ObjectPickingTests`
  was registered before `ObjectPicking.cpp/.h` existed, so CMake generation
  failed on the missing source file.
- Focused targets built:
  `cmake --build build --target ObjectPickingTests QmlUiAuditTests OWzxSlicer`.
- Focused tests passed:
  - `ObjectPickingTests` full suite
  - `QmlUiAuditTests::rhiViewportSelectionPickingBridgeStaysCppOwned`
  - full `QmlUiAuditTests`
- `OWzxSlicer` linked successfully after `ObjectPicking` was added to the app
  core target.
- `git diff --check` passed.
- Encoding guard passed on touched files.
- Canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## Deviations

- The first QML audit guard rejected the substring `gl`, which also appears in
  ordinary words such as `triangle`. The guard was narrowed to actual renderer
  API tokens (`QRhi`, `QQuick`, `QOpenGL`, `glBind`, `glDraw`, `GL_`) while
  preserving the intended C++ ownership check.
- The first application link attempt exposed that `ObjectPicking.cpp` was only
  registered for the test target. It was added to `owzx_app_core`, then the
  focused app build and canonical verifier passed.

## Self-Check

PASSED. Phase 72 requirement `GPICK-01` is covered by a pure picking helper,
RHI viewport integration, regression tests for projected-AABB false positives,
C++ ownership audit coverage, and canonical verification.
