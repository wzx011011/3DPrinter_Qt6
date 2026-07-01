---
gsd_state_version: 1.0
milestone: v3.6
milestone_name: Screenshot-Driven OrcaSlicer UI Restoration
status: ready_to_plan
last_updated: 2026-07-01T03:44:50.603Z
last_activity: 2026-07-01 -- Phase 51 execution started
progress:
  total_phases: 9
  completed_phases: 1
  total_plans: 5
  completed_plans: 62
  percent: 11
stopped_at: Phase 51 complete (3/3) — ready to discuss Phase 52
---

# Project State

**Milestone:** v3.6 - Screenshot-Driven OrcaSlicer UI Restoration
**Status:** Ready to plan
**Next step:** Start Phase 50, `Screenshot and Source-Truth Inventory`.

## Current Position

Phase: 52
Plan: Not started
Status: Executing Phase 51
Last activity: 2026-07-01

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; screenshot-driven UI milestones also use screenshots as the visual/layout source of truth.
**Current focus:** Phase 52 — prepare sidebar and preset controls

## Superseded Milestone Status

**v3.5 Preset Authoring Complete Workflow** is no longer the active milestone.

- Phase 44: Preset Bundle Service Foundation - complete historical evidence.
- Phase 45: Compatibility and Selection State - complete historical evidence.
- Phase 46: Config Editing, Dirty State, and Reset Semantics - complete historical evidence.
- Phase 47: Preset Lifecycle Actions - superseded by v3.6 Settings and full UI restoration.
- Phase 48: Create Presets and Bundle Workflows - superseded by v3.6 Settings and full UI restoration.
- Phase 49: Slice Integration, Verification, and Handoff - superseded by v3.6 end-to-end restoration.

Do not resume v3.5 Phase 47-49 unless the user explicitly reopens that milestone.

## Carry-Forward Status

**v3.4 Import to G-code Complete Workflow** reached Phase 43 with automated verification passing, but manual UAT is still pending because the user could not verify it at the time.

Do not mark v3.4 as fully complete until this checklist is actually run:

```text
Run build/OWzxSlicer.exe and complete .planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md
```

## Active Milestone Plan

| Phase | Name | Status |
|---|---|---|
| 50 | Screenshot and Source-Truth Inventory | Pending |
| 51 | Shell and Navigation Restoration | Pending |
| 52 | Prepare Sidebar and Preset Controls | Pending |
| 53 | Prepare Object, Plate, and Viewport Workflow | Pending |
| 54 | Preview Layout, Sliders, and Right Panels | Pending |
| 55 | G-code Preview Semantics and Rendering Stability | Pending |
| 56 | Parameter Settings Dialogs Restoration | Pending |
| 57 | Deprecated UI Removal and Architecture Cleanup | Pending |
| 58 | End-to-End Visual and Functional Verification | Pending |

## v3.6 Scope

This milestone restores the user-visible Prepare page, Preview page, and parameter settings workflows as complete source-truth-aligned UI flows:

```text
Import model -> select/edit printer/material/process settings -> prepare model/plate -> slice -> inspect G-code preview -> export G-code
```

Screenshots under `shotScreen/` are visual/layout truth. OrcaSlicer upstream source under `third_party/OrcaSlicer` is behavior truth.

If an existing Qt page is materially off-design and expensive to repair, replace it and remove the old files, routes, resource entries, registrations, imports, and tests in the same milestone.

## Deferred Items

| Category | Item | Target |
|---|---|---|
| carry-forward | v3.4 Phase 43 manual UAT | Before claiming v3.4 complete |
| superseded | v3.5 Phase 47-49 | Do not resume unless explicitly reopened |
| future | Device send/upload/cloud print and Monitor job lifecycle | After local workflow restoration |
| future | AssembleView | Future source-truth milestone |
| future | Auto filament-map recommendation | Future source-truth milestone |
| future | Wipe-tower geometry and rendering | Future source-truth milestone |
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | Future renderer/project milestone |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | Future fixture milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| opportunistic | D3D12 crash root cause | Dedicated backend investigation milestone |

## Handoff

Start with Phase 50:

```text
$gsd-plan-phase 50
```

or run the milestone autonomously:

```text
$gsd-autonomous --from 50
```
