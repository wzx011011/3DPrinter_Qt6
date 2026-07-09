---
gsd_state_version: 1.0
milestone: v4.2
milestone_name: AssembleView Source-Truth Restoration
status: executing
last_updated: 2026-07-09T20:45:00+08:00
last_activity: 2026-07-09 -- Phase 92 plan 01 complete (Assembly Measurement Gizmo: GizmoAssemblyMeasure enum + activability gate + AssemblyMeasureGeometry helper + overlay rendering + 测量 panel + Ctrl+Y shipped)
progress:
  total_phases: 5
  completed_phases: 3
  total_plans: 5
  completed_plans: 4
  percent: 80
stopped_at: Phase 92 plan 01 complete; Phase 93 ready to plan/execute
---

# Project State

**Milestone:** v4.2 - AssembleView Source-Truth Restoration
**Status:** Executing (Phase 92 Assembly Measurement Gizmo complete)
**Next step:** Plan Phase 93 (AssembleView Verification And Cleanup) with `/gsd-plan-phase 93`.

## Current Position

Phase: 92 (Assembly Measurement Gizmo) — complete
Plan: 01 — complete
Status: GizmoAssemblyMeasure = 19 enum (distinct from GizmoMeasure = 3) + EditorViewModel activability gate (AssembleView + explosion ≈ 1.0 + ≥2 volumes, mirrors GLGizmoAssembly.cpp:53-68) + AssemblyMeasureGeometry C++ helper (AABB-center distance + longest-axis angle) + RhiViewportRenderer overlay (white dashed dimension line + arrowheads + teal value box, gated to CanvasAssembleView + gizmo 19) + AssemblePage Ctrl+Y shortcut + right-side 测量 panel + assemblyMeasureSelectedA/B GLViewport bindings; 3 new test slots. Canonical build zero errors; 5 regression suites pass (Prepare/Preview unaffected). Scope simplification documented (full feature-picking deferred to Phase 93).
Last activity: 2026-07-09 — Phase 92 plan 01 executed (10 code/test commits + 1 verification fix + SUMMARY/VERIFICATION)

## Current Milestone (v4.2)

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 89 | AssembleView Source-Truth Gap Audit | Complete | ASMAUDIT-01, ASMAUDIT-02 |
| 90 | AssembleView Shell And Canvas Host Restoration | Complete | ASMSHELL-01, ASMSHELL-02, ASMROUTE-01 |
| 91 | Explosion Ratio And Assembly Rendering | Complete | ASMEXPLODE-01, ASMEXPLODE-02 |
| 92 | Assembly Measurement Gizmo | Complete | ASMMEASURE-01, ASMMEASURE-02 |
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
| active | AssembleView source-truth restoration | v4.2 (Phase 89-91 complete; Phase 92-93 pending) |
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

- Phase 89 gap audit, Phase 90 shell + canvas host, Phase 91 explosion-ratio + per-volume separation rendering, and Phase 92 Assembly measurement gizmo are complete. Phase 92 ships `GizmoAssemblyMeasure = 19` (distinct from `GizmoMeasure = 3`), an `EditorViewModel` activability gate (AssembleView + explosion ≈ 1.0 + ≥2 volumes) replacing the `availableGizmoMask()` AssembleView early-return, the `AssemblyMeasureGeometry` C++ helper (AABB-center distance + longest-axis angle), the RhiViewportRenderer assembly-measure overlay (white dashed dimension line + arrowheads + teal value box gated to CanvasAssembleView + gizmo 19), and the AssemblePage `Ctrl+Y` shortcut + right-side 测量 panel. The scope simplification (center-to-center + longest-axis angle instead of the full feature-picking engine) is documented in `92-01-SUMMARY.md` / `92-01-VERIFICATION.md`; full feature-picking is deferred to Phase 93. Prepare/Preview regression suites are green.
- Plan Phase 93 (AssembleView Verification And Cleanup) with `/gsd-plan-phase 93` — it covers ASMROUTE-02 (AssembleViewDataPool / selection routing cleanup), ASMVERIFY-01, ASMVERIFY-02 (the deferred full feature-picking engine prerequisites: per-volume indexed_triangle_set + scene raycaster).
- Phase 93 follows the Phase Routing table in `89-GAP-MATRIX.md`.

## Key Decisions (accumulating)

- v4.2 / Phase 90: AssembleView reuses the shared `EditorViewModel`/`ProjectServiceMock` model and the shared single `UndoRedoManager` stack (no scene/stack duplication), mirroring upstream's single-Plater-three-canvas architecture. A new `CanvasType::CanvasAssembleView = 2` and a new `activeCanvasType` int flow from `BackendContext` to `EditorViewModel` on every view-mode change; the `availableGizmoMask()` AssembleView early-return is the documented seam for Phase 92.
- v4.2 / Phase 90: The RHI render path is restored for AssembleView by widening two View3D guards to `!= CanvasPreview` (Preview strict guards unchanged). Phase 91 specializes per-volume explosion rendering on this branch.
- v4.2 / Phase 91: Per-volume separation is delivered by restructuring `ProjectServiceMock::meshData` to emit one `ObjBatch` per (object, volume) pair (parent `objectIndex`/`renderObjectId` repeated across sibling volume batches). The parallel-array contract (`meshBatchSourceObjectIndices.size() == objectCount`, `PrepareSceneData.cpp:143`) and unioned-bounds highlight/picking key on `batch.sourceObjectIndex` so Prepare/Preview are unaffected. The renderer applies the offset `(batchCenter - objectCenter) * (ratio - 1.0)` only on the `CanvasAssembleView` branch. Verification used a focused vcvars runner (`scripts/run_unit_tests_vcvars.ps1`) that reuses the canonical vcvars+Windows-Kits setup verbatim but skips the ~8-min libslic3r reconfigure — documented as a verification-method deviation (not a build deviation) in `91-01-VERIFICATION.md`.
- v4.2 / Phase 92: The Assembly measurement gizmo gets its own enum value `GizmoAssemblyMeasure = 19` (distinct from `GizmoMeasure = 3` = Prepare Ctrl+U), mirroring upstream `GLGizmoAssembly` being a separate class. The measurement is a documented simplification: AABB-center-to-center distance + longest-AABB-axis angle (instead of upstream's full feature-picking engine with ITS + raycaster + plane extraction) — enough for the screenshot's distance/angle values, with the full engine deferred to Phase 93 (needs per-volume `indexed_triangle_set` + scene raycaster + `AssembleViewDataPool`). The bounds source is the cached mesh blob parsed in the viewmodel (`selectedVolumeBoundsForAssemblyMeasure()`), avoiding a new `ProjectServiceMock` surface. Arrowheads use `m_fillPipeline` (raw world-space triangles) rather than `m_gizmoTriPipeline` (which applies a gizmoCenter+scale vertex-shader displacement that would offset them).
