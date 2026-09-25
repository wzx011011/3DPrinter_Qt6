---
phase: 90-assembleview-shell-and-canvas-host-restoration
plan: 01
subsystem: qml-gui
tags: [assembleview, canvas-host, qml, rhi, routing]
requires:
  - .planning/phases/89-assembleview-source-truth-gap-audit/89-GAP-MATRIX.md
  - .planning/phases/90-assembleview-shell-and-canvas-host-restoration/90-CONTEXT.md
  - src/qml_gui/pages/Plater.qml
  - src/qml_gui/Renderer/RhiViewport.h
provides:
  - AssemblePage.qml (4-region AssembleView shell)
  - CanvasAssembleView = 2 canvas host registration
  - BackendContext activeCanvasType routing hook
  - EditorViewModel AssembleView gizmo/selection routing
  - BBLTopbar AssembleView navigation toggle
  - QmlUiAuditTests AssembleView shell + canvas host assertions
affects: [91-explosion-ratio-and-assembly-rendering, 92-assembly-measurement-gizmo, 93-assembleview-verification-and-cleanup]
tech_stack_added: [CanvasAssembleView enum value, setActiveCanvasType routing hook]
patterns: [canvas-type routing, shared viewmodel single-stack, render-guard widening]
requirements_completed: [ASMSHELL-01, ASMSHELL-02, ASMROUTE-01]
completed: 2026-07-09
---

# Phase 90 Plan 01 Summary

## What Changed

