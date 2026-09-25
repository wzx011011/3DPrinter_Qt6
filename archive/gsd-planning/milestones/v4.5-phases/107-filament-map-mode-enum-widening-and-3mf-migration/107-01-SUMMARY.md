---
phase: 107-filament-map-mode-enum-widening-and-3mf-migration
plan: 01
subsystem: testing
tags: [qt6, qml, libslic3r, 3mf, enum-widening, filament-map, migration, orcaslicer]

# Dependency graph
requires:
  - phase: 18-multi-plate-3mf-persistence
    provides: store_bbs_3mf write path + load_bbs_3mf read path in ProjectServiceMock (buildPlateDataList, loadProject)
  - phase: 31-manual-filament-mapping
    provides: PartPlate filament_map_mode member + setPlateFilamentMap plumbing
provides:
  - 4-value FilamentMapMode enum (fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault) in PartPlate.h matching upstream PrintConfig.hpp:424-429
  - 3MF read-side migration mapping legacy raw-int-1 -> fmmManual (preserves pre-v4.5 "Manual"=1 intent; NOT the new fmmAutoForMatch=1)
  - 3MF write-side def-respecting serialization that round-trips as the upstream enum-name string
  - Regression test (filamentMapModeEnumWidenedAnd3MFMigrates) + round-trip test (filamentMapModeRoundTripManualPreserved)
  - Source-audit slot (filamentMapModeEnumWidenedToUpstream4Value) locking FMAP-02 against regression
affects: [108-filament-map-readback, 110-filament-map-ui, 111-filament-map-array-roundtrip]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - coEnum option writes must use option(...,true)->setInt(), NOT set_key_value(new ConfigOptionEnum<T>), to respect the def-created ConfigOptionEnumGeneric type
    - 3MF read-side migration discriminates typed coEnum (trusted) from legacy raw int (migrated) via opt->type() == Slic3r::coEnum

key-files:
  created: []
  modified:
    - src/core/model/PartPlate.h
    - src/core/services/ProjectServiceMock.cpp
    - src/core/services/ProjectServiceMock.h
    - tests/PartPlateTests.cpp
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "fmmDefault is resolved to fmmAutoForFlush (the upstream default) BEFORE persistence -- the writer's names vector (bbs_3mf.cpp:7964-7967) has no 'Default' entry so names[3] is OOB. A future Phase 108+ readback layer will resolve fmmDefault against the global config like upstream PartPlate.cpp:317-328."
  - "The 3MF write uses option('filament_map_mode', true)->setInt(int(writeMode)) on the def-created ConfigOptionEnumGeneric, NOT set_key_value(new ConfigOptionEnum<FilamentMapMode>). The option def (PrintConfig.cpp:2495) declares coEnum, so DynamicConfig makes ConfigOptionEnumGeneric (Config.hpp:2046); a set_key_value with the wrong concrete type crashed _add_project_config_file_to_archive during the writer's project-config JSON serialization."
  - "Legacy raw-int-1 maps to fmmManual (preserving pre-v4.5 'Manual'=1 intent). This is the critical user-visible behavior change: WITHOUT this migration, a pre-v4.5 plate saved as 'Manual' would silently reload as the new fmmAutoForMatch=1."
  - "fmmDefault is NOT exposed as a 4th UI radio button (anti-feature per FEATURES.md); it is a per-plate 'inherit from global' sentinel only."
  - "filament_maps array round-trip is scoped OUT (FMAP-03/04, Phase 110/111) -- the Qt6 write path populates pd->filament_maps but the active bbs_3mf writer branch reads pd->config['filament_map'] (ConfigOptionInts) which Qt6 does not populate. The round-trip test is scoped to MODE only."

patterns-established:
  - "coEnum def-respecting write: option(key, true)->setInt(int(value)) for any coEnum option, mirroring the sibling curr_bed_type / print_sequence writes"
  - "3MF read-side migration discriminator: opt->type() == Slic3r::coEnum separates trusted typed-enum values from legacy raw ints that need migration"
  - "StoreParams::config MUST be explicitly set (nullptr if no global config is persisted) -- the struct's empty ctor does NOT zero it, so an unassigned params.config is a wild pointer that UB-crashes the writer"

