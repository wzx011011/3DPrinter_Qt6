# Phase 157: Live Multi-Plate Round-Trip ctest Fixture

**Status:** Ready to execute
**Workstream:** CLOS (PLATE-06 closure)
**Requirement:** CLOS-04

## Goal

Ship the live multi-plate round-trip ctest that Phase 152 could only source-audit
lock. Exploration revealed the harness gap was already closed —
`multiPlate3mfRoundTripPreservesState` at `tests/ViewModelSmokeTests.cpp:3117`
already runs a real `store_bbs_3mf` + `read_from_archive` round-trip via a stack
`ProjectServiceMock` + `QSignalSpy` + `QTRY_VERIFY_WITH_TIMEOUT` — but it only
covers 3 of the 5 state dimensions (count, locked, bed type). Phase 157 extends
coverage to all 5 dimensions + thumbnails per CLOS-04.

## Source-truth mapping

Upstream `Plater::load_files` + `PartPlateList` round-trips: plate count/names,
per-plate config overrides, print sequence, bed type, locked/printable flags,
and per-plate thumbnails. The Qt `ProjectServiceMock` API surfaces all of these
as plain Q_INVOKABLE setters; the 3MF writer/reader persist them.

## Plan

### Wave 1 — New sibling test slot

1. `tests/ViewModelSmokeTests.h`: declare `multiPlateFullStateRoundTrip()`
   private slot (mirrors the existing `multiPlate3mfRoundTripPreservesState`
   declaration).

2. `tests/ViewModelSmokeTests.cpp`: implement `multiPlateFullStateRoundTrip()`
   using the proven stack-`ProjectServiceMock` + `loadFile(fixtureStl)` +
   `QSignalSpy` + `QTRY_VERIFY_WITH_TIMEOUT` pattern. Build a 3-plate project:
   - rename plates Alpha / Beta / Gamma
   - per-plate config override (`setPlateScopedOptionValue(1, "layer_height", 0.25)`)
   - non-default print sequence (`setPlatePrintSequence(2, 2)` = ByObject)
   - mixed bed types (`setPlateBedType(0/1/2, 1/3/4)`)
   - mixed locked/printable flags
   - per-plate thumbnail on plate 0 via `setPlateThumbnailFromBase64` (uses a
     tiny 1x1 PNG fixture so the assertion does not depend on GL capture)
   Save → fresh `ProjectServiceMock loader` → assert all 5 dimensions + thumbnail
   round-trip. Wrap `saveProject` in try/catch (defensive against store_bbs_3mf
   writer edge cases; mirrors the existing test at line 3148-3161).

### Wave 2 — Regression slot anchor

3. `tests/QmlUiAuditTests.cpp`: add `v51MultiPlateRoundTripLiveCtest`
   source-audit slot asserting the new live ctest exists and exercises all 5
   state dimensions + thumbnail (so a future regression that drops the test or
   shrinks its coverage fails the gate).

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- ViewModelSmokeTests (with HAS_LIBSLIC3R) runs `multiPlateFullStateRoundTrip`
  and PASSes (or skips cleanly if libslic3r unavailable — QSKIP guard)
- 5/5 ctest groups PASS
- 12 v5.0 regression slots + the new v51 slot still pass

## Note on PLATE-06 source-audit slot

The existing v50PartPlateSaveReloadRegressionWired source-audit slot stays as-is
(it is the source-text contract gate); the new v51 slot anchors the LIVE test
existence + coverage breadth.
