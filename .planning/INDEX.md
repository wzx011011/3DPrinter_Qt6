# Planning Index

Last updated: 2026-06-30

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

**v3.5 Preset Authoring Complete Workflow** is active.

Primary objective:

- Load preset bundle -> select compatible printer/filament/process presets -> edit config -> save/create/import/export presets -> slice/export with the edited config.

Active phases:

- Phase 44: Preset Bundle Service Foundation
- Phase 45: Compatibility and Selection State
- Phase 46: Config Editing, Dirty State, and Reset Semantics
- Phase 47: Preset Lifecycle Actions
- Phase 48: Create Presets and Bundle Workflows
- Phase 49: Slice Integration, Verification, and Handoff

Carry-forward:

- v3.4 Phase 43 manual UAT remains pending because the user cannot verify it right now. Do not mark v3.4 fully complete without running `.planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md`.

Deferred until after preset authoring:

- Device send/upload/cloud print and Monitor task workflow
- AssembleView
- Auto filament-map recommendation
- Wipe-tower geometry/rendering
- `THUMB-03` real GL/QRhi-capture thumbnails and 3MF pixel round-trip
- `FIXTURE-02` full PLATE-09 save/reload assertions after writer integration fix
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion
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

Start Phase 44:

```text
$gsd-plan-phase 44
$gsd-execute-phase 44
```