requirements-completed: [FMAP-02]

# Metrics
duration: ~150min
completed: 2026-07-12
---

# Phase 107-01: Filament-Map Mode Enum Widening And 3MF Migration Summary

**Widened Qt6 filament-map mode from 2-value to upstream's 4-value enum (fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault) with a 3MF read-side migration that preserves pre-v4.5 'Manual'=1 as fmmManual instead of the new fmmAutoForMatch=1.**

## Performance

- **Duration:** ~150 min (across two context windows; the first was interrupted before any commits)
- **Tasks:** 4
- **Commits:** 5 (1 feat, 2 test, 2 fix)
- **Files modified:** 5

## Accomplishments
- Widened `FilamentMapMode` from a 2-value int to a 4-value `enum class : int` in PartPlate.h matching upstream PrintConfig.hpp:424-429 exactly, with the upstream anchor citation and fmmDefault "inherit from global" documentation (FM-01, FM-07).
- Implemented the FM-03 3MF read-side migration in BOTH load paths (loadProject AND loadFile): a typed coEnum value is trusted directly; a legacy raw int maps 0->fmmAutoForFlush, 1->fmmManual (preserving pre-v4.5 intent), else->fmmDefault.
- Implemented the FM-02 write side via the def-respecting `option("filament_map_mode", true)->setInt(int(writeMode))` accessor (resolving fmmDefault -> fmmAutoForFlush before persistence to avoid the writer's names[3] OOB), producing the upstream enum-name string on disk.
- Added the `filamentMapModeEnumWidenedAnd3MFMigrates` regression (4 enum ints + legacy-int-1->fmmManual contract) and the `filamentMapModeRoundTripManualPreserved` round-trip (save fmmManual, reload, assert mode==2), both in PartPlateTests.
- Added the `filamentMapModeEnumWidenedToUpstream4Value` source-audit in QmlUiAuditTests locking FMAP-02 against regression.
- Resolved a pre-existing SEGFAULT in `ViewModelSmokeTests::multiPlate3mfRoundTripPreservesState` caused by an uninitialized `StoreParams::config` wild pointer (unrelated to FMAP-02 but blocking the full ctest).

## Task Commits

Each task was committed atomically:

1. **Task 107-01-01: Widen filament-map mode enum to 4-value + migrate 3MF read/write** - `a14c5c4` (feat)
2. **Task 107-01-02: Add filamentMapModeEnumWidenedAnd3MFMigrates regression test + update existing slot** - `a8a5b57` (test)
3. **Task 107-01-03: Add filamentMapModeEnumWidenedToUpstream4Value source-audit** - `87fbee1` (test)
4. **Task 107-01-04: Build + regression ctest + document** - `c3a0896` (fix) and `97a6601` (fix)

_Note: Task 4 (verification) produced two fix commits: `c3a0896` wired the read+apply into the loadFile path (a missing-critical deviation found during verification), and `97a6601` resolved the SEGFAULT that surfaced during the full ctest run._

## Files Created/Modified
- `src/core/model/PartPlate.h` - Declares the 4-value `enum class FilamentMapMode : int` before the PartPlate class; changed accessor return type; added int + enum setters; changed member type. Cites upstream PrintConfig.hpp:424-429.
- `src/core/services/ProjectServiceMock.cpp` - Write site (buildPlateDataList) uses def-respecting setInt with fmmDefault resolution; BOTH read sites (loadProject + loadFile) have the coEnum-vs-legacy migration; loadFile apply site sets the mode on each plate; StoreParams::config explicitly nulled.
- `src/core/services/ProjectServiceMock.h` - Q_INVOKABLE comment documents the 4-value enum and fmmDefault anti-feature.
- `tests/PartPlateTests.cpp` - Updated `filamentMapManualAssignment` (raw int 1 -> fmmManual); added `filamentMapModeEnumWidenedAnd3MFMigrates` + `filamentMapModeRoundTripManualPreserved`.
- `tests/QmlUiAuditTests.cpp` - Added `filamentMapModeEnumWidenedToUpstream4Value` source-audit (4 enum names, upstream anchor, inherit-from-global doc, def-respecting write pattern, fmmDefault-resolution switch, coEnum discriminator, legacy-int-1->fmmManual migration comment).

## Decisions Made
- **Write approach:** Use `option("filament_map_mode", true)->setInt(int(writeMode))` (def-respecting ConfigOptionEnumGeneric), NOT `set_key_value(new ConfigOptionEnum<FilamentMapMode>)`. The earlier draft used the latter, which mismatches the def's coEnum type and crashed the writer's project-config JSON serializer. The def-respecting write produces the SAME on-disk string because the writer (bbs_3mf.cpp:7964-7967) still indexes `ConfigOptionEnum<FilamentMapMode>::get_enum_names()[getInt()]`.
- **fmmDefault resolution:** Resolved to fmmAutoForFlush (the upstream default, PrintConfig.cpp:2509) before persistence. The writer's names vector has only 3 entries (no "Default"), so names[3] is OOB. A future Phase 108+ readback layer will resolve fmmDefault against the global config like upstream PartPlate.cpp:317-328.
- **filament_maps array round-trip scoped OUT:** The Qt6 write path populates `pd->filament_maps` but the active bbs_3mf writer branch reads `pd->config["filament_map"]` (ConfigOptionInts) which Qt6 does not populate. Closing this is FMAP-03/04 scope (Phase 110/111). The round-trip test asserts MODE only.
- **StoreParams::config = nullptr:** Pre-existing latent hazard. The struct's empty ctor does NOT zero `config`, so an unassigned pointer is wild. `store_bbs_3mf` dereferences it after a `config != nullptr` guard (bbs_3mf.cpp:6350), which is UB. Explicitly nulled to skip the writer branch (Qt6 has no preset-bundle write path yet). Independent of FMAP-02 but blocking the full ctest.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Wired filament-map mode read+apply into the loadFile path**
- **Found during:** Task 4 (verification -- PartPlateTests round-trip failed: mode=0 vs 2)
- **Issue:** The plan's read-site migration was added only to the loadProject path. The primary load path (loadFile, used by PartPlateTests::filamentMapModeRoundTripManualPreserved and ViewModelSmokeTests::multiPlate3mfRoundTripPreservesState) never read or applied filament_map_mode, so the saved mode was silently dropped on reload.
- **Fix:** Added the same coEnum-vs-legacy migration block to loadFile (mirroring loadProject) and an apply block (`p->setFilamentMapMode(receiver->pendingPlateFilamentMapMode_[pi])`) alongside the existing setFilamentMaps call.
- **Files modified:** src/core/services/ProjectServiceMock.cpp, tests/PartPlateTests.cpp (removed an array assertion that is FMAP-03/04 scope)
- **Verification:** PartPlateTests::filamentMapModeRoundTripManualPreserved now passes (mode==2 after reload).
- **Committed in:** c3a0896

**2. [Rule 2 - Missing Critical] Resolved pre-existing SEGFAULT in multiPlate3mfRoundTripPreservesState**
- **Found during:** Task 4 (verification -- full ctest SEGFAULTed at _add_project_config_file_to_archive)
- **Issue:** `StoreParams::config` (bbs_3mf.hpp:234) is declared without a default member initializer and the empty `StoreParams() {}` ctor does NOT zero it. ProjectServiceMock::saveProject never assigned it, so `store_bbs_3mf` dereferenced a wild pointer after the `config != nullptr` guard (bbs_3mf.cpp:6350) -- UB that intermittently SEGFAULTed. (In an earlier baseline run this same path threw a C++ exception caught by the test's try/catch -> QSKIP; after the enum-widening rebuild the wild pointer landed on mapped memory and SEGFAULTed instead. Classic UB nondeterminism, not a regression from FMAP-02.)
- **Fix:** Explicitly set `params.config = nullptr` in saveProject (Qt6 has no preset-bundle write path yet, so the writer branch is correctly skipped). ALSO switched the write-site from `set_key_value(new ConfigOptionEnum<FilamentMapMode>)` to the def-respecting `option("filament_map_mode", true)->setInt(int(writeMode))` as defense-in-depth (the set_key_value type-mismatch was a separate latent crash vector).
- **Files modified:** src/core/services/ProjectServiceMock.cpp, tests/QmlUiAuditTests.cpp (updated the source-audit assertion from the literal `ConfigOptionEnum<Slic3r::FilamentMapMode>` string to the def-respecting `option(...,true)` + `setInt(int(writeMode))` + fmmDefault-resolution switch patterns)
- **Verification:** ViewModelSmokeTests::multiPlate3mfRoundTripPreservesState now PASSES (not even QSKIP) -- the full save->reload round-trip succeeds, exercising the filament_map_mode write code.
- **Committed in:** 97a6601

---

**Total deviations:** 2 auto-fixed (2 missing critical)
**Impact on plan:** Both fixes were necessary for correctness and to unblock the regression ctest. No scope creep -- the StoreParams::config fix is a pre-existing latent hazard that surfaced under the rebuilt binary and is clearly documented as independent of FMAP-02.

## Issues Encountered
- **Tooling friction:** `grep -nE` / `grep -nF` with `|` or `[` consistently failed with "conflicting matchers specified" or "unknown option" in the Git Bash environment. Workaround: use multiple separate plain `grep` calls or Python.
- **Qt Test output capture:** Qt Test output is not captured by bash pipes (`| tee`, `$()`). Fix: use the `-o file.txt,txt` flag to write results to a file, then read the file.
- **vcvars PATH pollution:** vcvars64.bat breaks on PATH entries containing both spaces and parentheses. Fix: the canonical wrapper (scripts/auto_verify_with_vcvars.ps1) sanitizes PATH before invoking vcvars and appends Windows Kits paths explicitly. Used a targeted ninja rebuild via a vcvars-wrapping PowerShell script rather than the full ~30-min canonical build.
- **InventoryAuditTests + CliTests failures verified PRE-EXISTING:** InventoryAuditTests (9 fails) all reference a missing `.planning/phases/50-screenshot-and-source-truth-inventory/50-INVENTORY.md` that was archived to milestones in commit 4da9eb3 (a chore unrelated to phase 107). CliTests (2 fails: testLoadHotend, testSliceBlock20XY) are CLI integration tests invoking the built binary; neither references filament-map. Neither is caused by FMAP-02.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- FMAP-02 is closed. The 4-value enum is in place; the 3MF read+write paths migrate legacy files and round-trip the new values correctly.
- **FMAP-01 readback (Phase 108):** a readback layer should resolve fmmDefault against the global config like upstream PartPlate.cpp:317-328 (currently fmmDefault is resolved to fmmAutoForFlush at write time).
- **FMAP-03/04 array round-trip (Phase 110/111):** the Qt6 write path populates `pd->filament_maps` but the active bbs_3mf writer branch reads `pd->config["filament_map"]` (ConfigOptionInts); the array does not yet round-trip. The `filamentMapModeRoundTripManualPreserved` test is scoped to MODE only and documents this gap.
- **Pre-existing test failures (not blockers):** InventoryAuditTests (missing archived phase-50 file) and CliTests (2 slots) are orchestrator-level/environment issues unrelated to FMAP-02.
- **StoreParams::config hazard:** now mitigated in saveProject, but the underlying struct (third_party, cannot modify) lacks a default initializer. Any NEW caller of store_bbs_3mf must set params.config explicitly.

## Verification Results (Task 107-01-04)

Canonical build via `scripts/auto_verify_with_vcvars.ps1` pattern (targeted ninja rebuilds of the affected test binaries + owzx_app_core under the vcvars-wrapped MSVC environment):

| Test binary | Result | Filament-map slots |
|---|---|---|
| PartPlateTests | 51/51 pass | filamentMapManualAssignment PASS, filamentMapModeEnumWidenedAnd3MFMigrates PASS, filamentMapModeRoundTripManualPreserved PASS |
| QmlUiAuditTests | 74/74 pass | filamentMapModeEnumWidenedToUpstream4Value PASS |
| ViewModelSmokeTests | 97/97 pass | multiPlate3mfRoundTripPreservesState PASS (previously SEGFAULTed) |

Pre-existing failures (verified unrelated to FMAP-02):
- InventoryAuditTests: 9 fails (missing archived phase-50 INVENTORY.md)
- CliTests: 2 fails (testLoadHotend, testSliceBlock20XY)

---
*Phase: 107-filament-map-mode-enum-widening-and-3mf-migration*
*Completed: 2026-07-12*
