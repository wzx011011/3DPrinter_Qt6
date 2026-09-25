# Phase 29: Multi-Plate Arrangement Grid - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning
**Mode:** Smart Discuss (autonomous) — all 4 grey areas accepted verbatim

<domain>
## Phase Boundary

Add plate-grid geometry to `PartPlateList` (cols, stride, `compute_plate_index`) so that a plate index maps to a 2D world offset, and rewire `arrangeObjects` so an auto-arrange distributes models across multiple plates instead of dumping them all on plate 0.

This phase is the foundation of v3.2's "Multi-Plate Data Polish" theme. It lands the geometry layer the v3.0 PartPlate model deferred (Phase 16 stored origins as Vec3d but left them at 0/default; Phase 17 D-07 explicitly deferred geometry recompute), and it closes the gap where `arrangeObjects` calls `Slic3r::arrange_objects` but never translates the result back into per-plate membership (REQUIREMENTS ARRANGE-02; gap-analysis issue "Objects arranged into correct plates, not all on plate 0").

**In scope:**
- ARRANGE-01: `PartPlateList` plate-grid geometry (`m_plate_cols`, `compute_colum_count`, `plate_stride_x/y`, `compute_origin`, `compute_shape_position`, `compute_plate_index`) mirroring upstream `PartPlate.hpp:38-50,570,705-706,822` + `PartPlate.cpp:3905-3964,4836-4870,5365-5376`.
- ARRANGE-02: `PartPlateList::rebuildPlatesAfterArrangement()` + `ProjectServiceMock::arrangeObjects` rewiring so instances get distributed across plates via `compute_plate_index` on each instance's post-arrange translation, mirroring upstream `rebuild_plates_after_arrangement` (`PartPlate.cpp:6096-6139`) → `reload_all_objects` flow.
- ARRANGE-03: Locked plates excluded from arrangement — locked instances preserved, new objects arranged into non-locked slots.
- Origin realization: `compute_origin(i, m_plate_cols)` now actually writes `PartPlate::setOrigin()` (closes Phase 17 D-07 deferral); structural ops (create/delete/move) recompute origins.

