# Milestone v3.0 Requirements — PartPlate Core

**Started:** 2026-06-25
**Milestone:** v3.0 PartPlate Core
**Goal:** Replace the mock plate shell (`int plateCount_` + parallel vectors) with a real PartPlate-equivalent data model that fully round-trips multi-plate state through 3MF and supports upstream-equivalent multi-plate slice scheduling.

**Scope decision:** AssembleView deferred to v3.1 (it depends on the PartPlate data model landing first, and is a from-scratch implementation — separating it keeps v3.0 focused and lower-risk).

**Status terms** (per `.codex/rules/source-truth-migration.md`): Real / Hybrid / Mock / Blocked / Placeholder.

**Gap analysis basis:** `.planning/audits/2026-06-25-partplate-assembleview-gap.md`

---

## Status Terms (in force during v3.0)

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, protocol, credential, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

---

## v1 Requirements (this milestone)

### PartPlate Data Model

- [ ] **PLATE-01**: Qt6 has a real `PartPlate`-equivalent value object (geometry: origin/width/depth/height, instance-pair membership `set<pair<int,int>>`, per-plate config map, slice state machine: ready/valid/export-ready) that replaces the `int plateCount_` + parallel `QList` vector shell in `ProjectServiceMock`.
  - **Current status:** Mock (`ProjectServiceMock.h:336-361` — parallel vectors, no `PartPlate` class; upstream `PartPlate.hpp:77-557` has the full object).
  - **Gap evidence:** audit §1 (data model gap).
- [ ] **PLATE-02**: Qt6 has a `PartPlateList`-equivalent that owns the real plate objects, tracks current plate, and is the single source of truth for plate state (replacing scattered `plateNames_/plateLockedStates_/plateBedTypes_/...` vectors).
  - **Current status:** Mock.
  - **Gap evidence:** audit §1.

### Plate Lifecycle

- [ ] **PLATE-03**: User can clone/duplicate a plate (copies plate state + moves objects to relative positions in the new plate + rebuilds instance membership), matching upstream `PartPlateList::duplicate_plate` (`PartPlate.cpp:4484`).
  - **Current status:** Missing (no `clonePlate`/`duplicatePlate` API exists).
  - **Gap evidence:** audit §2.
- [ ] **PLATE-04**: User can reorder plates (move plate to a new index), matching upstream `PartPlateList::move_plate_to_index` (`PartPlate.cpp:4895`).
  - **Current status:** Missing (no `movePlate`/`reorderPlate` API exists).
  - **Gap evidence:** audit §2.
- [ ] **PLATE-05**: User can mark a plate as printable/non-printable, and plates support upstream "shared/unprintable plate" semantics.
  - **Current status:** Missing (Qt6 has per-object printable only).
  - **Gap evidence:** audit §2.
- [ ] **PLATE-06**: Existing plate operations (add/delete/rename/lock/select) are re-backed by the real PartPlate data model (not the parallel-vector mock) and preserve their current QML/viewmodel behavior.
  - **Current status:** Mock-backed (real API, mock implementation).
  - **Gap evidence:** audit §2.

### 3MF Multi-Plate Persistence

- [ ] **PLATE-07**: Saving a multi-plate project (real `store_bbs_3mf` path) writes all plate state (names, locked, bed type, print sequence, spiral mode, filament maps, layer sequences, per-plate config overrides) via a Qt6-side `store_to_3mf_structure` equivalent that populates `PlateData` before `store_bbs_3mf`.
  - **Current status:** Mock/broken — `saveProject` real path (`ProjectServiceMock.cpp:4491-4654`) serializes only `model_`, never populates `PlateData`. **This is the PLATE-02 blocker from v2.9 gap analysis.**
  - **Gap evidence:** audit §6.
- [ ] **PLATE-08**: Loading a 3MF project restores all plate state (names, locked, bed type, print sequence, spiral, filament maps, layer sequences, config overrides), not just names + object membership.
  - **Current status:** Partial — `loadProject` (`:4656-4803`) restores plate names + object→plate membership but not locked/bed-type/sequence/spiral/overrides.
  - **Gap evidence:** audit §6.
- [ ] **PLATE-09**: A multi-plate project round-trips through save→load with no loss (deterministic test: build multi-plate project → save → reload → assert plate count, names, locked, bed types, sequences, overrides preserved).
  - **Current status:** Broken (will collapse to defaults / possibly single plate).
  - **Gap evidence:** audit §6. **Requires runtime verification.**

### Per-Plate Slice Scheduling

