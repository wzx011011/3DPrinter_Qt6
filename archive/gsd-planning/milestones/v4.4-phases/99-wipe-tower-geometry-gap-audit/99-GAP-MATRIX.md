# Phase 99 Wipe-Tower Geometry Gap Matrix

**Scope:** v4.4 Wipe-Tower Geometry Readback And Real Rendering. The wipe-tower
capture surface (placeholder-box builder + hardcoded viewport defaults), the
RHI renderer buffer/upload/render path, the parallel software viewport, the
upstream `Print::wipe_tower_data()` / `WipeTowerData` source-truth anchors, the
post-slice readback integration point, the rendering-upgrade decision
(dimensioned box vs real mesh), and the `has_wipe_tower()` gate only. No
LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware, D3D12/
Vulkan, libslic3r slicing algorithm, auto filament-map, CLI fixture, per-plate
wipe-tower architecture refactor, or GLGizmoMeasure-engine scope is in Phase 99.

## Summary

Phase 99 is the v4.4 source-truth audit. Its job is to freeze the wipe-tower
geometry readback + rendering region map before implementation. The current Qt
surface ships only a hardcoded placeholder: a 36-vertex rectangular-prism
`buildWipeTowerVertices` (`GizmoGeometry.cpp:449-499`) whose dimensions come
from `RhiViewport` defaults of width=10/depth=10/height=50/x=100/z=25
(`RhiViewport.h:304-309`) that nothing in `PreparePage.qml` ever binds. The Qt6
side never reads libslic3r's `Print::wipe_tower_data()` — zero references in
`src/`. This matrix is the canonical routing artifact for Phase 100-102.

This phase is read-only with respect to production source: it modifies
documentation only and produces no QML/C++ changes. Runtime wipe-tower
visibility parity and visual proof are owned by Phase 102; Phase 99 does not
claim them.

## Current State (from pre-planning exploration)

The exact placeholder paths and unbound-binding surface, with line citations
read from the current Qt6 source:

- `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-499` —
  `buildWipeTowerVertices(x, z, width, depth, height)` builds a 36-vertex
  rectangular prism (6 faces x 2 triangles x 3 verts), hardcoded color
  `{0.35f, 0.60f, 0.85f, 0.50f}` (cyan translucent) at `.cpp:463`. Empty vector
  if any dimension is non-positive (`.cpp:459`). The box is centered on `(x, z)`
  in the X/Z plane with `kGroundY = -0.04f` as the bed surface (`.cpp:462`).
- `src/qml_gui/Renderer/RhiViewport.h:54-59` — Q_PROPERTY `showWipeTower` /
  `wipeTowerWidth` / `wipeTowerDepth` / `wipeTowerHeight` / `wipeTowerX` /
  `wipeTowerZ`; `:181-192` accessors (`showWipeTower()` ... `setWipeTowerZ()`);
  `:304-309` member defaults `m_showWipeTower=false`, `m_wipeTowerWidth=10.f`,
  `m_wipeTowerDepth=10.f`, `m_wipeTowerHeight=50.f`, `m_wipeTowerX=100.f`,
  `m_wipeTowerZ=25.f`.
