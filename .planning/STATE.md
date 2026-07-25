---
gsd_state_version: 1.0
milestone: v5.8
milestone_name: Submodule Baseline Alignment + Hollow Editing
status: completed
last_updated: "2026-07-25T13:30:00.000Z"
last_activity: 2026-07-25
progress:
  total_phases: 4
  completed_phases: 4
  total_plans: 4
  completed_plans: 4
  percent: 100
---

# Project State

**Latest completed milestone:** v5.8 Submodule Baseline Alignment + Hollow
Editing, closed on 2026-07-25.

**Completion evidence:** The v5.7→main merge build break is repaired — the
submodule now points to `bdbc7ec` on branch `owzx-cgal54-on-0632`
(`0632bae8` + 5 OWzx-compat patches), matching origin/main's API surface.
MMU extruder colours/count are config-driven (对齐 upstream
`m_extruders_colors`). The Hollow gizmo edits/persists drain holes end to end
(SLA slicing remains deferred). All verification gates PASS:
`multiPlateFullStateRoundTrip`, `test_slice_produces_gcode_file`,
`mmuSegmentationPaintFeedsSlice`, and the three v5.0 dialog regression locks.
See `.planning/milestones/v5.8-MILESTONE-AUDIT.md`.

**Next step:** select the next milestone from the deferred backlog (H2C/A2L,
ConfigWizard multi-vendor, AMS real backend, or SLA slice path now that
Hollow editing exists).

## Latest Milestone: v5.8 Submodule Baseline Alignment + Hollow Editing

**Result:** Reactive milestone — the v5.7→main merge pinned the submodule to
`4cb3b9ce` while origin/main's ProjectServiceMock.cpp had migrated to
`0632bae8`'s API surface, breaking the build. Phase 212 repaired that
(`0632bae8` + CGAL54/mcut-chrono/TriangleMeshDeal/DRC patches; ProjectServiceMock
split/auto_drop adapted). Phases 213–215 delivered the two feature gaps the
post-merge code-truth audit surfaced: MMU config-driven state and an
interactive Hollow (drain hole) editing gizmo.

| Phase | Name | Status | Requirement |
|---|---|---|---|
| 212 | Submodule 0632bae8 Baseline Alignment | Complete | BASE-01..04 |
| 213 | MMU Config-Driven Colours and Count | Complete | MMU-01..03 |
| 214 | Hollow Drain Hole Data and Logic Layer | Complete | HOLLOW-01..03,06 |
| 215 | Hollow Viewport Routing and Render | Complete | HOLLOW-04,05, VER-01..04 |

## Project Reference

See:

- `.planning/PROJECT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/milestones/v5.8-ROADMAP.md`
- `.planning/milestones/v5.8-REQUIREMENTS.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
