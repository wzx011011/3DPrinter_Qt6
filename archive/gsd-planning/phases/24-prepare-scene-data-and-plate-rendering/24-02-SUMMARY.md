---
phase: 24-prepare-scene-data-and-plate-rendering
plan: 02
subsystem: rendering
tags: [qt6, qrhi, prepare, plate-context, tests]

requires:
  - phase: 24-01
    provides: PrepareSceneData scene/cache contract
provides:
  - Renderer-facing active plate object index surface on EditorViewModel
  - Bed and plate Q_PROPERTY contract across RHI, software, and legacy OpenGL viewport implementations
  - PreparePage direct bindings from EditorViewModel to viewport properties
  - RhiViewportRenderer synchronization into PrepareSceneData
affects: [phase-24, phase-25, phase-28]

tech-stack:
  added: []
  patterns:
    - ViewModel owns active plate membership truth; QML only binds it
    - Every registered implementation of the QML GLViewport type must expose the same property surface
    - Rhi renderer synchronization receives discrete bed and plate state before GPU upload work

key-files:
  created:
    - .planning/phases/24-prepare-scene-data-and-plate-rendering/24-02-SUMMARY.md
  modified:
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/SoftwareViewport.h
    - src/qml_gui/Renderer/SoftwareViewport.cpp
    - src/qml_gui/Renderer/GLViewport.h
    - src/qml_gui/Renderer/GLViewport.cpp
    - src/qml_gui/pages/PreparePage.qml
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp
    - .planning/phases/24-prepare-scene-data-and-plate-rendering/24-02-PLAN.md
    - .planning/phases/24-prepare-scene-data-and-plate-rendering/24-VALIDATION.md
    - .planning/ROADMAP.md
    - .planning/STATE.md

key-decisions:
  - "activePlateObjectIndices is derived from ProjectServiceMock::currentPlateObjectIndices() and deliberately bypasses the UI show-all fallback."
  - "PreparePage binds bed and plate context directly; no object filtering, parsing, or membership calculation is moved into QML."
  - "SoftwareViewport and GLViewport expose the same bed/plate properties as RhiViewport because startup registers different C++ classes under the same QML type depending on backend gates."

patterns-established:
  - "Viewport QML property contracts must be audited across all backend implementations, not only the new RHI path."
  - "RhiViewportRenderer synchronizes CPU scene inputs into PrepareSceneData before Plan 24-03 adds QRhi buffer ownership."

requirements-progress: [RHI-04, PREP-01, PREP-05]

duration: 2h
completed: 2026-06-27
---

# Phase 24 Plan 02: Prepare Bed And Plate Context Binding Summary

**C++ plate membership and bed context now reach the viewport through direct bindings**

## Accomplishments

- Added `EditorViewModel::activePlateObjectIndices`, backed by `ProjectServiceMock::currentPlateObjectIndices()`, so renderer membership follows selected plate truth without using UI fallback behavior.
- Added bed metrics, current plate metadata, and active object index properties to `RhiViewport`; setters store state and request redraw.
- Synchronized the RHI viewport state into `PrepareSceneData` inside `RhiViewportRenderer::synchronize()`.
- Bound `PreparePage.qml` viewport properties directly to `EditorViewModel` bed and plate properties.
- Added the same property surface to `SoftwareViewport` and legacy `GLViewport`, because all three implementations can be registered as the QML `GLViewport` type depending on renderer gates.
- Added regression coverage in `ViewModelSmokeTests` and `QmlUiAuditTests`.

## TDD Evidence

- RED: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` failed while compiling `ViewModelSmokeTests` because `EditorViewModel` did not expose `activePlateObjectIndices`.
- GREEN iteration: the focused tests and QML audit passed after adding the viewmodel and RHI bindings, but app smoke failed at startup with `Cannot assign to non-existent property "activePlateObjectIndices"`.
- ROOT CAUSE: the default startup path registers `SoftwareViewport` as the QML `GLViewport` type, so the QML property contract had to be present on every backend implementation, not just `RhiViewport`.
- FULL GREEN: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0; output included `Prepare scene data tests passed`, `QML UI audit tests passed`, app smoke PID, and `All pipeline tests passed`.

## Files Created/Modified

- `src/core/viewmodels/EditorViewModel.h/.cpp` - Renderer-facing active plate object index property.
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` - Bed and plate Q_PROPERTY storage.
- `src/qml_gui/Renderer/RhiViewportRenderer.h/.cpp` - PrepareSceneData synchronization.
- `src/qml_gui/Renderer/SoftwareViewport.h/.cpp` - Default QML registration path property parity.
- `src/qml_gui/Renderer/GLViewport.h/.cpp` - Legacy OpenGL path property parity.
- `src/qml_gui/pages/PreparePage.qml` - Direct binding-only connection to viewport properties.
- `tests/ViewModelSmokeTests.cpp` - Active plate membership regression test.
- `tests/QmlUiAuditTests.cpp` - Static guard for binding-only QML and viewport property parity.

## Deviations from Plan

- The original plan listed only the RHI viewport for new properties. Verification showed the QML type is backed by `SoftwareViewport` in default startup, so `SoftwareViewport` and `GLViewport` were added to the contract and audit scope.

## Next Plan Readiness

Ready for Plan 24-03. The renderer now receives bed, plate, and active object context in C++ state; the next plan can replace the diagnostic triangle with QRhi bed/grid/plate buffers.
