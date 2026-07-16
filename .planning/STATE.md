---
gsd_state_version: 1.0
milestone: v5.0
milestone_name: Advanced Feature Recovery & Tech-Debt Closure
status: planning
last_updated: "2026-07-17T00:00:00.000Z"
last_activity: 2026-07-17
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# Project State

**Milestone:** v5.0 — Advanced Feature Recovery & Tech-Debt Closure
**Status:** Planning (2026-07-17). Defining requirements.
**Next step:** Define REQUIREMENTS.md → create ROADMAP.md (5 workstreams, phases starting from 141).

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

Phase: Not started (defining requirements)
Plan: —
Status: Defining requirements
Last activity: 2026-07-17 — Milestone v5.0 started
