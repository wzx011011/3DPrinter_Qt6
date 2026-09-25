# Phase 119: Tick Type Coverage And Drag Relocation - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close TICK-04 + TICK-05 (WS1 final phase): ensure all 5 upstream tick types (PausePrint/CustomGcode/Template/ToolChange/ColorChange) round-trip end-to-end (add → write-back → re-slice → re-parse), and add tick drag-to-relocate (the missing `moveTick`).

Phase 117 surfaced the UI; Phase 118 wired write-back + re-slice. Phase 119 closes the type-coverage and drag gaps.

</domain>

<decisions>
## Implementation Decisions

### TICK-04: 5-type coverage
The convertTicksToCustomGcodeInfo switch (Phase 118) already maps all 5 types. Gaps to close:
- **ColorChange add method**: `addColorChangeAtLayer(int layer, int extruder, const QString& color)` is MISSING (only `addFilamentChangeAtLayer` = ToolChange exists). Add it (type=ColorChange, extruder + color). Wire into PreviewLayerRail Add menu + CustomGcodeDialog.
- **Template add method**: `addTemplateAtLayer(int layer)` MISSING (data layer + convert switch handle it, but no Q_INVOKABLE add path). Add it. Upstream template = "save current state" anchor; full template UI is future, but the add + round-trip is in-scope for TICK-04.
- **Round-trip proof**: a deterministic test (QmlUiAuditTests or ViewModelSmokeTests) asserts add→convert→(the Info contains the type)→re-parse path covers all 5. The read-side parse (PreviewViewModel.cpp:993-1021) already re-parses all 5 from G-code comments.

### TICK-05: drag-to-relocate (moveTick)
- Add `Q_INVOKABLE void moveTick(int fromLayer, int toLayer)`: find tick at fromLayer, change its `tick` to toLayer, re-sort, emit tickMarksChanged, call writeTicksToModel (Phase 118 re-slice).
- QML: add a drag handler on the tick delegate in PreviewLayerRail.qml — on drag, compute the target layer from the new y position, call `previewVm.moveTick(tickLayer, targetLayer)`. Guard against dropping on an occupied layer (dedup).
- Upstream IMSlider supports tick drag; this matches the source-truth behavior.

</decisions>

<specifics>
## Code Access Points
- PreviewViewModel CRUD: `PreviewViewModel.cpp:1778-1945` (Phase 118 renumbered; addColorChangeAtLayer + addTemplateAtLayer + moveTick added here).
- convertTicksToCustomGcodeInfo: `PreviewViewModel.cpp` anonymous namespace (Phase 118) — already handles all 5 types.
- PreviewLayerRail.qml tick delegate (Phase 117): add drag MouseArea.
- CustomGcodeDialog.qml: may need a color/extruder picker for ColorChange/ToolChange add (check current dialog — it's CustomGcode-text-only).

## Source-Truth Anchors
- Upstream IMSlider tick drag: `third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.cpp` (on_mouse_drag / render_tick_on_mouse_pos).
- Upstream CustomGCode::Type: `CustomGCode.hpp:14-22` (all 5 types).
- Read-side parse: `PreviewViewModel.cpp:993-1021` (parses ;...COLOR_CHANGE/PAUSE_PRINT/CUSTOM_GCODE/MANUAL_TOOL_CHANGE).

</specifics>

<deferred>
## Deferred Ideas
None — Phase 119 closes WS1. Full template "save state as template" UI is future (data round-trip is in-scope per TICK-04).

</deferred>
