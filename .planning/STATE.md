---
gsd_state_version: 1.0
milestone: v3.4
milestone_name: Import to G-code Complete Workflow
status: executing
last_updated: 2026-06-29T03:41:40+08:00
last_activity: 2026-06-29 -- Phase 40 planning complete
progress:
  total_phases: 7
  completed_phases: 3
  total_plans: 4
  completed_plans: 3
  percent: 43
---

# Project State

**Milestone:** v3.4 - Import to G-code Complete Workflow
**Status:** Ready to execute
**Next step:** execute Phase 40 to complete Preview data and upstream view semantics.

## Current Position

Phase: 40
Plan: Not started
Status: Ready to execute
Last activity: 2026-06-29 -- Phase 40 planning complete

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Phase 40 -- complete preview data and upstream view semantics

## Latest Shipped Milestone

**v3.2 Multi-Plate Data Polish** - audited 2026-06-28, status `tech_debt`.

- Phases 29-32 complete.
- Requirements: 8/10 complete; `THUMB-02` and `FIXTURE-02` partial because the shared 3MF writer integration path is deferred.
- Details: `.planning/v3.2-MILESTONE-AUDIT.md` and `.planning/milestones/v3.2-MILESTONE-AUDIT.md`.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 37 | Complete Import and Project Restore | Complete |
| 38 | Prepare Readiness and Slice Invalidation | Complete |
| 39 | Complete Slicing and Reslicing State Machine | Complete |
| 40 | Complete Preview Data and Upstream View Semantics | Pending |
| 41 | D3D11 Preview Rendering and Interaction Stability | Pending |
| 42 | Local G-code Export and Finalization | Pending |
| 43 | End-to-End Verification and Handoff | Pending |

## v3.4 Scope

This milestone completes the local main workflow:

```text
Import model/project -> Prepare readiness -> Slice/reslice -> Preview -> Export local G-code
```

It intentionally does not include device send/upload/cloud printing or Monitor print-job lifecycle. Those workflows depend on a trustworthy local G-code result and remain future scope.

## Deferred Items

| Category | Item | Target |
|---|---|---|
| future | Device send/upload/cloud print and Monitor print-job lifecycle | v3.5+ |
| future | Full preset authoring and CreatePresetsDialog workflows | v3.5+ |
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | v3.5+ |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | v3.5+ |
| future | AssembleView | v3.5+ |
| future | Auto filament-map recommendation | v3.5+ |
| future | Wipe-tower geometry + rendering | v3.5+ |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | TBD |
| opportunistic | D3D12 crash root cause | TBD |
| polish | Preview marker at exact final move position | v3.4 |

## Handoff

Phase 40 is planned. Execute the Phase 40 plan:

```text
$gsd-execute-phase 40
```
