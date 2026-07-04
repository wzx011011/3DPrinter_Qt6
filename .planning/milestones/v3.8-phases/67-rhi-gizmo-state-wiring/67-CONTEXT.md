# Phase 67: RHI Gizmo State Wiring - Context

**Gathered:** 2026-07-04
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — no user-facing decisions)

<domain>
## Phase Boundary

Connect the broken gizmo state pipeline on the default D3D11 RHI path:
`RhiViewportRenderer::synchronize` must read `gizmoMode`/`cutAxis`/
`cutPosition` from the `RhiViewport` item, and `gizmoCenter` must be computed
from the selected object's AABB (via PrepareSceneData's ModelBatch bounds).

This phase delivers ONLY state plumbing + diagnostic logging:

- New private members on `RhiViewportRenderer`: `m_gizmoMode`, `m_cutAxis`,
  `m_cutPosition`, `m_gizmoCenter` (+ dirty-tracking as needed).
- `synchronize()` extended to read `viewport->m_gizmoMode` / `m_cutAxis` /
  `m_cutPosition` and compute `m_gizmoCenter` from
  `m_prepareScene.modelBatches()` for the batch matching
  `selectedSourceObjectIndex()`.
- Diagnostic `qInfo` log on state change confirming the values arrived.
- A new test verifying the state-propagation logic (gizmoCenter computation
  from a mock batch list).

Out of scope:
- RHI rendering of any gizmo -> Phase 68.
- Pick/drag interaction -> Phase 69.
- Cut plane / wipe tower rendering -> Phase 71.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure
phase (state wiring, no user-visible behavior). Use the existing synchronize
pattern, PrepareSceneData batch API, and the qInfo logging convention to
guide decisions.

Key constraints from the codebase:
- `RhiViewportRenderer` is a `friend` of `RhiViewport` (reads `viewport->m_*`
  directly). Follow the same pattern for gizmo state.
- `m_prepareScene.modelBatches()` returns `const QList<ModelBatch>&` with
  each batch carrying `sourceObjectIndex` + `bounds` (ModelBounds min/max XYZ).
- `m_prepareScene.selectedSourceObjectIndex()` returns the currently selected
  object index (-1 if none).
- `gizmoCenter` = midpoint of the selected batch's bounds: `((min+max)/2)`.
  If no selection, default to origin (0,0,0) — matches GL path behavior.
- Logging convention: `qInfo("[RHI] ...")` (see line 87 of synchronize).
- `setGizmoMode` already calls `update()` (RhiViewport.cpp:321), so the
  renderer's next synchronize() will pick up the change automatically.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `RhiViewport.h:50,54-55,168-169,180,183,199,252,256-257` — gizmoMode/cutAxis/
  cutPosition Q_PROPERTY + members already exist on the QML item.
- `RhiViewport.cpp:315-322` — `setGizmoMode` already calls `update()`.
- `PrepareSceneData.h:33-50,105` — `ModelBounds{minX..maxZ}`, `ModelBatch` with
  `sourceObjectIndex` + `bounds`, and `modelBatches()` accessor.
- `RhiViewportRenderer.cpp:31-100` — existing `synchronize()` pattern to extend.

### Established Patterns
- Renderer reads `viewport->m_*` members directly (friend access).
- qInfo logging with `[RHI]` tag prefix.
- GizmoMode enum values: `GizmoMove=0, GizmoRotate=1, GizmoScale=2, GizmoCut=3`
  (verify exact enum in RhiViewport.h).

### Integration Points
- `RhiViewportRenderer.h` — add private members + (optionally) a helper
  `computeGizmoCenter()`.
- `RhiViewportRenderer.cpp:31` — extend `synchronize()` body.
- `tests/` — new test target or extend an existing one for the
  gizmoCenter-from-bounds computation.

</code_context>

<specifics>
## Specific Ideas

- Log the gizmo state on every change (mode/axis/position/center) at qInfo
  level so Phase 68 debugging can confirm the pipeline is connected.
- If no object is selected (sourceObjectIndex == -1), set gizmoCenter to origin
  and skip rendering preparation (Phase 68 will early-return on gizmoMode != 0
  && gizmoCenter at origin with no selection).
- The gizmoCenter Z should sit at the object's vertical midpoint, NOT the bed
  (so the gizmo appears at the object, not the floor) — matches GL path.

</specifics>

<deferred>
## Deferred Ideas

- Actual RHI rendering of gizmo geometry -> Phase 68.
- Gizmo pick/drag interaction -> Phase 69.
- Cut plane / wipe tower geometry rendering -> Phase 71.
- Highlight (hover) color logic -> Phase 68 (decides uniform vs parallel buffer).

</deferred>
