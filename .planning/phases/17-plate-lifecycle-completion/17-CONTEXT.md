# Phase 17: Plate Lifecycle Completion - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Add the three plate lifecycle operations that are currently missing entirely from the Qt6 surface, and wire them through to QML so users can use them:
1. **clone/duplicate plate** (PLATE-03) — deep copy a plate including its ModelObjects
2. **reorder plates** (PLATE-04) — move a plate to a new index
3. **per-plate printable** (PLATE-05) — mark a plate printable/non-printable; non-printable excluded from slice-all

Phase 16 left the data model (PartPlate/PartPlateList) in place with the basic lifecycle (add/delete/rename/lock/select) re-backed on it. This phase extends the model + service + viewmodel + QML with the three missing operations.

**In scope:** PartPlateList methods (clonePlate, movePlate, per-plate printable already on PartPlate as m_printable from Phase 16); ProjectServiceMock Q_INVOKABLE wrappers; EditorViewModel exposure; QML UI (buttons/menu items); tests.
**Out of scope:** slice-scheduling changes for non-printable plates beyond excluding them from the slice-all queue (full per-plate slice context is Phase 19); shared/unprintable-plate "pool" semantics beyond the basic printable flag (deferred); 3MF persistence of the new state (Phase 18 will persist everything via store_to_3mf_structure).

</domain>

<decisions>
## Implementation Decisions

### Clone/Duplicate — full ModelObject copy (D-06)
- **D-06:** `clonePlate(sourceIndex)` performs a **deep copy of ModelObjects**, fully aligning with upstream `PartPlateList::duplicate_plate` (`PartPlate.cpp:4484-4512`). The cloned plate owns independent object copies (via the existing `duplicateObject` machinery), not shared references. Source objects remain on the source plate; new copies land on the new plate.
  - This means: for each object on the source plate, call the existing `ProjectServiceMock::duplicateObject`-equivalent to create a copy in `model_->objects` + all parallel object arrays (names/positions/printable/etc.), then add the new object index(es) to the cloned plate's PartPlate instance membership.
  - The object-instance translation (upstream `translate_all_instance` by plate-to-plate offset) is deferred to when plate geometry origins are real — Phase 16 stored origins as Vec3d but didn't compute plate-to-plate layout offsets (geometry is 0/default). So the cloned objects sit at the same positions as source (acceptable for now; upstream computes origin from `compute_origin(index, cols)` which Qt6 doesn't have yet). Recorded as deferred.
  - Plate metadata (name, locked, bed type, print sequence, spiral, layer sequences, config) is copied from source to the new plate.

### Reorder — pure metadata swap (D-07)
- **D-07:** `movePlate(oldIndex, newIndex)` is a **pure metadata reordering** in `m_plate_list` (std::rotate-style element shift + reindex), mirroring upstream `move_plate_to_index` (`PartPlate.cpp:4895-4935`) MINUS the geometry origin recomputation (which Phase 16 deferred). It moves the PartPlate in the vector, reindexes all plates' `m_plate_index`, and updates `m_current_plate`. No ModelObject changes.

### Per-plate printable (D-08)
- **D-08:** PartPlate already has `m_printable` (from Phase 16). This phase adds: `PartPlateList::setPlatePrintable(index, bool)`, a `ProjectServiceMock` Q_INVOKABLE `setPlatePrintable`/`isPlatePrintable`, and excludes non-printable plates from the slice-all queue in `EditorViewModel::requestSliceAll()` (which currently builds the queue from all non-locked plates — add `&& isPrintable` to that filter).
  - Upstream "shared/unprintable plate pool" semantics (unprintable_plate in PartPlateList) are deferred — the basic per-plate printable flag is the Phase 17 deliverable.

