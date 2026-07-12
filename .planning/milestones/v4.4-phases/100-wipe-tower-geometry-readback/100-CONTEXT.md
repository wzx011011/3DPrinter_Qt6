# Phase 100: Wipe-Tower Geometry Readback - Context

**Gathered:** 2026-07-11
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 100 implements the post-slice wipe-tower geometry readback. It reads real
geometry from `Print::wipe_tower_data()` inside the SliceService worker after
`print.process()` succeeds, delivers it to the GUI thread, and pushes it into
`RhiViewport`/`EditorViewModel` so the renderer has real dimensions instead of
the placeholder defaults (10/10/50/100/25).

Phase 100 delivers (per the Phase 99 frozen decisions):
- A readback of real wipe-tower geometry (bbx/depth/height/brim_width/rib_offset/
  position/width) from `Print::wipe_tower_data()` after a successful slice,
  captured before the Print is invalidated.
- Delivery of the captured geometry to the GUI thread via the existing
  `sliceFinished` path (no raw `Print*` escapes the worker).
- A push of the real dimensions into `RhiViewport` wipeTowerWidth/Depth/Height/X/Z
  Q_PROPERTYs (via EditorViewModel), replacing the placeholder defaults.
- `has_wipe_tower()` gating: when no wipe-tower exists, no geometry is pushed and
  `showWipeTower` stays false (no placeholder box leak on single-material).

Out of scope for Phase 100:
- The actual rendering upgrade (feeding dims into buildWipeTowerVertices / the
  Option A dimensioned box) → Phase 101. Phase 100 only delivers the real dims
  to the renderer-facing layer; the renderer still draws whatever dims it's given.
- Real-mesh wipe-tower (Option B) — future upgrade.
- Auto filament-map, per-plate wipe-tower architecture refactor, D3D12,
  GLGizmoMeasure, CLI fixtures.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

</domain>

<decisions>
## Implementation Decisions

### Phase 99 Frozen Decisions (MUST honor — these are locked)

1. **Post-slice readback integration point:**
   - Read `Print::wipe_tower_data()` inside the SliceService worker AFTER
     `print.process()` succeeds (`SliceService.cpp:584`).
   - Capture the geometry dims BEFORE the Print is invalidated
     (`activePrint_.store(nullptr)` at `:625`/`:629`/`:634`).
   - Print valid only between `:508` (activePrint_.store(&print)) and `:625/629/634`
     (clear) — NO `Print*` may escape the worker. Capture by value into a POD
     struct or a small data holder.
   - Deliver to GUI thread via the existing `sliceFinished` signal path (`:763`).
   - `EditorViewModel` (`EditorViewModel.h:860` holds `sliceService_`) pushes the
     real dims into `RhiViewport` Q_PROPERTYs.

2. **Rendering-upgrade = Option A (dimensioned box) LOCKED as v4.4 baseline:**
   - The real bbx/depth/height/position/width feed the EXISTING
     `buildWipeTowerVertices` + `uploadWipeTowerBuffer` (still a box, but real
     dims). Phase 100 delivers the dims; Phase 101 wires the renderer to use them.
   - Mirrors upstream `3DScene.cpp:840-885 load_wipe_tower_preview` (make_cube).
   - Option B (real mesh from wipe_tower_mesh_data via convex_hull_3d) is a
     documented future upgrade — NOT in v4.4 scope.

3. **`has_wipe_tower()` gate (WTREAD-02):**
   - `Print::has_wipe_tower()` (`Print.hpp:988`) is the gate. When false
     (single-material / enable_prime_tower off), no geometry is pushed and
     `showWipeTower` stays false — no placeholder box leak.
   - Note: SoftwareViewport has a divergent default `show=true`
     (`SoftwareViewport.h:231`) — Phase 100 should align it (gate on real data,
     not default-true) or at least not regress it.

### Claude's Discretion
- The exact data holder for the captured geometry (a small POD struct, or
  individual fields threaded through the sliceFinished path). Prefer a small
  struct so the dims travel together.
- Whether to add a new Q_PROPERTY/signal on EditorViewModel or reuse an existing
  slice-result delivery. Prefer reusing the sliceFinished path.
- How the dims reach RhiViewport (EditorViewModel → Q_PROPERTY binding, or a
  direct setWipeTower* call). The existing Q_PROPERTYs (wipeTowerWidth/Depth/
  Height/X/Z) are the natural target.

