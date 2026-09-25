---
phase: 31
slug: filament-map-manual
status: passed
verified: 2026-06-28
requirements: [FMAP-01, FMAP-02, FMAP-03]
---

# Phase 31 Verification: Filament Map (Manual)

## Status: PASSED ✅

FMAP-01/02/03 implemented and tested (FMAP-04 Auto deferred to v3.3+).

## Requirements verification

- **FMAP-01 ✅:** PartPlate has `std::vector<int> m_filament_maps` + `int m_filament_map_mode` (0=Auto, 1=Manual) mirroring upstream PartPlate.hpp:262-263. Accessors: `filamentMaps()`/`setFilamentMaps()`/`filamentMapMode()`/`setFilamentMapMode()`.
- **FMAP-02 ✅:** buildPlateDataList populates `pd->filament_maps` + writes `filament_map_mode` as a config key (round-trips via PlateData::filament_maps at bbs_3mf.hpp:98). Load path extracts into pendingPlateFilamentMaps_/pendingPlateFilamentMapMode_ and applies in the rebuild lambda.
- **FMAP-03 ✅:** Q_INVOKABLE `setPlateFilamentMap(plateIndex, mode, maps)` + `plateFilamentMaps(plateIndex)` + `plateFilamentMapMode(plateIndex)` on ProjectServiceMock. Manual mode surface; QML can bind.

## Test verification
- `PartPlateTests::filamentMapManualAssignment` — verifies FMAP-01 fields + FMAP-03 API (set/get, invalid-index handling). ✅
- PartPlateTests → **48 passed, 0 failed**.

## Files changed
- `src/core/model/PartPlate.h` — m_filament_maps + m_filament_map_mode + accessors.
- `src/core/services/ProjectServiceMock.h` — setPlateFilamentMap/plateFilamentMaps/plateFilamentMapMode Q_INVOKABLE + pendingPlateFilamentMaps_/Mode members.
- `src/core/services/ProjectServiceMock.cpp` — method impls + buildPlateDataList population + load-path extraction + rebuild application.
- `tests/PartPlateTests.cpp` — filamentMapManualAssignment slot.
