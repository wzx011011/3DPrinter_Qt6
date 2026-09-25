# Phase 34: G-code Preview Parser MVP - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning
**Source:** v3.3 ROADMAP and REQUIREMENTS

<domain>
## Phase Boundary

Phase 34 makes `PreviewViewModel` parse common slicer G-code into reliable preview data. It does not implement new renderer behavior beyond feeding the existing preview payload and state properties.
</domain>

<decisions>
## Implementation Decisions

### Parser Scope
- D-34-01: Support `G0`/`G1` travel and extrusion moves, `M82` absolute extrusion, `M83` relative extrusion, `G92 E` reset, Z layer increments, and `Tn` tool changes.
- D-34-02: Preserve existing `GCV1` payload format so Phase 35 can focus on D3D11 renderer interaction instead of a format migration.
- D-34-03: `showTravelMoves` must affect the packed preview payload so renderers can hide travel moves without re-parsing G-code.

### Tests
- D-34-04: Add deterministic parser fixture coverage in `E2EWorkflowTests` using a generated temporary `.gcode` file loaded through `SliceService::loadGCodeFromPrevious`.
- D-34-05: Keep real sliced fixture coverage from Phase 33 as regression evidence for non-empty production G-code preview data.

### the agent's Discretion
- Exact helper function layout inside `PreviewViewModel.cpp`.
- Whether travel visibility also adjusts legend counts, provided the payload itself honors the visibility toggle.
</decisions>

<canonical_refs>
## Canonical References

- `.planning/ROADMAP.md` - Phase 34 goal, deliverables, success criteria.
- `.planning/REQUIREMENTS.md` - `GCODE-01`, `GCODE-02`, `GCODE-03`, `TEST-02`.
- `src/core/viewmodels/PreviewViewModel.h` - UI-facing preview state contract.
- `src/core/viewmodels/PreviewViewModel.cpp` - current parser and payload packer.
- `src/core/services/SliceService.cpp` - `loadGCodeFromPrevious` signal path used by tests.
- `tests/E2EWorkflowTests.cpp` - existing slice/preview workflow regression suite.
</canonical_refs>

<deferred>
## Deferred Ideas

- D3D11 renderer visual interaction is Phase 35.
- Full upstream G-code preview parity is out of scope for v3.3 MVP.
</deferred>

---

*Phase: 34-g-code-preview-parser-mvp*
*Context gathered: 2026-06-28 via autonomous lifecycle defaults*
