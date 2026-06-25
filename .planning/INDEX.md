# Planning Index

Last updated: 2026-06-25

This is the canonical entry point for `.planning/`. Historical phase files are evidence; current work starts from the files below.

## Current Source Of Truth

- Project framing: [PROJECT.md](PROJECT.md)
- Current state: [STATE.md](STATE.md)
- Current roadmap: [ROADMAP.md](ROADMAP.md)
- Remaining migration plan: [REMAINING_MIGRATION_PLAN.md](REMAINING_MIGRATION_PLAN.md)
- Milestone history: [MILESTONES.md](MILESTONES.md)
- Latest milestone audit: [milestones/v2.9-MILESTONE-AUDIT.md](milestones/v2.9-MILESTONE-AUDIT.md)
- Retrospective: [RETROSPECTIVE.md](RETROSPECTIVE.md)
- Prior alignment audit: [audits/2026-06-24-plan-implementation-alignment.md](audits/2026-06-24-plan-implementation-alignment.md)
- Prior implementation audit: [audits/2026-06-23-implementation-audit.md](audits/2026-06-23-implementation-audit.md)
- Prior code drift audit: [audits/2026-06-23-code-drift-audit.md](audits/2026-06-23-code-drift-audit.md)

> Note: v2.9 requirements were archived to [milestones/v2.9-REQUIREMENTS.md](milestones/v2.9-REQUIREMENTS.md) at milestone close. The active `REQUIREMENTS.md` will be recreated by the next `$gsd-new-milestone`.

## Active Milestone

**None active.** v2.9 shipped 2026-06-25 and is archived.

Recommended next milestone: **v3.0 PartPlate and AssembleView**. Start with `$analyzing-source-truth-gap PartPlate and AssembleView`, then `$gsd-new-milestone`.

## Directory Map

| Path | Purpose | Use Rule |
|---|---|---|
| `PROJECT.md` | Living project charter and active milestone framing | Update at milestone boundaries or major source-truth decisions |
| `STATE.md` | Current milestone status and next handoff | Update at phase/milestone transitions |
| `REQUIREMENTS.md` | Requirement IDs, classifications, and traceability | Use for current milestone scope and future backlog |
| `ROADMAP.md` | Active milestone phase plan | Keep focused on current milestone |
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

## Known Drift Being Addressed by v2.9

- Planning previously named v2.6 as current while git history already contained v2.7/v2.8 work.
- Calibration, MQTT, FTP, camera, SSDP, and software viewport areas have real paths plus fallback/mock behavior.
- Some visible UI actions remain disabled or no-op.
- Missing `.Codex/rules/*` references need restoration or redirection.
- Some active files show encoding damage, literal escape artifacts, or residual backup files.

## Next Step

Start next-milestone preparation:

```text
$analyzing-source-truth-gap PartPlate and AssembleView
```

Then create the next milestone:

```text
$gsd-new-milestone
```