### QML UI integration (D-09)
- **D-09:** Add clickable controls in the QML plate surface:
  - Per-plate context: a **clone/duplicate plate** action and a **reorder** affordance (e.g., move-left/move-right buttons or drag — buttons are simpler and lower-risk for Phase 17).
  - A **printable toggle** per plate.
  - Route through `EditorViewModel` Q_INVOKABLE → `ProjectServiceMock`. Preserve all existing plate UI behavior (Phase 14's honest-UI contract — new controls must be wired to real behavior, not no-ops; QmlUiAuditTests must stay green).
  - Exact QML control placement: planner's discretion, but prefer the existing plate-list/plate-tab surface (Sidebar/ObjectList area) rather than inventing new top-level chrome. Match existing `Cx*` control patterns.

### Claude's Discretion
- Exact QML control components (CxButton vs CxIconButton vs menu) and their placement in the plate surface.
- Whether reorder uses left/right arrows, up/down, or drag — recommend arrows for Phase 17 (drag is more work + needs DropArea).
- Test count (at least one per operation: clone copies objects, reorder reindexes, printable excludes from slice).
- Whether to add a `clonePlate` smoke test that asserts `modelCount` increased by the source plate's object count (proving deep copy).

</decisions>

<canonical_refs>
## Canonical References

### Upstream Source Truth
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:4484-4512` — `duplicate_plate` (D-06 deep-copy semantics; the `m_model->add_object(*object)` loop + `translate_all_instance`).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:4895-4935` — `move_plate_to_index` (D-07 reorder; vector shift + reindex; geometry recompute deferred).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:430` — `is_printable()` / `m_printable` (D-08; already on Qt6 PartPlate from Phase 16).
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp` (slice-all loop) — `m_printable` gating in slice iteration (reference for the slice-all queue filter change).

### Phase 16 Outputs (the foundation this builds on)
- `src/core/model/PartPlate.h` — has m_printable, m_obj_to_instance_set, per-plate settings fields.
- `src/core/model/PartPlateList.h` — has createPlate/deletePlate/reindex; this phase adds clonePlate/movePlate/setPlatePrintable.
- `src/core/services/ProjectServiceMock.cpp:3970` — existing `duplicateObject` (the machinery D-06 reuses for deep object copy).
- `src/core/services/ProjectServiceMock.cpp` (EditorViewModel::requestSliceAll, ~line 3656 pre-migration) — slice-all queue builder (D-08 filter change target).

### Conventions / Rules
- `.codex/rules/source-truth-migration.md` — status terms; "do not mark complete merely because a method exists."
- `AGENTS.md` / `CONVENTIONS.md` — `final` QObject, Q_PROPERTY+NOTIFY, `Cx*` controls, `qsTr()` strings, 2-space indent.
- Phase 14 UI-SPEC (`.planning/phases/14-*/14-UI-SPEC.md`) — honest-UI contract; new QML controls must be wired (no empty handlers), QmlUiAuditTests must stay green.

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ProjectServiceMock::duplicateObject(int sourceIndex)` (`ProjectServiceMock.cpp:3970`) — already does the full ModelObject + parallel-array copy + plate-membership index shift. D-06's clonePlate will call this per source object.
- `PartPlateList::createPlate()` / `deletePlate()` / `reindex()` (Phase 16) — clonePlate appends via createPlate; movePlate reuses reindex logic.
- `PartPlate::m_printable` (Phase 16) — D-08 just needs the list-level setter + slice-all filter.

### Established Patterns
- Q_INVOKABLE plate methods on ProjectServiceMock emit `projectChanged()` + `plateDataLoaded(count)` + `plateSelectionChanged()` as appropriate (Phase 16 pattern — clone/reorder must follow).
- EditorViewModel proxies plate ops to ProjectServiceMock and re-exposes via its own Q_INVOKABLE (follow the existing `duplicateSelectedObjects` proxy pattern).
- QML plate UI binds to `editorVm` plate properties; new actions call `editorVm.clonePlate(index)` etc.

### Integration Points
- `EditorViewModel::requestSliceAll()` — D-08 adds `isPrintable` to the queue filter.
- QmlUiAuditTests — any new QML must not introduce forbidden patterns (empty handlers, placeholder copy, hardcoded chrome colors). Run after QML changes.

</code_context>

<specifics>
## Specific Ideas

- Consistent with the user's established preference: **complete source-truth alignment, no half-measures.** Clone does deep copy (not shallow); reorder is a real reorder (not cosmetic).
- One subtlety: upstream `duplicate_plate` numbers new objects by appending to `m_model->objects`, so the cloned plate's instance set references the NEW object indices (not the source's). D-06 clonePlate must build the new plate's instance membership from the newly-created object indices, not copy the source indices.

</specifics>

<deferred>
## Deferred Ideas

- **Object-instance translation on clone** (upstream `translate_all_instance` by plate-to-plate origin offset): deferred because Qt6 doesn't compute plate origins yet (Phase 16 stored geometry fields but origin = 0/default). Cloned objects sit at source positions. Real layout-aware clone needs plate geometry (future / Phase 18+ bed-geometry work).
- **"Shared/unprintable plate pool" semantics** (upstream `unprintable_plate` + shared plate concept): deferred — Phase 17 does the basic per-plate printable flag only.
- **Drag-to-reorder UI**: Phase 17 uses button-based reorder (simpler, lower-risk). Drag reorder can be added later.
- **3MF persistence of clone/reorder/printable state**: Phase 18 (store_to_3mf_structure) persists all plate state including these.

</deferred>

---

*Phase: 17-Plate Lifecycle Completion*
*Context gathered: 2026-06-25*
