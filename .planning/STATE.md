---
gsd_state_version: 1.0
milestone: v3.6
milestone_name: Screenshot-Driven OrcaSlicer UI Restoration
status: completed
last_updated: "2026-07-03T06:14:04.824Z"
last_activity: 2026-07-03 — Milestone v3.6 completed and archived
progress:
  total_phases: 9
  completed_phases: 8
  total_plans: 25
  completed_plans: 23
  percent: 89
---

# Project State

**Milestone:** v3.6 - Screenshot-Driven OrcaSlicer UI Restoration
**Status:** v3.6 milestone complete
**Next step:** User runs 58-UAT.md against the 4 screenshots, then the v3.6 milestone is complete.

## Current Position

Phase: Milestone v3.6 complete
Plan: —
Status: Awaiting next milestone
Last activity: 2026-07-03 — Milestone v3.6 completed and archived

## Project Reference

See: `.planning/PROJECT.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth; screenshot-driven UI milestones also use screenshots as the visual/layout source of truth.
**Current focus:** Milestone complete

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
| 50 | Screenshot and Source-Truth Inventory | Complete |
| 51 | Shell and Navigation Restoration | Complete |
| 52 | Prepare Sidebar and Preset Controls | Complete |
| 53 | Prepare Object, Plate, and Viewport Workflow | Complete |
| 54 | Preview Layout, Sliders, and Right Panels | Complete |
| 55 | G-code Preview Semantics and Rendering Stability | Complete |
| 56 | Parameter Settings Dialogs Restoration | Complete |
| 57 | Deprecated UI Removal and Architecture Cleanup | Complete (2/2) |
| 58 | End-to-End Visual and Functional Verification | Automated floor complete (2/2 plans); VERIFY-04 manual UAT human_needed |

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
| carry-forward | v3.6 Phase 58 manual visual UAT (VERIFY-04) — checklist at `.planning/phases/58-end-to-end-visual-and-functional-verification/58-UAT.md` (34 region items + 9 workflow items against the 4 screenshots). Automated verification passed; user deferred the visual UAT. | Before claiming v3.6 visually complete / any release |
| superseded | v3.5 Phase 47-49 | Do not resume unless explicitly reopened |
| future | Device send/upload/cloud print and Monitor job lifecycle | After local workflow restoration |
| future | AssembleView | Future source-truth milestone |
| future | Auto filament-map recommendation | Future source-truth milestone |
| future | Wipe-tower geometry and rendering | Future source-truth milestone |
| future | THUMB-03 real GL/QRhi-capture thumbnails and 3MF pixel round-trip | Future renderer/project milestone |
| future | FIXTURE-02 full PLATE-09 save/reload assertions after writer integration fix | Future fixture milestone |
| future | Missing CLI test fixtures (`hotend.stl`, `Block20XY.stl`) | Future fixture milestone |
| opportunistic | D3D12 crash root cause | Dedicated backend investigation milestone |

## Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 260702-1bn | 准备页收敛路线图审计降级：基线表+假绿清单+修复优先级 | 2026-07-02 | 260702-1bn | [260702-1bn-prepare-roadmap-audit-downgrade](./quick/260702-1bn-prepare-roadmap-audit-downgrade/) |
| 260702-1q7 | P0修复：CollapsibleSection吞点击+耗材编辑按钮+在参数表中编辑隐藏 | 2026-07-02 | 260702-1q7 | [260702-1q7-p0-collapsiblesection](./quick/260702-1q7-p0-collapsiblesection/) |

## Handoff

Phase 58 automated verification floor is complete (both 58-01 and 58-02
plans committed). The only remaining gate is the manual UAT:

```text
Run build/OWzxSlicer.exe and complete
.planning/phases/58-end-to-end-visual-and-functional-verification/58-UAT.md
```

Once the user records PASS/FAIL + outstanding gaps in 58-UAT.md, the v3.6
milestone (VERIFY-01..05) is fully closed.

## Operator Next Steps

- Start the next milestone with /gsd-new-milestone
