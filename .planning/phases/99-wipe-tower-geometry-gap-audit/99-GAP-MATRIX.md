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
| WT-PLACEHOLDER-BOX | `src/core/rendering/GizmoGeometry.h:74` + `.cpp:449-499` (`buildWipeTowerVertices(x, z, width, depth, height)`, 36-vertex box, color `{0.35, 0.60, 0.85, 0.50}` at `.cpp:463`). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-VIEWPORT-DEFAULTS | `src/qml_gui/Renderer/RhiViewport.h:54-59` (Q_PROPERTY show/width/depth/height/x/z), `:181-192` accessors, `:304-309` member defaults width=10/depth=10/height=50/x=100/z=25; `src/qml_gui/pages/PreparePage.qml:1648` GLViewport does NOT bind any wipe-tower Q_PROPERTY (grep returns nothing). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-RENDERER-BUFFER | `src/qml_gui/Renderer/RhiViewportRenderer.h:57` (`uploadWipeTowerBuffer`), `:75` (`renderWipeTower`), `:129` (`m_wipeTowerBuffer`), `:143` (`m_wipeTowerBufferUploaded`), `:152` (`m_wipeTowerBufferBytes`), `:177` (`m_wipeTowerVertexCount`), `:216-222` (`m_showWipeTower` + `m_wipeTowerWidth/Depth/Height/X/Z` + `m_wipeTowerDirty`); `RhiViewportRenderer.cpp:1064-1095` (`uploadWipeTowerBuffer` builds box via `buildWipeTowerVertices`, uploads `m_wipeTowerBuffer`) and `:1894-1908` (`renderWipeTower` via `m_translucentFillPipeline`). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-PRINT-DATA | N/A today — zero references to `wipe_tower_data` / `has_wipe_tower` (readback) / `get_wipe_tower_depth` / `get_wipe_tower_bbx` / `get_rib_offset` in `src/` (only `has_wipe_tower` is a g-code label at `EditGCodeDialog.qml:381`). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-READBACK-POINT | N/A today — no readback code exists; the `Print` object lives inside the `SliceService` worker lambda and is never queried for `wipe_tower_data`. | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-RENDER-UPGRADE | `src/core/rendering/GizmoGeometry.cpp:449-499` (`buildWipeTowerVertices`, the Option A target: keep the box builder, feed real dims). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-HAS-WIPE-GATE | `src/qml_gui/Renderer/RhiViewport.h:54` (`showWipeTower` Q_PROPERTY, default false at `:304`); `src/qml_gui/Renderer/RhiViewportRenderer.h:216` (`m_showWipeTower=false`). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
| WT-SOFTWARE-VIEWPORT | `src/qml_gui/Renderer/SoftwareViewport.h:35-40` (Q_PROPERTY show/width/depth/height/x/z), `:126-127` (accessors), `:231-236` (defaults `m_showWipeTower=true`, width/depth/height/x/z all `0.f`); `src/qml_gui/Renderer/SoftwareViewport.cpp:207-253` setters (each calls `update()`). | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ | _TBD (Task 2)_ |
