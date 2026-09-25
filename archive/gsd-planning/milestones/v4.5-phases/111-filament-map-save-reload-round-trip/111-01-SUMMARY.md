---
phase: 111-filament-map-save-reload-round-trip
plan: 01
subsystem: testing
tags: [qt-test, 3mf, filament-map, round-trip, enum-migration]

# Dependency graph
requires:
  - phase: 107-filament-map-mode-enum-widening-and-3mf-migration
    provides: 4-value FilamentMapMode enum + 3MF read-side migration (FM-03) + write-side typed-enum persistence (FM-02).
  - phase: 108-filament-map-auto-recommendation-readback
    provides: auto-recommended map readback (FMAP-01). Not directly exercised here but is the upstream of the modes surfaced.
  - phase: 110-filament-map-popup-ui-and-mode-surfacing
    provides: setPlateFilamentMapMode Q_INVOKABLE + R-02 enum range validation. Used by the round-trip test's auto leg.
provides:
  - "FMAP-04 closure: full save->reload round-trip test (filamentMapSaveReloadRoundTrip) covering fmmManual (mode + maps) and fmmAutoForFlush (mode)."
  - "Phase 107 REVIEW R-01 closure: the FM-03 legacy raw-int-1 -> fmmManual migration is now executed at runtime via filamentMapLegacyMigrationMapsInt1ToManual (the factored helper is unit-tested in isolation)."
  - "OWzx::migrateLegacyFilamentMapMode(int) free helper (PartPlate.h/PartPlate.cpp) -- the FM-03 predicate factored out of ProjectServiceMock so the legacy discriminator branch is unit-testable."
  - "pd->config['filament_map'] (ConfigOptionInts) population in buildPlateDataList so the filament_maps ARRAY round-trips through the bbs_3mf model.config writer/reader (closes the deferred Phase 107 array gap)."
  - "Source-audit slot filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration locking the FMAP-04 + R-01 anchors."
affects: [112-per-volume-its-accessor, future-filament-map-readback, future-round-trip-tests]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Factored migration predicate: legacy raw-int -> widened-enum mapping lives in a free function (OWzx::migrateLegacyFilamentMapMode) so the legacy branch is unit-testable in isolation without a synthetic 3MF fixture."
    - "Def-respecting ConfigOptionInts write: pd->config.option<ConfigOptionInts>('filament_map', true)->values = ... mirrors upstream Plater set_key_value and avoids the type-mismatch crash the Phase 107 draft hit on filament_map_mode."

key-files:
  created:
    - ".planning/phases/111-filament-map-save-reload-round-trip/111-01-SUMMARY.md"
  modified:
    - "src/core/model/PartPlate.h (declared OWzx::migrateLegacyFilamentMapMode)"
    - "src/core/model/PartPlate.cpp (defined OWzx::migrateLegacyFilamentMapMode)"
    - "src/core/services/ProjectServiceMock.cpp (both read sites delegate to the helper; buildPlateDataList now populates pd->config['filament_map'])"
    - "tests/PartPlateTests.cpp (filamentMapLegacyMigrationMapsInt1ToManual + filamentMapSaveReloadRoundTrip)"
    - "tests/QmlUiAuditTests.cpp (filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration + updated Phase 107 FM-03 audit)"

key-decisions:
  - "Factor the FM-03 migration into a testable helper rather than construct a synthetic legacy 3MF fixture. The helper (OWzx::migrateLegacyFilamentMapMode) is a pure int->enum function; the legacy-migration test feeds raw legacy ints through it directly, so the legacy discriminator branch is exercised at runtime without fixture-file complexity. This is the plan's preferred approach (FT-02 option a)."
  - "Populate pd->config['filament_map'] (ConfigOptionInts) in buildPlateDataList to close the array round-trip. The bbs_3mf model.config writer (_add_model_config_file_to_archive, bbs_3mf.cpp:7969-7980) reads the CONFIG option, NOT the pd->filament_maps member (that member is read only by the separate slice_info.config writer at bbs_3mf.cpp:8207, which the model-load path does not parse). Without the config write, FT-01's array assertion cannot pass. This is an additive production change approved mid-execution; it mirrors upstream Plater's set_key_value pattern."

patterns-established:
  - "Factored migration predicate for legacy-format -> widened-enum mappings: when a read-side discriminator branch is correct by inspection but unreachable by the round-trip test (because the write side produces typed values), factor the branch into a pure function so it has direct unit-test coverage."
  - "ConfigOptionInts round-trip through bbs_3mf model.config requires writing BOTH the PlateData member AND the pd->config[<key>] ConfigOption -- the writer branch reads the config option, the reader populates both."

requirements-completed: [FMAP-04]

# Metrics
duration: ~45min (3 build/verify cycles; the first caught the array gap, the second caught the Phase 107 audit coupling)
completed: 2026-07-13
---

# Phase 111 Plan 01: Filament-Map Save-Reload Round-Trip Summary

