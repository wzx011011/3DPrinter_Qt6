---
gsd_state_version: 1.0
milestone: v4.2
milestone_name: AssembleView Source-Truth Restoration
status: executing
last_updated: 2026-07-09T17:15:00+08:00
last_activity: 2026-07-09 -- Phase 90 plan 01 complete (AssembleView shell + canvas host shipped)
progress:
  total_phases: 5
  completed_phases: 1
  total_plans: 5
  completed_plans: 2
  percent: 40
stopped_at: Phase 90 plan 01 complete; Phase 91 ready to plan/execute
---

# Project State

**Milestone:** v4.2 - AssembleView Source-Truth Restoration
**Status:** Executing (Phase 90 shell + canvas host complete)
**Next step:** Plan Phase 91 (Explosion Ratio And Assembly Rendering) with `/gsd-plan-phase 91`.

## Current Position

Phase: 90 (AssembleView Shell And Canvas Host Restoration) — complete
Plan: 01 — complete
Status: AssembleView placeholder replaced with real canvas host + 4-region shell + nav toggle + CanvasAssembleView routing; canonical verifier exited 0, all 6 suites passed.
Last activity: 2026-07-09 — Phase 90 plan 01 executed (9 tasks, 8 commits + docs)

## Current Milestone (v4.2)

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 89 | AssembleView Source-Truth Gap Audit | Complete | ASMAUDIT-01, ASMAUDIT-02 |
| 90 | AssembleView Shell And Canvas Host Restoration | Complete | ASMSHELL-01, ASMSHELL-02, ASMROUTE-01 |
| 91 | Explosion Ratio And Assembly Rendering | Not started | ASMEXPLODE-01, ASMEXPLODE-02 |
| 92 | Assembly Measurement Gizmo | Not started | ASMMEASURE-01, ASMMEASURE-02 |
| 93 | AssembleView Verification And Cleanup | Not started | ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02 |

## Last Completed Milestone: v4.1

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 84 | Settings Source-Truth Gap Audit | Complete | SETAUDIT-01, SETAUDIT-02 |
| 85 | Settings Shell And Tab Layout Restoration | Complete | SETLAYOUT-01, SETLAYOUT-02, SETLAYOUT-03 |
| 86 | Settings Option Sections And Typed Controls | Complete | SETCTRL-01, SETCTRL-02, SETCTRL-03 |
| 87 | Settings Preset Semantics And Workflow Stability | Complete | SETSEM-01, SETSEM-02, SETSEM-03 |
| 88 | Settings Verification And Cleanup | Complete | SETCLEAN-01, SETVERIFY-01, SETVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-07-09)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** v4.2 AssembleView source-truth restoration — the fourth screenshot-driven UI restoration milestone.

## Milestone Context (v4.2)

**Goal:** Restore OrcaSlicer's AssembleView to screenshot/source-truth parity, replacing the current `Plater.qml` placeholder.

**Upstream source anchors (behavior truth):**
- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` — `class AssembleView : public wxPanel` (third GLCanvas3D host)
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:509-513` — `ECanvasType::CanvasAssembleView = 2`
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.hpp:596,770-771` — `m_explosion_ratio`, `get_explosion_ratio()`, `reset_explosion_ratio()`
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmoAssembly.hpp:9` — `class GLGizmoAssembly : public GLGizmoMeasure` (`Ctrl+Y`, `ONLY_ASSEMBLY` mode)
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268,274,299` — `AssembleViewDataID`, `AssembleViewDataPool`, `AssembleViewDataBase`
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4959,4431,7322,11601,11635,11744,11823` — AssembleView instantiation and CanvasAssembleView branching (selection/undo-redo/gizmo routing)

**Current Qt state (the gap):**
- `src/qml_gui/pages/Plater.qml:102-115` — placeholder `Item` showing "装配视图暂不可用"
- `src/qml_gui/BackendContext.h:159,193-199,227` — `ViewMode::AssembleView = 2` enum entry only
- No AssemblePage.qml, no AssembleViewModel, no explosion-ratio support, no Assembly gizmo, no data pool, no navigation entry in main.qml.

**Out of scope for v4.2:**
- Arrange (auto-arrangement) — already fully implemented (`Slic3r::arrange_objects` via libnest2d + full settings UI in PreparePage).
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-connected hardware workflows (removed scope).

**Screenshot dependency:**
- The three AssembleView screenshots are now present in `shotScreen/` (`装配页.png`, `装配页_爆炸.png`, `装配页_测量.png`) and are cited as visual truth in `89-GAP-MATRIX.md`. Phase 90-93 execute against them.

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.1 Parameter settings dialogs | Shipped in v4.1 |
| active | AssembleView source-truth restoration | v4.2 (Phase 89-90 complete; Phase 91-93 pending) |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | Auto filament-map recommendation + wipe-tower geometry/rendering | Future milestone |
| future | Real GL/QRhi-capture thumbnails + 3MF pixel round-trip | Future milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| future | Deterministic argv-based GUI fixture loading for screenshots | Partially addressed by v4.1 startup deep links |
| future | D3D12 root cause | Dedicated backend investigation milestone |

## Deferred Items

Items acknowledged and deferred at v4.1 milestone close on 2026-07-09:

| Category | Item | Status |
|---|---|---|
| debug | qrhi-d3d12-crash | D3D11 default path remains verified; D3D12 remains future opt-in investigation |
| evidence | Loaded-G-code runtime screenshot | Deterministic loaded fixture screenshots now possible via `--load-model` startup hook but fixtures remain future |
| process | per-phase VALIDATION.md | Phase 84-88 have deterministic verification artifacts (Nyquist compliant) but no separate Nyquist validation files |
| evidence | Direct SettingsDialog window capture | Blocked by Windows capture API; SETVERIFY-02 accepts manual click-through plus runtime evidence plus canonical verifier |

## Scope Guard

- v4.2 is local/offline AssembleView UI work only.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

## Operator Next Steps

- Phase 89 gap audit and Phase 90 shell + canvas host are complete. The `Plater.qml` placeholder is gone; `AssemblePage.qml` hosts the third `CanvasAssembleView` canvas; `BackendContext`/`EditorViewModel` carry the `activeCanvasType` routing; the canonical verifier exited 0.
- Plan Phase 91 (Explosion Ratio And Assembly Rendering) with `/gsd-plan-phase 91` — it adds the explosion-ratio slider and per-volume separation rendering on top of the `CanvasAssembleView` render branch in `RhiViewportRenderer.cpp:188,241`.
- Phase 92-93 follow the Phase Routing table in `89-GAP-MATRIX.md`.

## Key Decisions (accumulating)

- v4.2 / Phase 90: AssembleView reuses the shared `EditorViewModel`/`ProjectServiceMock` model and the shared single `UndoRedoManager` stack (no scene/stack duplication), mirroring upstream's single-Plater-three-canvas architecture. A new `CanvasType::CanvasAssembleView = 2` and a new `activeCanvasType` int flow from `BackendContext` to `EditorViewModel` on every view-mode change; the `availableGizmoMask()` AssembleView early-return is the documented seam for Phase 92.
- v4.2 / Phase 90: The RHI render path is restored for AssembleView by widening two View3D guards to `!= CanvasPreview` (Preview strict guards unchanged). Phase 91 specializes per-volume explosion rendering on this branch.