**Out of scope:**
- Wipe-tower geometry rendering (v3.3+, needs GL framebuffer — REQUIREMENTS Out of Scope).
- Real GL-capture thumbnails (v3.3+, blocked on QRhi framebuffer).
- AssembleView (v3.3+, needs second GL canvas).
- Auto filament-map (Phase 31 / v3.3+, blocked on ToolOrdering).
- `m_print_list` reuse/caching (Phase 19 D-14 confirmed Qt6's stack-local Print is equivalent; perf-only).
- Visual GL rendering of the plate grid itself — Phase 29 is the *data* layer; GLViewport already consumes `PartPlate::origin()` so rendering will follow the data automatically, but no new GL code is in scope.

</domain>

<decisions>
## Implementation Decisions

All four grey areas were accepted verbatim with the recommended answer. The user's established pattern from Phase 16/17/19 ("complete source-truth alignment, no half-measures") held — every decision chose the most upstream-faithful option.

### Geometry API Surface (Area 1 — accepted verbatim)
- **D-29-1:** Plate-grid fields live on **`PartPlateList`** (domain object), exposed/bridged to Qt by `ProjectServiceMock`. Continues Phase 16 D-01/D-05 pattern (domain logic in `src/core/model/`, ProjectServiceMock is the Qt adaptation layer). No grid fields on PartPlate (a single plate doesn't know its grid position; the list does).
- **D-29-2:** Mirror the **full upstream function set** from `PartPlate.cpp:3905-3964,4836-4870,5365-5376` + `PartPlate.hpp:38-50,570,705-706,822`:
  - `compute_colum_count(int count)` (inline in hpp:38-50) — `round(sqrt(count))` with +1 ceiling when not exact.
  - `m_plate_cols` member (recomputed via `update_plate_cols()` on structural change).
  - `plate_stride_x()`, `plate_stride_y()` → `m_plate_width * (1 + LOGICAL_PART_PLATE_GAP)`, `m_plate_depth * (1 + LOGICAL_PART_PLATE_GAP)` where `LOGICAL_PART_PLATE_GAP = 1.0/5.0` (PartPlate.cpp:55).
  - `compute_shape_position(int index, int cols)` → Vec2d `{col*stride_x, -row*stride_y}`.
  - `compute_origin(int index, int cols)` → Vec3d from shape_position, z=0.
  - `compute_plate_index(arrangement::ArrangePolygon&)` → `row*m_plate_cols + col` from translation (PartPlate.cpp:5365-5376).
  - `update_plate_cols()` → `m_plate_cols = compute_colum_count(plate_count)`.
  - `updatePlateOrigins()` → mirror upstream `update_all_plates_pos_and_size` (PartPlate.cpp:4872-4892) writing `compute_origin(i, m_plate_cols)` to each `PartPlate::setOrigin()`. (Unprintable plate is out of scope — that pool semantics is deferred.)
- **D-29-3:** Plate width/depth (`m_plate_width`, `m_plate_depth`) are kept **in sync with the configured bed geometry** that `arrangeObjects` uses for `printableArea`. They are set/updated whenever bed geometry changes (or read from the same config source). Mirrors upstream where `m_plate_width/depth` come from printer config during PartPlateList init. Do NOT hardcode a default — that drifts from the real bed.
- **D-29-4:** `m_plate_cols` is recomputed **eagerly** via `update_plate_cols()` after every `createPlate` / `deletePlate` / `movePlate` (mirrors upstream `update_plate_cols` calls after structural change). No lazy/on-demand recompute — that produces ordering bugs.

### Multi-Plate Distribution (Area 2 — accepted verbatim)
- **D-29-5:** Distribution uses **`compute_plate_index` on each instance's post-arrange world translation**, NOT `arrange_objects`'s native `bed_idx`. Reason: upstream `ModelArrange.cpp:98` resets `bed_idx = 0` for all instances, so the per-bed split has to be reconstructed from the arranged translation. The flow:
  1. `arrangeObjects` calls `Slic3r::arrange_objects(*model_, bed, params, vfn)` — instances get world translations.
  2. `arrangeObjects` calls new `PartPlateList::rebuildPlatesAfterArrangement(exceptLocked)`.
  3. `rebuildPlatesAfterArrangement`: clears non-locked plate instance memberships; for each `model_->objects[*].instances[*]`, computes the world translation → `compute_plate_index` → assigns the (objIdx, instIdx) pair to that plate; creates new plates as needed (up to `kMaxPlateCount`); deletes trailing empty non-locked plates.
  - Mirrors upstream `rebuild_plates_after_arrangement` (PartPlate.cpp:6096-6139) → `reload_all_objects` (PartPlate.cpp:5260+).
- **D-29-6:** `rebuildPlatesAfterArrangement` lives on **`PartPlateList`** (domain object) and is called by `ProjectServiceMock::arrangeObjects` after arrange succeeds. NOT in EditorViewModel — keeps business logic in `core/` per ARCHITECTURE.md.
- **D-29-7:** Origin-offset aware: when assigning membership, instance world translation = `col*stride_x + plate_origin.x()`, `-row*stride_y + plate_origin.y()`. The instance's plate-local offset is preserved by the reverse of upstream `preprocess_arrange_polygon` (PartPlate.cpp:5394-5395): the world translation IS the post-arrange result, and `compute_plate_index` decodes which plate it falls in. Do NOT ignore offsets — that would cluster objects at world origin.
- **D-29-8:** **Plate recycling ON** — `rebuildPlatesAfterArrangement` deletes trailing empty non-locked plates after rebuild (mirrors upstream `recycle_plates=true` at PartPlate.cpp:6109-6128). Locked plates preserved always. Keep >= 1 plate invariant.

### Locked-Plate Exclusion (Area 3 — accepted verbatim)
- **D-29-9:** Locked plates are excluded in `arrangeObjects`: their instance memberships are preserved before arrange runs; after arrange + rebuild, locked plates keep their instances untouched. Mirrors upstream `except_locked` param flowing through `reload_all_objects` (PartPlate.cpp:5260).
- **D-29-10:** **ArrangePolygon preprocessing for locked instances** — set `bed_idx` to the locked plate's index for instances on locked plates BEFORE `arrange_objects` runs (upstream `preprocess_arrange_polygon` PartPlate.cpp:5388-5396). This marks them as fixed items so new objects get arranged into non-locked plate slots, avoiding the locked plate. (Implementation note: the tolerant VirtualBedFn already in `arrangeObjects` is compatible — fixed items stay.)
- **D-29-11:** **ARRANGE-03 has a deterministic test**: place objects on plate 0 (locked), call `arrangeObjects`, assert plate 0 retains its instances AND new objects land on plate ≥ 1. Required by the project's "deterministic test coverage replacing transport confidence" pattern.
- **D-29-12:** Edge case — all plates locked → `arrangeObjects` returns false, instances unchanged. Mirrors upstream behavior when no movable plate exists. Do NOT throw, crash, or silently move locked objects.

### Origin Realization & Integration (Area 4 — accepted verbatim)
- **D-29-13:** `compute_origin` now **actually writes plate origins**. `rebuildPlatesAfterArrangement` calls `compute_origin(i, m_plate_cols)` per plate and writes via `PartPlate::setOrigin()`. This closes the Phase 17 D-07 deferral ("origin = 0/default; geometry recompute deferred"). GLViewport already reads `PartPlate::origin()` so rendering follows the data automatically — no new GL code.
- **D-29-14:** `movePlate` / `createPlate` / `deletePlate` now call `updatePlateOrigins()` after the structural change + reindex, mirroring upstream `update_all_plates_pos_and_size` (PartPlate.cpp:4872-4892). Phase 17 D-07 explicitly deferred this; Phase 29 closes it. So plate origins are always consistent with plate position.
- **D-29-15:** Q_PROPERTY exposure on ProjectServiceMock (read-only): `plateCols`, `plateStrideX`, `plateStrideY`. These are useful for debug/QML. `compute_origin` / `compute_plate_index` / `compute_shape_position` stay internal (no QML need). Don't over-expose internal geometry.
- **D-29-16:** Test scope = **unit tests on PartPlateList** (compute_colum_count parity with upstream values, compute_origin grid math for sample indices, compute_plate_index row/col mapping for sample translations, ARRANGE-02 multi-plate distribution, ARRANGE-03 locked exclusion) **+ integration test through arrangeObjects** (arrange N objects, assert they distribute across plates, assert locked plate 0 retains instances). Mirrors the Phase 16 test-first risk mitigation pattern.

### Claude's Discretion
- Exact C++ field naming for the new geometry members (`m_plate_cols` mirroring upstream vs Qt-style — recommend matching upstream `m_` prefix since the file already uses it).
- Whether `m_plate_width`/`m_plate_depth` are read from a single shared bed-config source or threaded as parameters — recommend the path that minimizes duplication with `arrangeObjects`'s `printableArea` parsing.
- How `rebuildPlatesAfterArrangement` enumerates instances (iterate `model_->objects[*].instances[*]` directly, or via the ArrangePolygon list) — match what gives clean plate-index computation.
- Whether to add an `updatePlateOrigins()` private helper vs inline the origin loop — recommend the helper for reuse across create/delete/move/rebuild.
- Exact unit test framework patterns (match existing `tests/PartPlateTests.cpp` / `tests/ProjectServiceMockTests.cpp` style).
- Whether ARRANGE-03's locked preprocessing needs the full upstream `preprocess_arrange_polygon` (PartPlate.cpp:5378-5422) or a minimal locked-bed_idx subset — recommend the minimal subset that satisfies the test (full preprocessing has more upstream coupling than Phase 29 needs).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`OWzx::PartPlateList`** (`src/core/model/PartPlateList.h/.cpp`) — owns `std::vector<std::unique_ptr<PartPlate>> m_plate_list` + `m_current_plate`. Has `createPlate`/`deletePlate`/`movePlate`/`reindex`/`resetToSinglePlate`. This is where the new geometry members + `compute_*` methods + `rebuildPlatesAfterArrangement` land.
- **`OWzx::PartPlate`** (`src/core/model/PartPlate.h`) — already has `m_origin` (Vec3d under HAS_LIBSLIC3R / 3-double fallback), `m_width/depth/height`, `setOrigin()`, `addInstance()/removeInstance()`, `objToInstanceSet()`. D-29-13 writes to `setOrigin()`; D-29-5 reads instance membership.
- **`ProjectServiceMock::arrangeObjects`** (`ProjectServiceMock.cpp:2416-2479`) — already calls real `Slic3r::arrange_objects(*model_, bed, params, tolerantVfn)` and parses `printableArea` into a bed shape. D-29-5/D-29-10 rewire this to add locked-bed_idx preprocessing + post-arrange `rebuildPlatesAfterArrangement`.
- **`ProjectServiceMock` holds `Slic3r::Model* model_`** (`ProjectServiceMock.h:372`) — `rebuildPlatesAfterArrangement` reads `model_->objects[*].instances[*]` translations for `compute_plate_index`.
- **`Slic3r::arrangement::ArrangePolygon`** (libslic3r Arrange.hpp) — has `bed_idx`, `translation`, `rotation`, `setter`. D-29-10 sets `bed_idx` for locked instances before arrange.

### Established Patterns
- **Service-as-Qt-adapter** (Phase 16 D-01): `ProjectServiceMock` exposes `PartPlateList` truth via Q_PROPERTY/Q_INVOKABLE with `#ifdef HAS_LIBSLIC3R` guards. New `plateCols`/`plateStrideX/Y` Q_PROPERTY follow this.
- **Q_INVOKABLE plate methods emit `projectChanged()` + `plateDataLoaded(count)`** (Phase 16 pattern). `arrangeObjects` already returns bool + triggers viewmodel refresh; `rebuildPlatesAfterArrangement` calls should emit `plateDataLoaded(newCount)` when plate count changes.
- **Test-first on the model** (Phase 16 D-05 risk posture): write PartPlateList unit tests before/independently of ProjectServiceMock rewiring. Existing `tests/PartPlateTests.cpp` is the home for the new geometry unit tests.

### Integration Points
- **`EditorViewModel::arrangeObjects` proxy** (`EditorViewModel.cpp:3859-3868`) — calls `projectService_->arrangeObjects(spacing, rotation, alignY, printableArea)`. No change needed at the viewmodel layer (the Q_INVOKABLE signature is unchanged), but it benefits from the new distribution (objects spread across plates).
- **`GLViewport` reads `PartPlate::origin()`** — D-29-13/D-29-14 make origins real, so multi-plate rendering will follow automatically. Verify visually in canonical-verify, but no GL code change is in scope.
- **3MF load path** (`ProjectServiceMock.cpp:4731` `read_from_archive` → plate reconstruction) — after load, plate origins should be consistent. Consider calling `updatePlateOrigins()` at the end of the load path (planner's discretion — it's defensive but cheap).
- **`tests/QmlUiAuditTests.cpp`** — no new QML in Phase 29 (only Q_PROPERTY read-only exposure), so this stays green trivially. Already shows as modified in git status — leave it.

</code_context>

<specifics>
## Specific Ideas

- The user's consistent theme across all four areas: **"complete source-truth alignment, no half-measures."** Every decision chose the most upstream-faithful option (full function set, origin realization now, locked preprocessing, deterministic tests). When the planner faces a "quick vs correct" trade-off within Phase 29 scope, choose correct.
- One subtlety from the upstream read: `compute_colum_count` (PartPlate.hpp:38-50) uses `round(sqrt(count))` with a `+1` ceiling when `sqrt(count) > round(sqrt(count))`. The unit test MUST verify parity with upstream for counts 1..36 (e.g., count=2 → cols=2, count=4 → cols=2, count=5 → cols=3, count=9 → cols=3, count=10 → cols=4). This is a frequent source of off-by-one drift.
- Another subtlety: upstream `compute_plate_index` (PartPlate.cpp:5365-5376) uses `round()` on the row/col float values, and the row value is `(stride_y - translation_y) / stride_y` (note the sign flip — y grows downward in plate-grid space). The unit test should cover translations that land on plate boundaries.
- Phase 17 D-07 explicitly deferred "geometry origin recomputation (which Phase 16 deferred)". Phase 29 closes BOTH deferrals. The planner should note this in the SUMMARY/VERIFICATION as carry-forward tech debt paid.

</specifics>

<deferred>
## Deferred Ideas

None new — discussion stayed within Phase 29 scope. The following are correctly deferred to other phases (already tracked in REQUIREMENTS / STATE):

- **Wipe-tower geometry + rendering** → v3.3+ (needs GL framebuffer, WT-01).
- **Real GL-capture thumbnails (4 variants)** → v3.3+ (blocked on QRhi framebuffer, THUMB-03).
- **Auto filament-map recommendation** → v3.3+ (blocked on ToolOrdering, FMAP-04). Phase 31 does Manual mode only.
- **AssembleView** → v3.3+ (needs second GL canvas, ASSEMBLE-01).
- **Full upstream `preprocess_arrange_polygon`** (PartPlate.cpp:5378-5422) — Phase 29 implements only the locked-bed_idx subset needed for ARRANGE-03. The "fixed item" / "non-selected" branches are upstream-coupled to selection state Qt6 doesn't fully mirror; defer until AssembleView-style multi-select arrives.
- **Unprintable plate pool** (`unprintable_plate`, `compute_origin_for_unprintable`) — upstream "shared/unprintable plate pool" semantics deferred from Phase 17. The basic per-plate printable flag (Phase 17 D-08) is sufficient for v3.2.
- **`m_print_list` reuse/caching** — Phase 19 D-14 confirmed Qt6's stack-local Print is equivalent for correctness; perf-only.

</deferred>

---

*Phase: 29-Multi-Plate Arrangement Grid*
*Context gathered: 2026-06-28 via smart discuss (autonomous mode)*
