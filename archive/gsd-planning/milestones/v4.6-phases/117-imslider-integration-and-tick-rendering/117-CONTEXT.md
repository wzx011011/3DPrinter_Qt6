# Phase 117: IMSlider Integration And Tick Rendering - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss=true + user override)

<domain>
## Phase Boundary

`PreviewPage` instantiates the functionally-complete `LayerSlider.qml` (or its integrated successor) instead of the tick-less `PreviewLayerRail.qml`, so the right-side layer rail and/or bottom slider renders tick marks driven by `previewVm.tickMarks`. Tick marks are color-coded by type and the existing right-click Add/Edit/Delete menus become reachable in the running Preview. The orphaned-component state is removed (No-Deprecated-UI rule).

This phase is UI-integration only — it surfaces the existing-but-orphaned `LayerSlider.qml` and its `previewVm.tickMarks` binding. It does NOT add write-back to `custom_gcode_per_print_z` (that is Phase 118) or new tick types (Phase 119). The read-side parse at `PreviewViewModel.cpp:993-1021` already feeds `tickMarks_`.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use ROADMAP phase goal, the success criteria below, and codebase conventions to guide decisions.

Key decision point (resolve during planning): **integrate `LayerSlider.qml` directly** (wire it into `PreviewPage` replacing `PreviewLayerRail.qml`) **vs. consolidate** its tick-rendering functionality into the in-use slider. The No-Deprecated-UI rule requires that no dead duplicate slider component remain. Prefer the approach that leaves the fewest orphaned files.

### Source-Truth Anchors
- Upstream OrcaSlicer IMSlider: `third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.cpp` / `.hpp` — the canonical tick+slider behavior.
- Tick type colors: upstream `CustomGCode::Type` + `IMSlider` rendering — match the upstream color scheme (PausePrint/ToolChange/ColorChange/CustomGcode/Template).
- Qt read-side parse: `PreviewViewModel.cpp:993-1021` (already feeds `tickMarks_` from sliced G-code comments).

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Known anchors (from the `/gsd:new-milestone` gap audit):
- `src/qml_gui/components/LayerSlider.qml` — functionally complete (tick render `:81-124`, right-click add menu `:531-557`, edit/delete menu `:561-624`, CustomGcodeDialog instantiation `:627-640`) but **orphaned — never instantiated anywhere**.
- `src/qml_gui/components/CustomGcodeDialog.qml` — functional add/edit dialog (`:55-62`), only instantiated by the orphaned `LayerSlider.qml`.
- `src/qml_gui/components/PreviewLayerRail.qml` — the tick-less vertical `RangeSlider` currently used by `PreviewPage.qml:449-453`.
- `src/qml_gui/components/MoveSlider.qml` — the bottom move slider used by `PreviewPage.qml:465-472`.
- `src/core/viewmodels/PreviewViewModel.h:222-231` / `.cpp:1758-1873` — tick data model + CRUD (addPauseAtLayer, addCustomGcodeAtLayer, addFilamentChangeAtLayer, removeTickAtLayer, editCustomGcodeAtLayer, tickAtLayer, clearAllTicks). `Q_PROPERTY(QVariantList tickMarks ...)` + `tickMarksChanged` signal at `.h:83-84`.
- `src/core/rendering/TickCodeTypes.h:7-24` — the 5-type enum (PausePrint/CustomGcode/Template/ToolChange/ColorChange) + TickCode struct.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and the 4 success criteria:
1. PreviewPage renders tick marks driven by `previewVm.tickMarks` (replacing tick-less PreviewLayerRail).
2. Tick color-coding by type (Pause=orange, ToolChange=blue, ColorChange=green, CustomGcode=teal) at correct layer positions.
3. Right-click Add/Edit/Delete menus reachable in running Preview.
4. No orphaned/dead slider component left behind (No-Deprecated-UI rule).

</specifics>

<deferred>
## Deferred Ideas

None — discuss phase skipped. Phase 118 handles `custom_gcode_per_print_z` write-back + re-slice loop; Phase 119 handles the remaining tick types + drag relocation.

</deferred>
