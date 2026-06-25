# Phase 18: 3MF Multi-Plate Persistence - Context

**Gathered:** 2026-06-26
**Status:** Ready for planning

<domain>
## Phase Boundary

Fix the v2.9-identified blocker: multi-plate state is lost on save. Implement the Qt6-side write path so all plate state round-trips through 3MF, and extend the load path to restore everything.

**In scope:** build `PlateDataPtrs` (a `std::vector<PlateData*>`) from `m_plateList` and feed it into `StoreParams.plate_data_list` before calling `store_bbs_3mf` (the save path); extend `loadProject`'s 3MF reconstruction to restore all plate state from the loaded `PlateDataPtrs` (the load path); add a deterministic round-trip test (PLATE-09).
**Out of scope:** filament_maps (deferred to future, Qt6 PartPlate has no field yet — PLATE-19); thumbnails (deferred — Qt6 has only the synthetic flat-color PNG, upstream has 5 kinds); modifying `store_bbs_3mf`/`load_bbs_3mf` internals (migration rule: do not modify libslic3r); slice/GCode result persistence (Phase 19).

</domain>

<decisions>
## Implementation Decisions

### Write path: build PlateDataPtrs into StoreParams (D-10)
- **D-10:** In `ProjectServiceMock::saveProject`, before calling `store_bbs_3mf(StoreParams&)`, build a `PlateDataPtrs` (vector of heap-allocated `PlateData*`) from `m_plateList`. For each plate: set `plate_index`, `plate_name`, `locked`, `objects_and_instances` (from the instance-pair membership set — already the right shape, upstream is `vector<pair<int,int>>`), and `config` (DynamicPrintConfig). Set `StoreParams.plate_data_list = builtList`. Then call `store_bbs_3mf(params)`. After, call `release_PlateData_list` to free the heap `PlateData` objects (`bbs_3mf.hpp:282`). This is the upstream-designed extension point — `StoreParams` exists precisely so callers can supply plate data.
  - This is the **PLATE-07 deliverable** (write path) and the gap-analysis blocker fix.

