# Phase 108: Filament-Map Auto Recommendation Readback - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close FMAP-01: after a successful slice, Qt6 reads back the auto-recommended per-plate filament map from libslic3r (`Print::get_filament_maps()` consumed inside the SliceService worker between `print.process()` and `activePrint_.store(nullptr)`, captured by value — no `Print*` escape; mirrors v4.4 WTREAD-01).

Phase 107 (FMAP-02) widened the enum; Phase 108 wires the readback. Phase 110 builds the FilamentGroupPopup UI on top.

</domain>

<decisions>
### Carry-Forward (research + v4.4 pattern)
- v4.4 WipeTowerGeometry POD pattern: SliceService.h:42 struct + wipeTowerGeometryReady signal (:167) + capture-by-value in worker (SliceService.cpp:425,634-658) + EditorViewModel onWipeTowerGeometryReady slot. Phase 108 mirrors this exactly for filament-map.
- Upstream `Print::get_filament_maps()` at Print.cpp:3051 (declared Print.hpp:996) returns `std::vector<int>` — the auto-recommended per-extruder mapping (the result of `ToolOrdering::get_recommended_filament_maps` at Print.cpp:2488, written back to config via `update_filament_maps_to_config` at :3011-3017).
- Upstream auto-recommendation fires INSIDE `print.process()` at Print.cpp:2484-2493: `if (map_mode < fmmManual) { filament_maps = ToolOrdering::get_recommended_filament_maps(...); update_filament_maps_to_config(filament_maps); }`. So `get_filament_maps()` reads back the result of that auto-computation.
- The readback window is the SAME as v4.4: between `print.process()` (SliceService.cpp:590) and `activePrint_.store(nullptr)` (:664).

### Scope
1. **FilamentMapResult POD** in SliceService.h (mirrors WipeTowerGeometry): `bool valid`, `FilamentMapMode mode` (from Phase 107 enum), `std::vector<int> maps` (the auto-recommended per-extruder mapping).
2. **Capture in worker** (SliceService.cpp, near the WipeTowerGeometry capture block): after `print.process()` succeeds, capture `print.get_filament_maps()` + the resolved mode into `FilamentMapResult`. NO `Print*` escapes.
3. **filamentMapReady signal** in SliceService.h (mirrors wipeTowerGeometryReady): emitted on the success branch of sliceFinished (same gate as v4.4 — cancel/error branches do not emit).
4. **EditorViewModel onFilamentMapReady slot** + Q_PROPERTY surface for the auto-recommended map (mirrors onWipeTowerGeometryReady). The map is exposed as a Q_PROPERTY for Phase 110 UI binding.
5. **Regression test**: a slot asserting the capture-by-value invariant + signal wiring (mirrors v4.4 ViewModelSmokeTests readback test).

</decisions>

<code_context>
- `src/core/services/SliceService.h:42-60` (WipeTowerGeometry POD — the pattern to mirror)
- `src/core/services/SliceService.h:167` (wipeTowerGeometryReady signal — mirror)
- `src/core/services/SliceService.cpp:425,634-658` (worker capture — add filament-map capture alongside)
- `src/core/services/SliceService.cpp:805` (the wipeTowerGeometryReady emit site — add filamentMapReady emit alongside)
- `src/core/viewmodels/EditorViewModel.h` (onWipeTowerGeometryReady slot + Q_PROPERTY — mirror)
- `src/core/model/PartPlate.h:96-101` (the Phase 107 FilamentMapMode enum)
- `third_party/OrcaSlicer/src/libslic3r/Print.cpp:2484-2493, 3051` (auto-recommendation + get_filament_maps)
- `third_party/OrcaSlicer/src/libslic3r/Print.hpp:996` (get_filament_maps declaration)

</code_context>

<deferred>
- FMAP-03 (FilamentGroupPopup UI) — Phase 110.
- FMAP-04 (full round-trip test) — Phase 111.
- fmmDefault resolution against global config — Phase 108 captures the per-plate mode; global inheritance resolution is a Phase 110 UI concern.
</deferred>
