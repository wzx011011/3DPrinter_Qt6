# Phase 111: Filament-Map Save-Reload Round-Trip - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close FMAP-04: an automated test asserts the auto-recommended map + selected mode round-trips through save→reload. ALSO close Phase 107 REVIEW R-01: the legacy raw-int-1 → fmmManual migration code (Phase 107 FM-03) was correct by inspection but never executed at runtime — add a test that exercises the legacy discriminator branch.

</domain>

<decisions>
### Carry-Forward
- Phase 107 (FMAP-02): 3MF read-side migration maps legacy raw-int-1 → fmmManual. Phase 107 REVIEW R-01: the migration branch is never executed by the existing round-trip test (the write side now produces typed coEnum values, so the reload takes the trusted branch).
- Phase 108 (FMAP-01): auto-recommended map readback (FilamentMapResult POD + signal + EditorViewModel Q_PROPERTYs).
- Phase 110 (FMAP-03): FilamentGroupPopup UI + R-02 enum range validation + setPlateFilamentMapMode Q_INVOKABLE.
- v4.3 Phase 97 thumbnail round-trip pattern: the model for a save→reload round-trip test (synthesized known state → saveProject to temp .3mf → fresh loadFile → assert survived).

### Scope
1. **FMAP-04 full round-trip test** (PartPlateTests): synthesize a plate with a known filament-map mode (e.g. fmmAutoForFlush) + known maps → saveProject to temp .3mf → fresh ProjectServiceMock → loadFile → assert the mode + maps survived (mirrors Phase 97 thumbnail round-trip). Cover at minimum: fmmAutoForFlush, fmmManual (the 2 most-used), and assert fmmDefault resolution (if applicable).
2. **R-01 legacy-branch test**: construct a config with a NON-coEnum int filament_map_mode=1 (simulating a pre-v4.5 file) → feed it through the read-side migration → assert it maps to fmmManual (NOT fmmAutoForMatch). This may require factoring the migration into a small testable helper OR constructing a synthetic legacy 3MF fixture. The executor decides the cleanest approach.
3. **Regression lock**: a source-audit (QmlUiAuditTests) confirming the round-trip test exists + covers both modes.

</decisions>

<code_context>
- `tests/PartPlateTests.cpp` (the Phase 97 thumbnailSaveReloadRoundTrip pattern + the Phase 107 filamentMapModeRoundTripManualPreserved slot to extend)
- `src/core/services/ProjectServiceMock.cpp:4997` (write site) + :5413 (read site + migration)
- `src/core/model/PartPlate.h:96-101` (the 4-value enum)
- `.planning/phases/107-filament-map-mode-enum-widening-and-3mf-migration/107-REVIEW.md` (R-01 finding)

</code_context>

<deferred>
- None — this is the last WS1 phase. Phase 112 starts WS5.
</deferred>
