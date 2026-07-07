---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: Preview Page UI Restoration
status: ready_to_plan
last_updated: 2026-07-07T11:47:26.8989764+08:00
last_activity: 2026-07-07
progress:
  total_phases: 5
  completed_phases: 3
  total_plans: 3
  completed_plans: 3
  percent: 60
stopped_at: Phase 81 complete (1/1) - ready to discuss Phase 82
---

# Project State

**Milestone:** v4.0 - Preview Page UI Restoration
**Status:** Ready to plan
**Next step:** Start Phase 82 with `$gsd-discuss-phase 82 --auto`.

## Current Position

Phase: 82
Plan: Not started
Status: Ready to plan
Last activity: 2026-07-07

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** Phase 82 - Preview G-code roles, color modes, and rendering

## Last Completed Milestone

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 74 | Prepare Source-Truth Gap Audit | Complete | AUDIT-01 |
| 75 | Prepare Sidebar Restoration | Complete | SIDE-01, SIDE-02, SIDE-03 |
| 76 | Prepare Workflow Panels Restoration | Complete | OBJ-01, PLATEUI-01, STATUS-01 |
| 77 | Prepare Viewport Controls And Gizmo UI | Complete | VIEWUI-01, GIZMOUI-01 |
| 78 | Prepare Verification And Cleanup | Complete | CLEAN-01, VERIFY-01, VERIFY-02 |

## Active Milestone Plan

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 79 | Preview Source-Truth Gap Audit | Complete | PVAUDIT-01 |
| 80 | Preview Layout And Panels Restoration | Complete | PVLAYOUT-01, PVLAYOUT-02, PVLAYOUT-03 |
| 81 | Preview Layer Move And Playback Controls | Complete | PVCTRL-01, PVCTRL-02, PVCTRL-03 |
| 82 | Preview G-code Roles Color Modes And Rendering | Not started | PVRENDER-01, PVRENDER-02, PVRENDER-03 |
| 83 | Preview Verification And Cleanup | Not started | PVCLEAN-01, PVVERIFY-01, PVVERIFY-02 |

## Verification Rule

- Do not run alternate build scripts or create alternate build directories.
- During Phases 79-82, prefer source reads, focused tests, QML/source audits, `git diff --check`, and the encoding guard unless a code path requires broader verification.
- In Phase 83, run the canonical verifier:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

- After the canonical build, launch `build/OWzxSlicer.exe` and record Preview page visual evidence against the target screenshot.

## Source And Visual Truth

- Visual truth: `shotScreen/预览页.png`.
- Behavior truth: `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`, `GCodeViewer.*`, `GLCanvas3D.*`, and `third_party/OrcaSlicer/src/libslic3r/GCode/*`.
- Qt targets start from `src/qml_gui/pages/PreviewPage.qml`, Preview panels/components, `PreviewViewModel.*`, `EditorViewModel.*`, `SliceService.*`, and RHI renderer classes.

## Target Screenshot

- `shotScreen/预览页.png` - Preview target screenshot

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v3.4 Phase 43 UAT | Closed by canonical E2E coverage plus current runtime launch evidence |
| superseded | v3.5 Phase 47-49 | Do not resume unless explicitly reopened |
| closed | v3.7 residual Prepare visual gaps | Reconciled in v3.9 |
| future | Device send/upload/cloud print and Monitor job lifecycle | Future milestone |
| future | AssembleView | Future source-truth milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| future | D3D12 root cause | Dedicated backend investigation milestone |

## Deferred Items

Historical items acknowledged before v3.9 and their current state:

| Category | Item | Status |
|---|---|---|
| debug | qrhi-d3d12-crash | resolved for default path; D3D12 remains future opt-in investigation |
| uat_gap | Phase 43 43-UAT.md | closed by E2E/runtime evidence |
| verification_gap | Phase 68 visual evidence | carried as optional v3.8 visual-evidence debt |

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|---|---|---|---|
| 260705-e8x | Fix RHI gizmo depth overlay and viewport drag review findings | 2026-07-05 | 1ba32eb | [260705-e8x-fix-rhi-gizmo-depth-overlay-and-viewport](./quick/260705-e8x-fix-rhi-gizmo-depth-overlay-and-viewport/) |
| 260705-vkn | Pixel restore Prepare page left sidebar against target screenshot | 2026-07-06 | eab371a | [260705-vkn-pixel-restore-prepare-page-left-sidebar](./quick/260705-vkn-pixel-restore-prepare-page-left-sidebar/) |
| 260706-r8m | Restore remaining Prepare page visual surfaces against target screenshot | 2026-07-06 | b62e224 | [260706-r8m-prepare-page-full-visual-parity](./quick/260706-r8m-prepare-page-full-visual-parity/) |
| 260706-uix | Make restored Prepare controls actionable and honestly gated | 2026-07-06 | f950545 | [260706-uix-prepare-actionable-controls](./quick/260706-uix-prepare-actionable-controls/) |

## Operator Next Steps

- Start Phase 82 with /gsd-discuss-phase 82 --auto