Phase 90 plan 01 replaced the `Plater.qml` AssembleView placeholder ("装配视图
暂不可用") with a real canvas host + 4-region page shell + navigation toggle +
`CanvasAssembleView` routing, on the default QRhi/D3D11 path. This is the first
phase of v4.2 to ship production C++/QML code, and it removes the last hardcoded
"view unavailable" dead-end in the Plater.

Production changes (8 files):

- `src/qml_gui/Renderer/RhiViewport.h` - added `CanvasAssembleView = 2` to the
  `CanvasType` enum (mirrors `ECanvasType::CanvasAssembleView = 2`,
  `GLCanvas3D.hpp:509-513`).
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - widened the View3D basic-mesh
  upload/draw guards from `== CanvasView3D` to `!= CanvasPreview` so AssembleView
  renders, without touching the strict `== CanvasPreview` Preview guards.
- `src/qml_gui/pages/AssemblePage.qml` - new 4-region page (top bar + LeftSidebar
  + GLViewport at `CanvasAssembleView` + bottom 装配体信息 panel), reusing the
  shared `editorVm`/`configVm`/`processCategory`.
- `src/qml_gui/pages/Plater.qml` - removed `assembleSlot` placeholder; now
  instantiates `AssemblePage` visible when `viewMode === vmAssembleView`.
- `src/qml_gui/qml.qrc` - registered `pages/AssemblePage.qml`.
- `src/qml_gui/BBLTopbar.qml` - added the 装配视图 toggle calling
  `requestChangeViewMode(vmAssembleView)`.
- `src/qml_gui/BackendContext.h` + `BackendContext.cpp` - injected
  `activeCanvasType` into `EditorViewModel` on every view-mode change (mirrors
  `Plater.cpp:7322/11744/11823`); added `currentCanvasType()` accessor;
  documented the shared single undo stack + AssembleView routing. `kLastVm`
  boundary preserved on both code paths.
- `src/core/viewmodels/EditorViewModel.h` + `EditorViewModel.cpp` - added
  `setActiveCanvasType`/`activeCanvasType`/`m_activeCanvasType = 0`;
  `availableGizmoMask()` early-returns 0 when the active canvas is
  `CanvasAssembleView` (mirrors `Plater.cpp:11601,11635`); documented shared
  selection routing in `selectSourceObject()`.

Test change (1 file):

- `tests/QmlUiAuditTests.cpp` - new slot
  `assembleViewShellReplacesPlaceholderAndRegistersCanvasHost` (9 source reads,
  25 QVERIFY2, 8 assertion groups covering placeholder removal, page shell,
  canvas host, qml.qrc registration, navigation toggle, BackendContext routing,
  EditorViewModel routing, and render-guard widening). CJK literals are emitted
  as `\uXXXX` escapes for ASCII-only source compliance.

## Completed Tasks

| Task | Result | Commit |
|---|---|---|
| 90-01-01 Add `CanvasAssembleView = 2` to `RhiViewport::CanvasType`. | Enum gains the third canvas type with a comment citing `GLCanvas3D.hpp:509-513`; `Q_ENUM` and `m_canvasType` default untouched. | `2444311` |
| 90-01-02 Add `CanvasAssembleView` basic-mesh render branch. | Two View3D guards in `RhiViewportRenderer.cpp` widened to `!= CanvasPreview` (upload ~line 188, draw ~line 241); Preview strict guards unchanged. | `c2d4963` |
| 90-01-03 Create `AssemblePage.qml`. | New 4-region page with `editorVm`/`configVm`/`processCategory` required props, `GLViewport { canvasType: CanvasAssembleView }`, bed/plate/object bindings mirroring PreparePage, and a 装配体信息 panel. All Theme tokens, all `qsTr()` strings. | `16df926` |
| 90-01-04 Replace `Plater.qml` placeholder. | Removed `assembleSlot` Item, the placeholder Text, and the Out-of-Scope comment; `AssemblePage` now hosts the slot, visible on `viewMode === vmAssembleView`. `vmAssembleView: 2` preserved; Prepare/Preview untouched. | `6598299` |
| 90-01-05 Register `AssemblePage` in `qml.qrc` + nav toggle in `BBLTopbar`. | `qml.qrc` lists `pages/AssemblePage.qml`; BBLTopbar gets the 装配视图 toggle in the tp3DEditor action row with accent/textPrimary Theme colors and a `border.width` active indicator. | `fe0dcca` |
| 90-01-06 Add `CanvasAssembleView` routing to `BackendContext`. | `setCurrentViewMode()` calls `editorViewModel_->setActiveCanvasType(...)` after emit; added `currentCanvasType()` accessor; documented shared single undo stack. `kLastVm` preserved. | `ce6c9c3` |
| 90-01-07 Add selection/gizmo routing to `EditorViewModel`. | `setActiveCanvasType` early-returns + emits stateChanged; `availableGizmoMask()` early-returns 0 when `m_activeCanvasType == 2`; `selectSourceObject()` documents shared selection routing. | `0cc7dd5` |
| 90-01-08 Add `QmlUiAuditTests` AssembleView assertions. | New slot with 9 source reads and 25 QVERIFY2 across 8 groups; CJK as `\uXXXX` escapes. | `6bb13a4` |
| 90-01-09 Canonical build + ctest regression. | Canonical verifier exited 0; all 6 suites passed (PrepareScene, PartPlate, ViewModel, UI incl. new slot, PreviewParser, E2E). | `90-01-VERIFICATION.md` (this plan) |

## Key Decisions

- AssembleView reuses the shared `EditorViewModel`/`ProjectServiceMock` model and
  the shared single `UndoRedoManager` stack. There is no AssembleView-specific
  scene or undo stack duplication - the `CanvasAssembleView` canvas is the third
  host on the same backing model, matching upstream's single-Plater-three-canvas
  architecture.
- The routing anchor is the existing `ViewMode::AssembleView = 2` /
  `vmAssembleView` enum/property. A new `CanvasType::CanvasAssembleView = 2` was
  added to mirror upstream `ECanvasType`, and a new `activeCanvasType` int flows
  from `BackendContext` to `EditorViewModel` on every view-mode change so the
  viewmodel knows which canvas is showing without a GUI-layer back-call.
- The RHI render path is restored for AssembleView by widening two View3D guards
  to `!= CanvasPreview`. This is the minimum change that makes AssembleView draw
  without weakening Preview's strict guards. Phase 91 will specialize per-volume
  explosion rendering on top of this branch.
- The Assembly gizmo mask is 0 on AssembleView for now. Phase 92 adds the
  Assembly gizmo to the mask; the early-return is the documented seam.
- Phase 90 does not capture a runtime AssembleView screenshot. Per the plan and
  Phase 89's handoff, runtime visual evidence belongs to Phase 93. The Phase 90
  acceptance bar is the canonical build + ctest regression + the new test slot
  passing, all confirmed in `90-01-VERIFICATION.md`.

## Artifacts

| Artifact | Purpose |
|---|---|
| `AssemblePage.qml` | New 4-region AssembleView page shell hosting the `CanvasAssembleView` GLViewport. |
| `90-01-VERIFICATION.md` | Phase 90 plan 01 verification report (canonical build exit 0, 6/6 suites passed, 9-item source-audit checklist). |
| `90-01-SUMMARY.md` | This plan execution summary and downstream handoff. |

## Verification

Canonical build command (the ONLY valid build command per AGENTS.md):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: exit code 0. `build/OWzxSlicer.exe` relinked; all test targets relinked
fresh; all 6 suites passed:

```
[PrepareScene] Prepare scene data tests passed
[PartPlate] PartPlate tests passed
[ViewModel] ViewModel smoke tests passed
[UI] QML UI audit tests passed
[PreviewParser] PreviewParser tests passed
[E2E] All pipeline tests passed
```

The new `QmlUiAuditTests::assembleViewShellReplacesPlaceholderAndRegistersCanvasHost`
slot compiled into `build/QmlUiAuditTests.exe` (binary grew 516096 -> 526336
bytes) and ran green. Prepare/Preview is regression-free (PrepareScene,
PartPlate, PreviewParser all passed). Full evidence, including the 9-item
source-audit checklist and the preservation checklist, is in
`90-01-VERIFICATION.md`.

## Deviations

One task-boundary deviation, documented for traceability:

- **Task 6 carried a minimal `EditorViewModel` declaration.** Task 6's plan
  listed only `BackendContext.cpp`, but `setCurrentViewMode()` could not call
  `editorViewModel_->setActiveCanvasType(...)` without first declaring the
  setter. To keep Task 6 compiling atomically, the commit `ce6c9c3` added the
  `setActiveCanvasType(int)` declaration, the `int m_activeCanvasType = 0;`
  member, and a minimal early-return implementation to `EditorViewModel.h/.cpp`.
  Task 7 (`0cc7dd5`) then layered the actual routing on top (the
  `availableGizmoMask()` AssembleView early-return and the
  `selectSourceObject()` shared-selection documentation). Both commits are
  conventional-commit-scoped to their task intent; the split was a compile-order
  necessity, not a scope change.

No other deviations. The plan executed as written; no scope was added, removed,
or deferred beyond what is already routed to Phase 91/92/93.

## Downstream Handoff

Phase 91 (Explosion Ratio And Assembly Rendering) should start from the
`CanvasAssembleView` render branch in `RhiViewportRenderer.cpp:188,241`. It adds
per-volume explosion separation driven by an explosion-ratio slider
(`m_explosion_ratio` analog), plus connector guide lines, on the default RHI/
D3D11 path. The `availableGizmoMask()` AssembleView early-return is the seam to
relax once the explosion slider exists.

Phase 92 (Assembly Measurement Gizmo) should start from the
`availableGizmoMask()` AssembleView early-return in `EditorViewModel.cpp:2003`.
It ports `GLGizmoAssembly`/`ONLY_ASSEMBLY` (`Ctrl+Y`) with measurement overlays
and the right-side 测量 panel.

Phase 93 (AssembleView Verification And Cleanup) should wire the
`AssembleViewDataID`/`AssembleViewDataPool` plumbing, remove any stale
placeholder artifacts, run the canonical verifier, launch the app, and capture
AssembleView runtime visual evidence against the three target screenshots.

## Self-Check: PASSED

- Placeholder Text, `assembleSlot`, and Out-of-Scope comment are gone from
  `Plater.qml` (0 occurrences).
- `AssemblePage` hosts the slot, visible on `viewMode === vmAssembleView`.
- `CanvasAssembleView = 2` is in `RhiViewport::CanvasType`.
- Render guards widened for AssembleView; Preview strict guards unchanged.
- `pages/AssemblePage.qml` is in `qml.qrc`.
- BBLTopbar 装配视图 toggle calls `requestChangeViewMode(vmAssembleView)`.
- BackendContext injects `activeCanvasType`; `kLastVm` preserved on both paths.
- EditorViewModel has `setActiveCanvasType`/`activeCanvasType`/`m_activeCanvasType`
  and the `availableGizmoMask()` AssembleView early-return.
- New QmlUiAuditTests slot compiled and ran green (binary grew, suite passed).
- Canonical verifier exited 0; all 6 test suites passed.
- Prepare/Preview regression-free.
