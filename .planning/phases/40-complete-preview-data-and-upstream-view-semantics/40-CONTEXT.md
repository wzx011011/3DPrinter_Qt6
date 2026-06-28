# Phase 40: Complete Preview Data and Upstream View Semantics - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 40 completes the Preview data model and G-code semantics needed by the visible local Preview workflow. It owns active-result association, parser metadata, view-mode source values, legend/statistics/tick data, marker values, and stale-preview reset behavior. It does not fix QRhi/D3D11 buffer disappearance, camera/render interaction bugs, or export finalization; those remain Phases 41 and 42.

</domain>

<decisions>

## Implementation Decisions

### Source-Truth Mapping

- Upstream source truth for this phase is `GCodeViewer`, `GCodeProcessorResult`, `IMSlider`, `TickCode`, and `CustomGCode` data as used by the OrcaSlicer Preview panel.
- Qt keeps its own parser and data transport because this repository renders Preview through Qt/QRhi rather than upstream `libvgcode`; behavior and visible semantics must still align with upstream where data is available.
- `PreviewViewModel` is the UI-facing owner of parsed Preview state. QML remains a binding layer and must not compute parser semantics.

### Active Result and Stale Data

- Preview must be built from the currently active valid `SliceService` result, including current plate association and previous-G-code reuse.
- `PreviewViewModel` must rebuild when `SliceService::resultChanged` switches to a different valid output path, not only when a slice finishes.
- Failure, cancellation, import invalidation, plate result removal, and switching to a plate without a valid result must clear Preview data instead of leaving the last visible toolpath.
- Local manual `loadGCodeForPreview()` remains available for deterministic tests and direct G-code preview, but it must reset stale parser state before parsing.

### Parser Coverage

- Parser coverage must include common OrcaSlicer output semantics required by Preview: `G0/G1` moves, travel/extrusion classification, absolute and relative extrusion (`M82/M83`), `G92 E`, layer/Z markers, feature/role comments, width/height, feedrate, fan commands (`M106/M107`), temperature commands (`M104/M109`), acceleration (`M204`), tool changes, elapsed time comments, and custom-gcode/tick markers when present.
- Z-hop travel must not create selectable empty print layers; printed extrusion layer changes remain the stable layer boundary for local Preview.
- Values must be stored per parsed segment so view-mode recoloring does not reparse text and does not use misleading renderer-side proxies when the real field is available.

### View Modes, Legends, and Statistics

- View modes required by v3.4 are feature type, height/layer height, line width, feedrate/speed, fan speed, temperature, tool/extruder, filament/color where data is available, volumetric/flow when derivable, and chronology/layer-time modes.
- Legend data, gradient labels, per-extruder usage, role time, layer-time chart, move-time labels, marker tooltip values, and tick/custom-code markers must all derive from the same parsed segment/tick state as the GCV1 payload.
- The existing `GCV1` payload should remain backward compatible if possible. This phase should prefer filling the current fields correctly over changing the binary payload. Any format expansion must be documented and kept in sync with `RhiViewportRenderer`.

### Renderer Boundary

- Phase 40 may touch renderer parsing only if the data contract requires it. It must not attempt the Phase 41 stability work for D3D11 blanking after slider/camera interaction.
- A Phase 40 pass means the data is internally coherent even if Phase 41 still has QRhi interaction defects.

### Verification

- Add RED tests before production changes for parser semantics, active-result switching, stale reset, view-mode legends, marker values, and tick/custom-code data.
- Use deterministic G-code fixtures wherever possible; real slicing tests remain for result lifecycle integration only.
- Run the canonical verification command before closing the phase.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/core/viewmodels/PreviewViewModel.{h,cpp}` already exposes Preview state, parses standalone G-code files, packs `GCV1`, recolors view modes, tracks role/layer/extruder stats, and exposes tick APIs.
- `src/core/services/SliceService.{h,cpp}` now stores per-plate output paths and result sources after Phase 39. `resultChanged`, `sliceFinished`, and `sliceResultCleared` are the Preview lifecycle signals.
- `src/qml_gui/pages/PreviewPage.qml` binds `previewVm.gcodePreviewData`, layer range, move end, travel/bed/marker flags, view mode, and marker position into `GLViewport`.
- `src/qml_gui/Renderer/RhiViewportRenderer.{h,cpp}` consumes the existing `GCV1` segment fields and uses CPU-prebaked colors from `PreviewViewModel`.
- `tests/E2EWorkflowTests.cpp` already includes parser regression tests for extrusion modes, travel filtering, Z-hop layer handling, and Preview data after a real slice.

### Current Gaps

- `PreviewViewModel` rebuilds on `sliceFinished` but does not listen to `resultChanged`, so switching plates to an existing valid result can leave Preview showing the previous output.
- `sliceFailed` only stops playback and resets `currentMove_`; it does not independently clear all Preview data.
- Tool marker data is invalid when `currentMove_ == moveCount_` because marker lookup rejects the final cursor value, even though the UI can set move to the end.
- Fan and temperature parsing currently looks for `M106/M104` only inside comments; real commands after comment stripping are not handled.
- Layer time handling tracks elapsed comments as the current scalar but does not consistently produce per-layer durations/cumulative move labels aligned with upstream slider behavior.
- Feature/role labels, legend strings, and role names contain mojibake in existing files; do not expand that debt unless touching those exact strings is required for correctness.

### Integration Points

- Active result: `PreviewViewModel` should consume `SliceService::outputPath()` after both `sliceFinished` and `resultChanged`, and reset when the output path is empty or invalid.
- Parser/view-mode data: `StoredSegment` is the current natural extension point for per-move values.
- Renderer contract: `PackedSegment` and `RhiViewportRenderer::GcvPackedSegment` currently match and should remain aligned.
- Tick/custom-code model: local `core/rendering/TickCodeTypes.h` mirrors upstream `TickCode` shape and can back parsed/manual markers.

</code_context>

<specifics>

## Specific Ideas

- The user wants the complete local workflow, not an MVP. Phase 40 should close Preview data completeness for v3.4, not only the minimal parser.
- The user has prioritized performance and is willing to accept implementation complexity for rendering, but this phase should avoid unnecessary renderer churn so Phase 41 can target QRhi performance/stability directly.
- Existing test fixtures should be extended with realistic Orca-style comments and commands instead of relying only on generated full-slice output.

</specifics>

<deferred>

## Deferred Ideas

- D3D11/QRhi disappearing after layer slider drag or mouse orbit remains Phase 41.
- Large-payload render performance tuning and buffer lifecycle hardening remain Phase 41.
- Export naming, safe copy/finalization, and all-plate export remain Phase 42.
- Device/cloud Preview/send workflows remain outside v3.4.

</deferred>