**Full save->reload round-trip test (FMAP-04) for filament-map mode + maps array across fmmManual/fmmAutoForFlush, plus runtime closure of the Phase 107 R-01 legacy raw-int-1 -> fmmManual migration gap via a factored testable helper.**

## Performance

- **Duration:** ~45 min (3 canonical build/verify cycles)
- **Tasks:** 3
- **Commits:** 4 (2 test, 2 fix)
- **Files modified:** 5

## Accomplishments
- Closed FMAP-04: a full save->reload round-trip test (filamentMapSaveReloadRoundTrip) asserts the filament-map MODE + the filament_maps ARRAY survive a real .3mf save->reload, covering both fmmManual (mode + maps) and fmmAutoForFlush (mode).
- Closed Phase 107 REVIEW R-01: the FM-03 legacy raw-int-1 -> fmmManual migration is now executed at runtime. The migration was factored into OWzx::migrateLegacyFilamentMapMode(int) (PartPlate.h/PartPlate.cpp) and both ProjectServiceMock read sites delegate to it. A pure-logic unit test (filamentMapLegacyMigrationMapsInt1ToManual) feeds legacy raw ints through the helper and asserts raw 1 -> fmmManual (value 2), NOT fmmAutoForMatch (value 1). R-01 observed the existing round-trip test always took the trusted coEnum branch, so the legacy discriminator (the actual headline FMAP-02 fix) had zero runtime coverage -- this is now closed.
- Closed the deferred Phase 107 array round-trip gap: buildPlateDataList now populates pd->config["filament_map"] (ConfigOptionInts) so the bbs_3mf model.config writer emits the <metadata key="filament_maps"> entry. The maps array now survives save->reload (previously only the mode round-tripped).
- Source-audit slot filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration locks the FMAP-04 + R-01 anchors (helper declared + defined, both read sites delegate, both test slots exist, the R-01 headline assertion is present).

## Task Commits

Each task was committed atomically:

1. **Task 111-01-01 (tests + helper):** `557daa2` (test) -- factored migrateLegacyFilamentMapMode + filamentMapLegacyMigrationMapsInt1ToManual + filamentMapSaveReloadRoundTrip.
2. **Task 111-01-01 (array-round-trip fix):** `39f3878` (fix) -- populate pd->config["filament_map"] so the maps array round-trips (caught by the first build).
3. **Task 111-01-02 (source-audit):** `bd45aeb` (test) -- filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration.
4. **Task 111-01-02 (Phase 107 audit update):** `6fad09c` (fix) -- updated the Phase 107 FM-03 audit to anchor on the factored-helper call form (caught by the second build).

(Task 111-01-03 is the build + ctest documentation task; its deliverable is this SUMMARY.)

## Files Created/Modified
- `src/core/model/PartPlate.h` -- declared `OWzx::migrateLegacyFilamentMapMode(int legacyRawInt)` (the factored FM-03 migration predicate).
- `src/core/model/PartPlate.cpp` -- defined `OWzx::migrateLegacyFilamentMapMode` (0->fmmAutoForFlush, 1->fmmManual, else->fmmDefault).
- `src/core/services/ProjectServiceMock.cpp` -- both read sites (loadFile ~657, loadProject ~5536) now call the factored helper; buildPlateDataList now populates `pd->config["filament_map"]` (ConfigOptionInts) so the array round-trips.
- `tests/PartPlateTests.cpp` -- added `filamentMapLegacyMigrationMapsInt1ToManual` (R-01, no libslic3r) + `filamentMapSaveReloadRoundTrip` (FMAP-04, HAS_LIBSLIC3R; fmmManual + fmmAutoForFlush legs).
- `tests/QmlUiAuditTests.cpp` -- added `filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration`; updated the Phase 107 `filamentMapModeEnumWidenedToUpstream4Value` FM-03 assertion to anchor on the factored-helper call.

## Decisions Made
- **Factor the migration (FT-02 approach choice):** chose plan option (a) -- factor the FM-03 legacy raw-int -> FilamentMapMode migration into a free function `OWzx::migrateLegacyFilamentMapMode(int)` -- over constructing a synthetic legacy 3MF fixture or a synthetic DynamicPrintConfig. Rationale: the predicate is pure int->enum logic; a free helper is the cleanest, most maintainable form and is directly unit-testable without fixture-file or libslic3r-config complexity. The existing inline migration block at both read sites was replaced with a one-line delegation, preserving call-site behavior exactly.
- **Close the array round-trip (user-approved scope addition):** the plan's FT-01 requires asserting "the mode + maps survived the round-trip," but the first build caught that the maps array came back empty. Root cause: the Qt6 write path set `pd->filament_maps` (the PlateData member) but not `pd->config["filament_map"]` (the ConfigOptionInts the bbs_3mf model.config writer reads). This was the deferred gap Phase 107 documented in filamentMapModeRoundTripManualPreserved's SCOPE NOTE. With user approval, closed it with an additive config write mirroring upstream Plater's set_key_value pattern (guarded on !filamentMaps().empty() so auto-mode plates emit no filament_map metadata).

