---
gsd_state_version: 1.0
milestone: v5.11
milestone_name: Process Settings Source-Truth Convergence
status: ready_to_plan
stopped_at: Phase 226 complete (1/1) — ready to discuss Phase 227
last_updated: 2026-08-13T07:09:51.380Z
last_activity: 2026-08-11 -- Phase 226 execution started
progress:
  total_phases: 5
  completed_phases: 0
  total_plans: 1
  completed_plans: 58
  percent: 0
---

# Project State

## Latest Completed Milestone

v5.10 Polish & Source-Truth Consolidation closed on 2026-07-26 at Phase 225.
Its completion is recorded here as the current baseline; this planning pass does
not invent or reconstruct v5.10 audit artifacts.

Relevant retained baseline: Hollow marker refresh and cylinder rendering,
configured `filament_colour` flow, MMU colour-source unification, and
ConfigWizard printer-material filtering were verified. SLA slicing remains
permanently out of scope; multi-nozzle UI and printer-hardware/network workflows
remain deferred or excluded.

## Project Reference

See:

- `.planning/PROJECT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/research/v5.11-PROCESS-SETTINGS-SUMMARY.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
**Current focus:** Phase 227 — process projection, disclosure, and filtering

## Current Position

Phase: 227
Plan: Not started
Status: Ready to plan
Last activity: 2026-08-13

Progress: [----------] 0%

## Performance Metrics

**v5.11:**

- Total plans completed: 1 of 7
- Completed phases: 0 of 5
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
