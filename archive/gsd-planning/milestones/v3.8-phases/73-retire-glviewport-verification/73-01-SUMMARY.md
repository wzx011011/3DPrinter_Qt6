---
phase: 73-retire-glviewport-verification
plan: 01
subsystem: rhi-renderer
tags: [rhi, cleanup, verification]
key-files:
  deleted:
    - src/qml_gui/Renderer/GLViewport.cpp
    - src/qml_gui/Renderer/GLViewport.h
    - src/qml_gui/Renderer/GLViewportRenderer.cpp
    - src/qml_gui/Renderer/GLViewportRenderer.h
    - src/qml_gui/Renderer/GCodeRenderer.cpp
    - src/qml_gui/Renderer/GCodeRenderer.h
  modified:
    - CMakeLists.txt
    - src/qml_gui/main_qml.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/core/rendering/GizmoGeometry.h
    - src/core/viewmodels/EditorViewModel.cpp
    - tests/QmlUiAuditTests.cpp
metrics:
  lines_removed: 3722
  canonical_verifier: passed
---

# Phase 73 Plan 01 Summary

## What Changed

- Deleted the retired OpenGL viewport implementation:
  - `GLViewport.cpp/.h`
  - `GLViewportRenderer.cpp/.h`
  - `GCodeRenderer.cpp/.h`
- Removed those files from `owzx_app_core` in `CMakeLists.txt`.
- Removed the `OWZX_OPENGL` startup branch from `main_qml.cpp`.
- Kept the stable QML type name `OWzxGL.GLViewport`, now backed only by:
  - `RhiViewport` on the normal QRhi path
  - `SoftwareViewport` when QRhi is unavailable
- Added `QmlUiAuditTests::legacyOpenGlViewportPathsStayDeleted()` to lock the
  deletion and prevent reintroducing the old env-var path.
- Updated existing QML audit assertions that previously required the legacy GL
  branch to remain present.
- Updated stale comments that described active behavior as owned by the old GL
  viewport.

## Source Truth

- Phases 65-72 already moved the behavior needed from the GL path into pure
  helpers and the RHI renderer.
- Phase 73 does not introduce product behavior. It retires the old implementation
  now that RHI owns gizmo rendering, gizmo interaction, precise picking, cut
  plane, wipe tower, and Preview G-code rendering.

## Verification

- RED check passed: the new deletion guard failed while `GLViewport.cpp` still
  existed.
- Focused build passed:
  `cmake --build build --target QmlUiAuditTests OWzxSlicer`.
- Focused tests passed:
  - `QmlUiAuditTests::legacyOpenGlViewportPathsStayDeleted`
  - full `QmlUiAuditTests` (46 passed, 0 failed)
- Runtime-source scan found no remaining `OWZX_OPENGL`, `GLViewport.h`,
  `qmlRegisterType<GLViewport>`, or `GCodeRenderer` references in active
  source/CMake.
- `git diff --check` passed.
- Encoding guard passed on phase-touched files.
- Canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## Deviations

- The QML type name `GLViewport` was intentionally retained. Renaming it would
  require broad QML enum/binding churn with no behavior benefit; the retirement
  target is the OpenGL implementation, not the QML compatibility alias.
- Historical comments in pure helper files that cite `GLViewportRenderer.cpp` as
  the original source location were left in place as provenance, not active
  dependencies.

## Self-Check

PASSED. Phase 73 requirements `GRET-01` and `GRET-02` are covered by file
deletion, CMake cleanup, startup branch removal, QML audit deletion guards,
focused tests, and canonical verification.