## Deviations from Plan

### Auto-fixed Issues

**1. Array round-trip gap (caught by the canonical build)**
- **Found during:** Task 111-01-03 (canonical build + ctest) -- the first verify run.
- **Issue:** `filamentMapSaveReloadRoundTrip` failed: the filament_maps ARRAY came back empty (size 0, expected size 3). The MODE round-tripped fine. Root cause: buildPlateDataList set `pd->filament_maps` (member) but not `pd->config["filament_map"]` (ConfigOptionInts); the bbs_3mf model.config writer (_add_model_config_file_to_archive, bbs_3mf.cpp:7969-7980) reads the config option, not the member.
- **Fix:** Added `pd->config.option<Slic3r::ConfigOptionInts>("filament_map", true)->values = p->filamentMaps()` in buildPlateDataList, guarded on `!p->filamentMaps().empty()`. Mirrors upstream Plater's set_key_value("filament_map", new ConfigOptionInts(...)).
- **Files modified:** src/core/services/ProjectServiceMock.cpp
- **Verification:** canonical build + ctest, PartPlateTests 53/53 pass (array + mode both round-trip).
- **Committed in:** `39f3878`

**2. Phase 107 audit coupled to the old inline comment (caught by the canonical build)**
- **Found during:** Task 111-01-03 -- the second verify run.
- **Issue:** The Phase 107 `filamentMapModeEnumWidenedToUpstream4Value` audit asserted the literal inline comment `raw 1 (old "Manual")  -> fmmManual` was present in ProjectServiceMock.cpp. Factoring the migration into the helper removed that inline comment text, so the audit failed (the discriminator assertion still passed; only the comment-text assertion failed).
- **Fix:** Updated the FM-03 read-side-migration assertion to anchor on the factored-helper call form (`OWzx::migrateLegacyFilamentMapMode(int(opt->getInt()))`) at the legacy branch. The audit's intent is preserved -- the legacy discriminator (`opt->type() == coEnum`) stays inline AND the legacy branch carries the documented mapping via the helper. The helper declaration + definition are independently locked by the Phase 111 filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration slot.
- **Files modified:** tests/QmlUiAuditTests.cpp
- **Verification:** QmlUiAuditTests passes (78 slots including the updated Phase 107 audit + the new Phase 111 audit).
- **Committed in:** `6fad09c`

---

**Total deviations:** 2 auto-fixed (1 correctness-required production fix, 1 audit-recoupling fix).
**Impact on plan:** Both auto-fixes were required for the canonical build + ctest to pass and for FT-01 (array round-trip) to be satisfiable. No scope creep beyond closing FMAP-04 + R-01 as specified.

## Issues Encountered
- The Phase 107 test `filamentMapModeRoundTripManualPreserved` explicitly documented the array round-trip as a deferred gap in its SCOPE NOTE; the plan's FT-01 implicitly assumed it could be asserted. The first build surfaced that the array genuinely did not round-trip, requiring the additive config write (auto-fix #1 above). This is a plan-accuracy note: FT-01's "mode + maps" requirement could not be met by a test-only change.
- The Phase 107 source-audit coupled to inline comment text rather than to a structural anchor; factoring the migration broke the text-coupling (auto-fix #2 above). Lesson: prefer structural anchors (call sites, helper names) over comment-text anchors for audit slots that lock refactored code.

## Verification
**Canonical build + regression ctest (3rd run, after both auto-fixes):** `scripts/auto_verify_with_vcvars.ps1` -- exit 0.

Test results (build_111_verify3.log):
- PrepareSceneDataTests: passed
- PartPlateTests: **53 passed, 0 failed** (was 51 in Phase 107; +2 new slots: filamentMapLegacyMigrationMapsInt1ToManual, filamentMapSaveReloadRoundTrip)
- ViewModelSmokeTests: passed
- QmlUiAuditTests: **78 passed, 0 failed** (+1 new slot: filamentMapSaveReloadRoundTripCoversModesAndLegacyMigration; the updated Phase 107 audit passes)
- PreviewParserTests: passed
- E2EWorkflowTests: passed
- `git diff --check` exit 0; no non-ASCII introduced in modified source.

The new R-01 test (filamentMapLegacyMigrationMapsInt1ToManual) runs in BOTH the HAS_LIBSLIC3R and non-HAS_LIBSLIC3R test builds (it is a pure-logic test of the factored helper, no libslic3r needed).

## Next Phase Readiness
- This is the LAST WS1 phase. Phase 112 starts WS5 (per-volume ITS accessor).
- FMAP-01/02/03/04 are all closed; R-01 + R-02 are closed.
- The factored `OWzx::migrateLegacyFilamentMapMode` helper is available for any future read-side migration needs.

---
*Phase: 111-filament-map-save-reload-round-trip*
*Plan: 01*
*Completed: 2026-07-13*
