# Requirements: OWzx Slicer v3.2 Multi-Plate Data Polish

**Defined:** 2026-06-28
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Status Terms (per `.codex/rules/source-truth-migration.md`)

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, protocol, credential, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## v3.2 Requirements

### Multi-Plate Arrangement Data Layer

- [ ] **ARRANGE-01**: PartPlateList has plate-grid geometry (`m_plate_cols`, `plate_stride_x/y`, `compute_plate_index`) matching upstream `PartPlateList:4836-4870`, enabling bed_idx → plate index → 2D world offset mapping.
- [ ] **ARRANGE-02**: `arrangeObjects` distributes models across multiple plates using the plate-grid, matching upstream `rebuild_plates_after_arrangement` (`PartPlate.cpp:6096-6139`). Objects arranged into correct plates, not all on plate 0.
- [ ] **ARRANGE-03**: Locked plates are excluded from arrangement (objects skip locked plates during auto-arrange).

### Thumbnail Persistence

- [ ] **THUMB-01**: `generatePlateThumbnail` produces at least 2 kinds (main flat-color + top-down 2D footprint), replacing the single flat-color kind.
- [ ] **THUMB-02**: Plate thumbnails are written to 3MF via `buildPlateDataList` (`PlateData::plate_thumbnail`), so round-trip preserves thumbnail data.

### Filament Map (Manual Mode)

- [ ] **FMAP-01**: PartPlate has `filament_maps` (`std::vector<int>`) and `filament_map_mode` fields, matching upstream `PartPlate.hpp:262-263`.
- [ ] **FMAP-02**: Filament map data round-trips through 3MF (`PlateData::filament_maps` already structured in bbs_3mf.hpp).
- [ ] **FMAP-03**: User can manually assign extruder→filament mapping per plate via a simple QML UI (Manual mode only; Auto recommendation deferred).

### Test Fixture

- [ ] **FIXTURE-01**: A real-model test fixture (STL or 3MF) is committed under `tests/data/`.
- [ ] **FIXTURE-02**: The PLATE-09 multi-plate 3MF round-trip test runs with the fixture (no QSKIP) and asserts plate state preservation.

## Future Requirements (deferred to v3.3+)

- **ASSEMBLE-01** (v3.3+): Non-placeholder AssembleView (XL, ~3000-4000 LOC, needs second GL canvas).
- **THUMB-03** (v3.3+): Real GL-capture thumbnails (4 variants, blocked on QRhi framebuffer).
- **FMAP-04** (v3.3+): Auto filament-map recommendation (blocked on ToolOrdering).
- **WT-01** (v3.3+): Wipe-tower geometry + rendering.
- **D3D12-01** (opportunistic): D3D12 crash root cause (v3.1 carry-forward).

## Out of Scope

- AssembleView (XL, needs second GL canvas — v3.3+).
- Wipe-tower GL rendering (needs GL framebuffer — v3.3+).
- Real GL-capture thumbnails (blocked on QRhi framebuffer — v3.3+).
- Auto filament-map (blocked on ToolOrdering — v3.3+).

## Traceability (filled by roadmap)

| Requirement | Phase | Status |
|---|---|---|
| ARRANGE-01 | Phase 29 | Not started |
| ARRANGE-02 | Phase 29 | Not started |
| ARRANGE-03 | Phase 29 | Not started |
| THUMB-01 | Phase 30 | Not started |
| THUMB-02 | Phase 30 | Not started |
| FMAP-01 | Phase 31 | Not started |
| FMAP-02 | Phase 31 | Not started |
| FMAP-03 | Phase 31 | Not started |
| FIXTURE-01 | Phase 32 | Not started |
| FIXTURE-02 | Phase 32 | Not started |

**Coverage:** 10 total · 10 to map · 0 unmapped.
