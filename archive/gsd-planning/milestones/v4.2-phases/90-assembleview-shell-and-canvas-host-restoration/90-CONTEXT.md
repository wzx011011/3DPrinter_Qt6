# Phase 90: AssembleView Shell And Canvas Host Restoration - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning
**Mode:** Smart Discuss (autonomous)

<domain>
## Phase Boundary

Phase 90 replaces the `Plater.qml` AssembleView placeholder with a real
AssembleView page shell and canvas host. It registers AssembleView as the
third `CanvasAssembleView`-equivalent canvas on the default RHI/D3D11 path,
adds the navigation entry and screenshot-aligned 4-region chrome, and wires
Plater-level `CanvasAssembleView` routing branches for selection/undo-redo —
without breaking Prepare/Preview.

Phase 90 delivers:
- A new `AssemblePage.qml` with the 4-region chrome from `装配页.png`
  (top bar, left settings sidebar, central 3D canvas, bottom controls with
  assembly-info panel).
- A third canvas host via `CanvasAssembleView = 2` added to the existing
  `RhiViewport::CanvasType` enum, instantiated in `Plater.qml`'s assembleSlot.
- Basic mesh rendering in the AssembleView canvas (reused RHI path) so the
  canvas is not empty at runtime — proves the canvas host works.
- Navigation entry (top toolbar view-mode toggle) to switch to AssembleView.
- Plater-level `CanvasAssembleView` routing branches in BackendContext /
  EditorViewModel mirroring upstream `Plater.cpp:7322,11601,11635,11744,11823`.

Out of scope for Phase 90 (deferred to later phases):
- Explosion-ratio slider UI + per-volume separation rendering → Phase 91.
- Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly`) → Phase 92.
- Data pool plumbing + final verification + cleanup → Phase 93.
- Arrange / auto-arrangement (already complete) and `assembleObjects` (already
  implemented) — never in scope.
- All removed network/device/cloud scope.

</domain>

<decisions>
## Implementation Decisions

### Canvas Host Strategy
- Add `CanvasAssembleView = 2` to the existing `RhiViewport::CanvasType` enum
  (`src/qml_gui/Renderer/RhiViewport.h:60-65`) and instantiate a third
  `RhiViewport` in `Plater.qml`'s assembleSlot — mirrors upstream's "third
  GLCanvas3D host" and reuses the default D3D11 RHI path. Do NOT create a
  separate `AssembleViewport` class.
- Share model data via existing `EditorViewModel` / `ProjectServiceMock`
  (single model source, like upstream `Plater::model`) — avoids data
  duplication. Do NOT create a separate scene copy.
- Extend `RhiViewportRenderer` with a `CanvasAssembleView` branch (Phase 91
  will add explosion rendering there). Do NOT create a dedicated
  `AssembleViewportRenderer`. Mirrors upstream's `GLCanvas3D` single-class
  multi-type design.

### Shell Layout And Navigation
- New `AssemblePage.qml` with the 4-region chrome from `装配页.png` (top bar,
  left settings sidebar, central canvas, bottom controls with assembly-info
  panel). Do NOT embed the shell directly in `Plater.qml`'s assembleSlot.
- Navigation entry: add a view-mode toggle in the top toolbar / BBLTopbar
  (like Prepare↔Preview switching), bound to
  `backend.setCurrentViewMode(vmAssembleView)`.
- Left sidebar: reuse Prepare's `LeftSidebar.qml` with AssembleView-specific
  visibility filtering (mirrors screenshot showing a settings sidebar).

### Plater Routing Branches
- Add `canvasType == CanvasAssembleView` branches in BackendContext /
  EditorViewModel mirroring upstream `Plater.cpp:7322` (render branch),
  `Plater.cpp:11601,11635` (gizmo routing + snapshot branch),
  `Plater.cpp:11744` (undo/redo routing), `Plater.cpp:11823` (selection clear
  on undo/redo). Prepare/Preview paths must stay untouched.
- Undo/redo: shared `UndoRedoManager` (single stack, like upstream `Plater`)
  — operations apply to the active canvas. Do NOT create isolated per-canvas
  stacks.

### Scope Boundaries
- Defer the explosion-ratio slider entirely to Phase 91. Phase 90's bottom
  controls row shows the assembly-info panel only.
- Phase 90 renders models in the AssembleView canvas (basic mesh render via
  reused RHI path) so the canvas is not empty at runtime — proves the canvas
  host works. Explosion-driven separation is Phase 91.

### Claude's Discretion
- Specific placement of the view-mode toggle button within the top toolbar.
- Exact column/spacing values for the 4-region chrome, as long as they match
  `装配页.png` density.
- Whether to add a dedicated `AssembleViewModel` or extend `EditorViewModel`
  with assemble-specific properties (either is acceptable; prefer the lighter
  option that avoids duplication).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/Renderer/RhiViewport.h` / `RhiViewport.cpp` — the default
  RHI viewport (`QQuickRhiItem` subclass). Has a `canvasType` Q_PROPERTY with
  `CanvasView3D=0, CanvasPreview=1`. **Phase 90 adds `CanvasAssembleView=2`
  here.** Registered as `OWzxGL.GLViewport` (stable alias) in
  `src/qml_gui/main_qml.cpp:303`.