### What gets persisted (D-11)
- **D-11:** Round-trip retains: `plate_index`, `plate_name`, `locked`, `objects_and_instances` (object+instance pairs), `config` (DynamicPrintConfig), and **bed-type/print-sequence/spiral/layer-sequences written INTO the per-plate config** (upstream `PlateData` has no standalone fields for these — they live in config; Qt6 must write them as config keys so they survive round-trip).
  - **Skipped this phase:** `filament_maps` (Qt6 PartPlate has no field — PLATE-19 future), `thumbnails` (synthetic PNG only — future). These will round-trip as defaults/empty, documented, not a regression (they weren't persisted before either).

### Load path: restore all fields (D-12)
- **D-12:** Extend `loadProject`'s existing 3MF reconstruction (currently reads `plate->plate_name` + `plate->objects_and_instances` only) to ALSO restore `locked`, `config` (DynamicPrintConfig → PartPlate fields), and the bed-type/sequence/spiral/layer-seq keys FROM the per-plate config. Build `m_plateList` fully from the loaded `PlateDataPtrs`.
  - This is the **PLATE-08 deliverable** (full load restore).

### Round-trip test (D-13)
- **D-13:** Add a deterministic round-trip test (PLATE-09): construct a multi-plate project (≥2 plates, with names/locked/bed-type/objects), save to a temp .3mf, reload, assert plate count + names + locked + bed-type + object membership preserved. This is the gate that proves the blocker is fixed.
  - Uses a real .3mf file in a temp dir (the test must exercise the actual `store_bbs_3mf` + `load_bbs_3mf` paths, not mocks).

### Claude's Discretion
- Exact config key names for bed-type/sequence/spiral (use upstream's `curr_bed_type`, `print_sequence`, `spiral_mode` if they exist in upstream config schema; else document the chosen key names).
- Whether to write a small helper `ProjectServiceMock::buildPlateDataList()` / `restorePlateListFromPlateData()` to keep saveProject/loadProject readable.
- Temp-dir handling in the test (QStandardPaths::writableLocation or QDir::temp).

</decisions>

<canonical_refs>
## Canonical References

### Upstream Source Truth (the API to use — DO NOT modify these files)
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.hpp:227-282` — `struct StoreParams` (has `PlateDataPtrs plate_data_list` at :231), `store_bbs_3mf(StoreParams&)` at :280, `release_PlateData_list` at :282, `load_bbs_3mf` at :253 (returns `PlateDataPtrs*`).
- `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.hpp:54-122` — `struct PlateData` (plate_index, plate_name, objects_and_instances, config, locked, filament_maps). The fields to populate.
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:6160` — upstream `store_to_3mf_structure` (reference for HOW upstream populates PlateData from PartPlateList — Qt6 mirrors this logic but reads from its own m_plateList).

### Phase 16/17 Outputs (the source of plate state to persist)
- `src/core/model/PartPlate.h` — has name/locked/bedType/printSequence/spiralMode/layer-seq/config(instance membership)/isPrintable fields to read when building PlateData.
- `src/core/model/PartPlateList.h` — plateCount/plate(i)/objectIndicesOnPlate to iterate.

### Current Qt6 save/load (the code being changed)
- `src/core/services/ProjectServiceMock.cpp` `saveProject` (~line 4491+) — currently builds `StoreParams` with only `model_`; D-10 adds plate_data_list.
- `src/core/services/ProjectServiceMock.cpp` `loadProject` (~line 4656+) — currently restores names+membership only; D-12 adds locked/config/bed-type/sequence.

### Rules
- `.codex/rules/source-truth-migration.md` — do NOT modify libslic3r source; status terms.
- `.codex/rules/build-rules.md` — canonical verify command.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `Slic3r::PlateData`, `PlateDataPtrs` (= `std::vector<PlateData*>`), `StoreParams`, `release_PlateData_list` — all exported from libslic3r, available under HAS_LIBSLIC3R.
- `PartPlate::config()` returns `DynamicPrintConfig&` (Phase 16) — directly assignable to `PlateData::config` (same type).
- `PartPlate::objToInstanceSet()` returns `std::set<std::pair<int,int>>` — directly convertible to `PlateData::objects_and_instances` (`std::vector<std::pair<int,int>>`).

### Established Patterns
- `saveProject` already calls `store_bbs_3mf(params)` with `SaveStrategy::Zip64` — D-10 just adds the plate_data_list field to the existing StoreParams.
- `loadProject` already receives `plateDataList` from `read_from_archive` and reads `plate->plate_name`/`plate->objects_and_instances` — D-12 extends the same loop to read more fields.
- `#ifdef HAS_LIBSLIC3R` guards all libslic3r API usage.

### Integration Points
- `store_bbs_3mf` writes PlateData to the 3MF's `/metadata/slice_info.config` + plate XML — automatic once plate_data_list is populated.
- `release_PlateData_list` MUST be called after store to avoid leaking the heap PlateData objects.

</code_context>

<deferred>
## Deferred Ideas

- **filament_maps persistence** — Qt6 PartPlate has no filament_maps field yet (PLATE-19 future). PlateData has the field but Qt6 leaves it empty.
- **Thumbnail persistence** — Qt6 has only the synthetic flat-color PNG; upstream has 5 thumbnail kinds. Deferred (future / PLATE-18).
- **slice/GCode result persistence** — Phase 19 scope (per-plate slice results in PlateData.slice_filaments_info / gcode_file).
- **`is_printable` persistence to 3MF** — upstream PlateData has no `is_printable` field; the Qt6 printable flag is Qt-local. Phase 18 could encode it as a config key, but that diverges from upstream schema. Defer: printable is currently a session-level flag (acceptable; non-printable exclusion from slice-all is the Phase 17 deliverable, persistence is a nice-to-have not required by PLATE-07/08/09).

</deferred>

---

*Phase: 18-3mf-multi-plate-persistence*
*Context gathered: 2026-06-26*
