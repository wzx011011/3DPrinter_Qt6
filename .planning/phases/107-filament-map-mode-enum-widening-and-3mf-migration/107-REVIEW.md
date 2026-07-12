---
phase: 107-filament-map-mode-enum-widening-and-3mf-migration
status: approve-with-notes
base: 6aec920
head: 6e2e770
files_reviewed:
  - src/core/model/PartPlate.h
  - src/core/services/ProjectServiceMock.cpp
  - tests/PartPlateTests.cpp
  - tests/QmlUiAuditTests.cpp
counts: {critical: 0, warning: 1, info: 3, total: 4}
ctest: PartPlateTests 51/51 + QmlUiAuditTests 74/74 + ViewModelSmokeTests 97/97 pass
---

# Phase 107 Code Review — Filament-Map Enum Widening + 3MF Migration

## Verdict: approve-with-notes

All 9 FM truths satisfied. The StoreParams::config null fix is clean (verified `bbs_3mf.hpp:234,246` lacks a default initializer — the wild-pointer claim is real; nulling at the call site is the correct remediation since the struct is unmodifiable third_party code). The parallel `setInt`-instead-of-`set_key_value` fix addresses a separate crash vector. FM-03 migration logic is correct by inspection.

## Findings

| # | Severity | Finding |
|---|----------|---------|
| R-01 | warning (medium) | **FM-03 legacy-int-1 → fmmManual migration has no runtime test coverage.** The round-trip test only exercises the trusted `coEnum` branch because the write side now produces typed values. The legacy discriminator branch (the actual headline fix) is never executed at runtime. Code is correct by inspection; coverage gap is real but not a correctness issue. **Defer to Phase 111 (FMAP-04 full round-trip): add a pre-v4.5 fixture file with raw-int-1 OR a unit test that constructs a non-coEnum int option=1 and feeds it through the migration predicate.** |
| R-02 | info (low) | `setFilamentMapMode(int)` overload at PartPlate.h:232 performs no range validation — a QML/Q_INVOKABLE caller passing mode=5 would silently store invalid enum, masked by the write-site `default:` case. Currently theoretical (only caller is the migrated read path producing 0/2/3; no QML caller exists yet). **Defer to Phase 110** (FilamentGroupPopup wires the Q_INVOKABLE boundary) — add `Q_ASSERT(mode >= 0 && mode <= 3)` or clamp to fmmDefault for out-of-range. |
| R-03 | info (nit) | Duplicate `pendingPlateFilamentMaps_/Mode_` `.clear()` calls at ProjectServiceMock.cpp:591-594 (loadFile path) — harmless (idempotent) but indicates the block was edited without cleanup; sibling loadProject path has clean single calls. Also a 4-space under-indentation on :591-592 that `git diff --check` tolerated. **Optional cleanup.** |
| R-04 | info | Plan/CONTEXT over-specified EditorViewModel caller updates (FM-04) — grep returned ZERO filamentMapMode/setFilamentMapMode matches in EditorViewModel.h/.cpp. The only caller is ProjectServiceMock itself. Plan-accuracy note; no code issue. |

## Verified

- **StoreParams::config null fix (clean, not masking):** `bbs_3mf.hpp:234` declares `DynamicPrintConfig* config;` with NO default initializer; `:246 StoreParams() {}` does NOT zero it. Wild-pointer claim accurate. Call-site nulling is correct since the struct is unmodifiable third_party code. Qt6 has no preset-bundle write path yet, so skipping the writer's config branch is the CORRECT behavior.
- **FM-03 migration logic:** `opt->type() == Slic3r::coEnum` discriminator correctly separates typed-enum values (trusted via getInt()) from legacy raw ints (0→fmmAutoForFlush, 1→fmmManual, else→fmmDefault). Present in BOTH load paths (loadFile :640-658 apply :908; loadProject :5502-5530 apply :5735). The loadFile wiring was the Rule-2 deviation auto-fixed during verification — without it the saved mode would have been silently dropped.
- **FM-02 write side:** `option("filament_map_mode", true)->setInt(int(writeMode))` writes through the def-created ConfigOptionEnumGeneric; fmmDefault resolved to fmmAutoForFlush before persist (avoids the writer's `names[3]` OOB — verified PrintConfig.cpp:579-583 has only 3 enum strings).
- **FM-07:** fmmDefault documented as inherit-sentinel, not a 4th radio (PartPlate.h:84-101 cites upstream PartPlate::get_real_filament_map_mode).
- **FM-04:** all callers updated (only ProjectServiceMock; no EditorViewModel/QML callers exist).

## Conclusion

Phase 107 ships a correct FMAP-02 implementation. The single MEDIUM finding (R-01) is a test-coverage gap, not a code defect — the migration logic is right by inspection but deserves runtime coverage, routed to Phase 111 (FMAP-04). R-02 (enum range validation) routed to Phase 110 (the UI phase that adds the Q_INVOKABLE boundary). R-03 (duplicate clear) optional cleanup. The pre-existing StoreParams::config wild pointer fix is a clean defensive remediation that surfaced incidentally and is correctly characterized as a hazard for future store_bbs_3mf callers.

Regression ctest 4/4-equivalent pass (PartPlateTests 51/51, QmlUiAuditTests 74/74, ViewModelSmokeTests 97/97 — incl. the previously-SEGFAULTing `multiPlate3mfRoundTripPreservesState` now fixed by the StoreParams null). `git diff --check` exit 0.