- `src/qml_gui/Renderer/RhiViewportRenderer.h` / `RhiViewportRenderer.cpp` —
  the actual D3D11 rendering (bed, mesh, gizmos). Phase 90 adds a
  `CanvasAssembleView` branch here for basic mesh render.
- `src/qml_gui/pages/Plater.qml:74-117` — hosts Prepare (View3D), Preview, and
  the AssembleView placeholder slot. The assembleSlot (`:102-115`) is the
  insertion point for the real `RhiViewport` / `AssemblePage.qml`.
- `src/qml_gui/BackendContext.h:159,193-199,227` — `ViewMode::AssembleView=2`
  enum, `vmAssembleView` Q_PROPERTY + accessor. Already the routing anchor.
- `src/qml_gui/BackendContext.cpp:358-376` — `setCurrentViewMode()` with
  `kLastVm` boundary already using `AssembleView` as max.
- `src/core/viewmodels/EditorViewModel.*` — owns model/object state shared
  across canvases (selection, transforms, gizmo mode).
- `src/core/services/UndoRedoManager.*` — shared undo stack (BackendContext-
  owned, injected into EditorViewModel).
- `src/qml_gui/panels/LeftSidebar.qml` — Prepare sidebar to reuse with
  AssembleView visibility filtering.

### Established Patterns
- **Canvas instantiation:** PreparePage/PreviewPage each instantiate their own
  `RhiViewport` QML element with a different `canvasType`. AssembleView follows
  the same pattern with `canvasType: GLViewport.CanvasAssembleView`.
- **View-mode switching:** `backend.currentViewMode` + `vmView3D`/`vmPreview`/
  `vmAssembleView` constants drive slot visibility in Plater.qml.
- **QML type registration:** `qmlRegisterType<RhiViewport>("OWzxGL", 1, 0,
  "GLViewport")` — no new registration needed; AssembleView reuses the same
  type with a different `canvasType`.

### Integration Points
- `Plater.qml` assembleSlot (`:102-115`) — replace placeholder `Text` with
  the real canvas host / AssemblePage.qml.
- `BackendContext::setCurrentViewMode()` (`:358-376`) — already handles
  AssembleView as a valid mode; no enum change needed.
- Top toolbar / BBLTopbar — add the view-mode toggle button.
- `EditorViewModel` — selection/undo-redo routing branches on canvas type.

</code_context>

<specifics>
## Specific Ideas

- The AssembleView canvas host is the architectural foundation for Phase 91
  (explosion rendering) and Phase 92 (Assembly gizmo). The `CanvasAssembleView`
  enum addition and routing branches established here must be clean extension
  points, not dead-end stubs.
- Basic mesh rendering in Phase 90 should prove the canvas host end-to-end:
  load a model, switch to AssembleView, see the mesh rendered. This is the
  smoke test for ASM-SHELL-02.

</specifics>

<deferred>
## Deferred Ideas

- Explosion-ratio slider UI + per-volume separation rendering → Phase 91.
- Yellow dashed connector guide lines → Phase 91.
- Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly`, `ONLY_ASSEMBLY`)
  → Phase 92.
- Measurement overlay (dimension lines, value boxes, 测量 panel) → Phase 92.
- AssembleView data pool (`AssembleViewDataID`/`AssembleViewDataPool`) → Phase 93.
- Final verification + runtime screenshots against target → Phase 93.

</deferred>

---

*Phase: 90-assembleview-shell-and-canvas-host-restoration*
*Context gathered: 2026-07-09 via smart discuss (autonomous mode)*
