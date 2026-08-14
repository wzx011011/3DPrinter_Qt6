---
gsd_state_version: 1.0
milestone: v5.15
milestone_name: Bed Texture & Model Lighting (Upstream Render Parity)
status: executing
stopped_at: Phase 229 (v5.15 bed texture + model lighting) complete — all suites green; tagged v5.15
last_updated: 2026-08-14
last_activity: 2026-08-14 -- Phase 229 render-parity batch (bed texture pipeline, gouraud model lighting, preview bed)
progress:
  total_phases: 1
  completed_phases: 0
  total_plans: 1
  completed_plans: 0
  percent: 80
---

# Project State

## Latest Completed Milestone

v5.13 AdvancedCut connector-pin system closed 2026-08-13 (tag v5.13 on
origin, HEAD 590c4fb, 372 tests green). v5.8-v5.13 milestone entries were
back-filled into MILESTONES.md during v5.14.

## Project Reference

See:

- `.planning/PROJECT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/research/v5.11-PROCESS-SETTINGS-SUMMARY.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** v5.15 render parity complete (2026-08-14): bed texture
image + gouraud model lighting aligned with upstream. Next: resume the
deferred v5.11 Phase 227 process-projection workstream (planning artifact at
`.planning/phases/227-process-projection-disclosure-and-filtering/`).

## Current Position

Phase: 229 (v5.15 render parity) — complete
Plan: Complete (single batch)
Status: Tagged v5.15
Last activity: 2026-08-14

Progress: [########--] 80%

## Performance Metrics

**v5.11:**

- Total plans completed: 1 of 7
- Completed phases: 1 of 5
- Average duration: Not available

## Accumulated Context

### Decisions

- v5.11 scope is FFF Process (`print`) SettingsDialog only.
- Process order and option membership follow OrcaSlicer `TabPrint::build()`.
- Group identity is an untranslated, page-qualified technical key.
- Collapse state is C++ presentation-model session state; it is not stored in
  presets, projects, or QSettings, and QML remains presentation and wiring.

- Existing `ConfigViewModel`, preset, edit, reset, validation, and
  slice-invalidation routes remain authoritative. `GroupNavSidebar` stays retired.

### Pending Todos

None.

### Blockers/Concerns

- Any dependency or availability behavior requires an exact upstream mapping and
  behavior test before it can be included; v5.11 does not add auto-correction.

## Deferred And Excluded Scope

- Printer and Material hierarchy expansion; H2C/A2L, multi-nozzle,
  per-extruder, and full vector-value editing.

- Device, hardware, network, cloud, camera, monitor, SLA settings or slicing,
  libslic3r changes, and unmapped cross-option auto-correction.

## Session Continuity

Last session: 2026-08-03T08:17:58.910Z
Stopped at: Phase 226 UI-SPEC approved
Resume file: .planning/phases/226-source-mapped-process-hierarchy/226-UI-SPEC.md
