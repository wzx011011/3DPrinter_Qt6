---
gsd_state_version: 1.0
milestone: v4.4
milestone_name: Wipe-Tower Geometry Readback And Real Rendering
status: planning
last_updated: 2026-07-11T12:00:00+08:00
last_activity: 2026-07-11 -- Milestone v4.4 started
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
stopped_at: Milestone v4.4 started; defining requirements and roadmap
---

# Project State

**Milestone:** v4.4 - Wipe-Tower Geometry Readback And Real Rendering
**Status:** Planning (defining requirements and roadmap)
**Next step:** Plan Phase 99 with `/gsd-plan-phase 99`.

## Current Position

Phase: 99 (Wipe-Tower Geometry Gap Audit) — not started
Plan: —
Status: Roadmap approved; ready to plan Phase 99
Last activity: 2026-07-11 — v4.4 roadmap created (Phases 99-102)

## Current Milestone (v4.4)

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 99 | Wipe-Tower Geometry Gap Audit | Not started | WTAUDIT-01, WTAUDIT-02 |
| 100 | Wipe-Tower Geometry Readback | Not started | WTREAD-01, WTREAD-02 |
| 101 | Wipe-Tower Real Rendering Upgrade | Not started | WTRENDER-01, WTRENDER-02 |
| 102 | Wipe-Tower Verification And Regression | Not started | WTVERIFY-01, WTVERIFY-02 |

## Last Completed Milestone: v4.3

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 94 | Thumbnail Capture Gap Audit | Complete | THUMBAUDIT-01, THUMBAUDIT-02 |
| 95 | QRhi Thumbnail Capture Infrastructure | Complete | THUMBCAP-01, THUMBCAP-02, THUMBCAP-03 |
| 96 | 3MF Thumbnail Write Integration | Complete | THUMBWRITE-01, THUMBWRITE-02, THUMBWRITE-03 |
| 97 | Thumbnail Save-Reload Round-Trip | Complete | THUMBRT-01, THUMBRT-02 |
| 98 | Thumbnail Verification And Cleanup | Complete | THUMBVERIFY-01, THUMBVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-07-11)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** v4.4 — wipe-tower geometry readback from `Print::wipe_tower_data()` + real rendering replacing the placeholder box.

## Milestone Context (v4.4)

**Goal:** Replace the hardcoded placeholder-box wipe-tower rendering with real libslic3r post-slice geometry.

**Current state (from pre-planning exploration):**
- Wipe-tower rendering pipeline is 90% ready (RHI `RhiViewportRenderer` has `m_wipeTowerBuffer` + `uploadWipeTowerBuffer()` + `renderWipeTower()`; Software viewport has parallel props).
- **Geometry is a hardcoded placeholder:** `GizmoGeometry::buildWipeTowerVertices` builds a 36-vertex rectangular prism (6 faces) with caller-supplied or default dims. `RhiViewport` defaults: width=10, depth=10, height=50, x=100, z=25.
- **The Qt6 side never reads libslic3r's `Print::wipe_tower_data()`** — zero references to `wipe_tower_data`, `get_wipe_tower_depth`, `get_wipe_tower_bbx` in `src/`.
- `WipeTowerDialog.qml:25-30` has a mock `flushMatrix` (not wired to real data).

**Upstream source anchors (behavior truth):**
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:740-786` — `struct WipeTowerData`: tool_changes, bbx (includes brim), rib_offset, wipe_tower_mesh_data (optional), depth, height, brim_width, position, width.
- `Print.hpp:988-989` — `has_wipe_tower()`, `wipe_tower_data()`.
- `Print.hpp:1078-1080` — `get_wipe_tower_depth()`, `get_wipe_tower_bbx()`, `get_rib_offset()`.
- `third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp:840-882` — `load_wipe_tower_preview` (make_cube box).
- `3DScene.cpp:887-923` — `load_real_wipe_tower_preview` (uses real mesh via convex_hull_3d).

**Out of scope for v4.4:**
- Auto filament-map recommendation (future milestone; cleanest impl is letting libslic3r auto-compute in `Print::` and reading back `filament_maps`).
- Per-plate wipe-tower architecture refactor (v3.0-audited per-plate filtered-copy slice path).
- D3D12, GLGizmoMeasure engine, CLI fixtures.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.3 Real Thumbnail Capture + 3MF round-trip | Shipped in v4.3 |
| active | Wipe-tower geometry readback + real rendering | v4.4 |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | Auto filament-map recommendation | Future milestone (loosely coupled, deferred from v4.4) |
| future | Missing CLI test fixtures | Future fixture milestone (FIXTURE-02 unblocked by v4.3) |
| future | D3D12 root cause | Dedicated backend investigation milestone |
| future | Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | Future milestone (needs per-volume ITS) |

## Scope Guard

- v4.4 is wipe-tower geometry readback + rendering only.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

## Operator Next Steps

- Define REQUIREMENTS.md for v4.4.
- Create ROADMAP.md (phases continue from 99).
