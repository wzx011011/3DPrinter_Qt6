# Planning Index

Last updated: 2026-06-28

This is the canonical entry point for `.planning/`. Historical phase files are evidence; current work starts from the files below.

## Current Source Of Truth

- Project framing: [PROJECT.md](PROJECT.md)
- Current state: [STATE.md](STATE.md)
- Current roadmap: [ROADMAP.md](ROADMAP.md)
- Current requirements: [REQUIREMENTS.md](REQUIREMENTS.md)
- Remaining migration plan: [REMAINING_MIGRATION_PLAN.md](REMAINING_MIGRATION_PLAN.md)
- Milestone history: [MILESTONES.md](MILESTONES.md)
- Latest milestone audit: [v3.2-MILESTONE-AUDIT.md](v3.2-MILESTONE-AUDIT.md)
- Archived milestone audit: [milestones/v3.2-MILESTONE-AUDIT.md](milestones/v3.2-MILESTONE-AUDIT.md)
- Retrospective: [RETROSPECTIVE.md](RETROSPECTIVE.md)
- Prior alignment audit: [audits/2026-06-24-plan-implementation-alignment.md](audits/2026-06-24-plan-implementation-alignment.md)
- Prior implementation audit: [audits/2026-06-23-implementation-audit.md](audits/2026-06-23-implementation-audit.md)
- Prior code drift audit: [audits/2026-06-23-code-drift-audit.md](audits/2026-06-23-code-drift-audit.md)

## Active Milestone

**v3.3 Slice Preview Main Flow MVP** is active.

Primary objective:

- Load model -> slice -> enter Preview -> render a non-empty D3D11 QRhi G-code preview with basic layer/move controls.

Active phases:

- Phase 33: Slice-to-Preview Navigation Gate
- Phase 34: G-code Preview Parser MVP
- Phase 35: D3D11 Preview Rendering Interaction
- Phase 36: Verification and Handoff

Deferred until after the main flow:

- `THUMB-03` real GL/QRhi-capture thumbnails and 3MF pixel round-trip
- `FIXTURE-02` full PLATE-09 save/reload assertions after writer integration fix
- AssembleView
- Auto filament-map recommendation
- Wipe-tower geometry/rendering
- D3D12 crash root cause
- Missing CLI test fixtures

## Directory Map

| Path | Purpose | Use Rule |
|---|---|---|
| `PROJECT.md` | Living project charter and active milestone framing | Update at milestone boundaries or major source-truth decisions |
| `STATE.md` | Current milestone status and next handoff | Update at phase/milestone transitions |
| `REQUIREMENTS.md` | Requirement IDs, classifications, and traceability | Use for current or most recently audited milestone scope and future backlog |
| `ROADMAP.md` | Active/latest milestone phase plan | Keep focused on the active or most recently audited milestone |
| `REMAINING_MIGRATION_PLAN.md` | Ordered post-v2.9 backlog | Update when a milestone changes backlog order |
| `MILESTONES.md` | Historical shipped milestone summary | Append milestone-level summaries only |
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

Do not treat phase completion as product completion unless the exact workflow is implemented and verified against upstream behavior.

## Next Step

Start Phase 33:

```text
$gsd-plan-phase 33
$gsd-execute-phase 33 --interactive
```
