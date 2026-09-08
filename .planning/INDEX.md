# Planning Index

Last updated: 2026-09-06

This is the canonical entry point for `.planning/`. Historical phase files are evidence; current work starts from the files below.

## Current Source Of Truth

- **Current task state: [WORK_ITEMS.yaml](WORK_ITEMS.yaml)** (canonical since
  2026-09-06 — open/in-progress/blocked/resolved work items; historical
  R-P0.*/R-P1.*/G-*/P* ids resolve through its alias map)
- Project framing: [PROJECT.md](PROJECT.md)
- Milestone status: [STATE.md](STATE.md)
- Current roadmap: [ROADMAP.md](ROADMAP.md)
- Current requirements: [REQUIREMENTS.md](REQUIREMENTS.md)
- v3.6 starting inventory: [research/v3.6-SCREENSHOT-SOURCE-TRUTH.md](research/v3.6-SCREENSHOT-SOURCE-TRUTH.md)
- Historical migration backlog (superseded as a state source): [REMAINING_MIGRATION_PLAN.md](REMAINING_MIGRATION_PLAN.md)
- Milestone history: [MILESTONES.md](MILESTONES.md)
- Historical milestone artifacts (v5.9 and earlier): [milestones/](milestones/)
- Retrospective: [RETROSPECTIVE.md](RETROSPECTIVE.md)
- Prior alignment audit: [audits/2026-06-24-plan-implementation-alignment.md](audits/2026-06-24-plan-implementation-alignment.md)
- Prior implementation audit: [audits/2026-06-23-implementation-audit.md](audits/2026-06-23-implementation-audit.md)
- Prior code drift audit: [audits/2026-06-23-code-drift-audit.md](audits/2026-06-23-code-drift-audit.md)

## Latest Completed Milestone

**v5.16 Full Gap Closure** is complete (closed 2026-08-16; 11/11 phases
231-241, canonical verify exit 0). See [STATE.md](STATE.md) and
[MILESTONES.md](MILESTONES.md). Post-milestone work items — stable plate
result identity, paint axis lock, preview G-code tokenization, QML binding
loops, external blockers — are tracked in [WORK_ITEMS.yaml](WORK_ITEMS.yaml).

Historical note: v5.9 ConfigWizard Multi-Vendor Selection closed 2026-07-25;
its roadmap/requirements/audit artifacts remain under [milestones/](milestones/)
as history. Earlier closed milestones (v5.8, v5.6, phases 193-205) are
recorded in [MILESTONES.md](MILESTONES.md).

## Directory Map

| Path | Purpose | Use Rule |
|---|---|---|
| `WORK_ITEMS.yaml` | Canonical CURRENT task state (open/in-progress/blocked/resolved + alias map) | Update whenever task state changes; all other docs reference it, never duplicate it |
| `PROJECT.md` | Living project charter and active milestone framing | Update at milestone boundaries or major source-truth decisions |
| `STATE.md` | Milestone-level status and handoff | Update at phase/milestone transitions |
| `REQUIREMENTS.md` | Requirement IDs and acceptance definitions for the most recent milestone | Checkboxes are milestone-close evidence; current state lives in WORK_ITEMS.yaml |
| `ROADMAP.md` | Active/latest milestone phase plan | Keep focused on the active or most recently audited milestone |
| `REMAINING_MIGRATION_PLAN.md` | Historical candidate backlog (v5.5-era) | Do not use as a current-state source; superseded by WORK_ITEMS.yaml |
| `MILESTONES.md` | Historical shipped milestone summary | Append milestone-level summaries only |
| `research/` | Current milestone research and inventory inputs | Link files from requirements/roadmap when they are execution inputs |
| `audits/` | Dated read-only audit and gap reports | New dated files; do not mix execution work here |
| `details/` | Focused module research notes | Keep only if linked from requirements or audits |
| `phases/` | Executed phase artifacts | Treat as historical evidence after phase close |
| `reviews/` | Code review outputs | Keep commit/review-id named files |

## Current Classification Rule

Use these status terms in requirements, audits, and handoffs:

- **Real:** source-truth behavior implemented and verified.
- **Hybrid:** real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** unavailable dependency, credential, protocol, or product decision.
- **Placeholder:** visible UI or enum exists but no meaningful backend behavior.
- **Superseded:** previous scope intentionally abandoned in favor of the active milestone.

`WORK_ITEMS.yaml` refines these into four independent dimensions
(implementation / parity / verification / availability). Do not collapse
them into a single checkbox, and do not treat phase completion as product
completion unless the exact workflow is implemented and verified against
upstream behavior and screenshot visual truth where applicable.

## Next Step

Select the next work item from [WORK_ITEMS.yaml](WORK_ITEMS.yaml) (unblocked
items with status `open`; verify no regression first). Keep the canonical
verification gate (`scripts/auto_verify_with_vcvars.ps1`) in place.