- [ ] **PLATE-10**: Per-plate config overrides are read from the real PartPlate config map during slicing (replacing the current "hand-patch 3 hardcoded keys: curr_bed_type/print_sequence/spiral_mode" at `SliceService.cpp:379-403`), so arbitrary upstream overrides (filament_maps, print_compatible_*, layer sequences) are honored.
  - **Current status:** Mock — `setPlateScopedOptionValue` returns `false` under `HAS_LIBSLIC3R` (`ProjectServiceMock.cpp:1336`); slicer injects 3 keys only.
  - **Gap evidence:** audit §4.
- [ ] **PLATE-11**: Slice-all iterates plates using per-plate slice context (matching upstream `update_slice_context_to_current_plate` semantics), producing correct G-code for locked plates, empty plates, multi-instance objects, and per-plate wipe-tower logic.
  - **Current status:** Hybrid — real clone→slice loop exists but re-slices filtered global-model clones, not per-plate `Print` objects.
  - **Gap evidence:** audit §3.

### Verification Gate

- [ ] **PLATE-12**: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passes after v3.0 PartPlate work, including build, app smoke launch, QML UI audit, CLI/E2E targets, and E2E pipeline.
  - **Current status:** Passing (v2.9 baseline). Must remain green throughout v3.0.
- [ ] **PLATE-13**: `ViewModelSmokeTests.exe` is run explicitly and includes deterministic coverage for: real PartPlate data model invariants, clone/duplicate/reorder lifecycle, 3MF multi-plate round-trip (PLATE-09), and per-plate config override injection (PLATE-10).
  - **Current status:** 32 passing (v2.9 baseline); v3.0 must extend coverage.
- [ ] **PLATE-14**: Each completed v3.0 requirement has source, test, or manual-verification evidence linked in this file's traceability section.
  - **Current status:** Pending (filled as phases complete).

---

## Future Requirements (deferred to v3.1+)

- **PLATE-15** (v3.1): Non-placeholder AssembleView — bird's-eye multi-plate layout canvas (`CanvasAssembleView` equivalent) and/or volume-assembly gizmo (`GLGizmoAssembly` equivalent). Currently a static `Text { "装配视图暂不可用" }` placeholder in `Plater.qml:99-112`.
- **PLATE-16** (v3.1+): Multi-plate arrangement integration (`preprocess_arrange_polygon*`, `compute_plate_index`, `rebuild_plates_after_arrangement`, `PartPlate.cpp` ~600 LOC) when arrange-on-multi-plate is needed.
- **PLATE-17** (v3.1+): Per-plate wipe-tower estimation + extruder-area geometry + bounding-box/outside-plate detection (`PartPlate.cpp` ~500 LOC).
- **PLATE-18** (v3.1+): Per-plate multi-thumbnail kinds (top/pick/no_light/obj_preview/cali) replacing the flat-color PNG synthesizer (`ProjectServiceMock.cpp:3820`).
- **PLATE-19** (future): Filament map UI per plate (`get_filament_maps`/`set_filament_maps`/`filament_map_mode`).

---

## Out of Scope (this milestone)

- **AssembleView implementation** — deferred to v3.1 (PLATE-15). It is a from-scratch implementation that depends on the PartPlate data model landing first. Including it would make v3.0 too large and too risky.
- **Mesh boolean / cut surface re-enablement** — blocked by CGAL version (need 5.6+, have 5.4), unrelated to PartPlate (uses CGAL, not OCCT). Future milestone.
- **Support paint / hollow gizmos** — blocked by OpenVDB link failure. Future milestone.
- **Modifying libslic3r slicing algorithms** — out of GUI migration scope per `.codex/rules/source-truth-migration.md`; v3.0 uses libslic3r's `Print`/`PlateData`/`store_bbs_3mf`/`read_from_archive` APIs as-is.
- **Upstream `PartPlate` GL rendering / wxWidgets dialog code** — ~65% of upstream PartPlate.cpp is GL/wx rendering that Qt6 does not need (Qt6 has its own QML/QtQuick renderer). Only the ~35% data/lifecycle/IO layer is in scope.

---

## Traceability (filled by roadmap)

| Requirement | Phase | Status |
|---|---|---|
| PLATE-01 | Phase 16 | Not started |
| PLATE-02 | Phase 16 | Not started |
| PLATE-03 | Phase 17 | Not started |
| PLATE-04 | Phase 17 | Not started |
| PLATE-05 | Phase 17 | Not started |
| PLATE-06 | Phase 16 | Not started |
| PLATE-07 | Phase 18 | Not started |
| PLATE-08 | Phase 18 | Not started |
| PLATE-09 | Phase 18 | Not started |
| PLATE-10 | Phase 19 | Not started |
| PLATE-11 | Phase 19 | Not started |
| PLATE-12 | Phase 20 | Not started |
| PLATE-13 | Phase 20 | Not started |
| PLATE-14 | Phase 20 | Not started |

**Coverage:** 14 total · 14 mapped · 14 to complete · 0 unmapped.
