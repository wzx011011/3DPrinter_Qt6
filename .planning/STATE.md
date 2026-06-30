---
gsd_state_version: 1.0
milestone: v3.5
milestone_name: Preset Authoring Complete Workflow
status: ready_to_plan
last_updated: 2026-06-30T09:09:33.635Z
last_activity: 2026-06-30 -- Phase 44 complete; ready to plan Phase 45
progress:
  total_phases: 6
  completed_phases: 1
  total_plans: 1
  completed_plans: 1
  percent: 17
stopped_at: Phase 44 complete (1/1) — ready to discuss Phase 45
---

# Project State

**Milestone:** v3.5 - Preset Authoring Complete Workflow
**Status:** Ready to plan
**Next step:** Plan Phase 45, `Compatibility and Selection State`.

## Current Position

Phase: 45
Plan: Not started
Status: Ready to discuss and plan
Last activity: 2026-06-30

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.
**Current focus:** Phase 45 — compatibility and selection state

## Latest Verified Shipped Milestone

**v3.2 Multi-Plate Data Polish** - audited 2026-06-28, status `tech_debt`.

- Phases 29-32 complete.
- Requirements: 8/10 complete; `THUMB-02` and `FIXTURE-02` partial because the shared 3MF writer integration path is deferred.
- Details: `.planning/v3.2-MILESTONE-AUDIT.md` and `.planning/milestones/v3.2-MILESTONE-AUDIT.md`.

## Carry-Forward Status

**v3.4 Import to G-code Complete Workflow** reached Phase 43 with automated verification passing, but manual UAT is still pending because the user cannot verify it right now.

Do not mark v3.4 as fully complete until this checklist is actually run:

```text
Run build/OWzxSlicer.exe and complete .planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md
```

This does not block v3.5 planning, but it remains a release/handoff fact.

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 44 | Preset Bundle Service Foundation | Complete |
| 45 | Compatibility and Selection State | Pending |
| 46 | Config Editing, Dirty State, and Reset Semantics | Pending |
| 47 | Preset Lifecycle Actions | Pending |
| 48 | Create Presets and Bundle Workflows | Pending |
| 49 | Slice Integration, Verification, and Handoff | Pending |

## v3.5 Scope

This milestone completes the preset authoring workflow:

```text
Load preset bundle -> select compatible printer/filament/process presets -> edit config -> save/create/import/export presets -> slice/export with the edited config
```

It intentionally does not include device send/upload/cloud printing, Monitor print-job lifecycle, AssembleView, auto filament-map recommendation, wipe-tower rendering, or renderer backend promotion.

## Deferred Items

| Category | Item | Target |
|---|---|---|
| carry-forward | v3.4 Phase 43 manual UAT | Before claiming v3.4 complete |
| future | Device send/upload/cloud print and Monitor job lifecycle | v3.6+ |
| future | AssembleView | v3.6+ |
| future | Auto filament-map recommendation | v3.6+ |
| future | Wipe-tower geometry + rendering | v3.6+ |
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | v3.6+ |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | v3.6+ |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| opportunistic | D3D12 crash root cause | Dedicated backend investigation milestone |

## Handoff

Continue with Phase 45:

```text
$gsd-autonomous --from 45
```
