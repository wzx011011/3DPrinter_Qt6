# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- 🚧 **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (in progress)

## Current Milestone: v4.4 Wipe-Tower Geometry Readback And Real Rendering

**Goal:** Replace the current hardcoded placeholder-box wipe-tower rendering with real libslic3r post-slice geometry. Read `Print::wipe_tower_data()` and feed it to the existing RHI renderer so the Prepare view wipe-tower reflects real slice results.

**Scope rule:** Local/offline only. Auto filament-map recommendation is explicitly deferred to a future milestone — v4.4 is wipe-tower geometry readback + rendering only. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

**Current state (from pre-planning exploration):**
- Wipe-tower rendering pipeline is 90% ready (RHI `m_wipeTowerBuffer` + `uploadWipeTowerBuffer()` + `renderWipeTower()`; Software viewport parallel).
- **Geometry is a hardcoded placeholder:** `GizmoGeometry::buildWipeTowerVertices` (36-vertex box); `RhiViewport` defaults width=10/depth=10/height=50/x=100/z=25.
- **Qt6 never reads `Print::wipe_tower_data()`** — zero references in `src/`.

## Phases

- [ ] Phase 99: Wipe-Tower Geometry Gap Audit
- [ ] Phase 100: Wipe-Tower Geometry Readback
- [ ] Phase 101: Wipe-Tower Real Rendering Upgrade
- [ ] Phase 102: Wipe-Tower Verification And Regression

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 99 | Wipe-Tower Geometry Gap Audit | Freeze the v4.4 readback + rendering region map: placeholder path, upstream `Print::wipe_tower_data()` anchors, post-slice readback integration point, rendering-upgrade decision (box dims vs real mesh), and verification expectations before edits. | WTAUDIT-01, WTAUDIT-02 |
| 100 | Wipe-Tower Geometry Readback | After a successful slice, read real wipe-tower geometry from `Print::wipe_tower_data()` (bbx, depth, height, brim, rib_offset, position, width) and push it into `RhiViewport`/`EditorViewModel`, respecting `has_wipe_tower()`. | WTREAD-01, WTREAD-02 |
| 101 | Wipe-Tower Real Rendering Upgrade | Feed the real geometry dimensions into the renderer (replace placeholder defaults) and upgrade `GizmoGeometry::buildWipeTowerVertices` from a placeholder box toward real geometry (dimensioned box minimum, real mesh if feasible), with brim/rib representation. | WTRENDER-01, WTRENDER-02 |
| 102 | Wipe-Tower Verification And Regression | Lock source/QML audits, run canonical verifier, confirm Prepare/Preview/AssembleView regression-free, and verify the wipe-tower renders at runtime when a multi-material slice produces one. | WTVERIFY-01, WTVERIFY-02 |

### Phase 99: Wipe-Tower Geometry Gap Audit

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. The wipe-tower capture + rendering surface is mapped to upstream `Print::wipe_tower_data()` / `WipeTowerData` anchors, the current placeholder path, the post-slice readback integration point, and verification expectations.
2. The readback design and the rendering-upgrade approach (dimensioned box from bbx/depth/height vs real mesh from `wipe_tower_mesh_data`) are frozen as locked decisions before implementation.

### Phase 100: Wipe-Tower Geometry Readback

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. After a successful slice, real wipe-tower geometry (bbx/depth/height/brim/rib_offset/position/width) is read from `Print::wipe_tower_data()` and pushed into the renderer-facing layer, replacing the placeholder defaults.
2. The readback respects `Print::has_wipe_tower()` — no wipe-tower geometry is pushed when none exists.

### Phase 101: Wipe-Tower Real Rendering Upgrade

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. The rendered wipe-tower reflects real sliced dimensions (not placeholder 10/10/50/100/25).
2. `GizmoGeometry::buildWipeTowerVertices` is upgraded toward real geometry (dimensioned box minimum; real mesh if feasible), with brim/rib representation where applicable.

### Phase 102: Wipe-Tower Verification And Regression

**Status:** Not started
**Plans:** 0/1

Success criteria:
1. Placeholder defaults are no longer steady-state when a real slice exists; source/QML audits cover the readback wiring + rendering-upgrade anchors.
2. The canonical verifier passes, `build/OWzxSlicer.exe` launches, Prepare/Preview/AssembleView rendering is regression-free, and the wipe-tower is visible at runtime when a multi-material slice produces one.

## Deferred Backlog

- Auto filament-map recommendation (let libslic3r auto-compute, Qt6 reads back + UI; widen mode enum).
- Missing CLI fixtures and deterministic argv-based GUI fixture loading for screenshots (FIXTURE-02 unblocked by v4.3).
- D3D12 root-cause investigation and future Vulkan/D3D12 backend promotion.
- Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper (needs per-volume ITS).

## Removed Scope

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware calibration are no longer backlog items.

## Next Step

Plan Phase 99 after this roadmap is approved:

```text
$gsd-plan-phase 99
```

---

*Last updated: 2026-07-11 at v4.4 milestone start.*
