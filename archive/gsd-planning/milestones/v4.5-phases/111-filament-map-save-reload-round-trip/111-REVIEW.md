---
status: APPROVED
phase: 111-filament-map-save-reload-round-trip
requirement: FMAP-04 + R-01
verdict: ship
counts: {blockers: 0, high: 0, medium: 0, low: 2, info: 2}
---

# Phase 111 Code Review — Filament-Map Save-Reload Round-Trip

## Verdict: APPROVED (ship)

Cleanly closes both FMAP-04 and Phase 107 R-01 gap. The `migrateLegacyFilamentMapMode` factoring is correct, minimal, genuinely unit-testable. Legacy raw-int-1 → fmmManual contract is now executed at runtime. Maps-array round-trip fix correctly guarded.

## Findings

| # | Severity | Finding |
|---|----------|---------|
| L-1 | low | fmmAutoForMatch ("Convenience Mode") has no round-trip leg — it's the mode whose enum value (1) collided with legacy raw-int-1 "Manual". A round-trip leg for fmmAutoForMatch would provide defense-in-depth that the NEW typed value 1 round-trips as fmmAutoForMatch (trusted coEnum branch) while legacy raw-int 1 migrates to fmmManual. Not required for FMAP-04; recommended for future hardening. |
| L-2 | low | Source-audit "both legs covered" check is structurally weak — asserts the tokens `fmmManual` + `fmmAutoForFlush` exist somewhere in PartPlateTests.cpp, not that the slot specifically exercises both legs. A tighter anchor would grep for the leg-specific temp-path prefixes. |
| I-1 | info | Phase 107 `filamentMapModeRoundTripManualPreserved` is now partially redundant (superseded by the Phase 111 fmmManual leg which asserts MODE + MAPS). Its SCOPE NOTE about the array gap is now stale (closed by 111). Harmless. |
| I-2 | info | Load-path restoration sites also route through the R-02 clamping int overload (verified). Values are pre-migrated upstream so the clamp is a no-op there in practice — but correct that no unguarded int path exists. |

## Verified

- **migrateLegacyFilamentMapMode (the CRITICAL migration contract):** PartPlate.cpp:21-38 — legacy raw-int 1 → fmmManual (NOT fmmAutoForMatch=1). Full mapping: 0→fmmAutoForFlush, 1→fmmManual, else→fmmDefault. Pure function, no deps, genuinely unit-testable.
- **Both read sites delegate:** ProjectServiceMock.cpp:657 (loadFile) + :5599 (loadProject) both call the factored helper. Source-audit locks ≥2 call sites.
- **Maps-array round-trip fix:** ProjectServiceMock.cpp:5153-5156 — `pd->config.option<ConfigOptionInts>("filament_map", true)->values = p->filamentMaps()` guarded on `!filamentMaps().empty()`. The bbs_3mf model.config writer reads `pd->config["filament_map"]`, not the member — this was the deferred Phase 107 gap. Correctly guarded so auto-mode plates emit no filament_map metadata.
- **Round-trip test coverage:** fmmManual (mode + maps={2,1,3}) + fmmAutoForFlush (mode only). Deterministic. Uses kStlPath + unique temp paths + cleanup.
- **R-01 closure:** `filamentMapLegacyMigrationMapsInt1ToManual` feeds the factored helper directly — legacy discriminator branch is now executed at runtime on every test run (no libslic3r needed; runs in both HAS_LIBSLIC3R and non-HAS builds).

Regression: PartPlateTests 53/53, QmlUiAuditTests 83/83 pass.
