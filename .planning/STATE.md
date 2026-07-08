---
gsd_state_version: 1.0
milestone: v4.1
milestone_name: Parameter Settings Dialogs Source-Truth Restoration
status: milestone_complete
last_updated: 2026-07-09T01:10:00+08:00
last_activity: 2026-07-09 -- v4.1 milestone completed, archived, and tagged
progress:
  total_phases: 5
  completed_phases: 5
  total_plans: 5
  completed_plans: 5
  percent: 100
stopped_at: v4.1 milestone complete; ready for next milestone planning
---

# Project State

**Milestone:** v4.1 - Parameter Settings Dialogs Source-Truth Restoration (SHIPPED 2026-07-09)
**Status:** Milestone complete
**Next step:** Run `/gsd-new-milestone` to plan the next source-truth restoration target.

## Current Position

Phase: (none active — milestone closed)
Plan: (none active)
Status: Between milestones
Last activity: 2026-07-09

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
**Current focus:** Planning next milestone — candidate backlog is in `ROADMAP.md` Deferred Backlog.

## Verification Result

- Last completed canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- App launched on the D3D11 QRhi path.
- Final settings runtime screenshots:
  `.planning/phases/88-settings-verification-and-cleanup/visual-evidence/runtime-app-main.png`
  `.planning/phases/88-settings-verification-and-cleanup/visual-evidence/runtime-settings-prepare-panel.png`
- Pre-close fix commit `a218972` restored Prepare topbar/settings-entry/viewport layout with regression-guarding QmlUiAuditTests; 5/5 relevant ctest targets passed.

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | Parameter settings dialog source-truth restoration | Shipped in v4.1 |
| closed | v4.0 Preview screenshot restoration | Closed by Phase 83 audit, canonical verifier, and runtime Preview evidence |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | AssembleView | Future source-truth milestone |
| future | Auto filament-map recommendation + wipe-tower geometry/rendering | Future milestone |
| future | Real GL/QRhi-capture thumbnails + 3MF pixel round-trip | Future milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| future | Deterministic argv-based GUI fixture loading for screenshots | Partially addressed by v4.1 startup deep links; future milestone for fixtures |
| future | D3D12 root cause | Dedicated backend investigation milestone |

## Deferred Items

Items acknowledged and deferred at milestone close on 2026-07-09:

| Category | Item | Status |
|---|---|---|
| debug | qrhi-d3d12-crash | D3D11 default path remains verified; D3D12 remains future opt-in investigation |
| evidence | Loaded-G-code runtime screenshot | Default no-G-code Preview screenshot captured; deterministic loaded fixture screenshots now possible via `--load-model` startup hook but fixtures remain future |
| process | per-phase VALIDATION.md | Phase 84-88 have deterministic verification artifacts (Nyquist compliant per audit) but no separate Nyquist validation files |
| evidence | Direct SettingsDialog window capture | Blocked by Windows capture API; SETVERIFY-02 accepts manual click-through plus runtime evidence plus canonical verifier |

## Scope Guard

- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.
- Do not resume v3.5 Phase 47-49 unless explicitly reopened.

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260708-e60 | Add extensible GUI startup deep-link arguments for pages and dialogs | 2026-07-08 | fc97507 | [260708-e60-add-extensible-gui-startup-deep-link-arg](./quick/260708-e60-add-extensible-gui-startup-deep-link-arg/) |
| 260707-lan | Remove LAN/device/network/cloud work from future scope | 2026-07-07 | docs-only | [260707-lan-network-scope-removal](./quick/260707-lan-network-scope-removal/) |

## Operator Next Steps

- Run `/gsd-new-milestone` to select the next source-truth restoration target.
- Candidate backlog: AssembleView, auto filament-map + wipe-tower, real thumbnails + 3MF round-trip, missing CLI fixtures, D3D12 root cause.
