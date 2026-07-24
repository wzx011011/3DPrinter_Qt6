# Planning Index

Last updated: 2026-07-24

This is the canonical entry point for `.planning/`. Historical phase files are evidence; current work starts from the files below.

## Current Source Of Truth

- Project framing: [PROJECT.md](PROJECT.md)
- Current state: [STATE.md](STATE.md)
- Current roadmap: [ROADMAP.md](ROADMAP.md)
- Current requirements: [REQUIREMENTS.md](REQUIREMENTS.md)
- v3.6 starting inventory: [research/v3.6-SCREENSHOT-SOURCE-TRUTH.md](research/v3.6-SCREENSHOT-SOURCE-TRUTH.md)
- Remaining migration plan: [REMAINING_MIGRATION_PLAN.md](REMAINING_MIGRATION_PLAN.md)
- Milestone history: [MILESTONES.md](MILESTONES.md)
- Active milestone roadmap: [milestones/v5.7-ROADMAP.md](milestones/v5.7-ROADMAP.md)
- Active milestone requirements: [milestones/v5.7-REQUIREMENTS.md](milestones/v5.7-REQUIREMENTS.md)
- Latest completed milestone audit: [milestones/v5.7-MILESTONE-AUDIT.md](milestones/v5.7-MILESTONE-AUDIT.md)
- Archived milestone audit: [milestones/v3.2-MILESTONE-AUDIT.md](milestones/v3.2-MILESTONE-AUDIT.md)
- Retrospective: [RETROSPECTIVE.md](RETROSPECTIVE.md)
- Prior alignment audit: [audits/2026-06-24-plan-implementation-alignment.md](audits/2026-06-24-plan-implementation-alignment.md)
- Prior implementation audit: [audits/2026-06-23-implementation-audit.md](audits/2026-06-23-implementation-audit.md)
- Prior code drift audit: [audits/2026-06-23-code-drift-audit.md](audits/2026-06-23-code-drift-audit.md)

## Latest Completed Milestone

**v5.7 D3D12 Backend Investigation** is complete (closed 2026-07-24).

Primary outcome:

- D3D12 promotion attempted, verified on real hardware, **reverted** — D3D12
  crashes at QQuickWindow swapchain init on AMD Radeon APU (mainstream iGPU).
  D3D11 remains the default; D3D12 is opt-in. Phase 207-210 retained
  (diagnostics + seam A/B/C mitigations). Canonical build (D3D11 default) exited
  `0`, `APP_RUNNING_PID=80404`, all ctest + E2E passed.

Previous milestone **v5.6 Deferred Backlog Closure** is complete (closed
2026-07-24). Canonical build exited `0`, `APP_RUNNING_PID=79708`, all ctest +
E2E passed.

Phases (193-205):

- Phase 193: Planning State Reconciliation
- Phase 194: Cmp-03 OptionRow and Slider Unification
- Phase 195: KBShortcutsDialog Extraction and Grouping
- Phase 196: XD-02 Emboss Spinner and SliceProgress States
- Phase 197: Calibration Dedicated Tower Geometry
- Phase 198: ObjectList Tree Deepening (Auxiliary file-tree panel)
- Phase 199: ConfigWizard Vendor/Model Enumeration Layer
- Phase 200: ConfigWizard Single-Vendor Wizard Rewrite
- Phase 201: AMS Architecture Cleanup (mock to ViewModel)
- Phase 202: Plugin Manager UI Real Backend (no Python)
- Phase 203: D3D12 Root-Cause Confirmation (no default promotion)
- Phase 204: de/fr/ja/ko Translation Long Tail to >=85%
- Phase 205: Cross-Workstream Regression Gate and Milestone Audit

Carry-forward from v5.5:

- The canonical build/run contract is the entry gate for every v5.6 phase.
- `third_party/OrcaSlicer` submodule is not checked out locally; upstream
  comparison uses `D:/work/OrcaSlicer`.

Deferred after v5.6 (per user decisions 2026-07-24):

- H2C/A2L multi-nozzle UI (bb3 fork submodule + product decision pending).
- Per-extruder config editor UI (Cmp-03 sub-item; needs multi-extruder fixture).
- ConfigWizard multi-vendor selection + PresetUpdater + AppConfig.
- D3D12 default-backend promotion itself (pending root cause).
- AMS real device/cloud data sources (printer-hardware scope).
- Python script/macro framework / CPython embedding.

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

Select the next product migration milestone from the deferred backlog (H2C/A2L,
per-extruder editor, ConfigWizard multi-vendor, D3D12 promotion, AMS real
backend, Python framework). Keep the canonical verification gate in place.
