---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: Preview Page UI Restoration
status: milestone_complete
last_updated: 2026-07-07T14:35:44+08:00
last_activity: 2026-07-07
progress:
  total_phases: 5
  completed_phases: 5
  total_plans: 5
  completed_plans: 5
  percent: 100
stopped_at: v4.0 complete - Preview Page UI Restoration archived
---

# Project State

**Milestone:** v4.0 - Preview Page UI Restoration
**Status:** Milestone complete
**Next step:** Plan the next local/offline source-truth milestone.

## Current Position

Phase: 83
Plan: Complete
Status: Complete
Last activity: 2026-07-07

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

- Canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- App launched on the D3D11 QRhi path.
- Final Preview runtime screenshot:
  `.planning/milestones/v4.0-phases/83-preview-verification-and-cleanup/visual-evidence/runtime-preview-page-button-invoked.png`

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | v4.0 Preview screenshot restoration | Closed by Phase 83 audit, canonical verifier, and runtime Preview evidence |
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

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260707-lan | Remove LAN/device/network/cloud work from future scope | 2026-07-07 | docs-only | [260707-lan-network-scope-removal](./quick/260707-lan-network-scope-removal/) |

## Operator Next Steps

- Start the next local/offline milestone with `/gsd-new-milestone` or a source-truth migration command.
- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.
