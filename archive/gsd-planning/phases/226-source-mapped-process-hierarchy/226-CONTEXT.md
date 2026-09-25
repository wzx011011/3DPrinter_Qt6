# Phase 226: Source-Mapped Process Hierarchy - Context

**Gathered:** 2026-08-03
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Establish the exact OrcaSlicer FFF Process page and group hierarchy as the
single navigation contract for the Qt6 Process Settings dialog. This phase
covers source mapping, order, and membership only. Disclosure interaction,
typed option editing, preset workflow polish, and final evidence remain in
later v5.11 phases.
</domain>

<decisions>
## Implementation Decisions

### the agent's Discretion
All implementation choices are at the agent's discretion because discuss was
skipped. Follow the ROADMAP success criteria, v5.11 research summary, project
conventions, and the locked OrcaSlicer source.
</decisions>

<code_context>
## Existing Code Insights

- `ConfigOptionModel` owns Process schema metadata and current page/group
  mapping.
- `SettingsDialog.qml` currently hard-codes a divergent Process tab list.
- `TabPrint::build()` in upstream `Tab.cpp` is the hierarchy source of truth.
</code_context>

<specifics>
## Specific Ideas

No additional requirements. Use the v5.11 roadmap and research summary.
</specifics>

<deferred>
## Deferred Ideas

Group disclosure, filtering, typed rows, preset feedback, multi-nozzle UI,
Printer/Material expansion, network, and SLA are out of this phase.
</deferred>
