---
gsd_state_version: 1.0
milestone: v4.2
milestone_name: AssembleView Source-Truth Restoration
status: milestone_complete
last_updated: 2026-07-09T21:50:00+08:00
last_activity: 2026-07-09 -- v4.2 milestone completed, archived, and tagged
progress:
  total_phases: 5
  completed_phases: 5
  total_plans: 5
  completed_plans: 5
  percent: 100
stopped_at: v4.2 milestone complete; ready for next milestone planning
---

# Project State

**Milestone:** v4.2 - AssembleView Source-Truth Restoration (SHIPPED 2026-07-09)
**Status:** Milestone complete
**Next step:** Run `/gsd-new-milestone` to plan the next target.

## Current Position

Phase: (none active — milestone closed)
Plan: (none active)
Status: Between milestones
Last activity: 2026-07-09

## Last Completed Milestone: v4.2

| Phase | Name | Status | Requirements |
|---|---|---|---|
| 89 | AssembleView Source-Truth Gap Audit | Complete | ASMAUDIT-01, ASMAUDIT-02 |
| 90 | AssembleView Shell And Canvas Host Restoration | Complete | ASMSHELL-01, ASMSHELL-02, ASMROUTE-01 |
| 91 | Explosion Ratio And Assembly Rendering | Complete | ASMEXPLODE-01, ASMEXPLODE-02 |
| 92 | Assembly Measurement Gizmo | Complete | ASMMEASURE-01, ASMMEASURE-02 |
| 93 | AssembleView Verification And Cleanup | Complete | ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02 |

## Project Reference

See: `.planning/PROJECT.md` (updated 2026-07-09)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** Planning next milestone — candidate backlog is in `ROADMAP.md` Deferred Backlog.

## Verification Result

- Milestone audit passed: 12/12 requirements, 5/5 phases, 5/5 integration chains, 3/3 E2E flows.
- Canonical build: production code compiles/links clean (zero errors); OWzxSlicer.exe 33.7 MB.
- Regression ctest: all 5 suites pass (PrepareSceneDataTests, ViewModelSmokeTests 94, QmlUiAuditTests 66, PartPlateTests, PreviewParserTests). Prepare/Preview regression-free.
- Runtime: OWzxSlicer.exe launched, AssembleView reachable. Visual evidence capture-blocked (Windows capture API; SETVERIFY-02 precedent).

## Carry-Forward Status

| Category | Item | Target |
|---|---|---|
| closed | AssembleView source-truth restoration | Shipped in v4.2 |
| closed | v4.1 Parameter settings dialogs | Shipped in v4.1 |
| removed | LAN/device/cloud/network/Monitor workflows | Removed from future scope by user direction on 2026-07-07 |
| future | Auto filament-map recommendation + wipe-tower geometry/rendering | Future milestone |
| future | Real GL/QRhi-capture thumbnails + 3MF pixel round-trip | Future milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| future | Deterministic argv-based GUI fixture loading for screenshots | Partially addressed by startup deep links |
| future | D3D12 root cause | Dedicated backend investigation milestone |
| future | Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper | Future milestone (needs per-volume ITS) |

## Deferred Items

Items acknowledged and deferred at milestone close on 2026-07-09:

| Category | Item | Status |
|---|---|---|
| process | Nyquist VALIDATION.md files | Phases 89-93 have deterministic verification artifacts but no separate Nyquist validation files (carried from v4.1) |
| evidence | Runtime visual evidence | Windows capture API blocked; reachability via process-liveness + canonical verifier + regression ctest (SETVERIFY-02 precedent) |
| build | Canonical build libslic3r reconfigure | Per-invocation ~8 min reconfigure timed out executor wrapper in code phases; production code clean, regression via focused runner |

## Scope Guard

- Do not promote LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

## Operator Next Steps

- Run `/gsd-new-milestone` to select the next target.
- Candidate backlog: auto filament-map + wipe-tower, real thumbnails + 3MF round-trip, missing CLI fixtures, D3D12 root cause, full GLGizmoMeasure engine + clipper.
