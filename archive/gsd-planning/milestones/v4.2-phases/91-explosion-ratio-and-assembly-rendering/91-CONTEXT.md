# Phase 91: Explosion Ratio And Assembly Rendering - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss); Area 1 decisions pre-accepted by user

<domain>
## Phase Boundary

Phase 91 implements the explosion-ratio control and multi-part volume separation
rendering for the AssembleView canvas on the default RHI/D3D11 path, plus the
yellow dashed connector guide lines visible when the ratio exceeds 1.0. This
builds on the Phase 90 `CanvasAssembleView` canvas host and render branch.

Phase 91 delivers:
- An `explosionRatio` Q_PROPERTY on `EditorViewModel` (default 1.0) mirroring
  upstream `m_explosion_ratio`, with a reset action mirroring
  `reset_explosion_ratio()`.
- A "爆炸比例" slider UI in `AssemblePage.qml` bottom controls (0.00–3.00 range)
  bound to `editorVm.explosionRatio`, plus a reset affordance.
- Per-volume radial separation rendering in the `CanvasAssembleView` branch of
  `RhiViewportRenderer`: each volume offset by `(volumeCenter - objectCenter) *
  (ratio - 1.0)`, re-rendering on every ratio change.
- Yellow dashed connector guide lines between originally-adjacent volumes,
  visible only when ratio > 1.0 (matching `shotScreen/装配页_爆炸.png`).

Out of scope for Phase 91:
- Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly`) → Phase 92.
- Measurement overlay (dimension lines, value boxes, 测量 panel) → Phase 92.
- AssembleView data pool plumbing + final verification + cleanup → Phase 93.
- Arrange / auto-arrangement (already complete) and `assembleObjects` (already
  implemented) — never in scope.
- All removed network/device/cloud scope.

</domain>

<decisions>
## Implementation Decisions

### Explosion Ratio Control (Area 1 — pre-accepted by user)
- Add `explosionRatio` Q_PROPERTY to `EditorViewModel` (default 1.0, mirroring
  upstream `m_explosion_ratio` at `GLCanvas3D.hpp:596`). Setter emits
  `stateChanged` + invalidates the assemble render. Do NOT create a separate
  `AssembleViewModel`.
- Slider UI: a CxSlider in `AssemblePage.qml` bottom controls bound to
  `editorVm.explosionRatio`, matching `装配页.png`'s "爆炸比例" label with a
  0.00–3.00 range. Do NOT use a separate popup/dialog.
- Reset capability: add a reset action (button or double-click) calling
  `editorVm.resetExplosionRatio()` mirroring upstream `reset_explosion_ratio()`
  (`GLCanvas3D.hpp:770-771`).

### Per-Volume Separation Rendering (Claude's Discretion, recommended approach noted)
- In the `CanvasAssembleView` branch of `RhiViewportRenderer`, offset each
  volume's mesh by `(volumeCenter - objectCenter) * (ratio - 1.0)` along its
  assembly axis — mirrors upstream radial separation.
- Volume geometry comes from existing `EditorViewModel`/`ProjectServiceMock`
  model data (volumes + transforms already available for Prepare).
- Re-render triggers on `explosionRatio` changes (re-upload/offset on every
  ratio change), like Prepare re-renders on transform change.

### Connector Guide Lines (Claude's Discretion, recommended approach noted)
- Render yellow dashed connector guide lines between originally-adjacent
  volumes in the `CanvasAssembleView` render branch, visible only when
  ratio > 1.0 (matching `装配页_爆炸.png`).
- Color/style: yellow dashed (matching screenshot); render as GL lines with a
  stipple pattern.

### Claude's Discretion
- Exact QML layout/spacing of the slider control block, as long as it matches
  `装配页.png` density.
- The reset affordance form (inline button vs double-click-to-reset) — either
  is acceptable.
- Implementation details of the stipple/dashed line rendering.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets (from Phase 90)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` — has the `CanvasAssembleView`
  render branch (added in Phase 90, basic mesh render). Phase 91 extends this
  branch with per-volume offset + connector guide lines.
- `src/qml_gui/Renderer/RhiViewport.h:60-67` — `CanvasType` enum with
  `CanvasAssembleView = 2`.
- `src/core/viewmodels/EditorViewModel.*` — owns model/object state; Phase 91
  adds `explosionRatio` Q_PROPERTY here.
- `src/qml_gui/pages/AssemblePage.qml` — the 4-region shell from Phase 90;
  Phase 91 adds the explosion slider to its bottom controls.
- `src/core/services/ProjectServiceMock.*` — model/volume/transform data source
  (shared with Prepare).

### Upstream behavior truth anchors
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596` —
  `mutable float m_explosion_ratio = 1.0;`
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:770-771` —
  `float get_explosion_ratio()` / `void reset_explosion_ratio()`
- `shotScreen/装配页_爆炸.png` — Explosion Ratio = 3.00 with yellow dashed
  connector guide lines (visual truth for separation rendering + connectors).

### Integration Points
- `RhiViewportRenderer` `CanvasAssembleView` branch — add offset + connector
  rendering.
- `EditorViewModel` — add `explosionRatio` Q_PROPERTY + `resetExplosionRatio()`.
- `AssemblePage.qml` bottom controls — add the slider bound to the property.
- Renderer must re-render on ratio change (bind to the property like Prepare
  binds to transforms).

</code_context>

<specifics>
## Specific Ideas

- The explosion rendering must prove end-to-end: load a multi-part model,
  switch to AssembleView, drag the slider, see volumes separate radially with
  connector lines. This is the smoke test for ASMEXPLODE-01 + ASMEXPLODE-02.
- The connector guide lines are screenshot-visible (`装配页_爆炸.png`) and must
  not be skipped — they are part of ASMEXPLODE-02's success criteria.

</specifics>

<deferred>
## Deferred Ideas

- Assembly measurement gizmo (`Ctrl+Y`) → Phase 92.
- Measurement overlay (dimension lines, value boxes, 测量 panel) → Phase 92.
- AssembleView data pool → Phase 93.
- Final verification + runtime screenshots → Phase 93.

</deferred>

---

*Phase: 91-explosion-ratio-and-assembly-rendering*
*Context gathered: 2026-07-09 (Area 1 pre-accepted; discuss skipped via workflow.skip_discuss)*