### Recommended approach (Claude's Discretion confirmed, noted for planning)
- Add a small struct (e.g., `WipeTowerGeometry { bool valid; float x, z, width,
  depth, height; }`) captured in SliceService after process() succeeds + has_wipe_tower().
- Thread it through the sliceFinished delivery (or a sibling signal) to
  EditorViewModel.
- EditorViewModel pushes the real dims into the RhiViewport via the existing
  setWipeTowerWidth/Depth/Height/X/Z + setShowWipeTower (only when valid).
- Gate everything on has_wipe_tower(): when false, setShowWipeTower(false).
- PreparePage.qml:1648 GLViewport currently does NOT bind wipe-tower Q_PROPERTYs
  — Phase 100 or 101 must add the bindings (wipeTowerWidth/Depth/Height/x/z +
  showWipeTower) so the real dims actually reach the renderer. (This unbound
  surface is WT-VIEWPORT-DEFAULTS / WT-READBACK-POINT in the gap matrix.)

</decisions>

<code_context>
## Existing Code Insights

### SliceService (the readback site)
- `src/core/services/SliceService.cpp:508` — `activePrint_.store(&print, release)` (Print becomes valid).
- `:584` — `print.process()` (populates wipe_tower_data).
- `:625`/`:629`/`:634` — `activePrint_.store(nullptr, release)` (Print invalidated).
- `:763` — `emit sliceFinished` (GUI-thread delivery).
- The worker runs on a background thread; GUI-thread delivery must go through a signal.

### Upstream geometry source (read here)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` — `struct WipeTowerData`: bbx (:764), rib_offset (:765), depth, height, brim_width, position, width, tool_changes (:754).
- `Print.hpp:988` — `has_wipe_tower()`.
- `Print.hpp:989` — `wipe_tower_data()`.
- `Print.hpp:1078-1080` — `get_wipe_tower_depth()`, `get_wipe_tower_bbx()`, `get_rib_offset()`.

### EditorViewModel (the bridge)
- `src/core/viewmodels/EditorViewModel.h:860` — holds `sliceService_`.
- Connects to SliceService signals; pushes state to RhiViewport.

### RhiViewport (the Q_PROPERTY target)
- `src/qml_gui/Renderer/RhiViewport.h:54-59,181-192,304-309` — Q_PROPERTY show/width/depth/height/x/z; defaults 10/10/50/100/25 (the placeholders to replace).
- `PreparePage.qml:1648` — GLViewport instantiation; currently does NOT bind wipe-tower Q_PROPERTYs (the unbound gap).

### Renderer (consumes the Q_PROPERTYs — Phase 101 wires the upgrade)
- `RhiViewportRenderer.h:57,75,129,143,152,177,216-222` — m_wipeTowerBuffer, m_wipeTowerWidth/Depth/Height/X/Z, m_wipeTowerDirty, m_showWipeTower.
- `RhiViewportRenderer.cpp:1064-1095 uploadWipeTowerBuffer()` (uses the dims), `:1894-1908 renderWipeTower()`.

</code_context>

<specifics>
## Specific Ideas

- The readback must capture dims BY VALUE (no Print* escapes the worker). A small
  POD struct is the cleanest.
- has_wipe_tower() is the single gate — both for "push geometry" and "show
  wipe-tower". Single-material slices must show no wipe-tower.
- PreparePage.qml:1648 GLViewport must gain wipe-tower Q_PROPERTY bindings so the
  real dims reach the renderer. Decide in planning whether Phase 100 adds the
  bindings (so dims flow end-to-end) or Phase 101 does (so Phase 100 is purely
  the readback + EditorViewModel push). Recommendation: Phase 100 adds the
  bindings so the readback is testable end-to-end (real dims reach the renderer,
  verifiable via a source audit that the placeholder defaults are overridden).

</specifics>

<deferred>
## Deferred Ideas

- Rendering upgrade (Option A dimensioned box wiring in the renderer) → Phase 101.
- Real-mesh wipe-tower (Option B) → future upgrade.
- Auto filament-map recommendation → future milestone.

</deferred>

---

*Phase: 100-wipe-tower-geometry-readback*
*Context gathered: 2026-07-11 (discuss skipped via workflow.skip_discuss; Phase 99 frozen decisions embedded)*
