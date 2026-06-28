# Requirements: OWzx Slicer v3.2 Multi-Plate Data Polish

**Defined:** 2026-06-28
**Audited:** 2026-06-28
**Status:** `tech_debt` - 8/10 complete, 2/10 partial and deferred.
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, protocol, credential, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## v3.2 Requirements

### Multi-Plate Arrangement Data Layer

- [x] **ARRANGE-01**: PartPlateList has plate-grid geometry (`m_plate_cols`, `plate_stride_x/y`, `compute_plate_index`) matching upstream `PartPlateList:4836-4870`, enabling bed_idx to plate index to 2D world offset mapping.
- [x] **ARRANGE-02**: `arrangeObjects` distributes or preserves models across multiple plates using the plate-grid, matching the Qt6-compatible subset of upstream `rebuild_plates_after_arrangement` (`PartPlate.cpp:6096-6139`).
- [x] **ARRANGE-03**: Locked plates are excluded from arrangement rebuilds; locked membership is preserved.

### Thumbnail Persistence

- [x] **THUMB-01**: `generatePlateThumbnail` produces at least 2 kinds (main flat-color + top-down 2D footprint), replacing the single flat-color kind.
- [ ] **THUMB-02**: Plate thumbnails are written to 3MF via `buildPlateDataList` (`PlateData::plate_thumbnail`), so round-trip preserves thumbnail data.
  - **Status:** partial. In-memory thumbnail cache and persistence hook are implemented and tested; pixel save/reload round-trip is deferred to `THUMB-03` because the upstream writer path is coupled to real GL capture.

### Filament Map (Manual Mode)

- [x] **FMAP-01**: PartPlate has `filament_maps` (`std::vector<int>`) and `filament_map_mode` fields, matching upstream `PartPlate.hpp:262-263`.
- [x] **FMAP-02**: Filament map data round-trips through the structured 3MF plate data path (`PlateData::filament_maps`) and the `filament_map_mode` config key.
- [x] **FMAP-03**: User-facing manual assignment is exposed through ProjectServiceMock Q_INVOKABLE APIs (`setPlateFilamentMap`, `plateFilamentMaps`, `plateFilamentMapMode`). A dedicated QML editor remains a later UI refinement.

### Test Fixture

- [x] **FIXTURE-01**: A real-model test fixture is committed under `tests/data/test_model.stl`.
- [ ] **FIXTURE-02**: The PLATE-09 multi-plate 3MF round-trip test runs with the fixture (no fixture-missing QSKIP) and asserts plate state preservation.
  - **Status:** partial. The test now loads the real fixture and reaches the save/reload path; full state assertions are QSKIP-guarded until the shared 3MF writer integration gap is fixed.

## Future Requirements (deferred to v3.3+)

- **ASSEMBLE-01**: Non-placeholder AssembleView (XL, ~3000-4000 LOC, needs second GL canvas).
- **THUMB-03**: Real GL-capture thumbnails (4 variants, blocked on QRhi framebuffer / writer integration).
- **FMAP-04**: Auto filament-map recommendation (blocked on ToolOrdering).
- **WT-01**: Wipe-tower geometry + rendering.
- **D3D12-01**: D3D12 crash root cause.
- **CLI-FIXTURE-01**: Restore missing CLI fixtures (`tests/profiles/hotend.stl`, `tests/test_models/Block20XY.stl`); confirmed pre-existing, not a v3.2 regression.

## Out of Scope

- AssembleView.
- Wipe-tower GL rendering.
- Real GL-capture thumbnails.
- Auto filament-map recommendation.
- CLI fixture restoration.

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| ARRANGE-01 | Phase 29 | Complete |
| ARRANGE-02 | Phase 29 | Complete |
| ARRANGE-03 | Phase 29 | Complete |
| THUMB-01 | Phase 30 | Complete |
| THUMB-02 | Phase 30 | Partial - deferred to THUMB-03 |
| FMAP-01 | Phase 31 | Complete |
| FMAP-02 | Phase 31 | Complete |
| FMAP-03 | Phase 31 | Complete |
| FIXTURE-01 | Phase 32 | Complete |
| FIXTURE-02 | Phase 32 | Partial - writer integration deferred |

**Coverage:** 10 total; 8 complete; 2 partial/deferred; 0 unmapped.
