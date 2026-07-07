# Planning Index

Last updated: 2026-07-07

This is the canonical entry point for `.planning/`. Historical phase files are evidence; current work starts from the files below.

## Current Source Of Truth

- Project framing: [PROJECT.md](PROJECT.md)
- Current state: [STATE.md](STATE.md)
- Current roadmap: [ROADMAP.md](ROADMAP.md)
- Current requirements: [REQUIREMENTS.md](REQUIREMENTS.md)
- v3.6 starting inventory: [research/v3.6-SCREENSHOT-SOURCE-TRUTH.md](research/v3.6-SCREENSHOT-SOURCE-TRUTH.md)
- Remaining migration plan: [REMAINING_MIGRATION_PLAN.md](REMAINING_MIGRATION_PLAN.md)
- Milestone history: [MILESTONES.md](MILESTONES.md)
- Latest completed milestone audit: [milestones/v4.0-MILESTONE-AUDIT.md](milestones/v4.0-MILESTONE-AUDIT.md)
- Archived milestone audit: [milestones/v3.2-MILESTONE-AUDIT.md](milestones/v3.2-MILESTONE-AUDIT.md)
- Retrospective: [RETROSPECTIVE.md](RETROSPECTIVE.md)
- Prior alignment audit: [audits/2026-06-24-plan-implementation-alignment.md](audits/2026-06-24-plan-implementation-alignment.md)
- Prior implementation audit: [audits/2026-06-23-implementation-audit.md](audits/2026-06-23-implementation-audit.md)
- Prior code drift audit: [audits/2026-06-23-code-drift-audit.md](audits/2026-06-23-code-drift-audit.md)

## Active Milestone

No active milestone is open. The latest completed milestone is **v4.0 Preview Page UI Restoration**.

Recommended next objective:

- Plan the next local/offline source-truth milestone. Recommended first candidate: parameter settings dialogs restoration.

Carry-forward:

- v3.4 Phase 43 manual UAT is closed by canonical E2E coverage and current runtime launch evidence, not by a separate user click-through.
- v3.5 Phase 47-49 are superseded by v3.6 and should not be resumed as standalone work unless explicitly reopened.

Deferred until the next local/offline milestone:

- AssembleView
- Auto filament-map recommendation
- Wipe-tower geometry/rendering
- `THUMB-03` real GL/QRhi-capture thumbnails and 3MF pixel round-trip
- `FIXTURE-02` full PLATE-09 save/reload assertions after writer integration fix
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion
- Missing CLI test fixtures

Removed from forward scope:

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows.

## Directory Map

| Path | Purpose | Use Rule |
|---|---|---|
| `PROJECT.md` | Living project charter and active milestone framing | Update at milestone boundaries or major source-truth decisions |
| `STATE.md` | Current milestone status and next handoff | Update at phase/milestone transitions |
| `REQUIREMENTS.md` | Requirement IDs, classifications, and traceability | Use for current or most recently audited milestone scope and future backlog |
| `ROADMAP.md` | Active/latest milestone phase plan | Keep focused on the active or most recently audited milestone |
| `REMAINING_MIGRATION_PLAN.md` | Ordered post-v2.9 backlog | Update when a milestone changes backlog order |
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

Do not treat phase completion as product completion unless the exact workflow is implemented and verified against upstream behavior and screenshot visual truth where applicable.

## Next Step

Plan the next local/offline milestone:

```text
$gsd-new-milestone
```

or run an explicitly scoped autonomous milestone after planning:

```text
$gsd-autonomous --auto
```
