---
gsd_state_version: 1.0
milestone: v5.0
milestone_name: Advanced Feature Recovery & Tech-Debt Closure
status: in_progress
last_updated: "2026-07-17T04:45:00.000Z"
last_activity: 2026-07-17
progress:
  total_phases: 13
  completed_phases: 3
  total_plans: 3
  completed_plans: 3
  percent: 23
---

# Project State

**Milestone:** v5.0 — Advanced Feature Recovery & Tech-Debt Closure
**Status:** Planning (2026-07-17). ROADMAP.md created (13 phases, 141-153, 32 requirements mapped).
**Next step:** `/gsd:plan-phase 141` (or `/gsd:discuss-phase 141` if discuss is re-enabled). Skip_discuss=true per config — phases go straight to plan-phase.

## Last Shipped Milestone: v4.8 (2026-07-16, tech_debt)

**Audit status:** tech_debt — 7/7 requirements satisfied, no blockers. 5 phases (136-140), 8 plans.

**Carried tech_debt into v5.0 (to be closed by WS1 / WS2):**

- CGAL-02 intersection boolean returns subtraction (union/difference/drill work) — **v5.0 WS1 closes**.
- Orphaned `meshBooleanSelected` menu stub — **v5.0 WS1 removes**.
- `ProjectServiceMock::drillObject` MSVC C4715 — **v5.0 WS1 fixes**.
- Assemble rotate/scale live-visual compose (translate-only render) — **v5.0 WS1 closes**.
- 2-line CGAL submodule compat patch — droppable if CGAL upgraded in DEPS_PREFIX (out of v5.0 scope unless convenient).
- de/fr/ja/ko ~906 messages remaining each — non-code; remains Future.

## v5.0 Scope (5 workstreams)

| WS | Name | Closes / Adds |
|---|---|---|
| WS1 | Tech-debt closure | CGAL-02 intersect, orphaned menu, C4715, ASM rotate/scale render, v5.0 regression lock |
| WS2 | OpenVDB unlock (corrected premise) | Link OpenVDB → unlock Hollow gizmo |
| WS3 | Emboss text gizmo | Port `GLGizmoEmboss.cpp` (62KB) — new feature |
| WS4 | Preset Bundle full chain | Upstream-compatible bundle metadata + CreatePresetsDialog + dirty-state + Compare/Diff + round-trip |
| WS5 | PartPlate multi-plate completion | Real plate data ownership + per-plate CRUD + config override + AssembleView minimal real view |

## Project Reference

See: .planning/PROJECT.md (v5.0 Current Milestone section + revised OpenVDB constraint)
See: .planning/ROADMAP.md (to be created)
See: .planning/milestones/v4.8-* (last shipped milestone archive)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.

## Current Position

Phase: 144 (Emboss Font + Text2Shapes — next to plan)
Plan: —
Status: Phase 143 partial ✓ (3/13 phases complete, 23%). VDB-03/04/05 satisfied (Hollow gizmo reachable + button + panel); VDB-06 (SLA slice) explicitly deferred to v5.1+ SLA sub-milestone (requires wiring SLAPrint from scratch). 102/102 QmlUiAuditTests passing.
Last activity: 2026-07-17 — Phase 143 shipped partial (3/4 reqs; v50HollowGizmoReachable locked). Ready for Phase 144.

## v5.0 Roadmap Snapshot (13 phases, 141-153)

| Phase | WS | Name | Requirements |
|---|---|---|---|
| 141 | WS1 | v4.x Tech-Debt Closure | DEBT-01..05 |
| 142 | WS2 | OpenVDB CMake Unlock And libslic3r Link | VDB-01, VDB-02 |
| 143 | WS2 | Hollow Gizmo Availability + Button + Panel + SLA Slice | VDB-03..06 |
| 144 | WS3 | Emboss Font Loading And Text2Shapes Extrude | EMB-01, EMB-02 |
| 145 | WS3 | Async EmbossJob And Gizmo Panel | EMB-03, EMB-04 |
| 146 | WS3 | Emboss Wiring, 3MF Round-Trip, And SVG Path | EMB-05..07 |
| 147 | WS4 | Preset Bundle INI Format And CreatePresetsDialog | PSET-01, PSET-02 |
| 148 | WS4 | UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter | PSET-03, PSET-04 |
| 149 | WS4 | Compare/Diff, Dirty Propagation, And Bundle Round-Trip | PSET-05..07 |
| 150 | WS5 | PartPlate UI Gap Analysis (read-only) | PLATE-01 |
| 151 | WS5 | PartPlate UI Implementation | PLATE-02..05 |
| 152 | WS5 | PartPlate Multi-Plate Save/Reload Regression | PLATE-06 |
| 153 | Cross | v5.0 Cross-Workstream Regression Gate | REGRESS-04 |

**Coverage:** 32/32 requirements mapped (DEBT 5 + VDB 6 + EMB 7 + PSET 7 + PLATE 6 + REGRESS-04 1 = 32). 0 unmapped. (REQUIREMENTS.md header previously stated "33" — corrected to 32 by roadmapper; the literal ID list contains 32 unique IDs.)
