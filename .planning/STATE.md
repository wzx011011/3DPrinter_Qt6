---
gsd_state_version: 1.0
milestone: v3.3
milestone_name: Slice Preview Main Flow MVP
status: planning
last_updated: "2026-06-28T04:47:46.980Z"
last_activity: 2026-06-28 -- v3.3 milestone defined
progress:
  total_phases: 4
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# Project State

**Milestone:** v3.3 - Slice Preview Main Flow MVP
**Status:** Planning
**Next step:** execute Phase 33, the slice-to-preview navigation gate.

## Current Position

Phase: 33 - Not started
Plan: define and execute the first slice-to-preview path
Status: Ready for implementation
Last activity: 2026-06-28 -- Milestone v3.3 started and scoped to the Preview main flow.

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Load model -> slice -> enter Preview -> render non-empty D3D11 QRhi G-code preview.

## Latest Shipped Milestone

**v3.2 Multi-Plate Data Polish** - audited 2026-06-28, status `tech_debt`.

- Phases 29-32 complete.
- Requirements: 8/10 complete; `THUMB-02` and `FIXTURE-02` partial because the shared 3MF writer integration path is deferred.
- Details: `.planning/v3.2-MILESTONE-AUDIT.md` and `.planning/milestones/v3.2-MILESTONE-AUDIT.md`.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 33 | Slice-to-Preview Navigation Gate | Not started |
| 34 | G-code Preview Parser MVP | Not started |
| 35 | D3D11 Preview Rendering Interaction | Not started |
| 36 | Verification and Handoff | Not started |

## Deferred Items

| Category | Item | Target |
|---|---|---|
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | v3.4+ |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | v3.4+ |
| future | AssembleView | v3.4+ |
| future | Auto filament-map recommendation | v3.4+ |
| future | Wipe-tower geometry + rendering | v3.4+ |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | TBD |
| opportunistic | D3D12 crash root cause | TBD |

## Handoff

Start Phase 33 with a narrow TDD loop:

```text
$gsd-plan-phase 33
$gsd-execute-phase 33 --interactive
```
