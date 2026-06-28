# Requirements: OWzx Slicer v3.3 Slice Preview Main Flow MVP

**Defined:** 2026-06-28
**Status:** In progress - Phase 35 complete
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, protocol, credential, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## v3.3 Requirements

### Main Flow

- [x] **FLOW-01:** A user-facing Prepare page slice request can complete with the committed real STL fixture or an equivalent loaded model, producing a real G-code output path.
- [x] **FLOW-02:** After slicing succeeds, the app switches to Preview automatically or presents Preview as the immediate completion action without requiring hidden/debug steps.
- [x] **FLOW-03:** The Preview page shows current sliced G-code state instead of an empty placeholder when G-code preview data exists.

### G-code Preview Data

- [x] **GCODE-01:** The parser handles G0/G1 travel and extrusion moves, absolute and relative extrusion modes (`M82`/`M83`), extrusion reset (`G92 E`), layer Z detection, and tool changes (`Tn`).
- [x] **GCODE-02:** PreviewViewModel builds non-empty preview payload, layer count, move count, layer range, and move range from sliced G-code.
- [x] **GCODE-03:** MVP color classification supports at least line type and tool/extruder distinctions, with travel visibility honored.

### Rendering and Interaction

- [x] **RENDER-01:** Default Windows Preview uses the D3D11 QRhi viewport on capable systems and renders nonblank G-code segments after slicing.
- [x] **RENDER-02:** Layer and move controls update the rendered range without crashing, freezing, or resizing the layout unexpectedly.
- [x] **RENDER-03:** Preview remains responsive on the committed STL fixture and a larger generated or fixture G-code sample.

### Verification

- [x] **TEST-01:** Regression coverage exercises the UI-facing slice-to-preview path and asserts output path, preview data, layer count, and move count.
- [x] **TEST-02:** Parser fixture tests cover absolute extrusion, relative extrusion, `G92 E`, layer Z, travel/extrude split, and tool changes.
- [x] **TEST-03:** QML/UI audit or smoke coverage guards D3D11 default Preview bindings and prevents silent fallback to `SoftwareViewport` as the normal path.

## Deferred From v3.2

- **THUMB-03:** Real GL/QRhi-capture thumbnails and 3MF pixel round-trip.
- **FIXTURE-02:** Full PLATE-09 save/reload assertions after shared 3MF writer integration is fixed.
- **ASSEMBLE-01:** Non-placeholder AssembleView.
- **FMAP-04:** Auto filament-map recommendation.
- **WT-01:** Wipe-tower geometry and rendering.
- **D3D12-01:** D3D12 crash root cause.
- **CLI-FIXTURE-01:** Restore missing CLI fixtures (`tests/profiles/hotend.stl`, `tests/test_models/Block20XY.stl`).

## Out of Scope

- AssembleView implementation.
- Real thumbnail capture and 3MF pixel round-trip.
- Auto filament-map recommendation.
- Wipe-tower geometry/rendering.
- D3D12 default promotion or Vulkan backend promotion.
- Full upstream Preview parity beyond the MVP motion, layer, tool, travel, and range behavior listed above.

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| FLOW-01 | Phase 33 | Complete |
| FLOW-02 | Phase 33 | Complete |
| FLOW-03 | Phase 33 | Complete |
| GCODE-01 | Phase 34 | Complete |
| GCODE-02 | Phase 34 | Complete |
| GCODE-03 | Phase 34 | Complete |
| RENDER-01 | Phase 35 | Complete |
| RENDER-02 | Phase 35 | Complete |
| RENDER-03 | Phase 35 | Complete |
| TEST-01 | Phase 33, Phase 36 | Complete for Phase 33; final coverage review in Phase 36 |
| TEST-02 | Phase 34, Phase 36 | Complete for Phase 34; final coverage review in Phase 36 |
| TEST-03 | Phase 35, Phase 36 | Complete for Phase 35; final coverage review in Phase 36 |

**Coverage:** 12 total; 12 complete; 0 partial; final milestone handoff still planned.