- `src/qml_gui/Renderer/RhiViewportRenderer.h:57` (`uploadWipeTowerBuffer`),
  `:75` (`renderWipeTower`), `:129` (`m_wipeTowerBuffer`),
  `:143` (`m_wipeTowerBufferUploaded`), `:152` (`m_wipeTowerBufferBytes`),
  `:177` (`m_wipeTowerVertexCount`), `:216-222` (`m_showWipeTower=false` +
  `m_wipeTowerWidth/Depth/Height/X/Z=10/10/50/100/25` + `m_wipeTowerDirty=true`).
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1095` —
  `uploadWipeTowerBuffer` builds the box via
  `GizmoGeometry::buildWipeTowerVertices(m_wipeTowerX, m_wipeTowerZ,
  m_wipeTowerWidth, m_wipeTowerDepth, m_wipeTowerHeight)` (`.cpp:1075-1079`)
  when `m_showWipeTower` is true, then uploads `m_wipeTowerBuffer`; and
  `:1894-1908` — `renderWipeTower` draws via `m_translucentFillPipeline` (gated
  on `m_prepareScene.showBed() && m_showWipeTower`).
- `src/qml_gui/Renderer/SoftwareViewport.h:35-40` (Q_PROPERTY show/width/depth/
  height/x/z), `:126-127` (showWipeTower accessor + setter), `:231-236` (member
  defaults `m_showWipeTower=true`, width/depth/height/x/z all `0.f`) +
  `src/qml_gui/Renderer/SoftwareViewport.cpp:207-253` setters (each calls
  `update()`).
- `src/qml_gui/pages/PreparePage.qml:1648` — the `GLViewport` instance
  (`qmlRegisterType<RhiViewport>("OWzxGL",1,0,"GLViewport")` at
  `src/qml_gui/main_qml.cpp:303`; `SoftwareViewport` is the parallel registration
  at `main_qml.cpp:305`) does NOT bind any wipe-tower Q_PROPERTY
  (show/width/depth/height/x/z). Grep for `wipeTower`/`showWipeTower` in
  `PreparePage.qml` returns nothing — the renderer falls through to the
  hardcoded `RhiViewport.h:304-309` defaults. This unbound-binding is the
  surface Phase 100 must wire.
- `src/core/viewmodels/EditorViewModel.h:860` — `SliceService *sliceService_ =
  nullptr;` is the bridge that can push geometry into `RhiViewport`.
- `src/core/services/SliceService.cpp:508` —
  `receiver->activePrint_.store(&print, std::memory_order_release)` (the `Print`
  becomes addressable mid-slice); `:584` — `print.process()` (the slice that
  populates `wipe_tower_data`); `:625/:629/:634` —
  `receiver->activePrint_.store(nullptr, ...)` (the `Print` goes invalid when
  the worker lambda exits); `:763` —
  `emit receiver->sliceFinished(receiver->estimatedTimeLabel_)` (GUI-thread
  delivery after a successful slice).
- Zero references to `wipe_tower_data` / `get_wipe_tower_depth` /
  `get_wipe_tower_bbx` / `get_rib_offset` anywhere in `src/`. The only
  `has_wipe_tower` string is a g-code placeholder label at
  `src/qml_gui/dialogs/EditGCodeDialog.qml:381`, not a readback.

## Canonical Region Matrix

| Region | Placeholder Path | Upstream Anchor | Qt Integration Point | Decision | Gap | Severity | Owner Phase | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|
| WT-PLACEHOLDER-BOX | `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-499` (`buildWipeTowerVertices(x, z, width, depth, height)`, 36-vertex box, color `{0.35, 0.60, 0.85, 0.50}` at `.cpp:463`). | `3DScene.cpp:840-885 load_wipe_tower_preview` — box-fidelity source-truth anchor: builds `make_cube(width, depth, height)` (`:855`), sets `v.is_wipe_tower = true` (`:882`), per-extruder color slices at alpha 0.66 (`:864-873`). The Qt box is the single-color mirror of this cube preview. | `src/core/rendering/GizmoGeometry.h:74` (`buildWipeTowerVertices` decl) + `.cpp:449-499` (36-vertex rectangular prism, `kGroundY=-0.04f`, hardcoded color). | Replace dims (Option A: keep the box builder but feed real dims from `wipe_tower_data`; see WT-RENDER-UPGRADE). | Box uses caller-supplied or default dims (10/10/50/100/25 from `RhiViewport.h:304-309`); never the real sliced width/depth/height/position. Single hardcoded color vs upstream per-extruder coloring. | High | Phase 101 | WTRENDER-01, WTRENDER-02 | Source audit (Phase 102) proving `buildWipeTowerVertices` is fed real dims from `wipe_tower_data`; downstream runtime visual evidence (Phase 102) showing a box of the correct sliced size. |
| WT-VIEWPORT-DEFAULTS | `src/qml_gui/Renderer/RhiViewport.h:54-59` (Q_PROPERTY show/width/depth/height/x/z), `:181-192` accessors, `:304-309` member defaults width=10/depth=10/height=50/x=100/z=25; `src/qml_gui/pages/PreparePage.qml:1648` GLViewport does NOT bind any wipe-tower Q_PROPERTY (grep returns nothing). | N/A — no single upstream line: the Q_PROPERTY layer is Qt-only; upstream drives geometry via `GLVolumeCollection` (`3DScene.cpp:840 load_wipe_tower_preview`) and `GLCanvas3D` volume state, not a property bag. The Qt6 Q_PROPERTY layer is the migration's own bridge. | `src/qml_gui/Renderer/RhiViewport.h:54-59` (Q_PROPERTY showWipeTower/wipeTowerWidth/wipeTowerDepth/wipeTowerHeight/wipeTowerX/wipeTowerZ), `:181-192` accessors, `:304-309` member defaults (show=false, 10/10/50/100/25); `src/qml_gui/pages/PreparePage.qml:1648` GLViewport instance binds meshData/bed*/plate props but NOT any wipeTower Q_PROPERTY — renderer reads the hardcoded defaults. | Replace defaults with real dims pushed from readback (Phase 100 wires PreparePage.qml bindings + EditorViewModel push). | The Prepare page GLViewport never binds wipeTower*; the renderer falls through to the 10/10/50/100/25 defaults, so even after readback lands the Q_PROPERTYs would stay at defaults unless PreparePage.qml binds them. | Critical | Phase 100 | WTREAD-01 | Source audit (Phase 102) proving PreparePage.qml:1648 GLViewport binds wipeTowerWidth/Depth/Height/X/Z + showWipeTower to EditorViewModel-exposed props; downstream runtime visual evidence (Phase 102). |
| WT-RENDERER-BUFFER | `src/qml_gui/Renderer/RhiViewportRenderer.h:57` (`uploadWipeTowerBuffer`), `:75` (`renderWipeTower`), `:129` (`m_wipeTowerBuffer`), `:143` (`m_wipeTowerBufferUploaded`), `:152` (`m_wipeTowerBufferBytes`), `:177` (`m_wipeTowerVertexCount`), `:216-222` (`m_showWipeTower` + `m_wipeTowerWidth/Depth/Height/X/Z` + `m_wipeTowerDirty`); `RhiViewportRenderer.cpp:1064-1095` (`uploadWipeTowerBuffer` builds box via `buildWipeTowerVertices`, uploads `m_wipeTowerBuffer`) and `:1894-1908` (`renderWipeTower` via `m_translucentFillPipeline`). | `3DScene.cpp:840-885` (box upload via `make_cube` + `init_from`) + `3DScene.cpp:887-925 load_real_wipe_tower_preview` (real mesh upload via `init_from(mesh.convex_hull_3d())` at `:914-915`). The Qt upload/render pipeline is the RHI mirror of upstream's GLVolume geometry path. | `src/qml_gui/Renderer/RhiViewportRenderer.h:57,75,129,143,152,177,216-222` (buffer + state members); `RhiViewportRenderer.cpp:1064-1095` (`uploadWipeTowerBuffer` rebuilds the box from `m_wipeTowerX/Z/Width/Depth/Height` when `m_showWipeTower`, uploads `m_wipeTowerBuffer`, clears `m_wipeTowerDirty`); `:1894-1908` (`renderWipeTower` draws via `m_translucentFillPipeline`, gated on `m_prepareScene.showBed() && m_showWipeTower`). | Preserve + feed real dims: the buffer/upload/render pipeline is already correct; once WT-VIEWPORT-DEFAULTS pushes real dims through `synchronize()`, the `m_wipeTowerDirty` path rebuilds the box with them. No structural change needed for Option A. | None for the pipeline itself — it correctly rebuilds on `m_wipeTowerDirty`; the gap is upstream (the dims it receives), not in the buffer path. (Option B real-mesh would require an ITS vertex format extension here — deferred.) | High | Phase 101 | WTRENDER-01 | Source audit (Phase 102) confirming `uploadWipeTowerBuffer` + `renderWipeTower` are unchanged in structure and the `m_wipeTowerDirty` rebuild fires on dim change; downstream runtime visual evidence (Phase 102). |
| WT-PRINT-DATA | N/A today — zero references to `wipe_tower_data` / `has_wipe_tower` (readback) / `get_wipe_tower_depth` / `get_wipe_tower_bbx` / `get_rib_offset` in `src/` (only `has_wipe_tower` is a g-code label at `EditGCodeDialog.qml:381`). | `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` (`struct WipeTowerData`: `tool_changes` at `:754`, `depth` at `:760`, `brim_width` at `:762`, `height` at `:763`, `bbx` at `:764` "including brim", `rib_offset` at `:765`, optional `wipe_tower_mesh_data` at `:766`, `position`/`width` held on `Print`/`FakeWipeTower`); `Print.hpp:988-989` (`has_wipe_tower()`, `wipe_tower_data()`); `Print.hpp:1078-1080` (`get_wipe_tower_depth()`, `get_wipe_tower_bbx()`, `get_rib_offset()`). | N/A today — no Qt integration point reads `wipe_tower_data`/`has_wipe_tower`/`get_wipe_tower_*`. The `Print` is held only inside the `SliceService` worker lambda (see WT-READBACK-POINT). | Add: Phase 100 reads `wipe_tower_data()` (and the `has_wipe_tower()` gate) inside the SliceService worker where the `Print` is valid, captures `bbx`/`depth`/`height`/`position`/`width`/`brim_width`/`rib_offset`, and delivers them to the GUI thread. | Qt6 never reads the real sliced wipe-tower geometry; the entire `WipeTowerData` struct and its accessors are unused on the Qt side. | Critical | Phase 100 | WTREAD-01 | Source audit (Phase 102) proving `Print::wipe_tower_data()` / `has_wipe_tower()` are read inside SliceService after `print.process()` succeeds; downstream runtime visual evidence (Phase 102). |
| WT-READBACK-POINT | N/A today — no readback code exists; the `Print` object lives inside the `SliceService` worker lambda and is never queried for `wipe_tower_data`. | Upstream `Print::process()` populates `wipe_tower_data` at slice completion; upstream GUI reads it after the slice to drive the preview (`3DScene.cpp:840 load_wipe_tower_preview` consumes `width`/`depth`/`height`/`position`). | The `Print` object lives in the `SliceService` worker lambda: `SliceService.cpp:508` `receiver->activePrint_.store(&print, std::memory_order_release)` (Print becomes addressable mid-slice); `:584` `print.process()` (the slice that populates `wipe_tower_data`); `:625`/`:629`/`:634` `receiver->activePrint_.store(nullptr, ...)` (Print goes invalid when the worker exits — readback MUST capture before this); `:763` `emit receiver->sliceFinished(...)` (GUI-thread delivery after a successful slice). `src/core/viewmodels/EditorViewModel.h:860` holds `SliceService *sliceService_` and is the bridge that can push geometry into `RhiViewport`. | Add: Phase 100 reads `wipe_tower_data()` after `print.process()` succeeds (between `:584` and the `:625` invalidation), captures the dims inside the worker, delivers them to the GUI thread alongside the existing `sliceFinished` path, and has `EditorViewModel` push them into the `RhiViewport` Q_PROPERTYs (see Frozen Decisions). | No readback path exists; the `Print` and its `wipe_tower_data` are constructed, used for slicing, and destroyed without ever exposing geometry to the GUI/renderer. | Critical | Phase 100 | WTREAD-01 | Source audit (Phase 102) proving the readback reads `wipe_tower_data()` between `print.process()` and `activePrint_.store(nullptr)` and delivers dims via the `sliceFinished` GUI-thread path; downstream runtime visual evidence (Phase 102). |
| WT-RENDER-UPGRADE | `src/core/rendering/GizmoGeometry.cpp:449-499` (`buildWipeTowerVertices`, the Option A target: keep the box builder, feed real dims). | Option A source-truth: `3DScene.cpp:840-885 load_wipe_tower_preview` (`make_cube(width, depth, height)` box from real dims, `:855`). Option B source-truth: `3DScene.cpp:887-925 load_real_wipe_tower_preview` (real mesh via `mesh.convex_hull_3d()` at `:914` from `wipe_tower_mesh_data`; shows brim/rib/cone base). | `src/core/rendering/GizmoGeometry.cpp:449-499` (`buildWipeTowerVertices`, Option A target: keep the box builder, feed real dims); for Option B the `indexed_triangle_set` from `wipe_tower_mesh_data->real_wipe_tower_mesh` + `real_brim_mesh` (`Print.hpp:745-746`) would need ITS handling in `GizmoGeometry` + `RhiViewportRenderer`. | Freeze Option A as v4.4 baseline; document Option B as future upgrade (see Frozen Decisions WTAUDIT-02). | The box is a placeholder shape (no brim/rib/cone base) until Option B lands; Option A closes the dim gap with minimal risk and reuses the existing `buildWipeTowerVertices` + `uploadWipeTowerBuffer` pipeline unchanged except for the fed dims. | High | Phase 101 | WTRENDER-02 | Source audit (Phase 102) confirming Option A is the v4.4 baseline (real dims fed to the existing box builder); Option B documented as deferred; downstream runtime visual evidence (Phase 102). |
| WT-HAS-WIPE-GATE | `src/qml_gui/Renderer/RhiViewport.h:54` (`showWipeTower` Q_PROPERTY, default false at `:304`); `src/qml_gui/Renderer/RhiViewportRenderer.h:216` (`m_showWipeTower=false`). | `Print.hpp:988 has_wipe_tower()` is the gate — returns false for single-material / `enable_prime_tower` off. Upstream only loads a wipe-tower volume when `has_wipe_tower()` is true. | `src/qml_gui/Renderer/RhiViewport.h:54` (`showWipeTower` Q_PROPERTY) + `:304` (`m_showWipeTower=false` default); `src/qml_gui/Renderer/RhiViewportRenderer.h:216` (`m_showWipeTower=false`); `renderWipeTower` early-returns when `!m_showWipeTower` (`RhiViewportRenderer.cpp:1898`). | Preserve + enforce: when `has_wipe_tower()` is false, no geometry is pushed and `m_showWipeTower` stays false — no placeholder box leak on single-material slices (see Frozen Decisions WTAUDIT-02). | Today the gate is purely a Q_PROPERTY default (`false`); nothing in the readback path enforces it from the real `Print::has_wipe_tower()` result, so the guarantee is currently structural (default off) not data-driven. | Critical | Phase 100 | WTREAD-02 | Source audit (Phase 102) proving the readback sets `showWipeTower` from `Print::has_wipe_tower()` and pushes no geometry when false; downstream runtime evidence (Phase 102) showing no placeholder box on a single-material slice. |
| WT-SOFTWARE-VIEWPORT | `src/qml_gui/Renderer/SoftwareViewport.h:35-40` (Q_PROPERTY show/width/depth/height/x/z), `:126-127` (accessors), `:231-236` (defaults `m_showWipeTower=true`, width/depth/height/x/z all `0.f`); `src/qml_gui/Renderer/SoftwareViewport.cpp:207-253` setters (each calls `update()`). | Same as WT-VIEWPORT-DEFAULTS — `SoftwareViewport` is a parallel non-RHI (`QQuickPaintedItem`) renderer; upstream has no equivalent (upstream is GL-only). The source-truth is the same `wipe_tower_data` dims. | `src/qml_gui/Renderer/SoftwareViewport.h:35-40,126-137,231-236` (show/width/depth/height/x/z props, defaults show=true + zeros); `SoftwareViewport.cpp:207-253` setters (each calls `update()`). Registered as the alternate `GLViewport` QML type at `main_qml.cpp:305`. | Mirror readback into SoftwareViewport too: Phase 100/101 push the same real dims so the software path does not lag the RHI path. Note the default `show=true` + zero dims differs from RhiViewport's `show=false` + 10/10/50/100/25 — Phase 100 should align both to the data-driven gate. | SoftwareViewport defaults differ from RhiViewport (show=true + zero dims vs show=false + 10/10/50/100/25); without mirroring the readback, the two renderers would show inconsistent wipe-towers. | Medium | Phase 100 | WTREAD-01 | Source audit (Phase 102) confirming SoftwareViewport receives the same real dims + gate as RhiViewport; downstream runtime visual evidence (Phase 102) on both render paths. |

## Frozen Decisions (WTAUDIT-02)

The three designs below are locked before implementation. Phase 100 implements
exactly the readback + gate choices; Phase 101 implements the render-upgrade
baseline. Deviations require re-opening WTAUDIT-02.

### 1. Post-slice readback integration point (WT-READBACK-POINT) — LOCKED: read `wipe_tower_data()` inside the SliceService worker after `print.process()` succeeds, deliver dims to the GUI thread alongside `sliceFinished`, push into `RhiViewport` via `EditorViewModel`

The `Print` object that holds `wipe_tower_data` is constructed and destroyed
entirely inside the `SliceService` worker lambda. Its lifetime is bounded:

- `SliceService.cpp:508` — `receiver->activePrint_.store(&print,
  std::memory_order_release)`: the `Print` becomes addressable to the GUI
  thread mid-slice (this is what `SliceService::cancelSlice()` taps at
  `SliceService.cpp:775` to call `active->cancel()`).
- `SliceService.cpp:584` — `print.process()`: the slice that populates
  `wipe_tower_data` (and all other print results). After this returns
  successfully, `Print::wipe_tower_data()` is valid.
- `SliceService.cpp:625` / `:629` / `:634` —
  `receiver->activePrint_.store(nullptr, std::memory_order_release)`: the
  `Print` goes invalid when the worker lambda exits (normal exit at `:625`,
  exception at `:629`, catch-all at `:634`). The `print` local is destroyed
  when the lambda's scope ends, so any pointer captured before this is dangling
  after.
- `SliceService.cpp:763` — `emit receiver->sliceFinished(receiver->estimatedTimeLabel_)`:
  the GUI-thread delivery point after a successful slice (queued connection,
  emitted from the outer `QMetaObject::invokeMethod` that wraps the whole
  worker).

`EditorViewModel.h:860` — `SliceService *sliceService_ = nullptr;` is the
viewmodel-side bridge that already holds the service and can push geometry into
`RhiViewport` Q_PROPERTYs (PreparePage.qml:1648 GLViewport is bound to
`root.editorVm`).

**Upstream behavior truth:** upstream OrcaSlicer reads
`Print::wipe_tower_data()` after the slice completes (via the plater's
`update_schedule`/`arrange` refresh) to drive the 3D preview —
`3DScene.cpp:840 load_wipe_tower_preview` consumes `width`/`depth`/`height`/
`position` from the real slice result, not placeholders.

**Locked readback design:** after `print.process()` succeeds (between `:584`
and the `:625` invalidation), read `Print::wipe_tower_data()` inside the worker
— capturing `bbx`, `depth`, `height`, `position`, `width`, `brim_width`,
`rib_offset`, plus the `has_wipe_tower()` gate result — into plain value
captures (no `Print*` escapes the worker). Deliver those captured values to the
GUI thread alongside the existing `sliceFinished` path (e.g. extend the queued
`invokeMethod` payload or a dedicated signal), and have `EditorViewModel`
(which already holds `sliceService_` at `EditorViewModel.h:860`) push them into
the `RhiViewport` Q_PROPERTYs (`wipeTowerWidth`/`Depth`/`Height`/`X`/`Z` +
`showWipeTower`) that Phase 100's WT-VIEWPORT-DEFAULTS wiring exposes to
PreparePage.qml. The `Print` is only valid mid-slice between
`activePrint_.store(&print)` (`:508`) and `activePrint_.store(nullptr)`
(`:625`/`:629`/`:634`), so the readback MUST capture the values into
worker-local storage before the lambda exits — never capture a `Print*` for
later GUI-thread use. Severity Critical; Owner Phase 100; Requirement
WTREAD-01.

### 2. Rendering-upgrade approach (WT-RENDER-UPGRADE) — LOCKED: Option A (dimensioned box) as v4.4 baseline; Option B (real mesh) documented as future upgrade

Two candidate approaches were evaluated against the two upstream paths:

- **Option A — dimensioned box.** Read `bbx`/`depth`/`height`/`position`/
  `width` from `wipe_tower_data`, push as dims to the existing
  `buildWipeTowerVertices`. Still a box, but real dimensions. Low risk, reuses
  the entire `buildWipeTowerVertices` + `uploadWipeTowerBuffer` +
  `renderWipeTower` pipeline unchanged except for the fed dims. Mirrors
  `3DScene.cpp:840-885 load_wipe_tower_preview`, which uses
  `make_cube(width, depth, height)` (`:855`) as the default box preview and
  sets `v.is_wipe_tower = true` (`:882`).
- **Option B — real mesh.** Read optional `wipe_tower_mesh_data`
  (`Print.hpp:766`, type `std::optional<WipeTowerMeshData>`), extract the
  `indexed_triangle_set` from `real_wipe_tower_mesh` + `real_brim_mesh`
  (`Print.hpp:745-746`), build a real vertex buffer mirroring
  `3DScene.cpp:887-925 load_real_wipe_tower_preview` via
  `mesh.convex_hull_3d()` (`:914`). Higher fidelity (shows brim, rib, cone
  base), but more work: needs ITS handling in `GizmoGeometry` +
  `RhiViewportRenderer` (the current `buildWipeTowerVertices` emits a fixed
  36-vertex layout, not an arbitrary ITS), and `wipe_tower_mesh_data` may be
  `std::nullopt` depending on config (its `clear()` resets it to `nullopt` at
  `Print.hpp:776`).

**Upstream behavior truth:** upstream OrcaSlicer ships BOTH paths —
`load_wipe_tower_preview` (box) as the default and `load_real_wipe_tower_preview`
(real mesh) as the high-fidelity path used when `wipe_tower_mesh_data` is
populated. The box path is the steady-state preview; the real-mesh path is the
upgrade.

**Locked choice: Option A as the v4.4 baseline.** Rationale: (a) it closes the
"placeholder 10/10/50/100/25 defaults" gap with minimal risk, reusing the
existing `buildWipeTowerVertices` + `uploadWipeTowerBuffer` pipeline unchanged
except for the fed dims; (b) it mirrors upstream's default box-preview path
(`3DScene.cpp:840 load_wipe_tower_preview`), so the v4.4 box is source-truth
for the default fidelity; (c) Option B's `wipe_tower_mesh_data` may be
`std::nullopt` depending on config, and the ITS extraction adds renderer scope
(ITS vertex format in `GizmoGeometry` + a non-box upload path in
`RhiViewportRenderer`) that v4.4 defers. Option B is documented here as a
future upgrade; its `load_real_wipe_tower_preview` anchor (`3DScene.cpp:887`)
and `wipe_tower_mesh_data` / `convex_hull_3d` citations are preserved so a
future milestone can pick it up without re-discovery. Severity High; Owner
Phase 101; Requirement WTRENDER-02.

### 3. has_wipe_tower() gating (WT-HAS-WIPE-GATE) — LOCKED: when `Print::has_wipe_tower()` is false, no geometry is pushed and `showWipeTower` stays false

`Print::has_wipe_tower()` (`Print.hpp:988`) is the gate: it returns false for
single-material prints or when `enable_prime_tower` is off. Upstream only loads
a wipe-tower volume when `has_wipe_tower()` is true; otherwise no wipe-tower
volume is created.

**Locked gating design:** the Phase 100 readback (Frozen Decision 1) captures
`Print::has_wipe_tower()` alongside the dims. When it is false, the readback
pushes NO geometry and `RhiViewport::showWipeTower` (`RhiViewport.h:54`,
default false at `:304`; mirrored in `RhiViewportRenderer.h:216
m_showWipeTower=false`) stays false — `renderWipeTower` early-returns on
`!m_showWipeTower` (`RhiViewportRenderer.cpp:1898`), so no placeholder box
leaks onto single-material slices. This is the WTREAD-02 guarantee. The current
structural default (`showWipeTower=false`) already prevents a leak today, but
the gate becomes data-driven once the readback lands: the readback must set
`showWipeTower` from the real `has_wipe_tower()` result, not assume the
default. `SoftwareViewport` (default `m_showWipeTower=true` at
`SoftwareViewport.h:231`) must be aligned to the same data-driven gate so its
differing default does not leak a box either. Severity Critical; Owner Phase
100; Requirement WTREAD-02.

## v4.4 Requirement Routing

Every v4.4 requirement is routed to its owner phase and matrix region(s).

| Requirement | Owner | Matrix Region(s) |
|---|---|---|
| WTAUDIT-01 | Phase 99 | All canonical regions in this matrix (WT-PLACEHOLDER-BOX through WT-SOFTWARE-VIEWPORT). |
| WTAUDIT-02 | Phase 99 | Frozen Decisions section above (WT-READBACK-POINT worker readback; WT-RENDER-UPGRADE Option A baseline; WT-HAS-WIPE-GATE data-driven gate). |
| WTREAD-01 | Phase 100 | WT-PRINT-DATA, WT-READBACK-POINT, WT-VIEWPORT-DEFAULTS, WT-SOFTWARE-VIEWPORT. |
| WTREAD-02 | Phase 100 | WT-HAS-WIPE-GATE. |
| WTRENDER-01 | Phase 101 | WT-PLACEHOLDER-BOX, WT-RENDERER-BUFFER. |
| WTRENDER-02 | Phase 101 | WT-RENDER-UPGRADE (Option A baseline; Option B documented as future upgrade). |
| WTVERIFY-01 | Phase 102 | Source/QML audits across WT-VIEWPORT-DEFAULTS, WT-PRINT-DATA, WT-READBACK-POINT, WT-HAS-WIPE-GATE, WT-PLACEHOLDER-BOX (placeholder 10/10/50/100/25 no longer steady-state when a real slice exists). |
| WTVERIFY-02 | Phase 102 | Final runtime launch, canonical verifier, Prepare/Preview/AssembleView regression ctest, and runtime wipe-tower visibility evidence when a multi-material slice produces one. |

## Out-of-Scope Classification

The following items are explicitly out of scope for v4.4. No phase in v4.4
re-touches them unless the user explicitly reopens them.

| Item | Status | Evidence / Reason |
|---|---|---|
| Auto filament-map recommendation | Out of scope — future milestone | Separate algorithm + UI milestone; the cleanest impl leans on libslic3r's auto-firing in `Print::` + a QML popup. Loosely coupled to wipe-tower but deferred to avoid scope creep (AUTOMAP-FUTURE-01). |
| Per-plate wipe-tower architecture refactor | Out of scope — future milestone | The v3.0 audit flags Qt6's per-plate filtered-copy slice path; multi-plate wipe-tower correctness depends on that deeper architecture. v4.4 reads geometry from the slice result that exists, not a refactor. |
| D3D12 or Vulkan backend promotion | Out of scope — future backend work | D3D12 has a known startup crash and remains explicit opt-in; Vulkan is disabled in the current Qt 6.10 SDK. Wipe-tower readback + rendering runs on the default RHI/D3D11 path (BACKEND-FUTURE-01). |
| Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | Out of scope — future milestone | Needs per-volume ITS + scene raycaster. Not required for wipe-tower geometry (MEASURE-FUTURE-01). |
| Missing CLI fixtures + deterministic argv-based GUI fixture loading for screenshots | Out of scope — future fixture milestone | FIXTURE-FUTURE-01 is unblocked by v4.3, but the fixtures themselves are a future milestone. |
| LAN/device/cloud/network/Monitor/ModelMall/Home WebView/camera/printer-hardware workflows | Out of scope — removed scope | Removed from forward scope by user direction on 2026-07-07 (NETWORK-REMOVED-01). The v4.4 milestone is local/offline wipe-tower geometry readback + rendering only. Not reintroduced unless the user explicitly reopens it. |
| libslic3r slicing algorithm changes | Out of scope — engine preserved | The migration rewrites the GUI layer only; libslic3r slicing algorithms are preserved unchanged. Wipe-tower readback must not change slicing engine behavior. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| WTAUDIT-01 | This matrix maps all 8 wipe-tower regions (WT-PLACEHOLDER-BOX through WT-SOFTWARE-VIEWPORT) to OrcaSlicer upstream source files (with line anchors: `Print.hpp:740-786,988-989,1078-1080`, `3DScene.cpp:840-885,887-925`), current Qt placeholder paths (`GizmoGeometry.h:74`/`.cpp:449-499`, `RhiViewport.h:54-59,181-192,304-309`, `RhiViewportRenderer.h:57,75,129,143,152,177,216-222`/`.cpp:1064-1095,1894-1908`, `SoftwareViewport.h:35-40,126-137,231-236`/`.cpp:207-253`, `PreparePage.qml:1648`), the post-slice readback design (Frozen Decision 1, citing `SliceService.cpp:508,584,625/629/634,763` + `EditorViewModel.h:860`), the rendering-upgrade decision (Frozen Decision 2, Option A baseline + Option B future), and verification expectations (Phase 102 source audits + canonical verifier + runtime visual evidence). |
| WTAUDIT-02 | The Frozen Decisions section locks three designs before implementation: (1) post-slice readback integration point = read `Print::wipe_tower_data()` inside the SliceService worker after `print.process()` succeeds (between `:584` and the `:625` invalidation), capture dims into worker-local storage, deliver to the GUI thread alongside `sliceFinished` (`:763`), push into `RhiViewport` Q_PROPERTYs via `EditorViewModel` (`EditorViewModel.h:860`) — because the `Print` is only valid mid-slice between `activePrint_.store(&print)` (`:508`) and `activePrint_.store(nullptr)` (`:625`/`:629`/`:634`); (2) rendering-upgrade approach = Option A (dimensioned box from real `bbx`/`depth`/`height`/`position`/`width`, fed to the existing `buildWipeTowerVertices`, mirroring `3DScene.cpp:840-885 load_wipe_tower_preview`) LOCKED as the v4.4 baseline, with Option B (real mesh from optional `wipe_tower_mesh_data` via `convex_hull_3d`, mirroring `3DScene.cpp:887-925 load_real_wipe_tower_preview`) documented as a future upgrade; (3) `has_wipe_tower()` gating (`Print.hpp:988`) = when false, no geometry is pushed and `showWipeTower` stays false (`RhiViewport.h:54`/`:304`, `RhiViewportRenderer.h:216`), so no placeholder box leaks on single-material slices — the gate becomes data-driven from the readback (SoftwareViewport's differing default `show=true` at `SoftwareViewport.h:231` is aligned to the same gate). |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 100 | Implement the post-slice readback reading `Print::wipe_tower_data()` inside the SliceService worker after `print.process()` succeeds, capturing `bbx`/`depth`/`height`/`position`/`width`/`brim_width`/`rib_offset` + `has_wipe_tower()` before the `Print` is invalidated, and delivering dims to the GUI thread via the `sliceFinished` path (WT-READBACK-POINT, WT-PRINT-DATA); wire `PreparePage.qml:1648` GLViewport wipe-tower Q_PROPERTY bindings + the EditorViewModel push (WT-VIEWPORT-DEFAULTS); enforce the `has_wipe_tower()` gate so no placeholder box leaks on single-material slices (WT-HAS-WIPE-GATE); mirror the readback into SoftwareViewport (WT-SOFTWARE-VIEWPORT). |
| 101 | Feed the real dims into `buildWipeTowerVertices` so the rendered wipe-tower reflects the real sliced dimensions (WT-PLACEHOLDER-BOX); confirm the `uploadWipeTowerBuffer` + `renderWipeTower` pipeline rebuilds on `m_wipeTowerDirty` with the new dims (WT-RENDERER-BUFFER); ship Option A as the v4.4 baseline, documenting Option B (real mesh via `wipe_tower_mesh_data`/`convex_hull_3d`) as a future upgrade (WT-RENDER-UPGRADE). |
| 102 | Run source/QML audits proving the placeholder 10/10/50/100/25 defaults are no longer steady-state when a real slice exists (WTVERIFY-01); run the canonical verifier, launch `build/OWzxSlicer.exe`, confirm Prepare/Preview/AssembleView regression-free, and record runtime wipe-tower visibility evidence when a multi-material slice produces one (WTVERIFY-02). |
