---
gsd_state_version: 1.0
milestone: v3.4
milestone_name: Import to G-code Complete Workflow
status: Awaiting Phase 43 manual UAT
last_updated: "2026-06-29T02:40:56.438Z"
last_activity: 2026-06-29 -- Phase 43 automated verification passed; manual UAT pending
progress:
  total_phases: 7
  completed_phases: 6
  total_plans: 7
  completed_plans: 6
  percent: 86
---

# Project State

**Milestone:** v3.4 - Import to G-code Complete Workflow
**Status:** Awaiting Phase 43 manual UAT
**Next step:** Run build/OWzxSlicer.exe and complete Phase 43 manual UAT checklist.

## Current Position

Phase: 43 (End-to-End Verification and Handoff) - AWAITING MANUAL UAT
Plan: 1 of 1
Status: Awaiting Phase 43 manual UAT
Last activity: 2026-06-29 -- Phase 43 automated verification passed; manual UAT pending

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Phase 43 鈥?End-to-End Verification and Handoff

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
| 40 | Complete Preview Data and Upstream View Semantics | Complete |
| 41 | D3D11 Preview Rendering and Interaction Stability | Complete |
| 42 | Local G-code Export and Finalization | Complete |
| 43 | End-to-End Verification and Handoff | Awaiting manual UAT |

## v3.4 Scope

This milestone completes the local main workflow:

```text
Import model/project -> Prepare readiness -> Slice/reslice -> Preview -> Export local G-code
```

It intentionally does not include device send/upload/cloud printing or Monitor print-job lifecycle. Those workflows depend on a trustworthy local G-code result and remain future scope.

## Deferred Items

| Category | Item | Target |
|---|---|---|
| future | Device send/upload/cloud print and Monitor job lifecycle | v3.5+ |
| future | Full preset authoring and CreatePresetsDialog workflows | v3.5+ |
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | v3.5+ |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | v3.5+ |
| future | AssembleView | v3.5+ |
| future | Auto filament-map recommendation | v3.5+ |
| future | Wipe-tower geometry + rendering | v3.5+ |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | TBD |
| opportunistic | D3D12 crash root cause | TBD |

## Handoff

Phase 43 automated verification passed. Complete manual UAT:

```text
Run build/OWzxSlicer.exe and complete .planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md
```
