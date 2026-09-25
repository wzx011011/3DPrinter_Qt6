# Phase 138: Assembly Transformation Actions ASM-01 - Context

**Gathered:** 2026-07-16
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss) + enriched by codebase scout.

<domain>
## Phase Boundary

Assembly view supports move/rotate/scale transformation actions on assembled volumes (RhiViewport assembly canvas gizmo + ViewModel + AssemblePage). Transformations apply per-volume and round-trip through the model (3MF save → reload preserves per-volume assemble transforms).

**Upstream behavior truth (source-truth scout):** In OrcaSlicer assembly mode, the user uses the ordinary Move/Rotate/Scale gizmos; the resulting transform is written into the selected instances' `ModelInstance::m_assemble_transformation` (and `m_assemble_initialized=true`). This persists to 3MF via the `<assemble>` block (`bbs_3mf.cpp:8070-8088` write, `4734-4741` read) and reloads intact. The explosion slider is a separate, non-persisted visualization knob. There is NO separate "assembly move/rotate/scale gizmo" — the same `GLGizmoMove3D/Rotate3D/Scale3D` operate on whichever canvas is current.

**Key realization:** This is a routing/wiring gap, NOT new gizmo machinery. The existing move/rotate/scale gizmo infrastructure (rendering, geometry, hit-testing, drag signals, ViewModel apply-slots) is fully canvas-agnostic and reusable. The 3MF round-trip contract already exists upstream. The work is: (1) advertise the gizmo bits on AssembleView, (2) add assemble-transform accessors in ProjectServiceMock, (3) route the apply-slots to the assemble transform when on the assembly canvas, (4) thread the assemble pose into the renderer, (5) add AssemblePage transform UI, (6) add a round-trip test.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use ROADMAP phase goal, success criteria, codebase conventions, and the scout findings below to guide decisions. The following scout conclusions are load-bearing and should be respected unless a better option is found during planning:

### Confirmed design directions (from source-truth scout)
- **No new AssemblyViewModel file.** All assembly state currently lives in EditorViewModel (consistent with Phases 90-93). Add the assemble-transform state there. (A dedicated AssemblyViewModel is an open alternative but diverges from established convention.)
- **No new gizmo machinery.** Reuse the existing Move/Rotate/Scale gizmo rendering + drag signals. They are canvas-agnostic.
- **Mirror upstream's transform target.** When `m_activeCanvasType == 2` (CanvasAssembleView), gizmo apply-slots must write `ModelInstance::m_assemble_transformation` (NOT `m_transformation`), matching upstream `Model.hpp:1280-1293`. The Prepare/View3D path (writing `objectPosition/Rotation/Scale`) stays unchanged.
- **Persist via the existing `<assemble>` block.** The real `bbs_3mf.cpp` already serializes `m_assemble_transformation` + `m_offset_to_assembly`. No new 3MF code needed — just ensure the Qt layer writes the Model field before save.

### Open choices for the planner
- **Per-volume vs per-instance granularity.** Upstream stores assemble transform on `ModelInstance`. The Qt6 service currently has per-OBJECT stores. Decide whether to proxy to the real `ModelInstance` fields (preferred — matches upstream + gets 3MF for free) or add parallel Qt stores. Scout recommends proxying to real Model fields under `HAS_LIBSLIC3R`.
- **Renderer threading.** How to get the assemble pose into `buildModelVertices` — extend the mesh blob from PrepareSceneData, or apply in the shared transform path. Planner to pick the lower-risk option.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/pages/AssemblePage.qml:102` — GLViewport host with `canvasType: GLViewport.CanvasAssembleView`; already binds shared editorVm mesh data.
- `src/qml_gui/Renderer/RhiViewport.h:119-146` — `GizmoMode { GizmoMove=0, GizmoRotate=1, GizmoScale=2, ... GizmoAssemblyMeasure=19 }`.
- `src/qml_gui/Renderer/RhiViewport.h:314-316` — `gizmoMoveRequested/Rotate/Scale` drag signals (canvas-agnostic).
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1862-1920` — `renderMoveGizmo/Rotate/Scale`, gated on `m_gizmoMode` + `selectedSourceObjectIndex() >= 0` (NOT canvas-gated — would draw on AssembleView if gizmoMode set).
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1617-1677` — `buildModelVertices` CanvasAssembleView branch already applies per-volume explosion offset.
- `src/core/rendering/GizmoGeometry.cpp:225,256,325` — `buildMoveGizmoVertices/Rotate/Scale`.
- `src/core/viewmodels/EditorViewModel.cpp:347-590` — `applyGizmoMoveDelta/Rotate/Scale` + `begin/apply/endGizmoMove/Rotate/ScaleDrag` (canvas-agnostic today; route on `m_activeCanvasType==2`).
- `src/core/viewmodels/EditorViewModel.cpp:3170-3186` — `availableGizmoMask()` hard-returns 0 (or bit 19) on AssembleView. This is THE gate to flip.
- `src/core/services/ProjectServiceMock.h:382-387, 555-557` — per-OBJECT `objectPosition/Rotation/Scale` getters/setters + parallel `QList<QVector3D>` stores. NO per-volume/per-instance assemble-transform accessors yet.
- `src/core/services/ProjectServiceMock.cpp:1059-1086` — real 3MF save/load via `Slic3r::store_3mf` / loader (under `HAS_LIBSLIC3R`).
- `third_party/OrcaSlicer/src/libslic3r/Model.hpp:1253-1298` — `ModelInstance::m_assemble_transformation` + `m_offset_to_assembly` + accessors (the upstream target fields).
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp:8070-8088, 4734-4741` — `<assemble>` block write/read (already handles round-trip).

### Established Patterns
- Gizmo availability gated by `availableGizmoMask()` returning a bitmask; `canActivateGizmo(mode)` per-mode check.
- Transform apply-slots push a `TransformCommand` (QUndoCommand) for undo, then call the service setter.
- ViewModel ↔ Service via raw injected pointer; service owns the `Slic3r::Model*` under `HAS_LIBSLIC3R`.

### Integration Points
- `EditorViewModel::availableGizmoMask()` — add bits 0/1/2 on AssembleView when a volume is selected.
- `EditorViewModel::applyGizmoMove/Rotate/Scale` — branch on `m_activeCanvasType==2` to write assemble transform.
- `ProjectServiceMock` — new `assembleOffset/Rotation/Scale` getters/setters proxying to `ModelInstance` fields.
- `RhiViewportRenderer::buildModelVertices` (or PrepareSceneData mesh blob) — compose the assemble pose.
- `AssemblePage.qml` — transform-mode selector + gizmoMode binding + drag-signal handlers (mirror PreparePage:1758-1770).

</code_context>

<specifics>
## Specific Ideas

- The scout confirms the infrastructure exists; ASM-01 is wiring + accessors + a renderer thread-through + a UI selector + a test. Smallest viable slice preferred.
- Round-trip test should use the real `saveProjectAs` + `loadProject` path (goes through `bbs_3mf.cpp`), asserting `assembleOffset/Rotation/Scale` survives — NOT a mocked persistence test.

</specifics>

<deferred>
## Deferred Ideas

- A dedicated AssemblyViewModel (current convention keeps assembly state in EditorViewModel).
- Visual/UX polish of the assembly transform toolbar beyond minimum parity.
- Upstream's full `m_offset_to_assembly` explosion-persist behavior (explosion stays a non-persisted visualization knob unless a later phase asks otherwise).

</deferred>
