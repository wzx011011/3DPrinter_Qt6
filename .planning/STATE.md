---
gsd_state_version: 1.0
milestone: v4.1
milestone_name: Parameter Settings Dialogs Source-Truth Restoration
status: ready_to_plan
last_updated: 2026-07-07T10:50:45.113Z
last_activity: 2026-07-07 -- Phase 86 complete; ready to discuss Phase 87
progress:
  total_phases: 5
  completed_phases: 3
  total_plans: 5
  completed_plans: 3
  percent: 60
stopped_at: Phase 86 complete (1/1); ready to discuss Phase 87
---

# Project State

**Milestone:** v4.1 - Parameter Settings Dialogs Source-Truth Restoration
**Status:** Ready to plan
**Next step:** Start Phase 87 settings preset semantics and workflow stability.

## Current Position

Phase: 87
Plan: Not started
Status: Ready to execute
Last activity: 2026-07-07

## Current Milestone

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 84 | Settings Source-Truth Gap Audit | Complete | SETAUDIT-01, SETAUDIT-02 |
| 85 | Settings Shell And Tab Layout Restoration | Complete | SETLAYOUT-01, SETLAYOUT-02, SETLAYOUT-03 |
| 86 | Settings Option Sections And Typed Controls | Complete | SETCTRL-01, SETCTRL-02, SETCTRL-03 |
| 87 | Settings Preset Semantics And Workflow Stability | Planned | SETSEM-01, SETSEM-02, SETSEM-03 |
| 88 | Settings Verification And Cleanup | Planned | SETCLEAN-01, SETVERIFY-01, SETVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.

## Last Completed Milestone

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 79 | Preview Source-Truth Gap Audit | Complete | PVAUDIT-01 |
| 80 | Preview Layout And Panels Restoration | Complete | PVLAYOUT-01, PVLAYOUT-02, PVLAYOUT-03 |
| 81 | Preview Layer Move And Playback Controls | Complete | PVCTRL-01, PVCTRL-02, PVCTRL-03 |
| 82 | Preview G-code Roles Color Modes And Rendering | Complete | PVRENDER-01, PVRENDER-02, PVRENDER-03 |
| 83 | Preview Verification And Cleanup | Complete | PVCLEAN-01, PVVERIFY-01, PVVERIFY-02 |

## Verification Result

- Last completed canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

- App launched on the D3D11 QRhi path.
- Final Preview runtime screenshot:
  `.planning/milestones/v4.0-phases/83-preview-verification-and-cleanup/visual-evidence/runtime-preview-page-button-invoked.png`

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.0 Preview screenshot restoration | Closed by Phase 83 audit, canonical verifier, and runtime Preview evidence |
| active | Parameter settings dialog source-truth restoration | v4.1 |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | AssembleView | Future source-truth milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| future | Deterministic argv-based GUI fixture loading for screenshots | Future testability milestone |
| future | D3D12 root cause | Dedicated backend investigation milestone |

## Deferred Items

| Category | Item | Status |
|---|---|---|
| debug | qrhi-d3d12-crash | D3D11 default path remains verified; D3D12 remains future opt-in investigation |
| evidence | Loaded-G-code runtime screenshot | Default no-G-code Preview screenshot captured; deterministic loaded fixture screenshots require app argv/file-loading support |
| process | per-phase VALIDATION.md | Phase 79-83 have deterministic verification artifacts but no separate Nyquist validation files |

## Scope Guard

- v4.1 is local/offline settings UI work only.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260707-lan | Remove LAN/device/network/cloud work from future scope | 2026-07-07 | docs-only | [260707-lan-network-scope-removal](./quick/260707-lan-network-scope-removal/) |

## Operator Next Steps

- Start Phase 86 with `$gsd-discuss-phase 86 --auto` or `$gsd-plan-phase 86`.
