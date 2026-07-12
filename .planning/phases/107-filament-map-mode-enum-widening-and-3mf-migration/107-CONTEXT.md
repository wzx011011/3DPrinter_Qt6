# Phase 107: Filament-Map Mode Enum Widening And 3MF Migration - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close FMAP-02: widen the Qt6 filament-map mode from 2-value (`0=Auto, 1=Manual`) to upstream's 4-value (`fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2, fmmDefault=3`), with 3MF persistence read-side migration so pre-v4.5 "Manual" (=1) plates do not silently reload as "AutoForMatch" (=1).

This MUST ship before Phase 108 (FMAP-01 readback) so the auto-mode code doesn't entrench the new enum before legacy files migrate (Pitfall 2).

</domain>

<decisions>
### Carry-Forward (research)
- Current Qt6: `int m_filament_map_mode = 0; // 0=Auto, 1=Manual` at PartPlate.h:273; accessor at :202; setter at :203.
- Upstream: `enum FilamentMapMode { fmmAutoForFlush=0, fmmAutoForMatch=1, fmmManual=2, fmmDefault=3 }` at PrintConfig.hpp:424-429.
- 3MF persistence hazard: `opt->setInt(p->filamentMapMode())` at ProjectServiceMock.cpp:4997 writes raw int. Pre-v4.5 "Manual"=1 would reload as fmmAutoForMatch=1.
- Read site: ProjectServiceMock.cpp:5413 `if (auto *opt = plate->config.option("filament_map_mode"))` reads raw int.
- `fmmDefault` semantics (research FEATURES.md): per-plate "inherit from global" sentinel resolved by `PartPlate::get_real_filament_map_mode` — NOT a 4th popup radio button (anti-feature).
- Pitfall 2: the migration is at the 3MF read side, not the write side — pre-v4.5 files on disk have raw int 1 meaning "Manual" but the new enum's 1 means "AutoForMatch."

### Scope
1. **Widen the enum:** Replace the `int m_filament_map_mode = 0; // 0=Auto, 1=Manual` with the 4-value enum (either a proper `enum class FilamentMapMode` or a documented int with the 4 named constants). Update the comment to cite upstream PrintConfig.hpp:424-429.
2. **3MF read-side migration:** At ProjectServiceMock.cpp:5413 (the read site), if the raw int read from a legacy file is `1` (old "Manual") AND there's no way to distinguish it from new "AutoForMatch"=1, document the migration logic. **The cleanest fix:** write the enum as a STRING ("fmmAutoForFlush"/"fmmAutoForMatch"/"fmmManual"/"fmmDefault") on the write side (forward-compat) AND read with a fallback that maps legacy raw-int-1 → fmmManual (preserving pre-v4.5 intent). Upstream uses ConfigOptionEnum<FilamentMapMode> which serializes as the enum name string (PrintConfig.cpp:580-583 — no "Default" string per FEATURES.md).
3. **Caller updates:** All callers of `setFilamentMapMode` / `filamentMapMode` updated for the new enum (ProjectServiceMock, EditorViewModel, PartPlateTests, QML).
4. **Regression test:** a PartPlateTests or ViewModelSmokeTests slot asserting the 3MF round-trip preserves the mode (legacy-int-1 → fmmManual; new string → correct enum).

</decisions>

<code_context>
- `src/core/model/PartPlate.h:202-203, 268-273` (the enum + accessor + setter + member)
- `src/core/services/ProjectServiceMock.cpp:4996-4997` (write site — setInt hazard)
- `src/core/services/ProjectServiceMock.cpp:5413` (read site — migration goes here)
- `src/core/services/ProjectServiceMock.cpp:1169-1187` (setPlateFilamentMap API)
- `src/core/viewmodels/EditorViewModel.h/.cpp` (Q_PROPERTY + QML-facing API)
- `tests/PartPlateTests.cpp` (existing filamentMapManualAssignment slot — update for new enum)
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424-429` (upstream 4-value enum)
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.cpp:580-583` (upstream enum string serialization — no "Default")

</code_context>

<deferred>
- FMAP-01 (auto-recommendation readback) — Phase 108.
- FMAP-03 (FilamentGroupPopup UI) — Phase 110.
- FMAP-04 (round-trip test) — Phase 111 (this phase ships a minimal round-trip test for the migration; Phase 111 adds the full auto-map round-trip).
</deferred>
