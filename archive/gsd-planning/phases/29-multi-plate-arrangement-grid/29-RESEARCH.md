# Phase 29 Research: Multi-Plate Arrangement Grid

**Gathered:** 2026-06-28 | **Requirements:** ARRANGE-01, ARRANGE-02, ARRANGE-03 | **Mode:** Read-only research (every code cite below is from the actual file)

## 1. Upstream Plate-Grid Geometry (ARRANGE-01)

### 1.1 `compute_colum_count` — the column-count algorithm

**Source:** `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:38-50` (inline, file-scope, NOT a member)

```cpp
inline int compute_colum_count(int count)
{
    float value = sqrt((float)count);
    float round_value = round(value);
    int cols;
    if (value > round_value)
        cols = round_value +1;
    else
        cols = round_value;
    return cols;
}
```

**Algorithm in plain terms:** `cols = ceil(sqrt(count))` — but implemented via float comparison rather than an integer `ceil`. The strict `value > round_value` check means perfect squares (1,4,9,16,25,36) land exactly on the integer (no +1), and all non-squares get the +1 ceiling. This is mathematically identical to `(int)ceil(sqrt((double)count))` but the Qt6 port MUST replicate the exact float-comparison form to guarantee parity (don't "simplify" to integer ceil — it could differ on float rounding).

### 1.2 Parity table — `compute_colum_count(count)` for counts 1..36

Computed by hand from the exact upstream algorithm. These are the values to encode directly into the unit test:

| count | sqrt  | round | cols |  | count | sqrt  | round | cols |
|-------|-------|-------|------|--|-------|-------|-------|------|
| 1     | 1.000 | 1     | **1** |  | 19    | 4.359 | 4     | **5** |
| 2     | 1.414 | 1     | **2** |  | 20    | 4.472 | 4     | **5** |
| 3     | 1.732 | 2     | **2** |  | 21    | 4.583 | 5     | **5** |
| 4     | 2.000 | 2     | **2** |  | 22    | 4.690 | 5     | **5** |
| 5     | 2.236 | 2     | **3** |  | 23    | 4.796 | 5     | **5** |
| 6     | 2.449 | 2     | **3** |  | 24    | 4.899 | 5     | **5** |
| 7     | 2.646 | 3     | **3** |  | 25    | 5.000 | 5     | **5** |
| 8     | 2.828 | 3     | **3** |  | 26    | 5.099 | 5     | **6** |
| 9     | 3.000 | 3     | **3** |  | 27    | 5.196 | 5     | **6** |
| 10    | 3.162 | 3     | **4** |  | 28    | 5.292 | 5     | **6** |
| 11    | 3.317 | 3     | **4** |  | 29    | 5.385 | 5     | **6** |
| 12    | 3.464 | 3     | **4** |  | 30    | 5.477 | 5     | **6** |
| 13    | 3.606 | 4     | **4** |  | 31    | 5.568 | 6     | **6** |
| 14    | 3.742 | 4     | **4** |  | 32    | 5.657 | 6     | **6** |
| 15    | 3.873 | 4     | **4** |  | 33    | 5.745 | 6     | **6** |
| 16    | 4.000 | 4     | **4** |  | 34    | 5.831 | 6     | **6** |
| 17    | 4.123 | 4     | **5** |  | 35    | 5.916 | 6     | **6** |
| 18    | 4.243 | 4     | **5** |  | 36    | 6.000 | 6     | **6** |

**Off-by-one drift traps the test must cover:**
- Perfect squares (1,4,9,16,25,36) — boundary; `value == round_value` exactly, so cols = sqrt with NO +1.
- The transition points: count 5→cols 3, count 10→cols 4, count 17→cols 5, count 26→cols 6. These are one-above-perfect-square counts where `value > round_value` is true and the +1 fires. Easy to get wrong with integer `ceil`.
- count=1 must yield cols=1 (degenerate — single plate is a 1×1 grid).

### 1.3 `LOGICAL_PART_PLATE_GAP` constant

**Source:** `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp:55`
```cpp
static const double LOGICAL_PART_PLATE_GAP = 1. / 5.;
```
Value = **0.2** (1/5). Used in `plate_stride_x/y` and `compute_origin_using_new_size`. The Qt6 port should declare this as a file-scope `constexpr double` in `PartPlateList.cpp` (or a private static) — it is NOT exported upstream.

### 1.4 Stride, cols, and origin field declarations

**Source:** `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:569-576`
```cpp
int m_plate_count;          // 569
int m_plate_cols;           // 570
int m_current_plate;        // 571
int m_print_index;          // 572
int m_plate_width;          // 574
int m_plate_depth;          // 575
int m_plate_height;         // 576
```

**Method declarations:** `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:699` (`compute_origin_using_new_size`), `:704-706` (`plate_stride_x()`, `plate_stride_y()`), `:815` (`reload_all_objects`), `:822` (`compute_plate_index`).

**Note on `m_plate_width/depth`:** upstream uses `int` (line 574-575). The Qt6 `PartPlate` already has `int m_width/depth/height` (`PartPlate.h:210-212`) — keep `m_plate_width`/`m_plate_depth` as `int` on `PartPlateList` to mirror upstream. But stride math (`size * (1 + gap)`) produces `double`, so stride return types are `double`.

### 1.5 `plate_stride_x / plate_stride_y` — `PartPlate.cpp:4836-4850`

```cpp
double PartPlateList::plate_stride_x()  { return m_plate_width  * (1. + LOGICAL_PART_PLATE_GAP); }
double PartPlateList::plate_stride_y()  { return m_plate_depth  * (1. + LOGICAL_PART_PLATE_GAP); }
```
**stride = size × (1 + 0.2) = size × 1.2.** For a 220mm bed: stride = 264mm.

### 1.6 `update_plate_cols` — `PartPlate.cpp:4862-4870`

```cpp
void PartPlateList::update_plate_cols() {
    m_plate_count = m_plate_list.size();
    m_plate_cols = compute_colum_count(m_plate_count);
    ...
}
```
Called after every structural change (D-29-4). Note it also refreshes `m_plate_count` from the list size.

### 1.7 `compute_shape_position` and `compute_origin` — `PartPlate.cpp:3905-3964`

```cpp
Vec2d PartPlateList::compute_shape_position(int index, int cols) {
    int row = index / cols;          // integer floor division
    int col = index % cols;
    pos(0) = col * plate_stride_x();    // +X to the right
    pos(1) = -row * plate_stride_y();   // NEGATIVE Y downward
    return pos;
}
Vec3d PartPlateList::compute_origin(int i, int cols) {
    Vec2d pos = compute_shape_position(i, cols);
    return Vec3d(pos.x(), pos.y(), 0);
}
```

**Sign convention subtlety:** plate index increases left-to-right, top-to-bottom. Y goes NEGATIVE for rows below the first. Plate 0 is at origin (0,0,0). Plate 1 (col 1) at (+stride_x, 0). Plate `cols` (start of row 2) at (0, -stride_y).

`compute_origin_using_new_size(i, new_width, new_depth)` (`:3925-3938`) is the same math but using passed-in sizes (used during bed-size changes). Out of scope for Phase 29 per CONTEXT (D-29-2 only lists `compute_origin`/`compute_shape_position`).

`compute_origin_for_unprintable` (`:3942-3949`) is the unprintable-plate pool — **explicitly out of scope** (deferred from Phase 17, see CONTEXT `deferred`).

### 1.8 `update_all_plates_pos_and_size` — `PartPlate.cpp:4872-4892` (the origin-update loop)

```cpp
void PartPlateList::update_all_plates_pos_and_size(bool adjust_position, ...) {
    for (unsigned int i = 0; i < m_plate_list.size(); ++i) {
        PartPlate* plate = m_plate_list[i];
        origin1 = compute_origin(i, m_plate_cols);
        plate->set_pos_and_size(origin1, m_plate_width, m_plate_depth, m_plate_height, ...);
        if (switch_plate_type && m_plater) set_default_wipe_tower_pos_for_plate(i);  // out of scope
    }
    origin2 = compute_origin_for_unprintable();  // out of scope
    unprintable_plate.set_pos_and_size(origin2, ...);  // out of scope
}
```
**Qt6 port shape (`updatePlateOrigins()` per D-29-2/D-29-14):** strip the wipe-tower and unprintable branches. The core loop is: for each plate i, `plate->setOrigin(compute_origin(i, m_plate_cols))`. This is what create/delete/move/rebuild call after reindex.

### 1.9 `compute_plate_index` — `PartPlate.cpp:5365-5376` (CRITICAL — has sign flip)

```cpp
int PartPlateList::compute_plate_index(arrangement::ArrangePolygon& arrange_polygon) {
    float col_value = (unscale<double>(arrange_polygon.translation(X))) / plate_stride_x();
    float row_value = (plate_stride_y() - unscale<double>(arrange_polygon.translation(Y))) / plate_stride_y();
    row = round(row_value);
    col = round(col_value);
    return row * m_plate_cols + col;
}
```

**Sign-flip subtlety (CONTEXT §specifics flagged this):** because origins encode `y = -row*stride_y`, decoding inverts: `row_value = (stride_y - translation_y) / stride_y = 1 - (translation_y / stride_y)`. So a translation.y of `0` → row 0; a translation.y of `-stride_y` → row 1. The `round()` (not floor/trunc) means translations near a plate boundary snap to the nearest grid line — this is intentional (handles arrange output that lands slightly inside a cell).

**Qt6 port note:** the upstream version takes an `ArrangePolygon&` (libslic3r type) and reads its scaled integer `translation`. The Qt6 method must operate on a **world translation in mm** (Vec2d or Vec3d from `model_->objects[*].instances[*]->get_offset()`), since `rebuildPlatesAfterArrangement` enumerates model instances directly (D-29-5). The conversion: upstream `unscale<double>(scaled_int)` converts internal μm-scale int to mm — so the Qt6 port reads instance offset in mm directly and skips the unscale. The recommended signature (from D-29-6 + Claude discretion):
```cpp
int compute_plate_index(double translationX_mm, double translationY_mm) const;
```
This keeps `PartPlateList` free of the libslic3r `ArrangePolygon` dependency (it's a pure domain object), while `ProjectServiceMock::arrangeObjects` extracts the translation and calls it.

## 2. Multi-Plate Distribution (ARRANGE-02)

### 2.1 `rebuild_plates_after_arrangement` — `PartPlate.cpp:6096-6139`

```cpp
int PartPlateList::rebuild_plates_after_arrangement(bool recycle_plates, bool except_locked, int plate_index) {
    // 1. sort model objects by their first instance's arrange_order
    std::sort(m_model->objects.begin(), m_model->objects.end(),
              [](auto a, auto b){ return a->instances[0]->arrange_order < b->instances[0]->arrange_order; });
    // 2. reload instance→plate membership (rebuilds from bounding-box intersection)
    ret = reload_all_objects(except_locked, plate_index);
    // 3. recycle: delete trailing empty/locked plates (locked always preserved)
    if (recycle_plates) {
        for (unsigned int i = m_plate_list.size() - 1; i > 0; --i) {
            if (m_plate_list[i]->empty() || !m_plate_list[i]->has_printable_instances())
                delete_plate(i);
            else if (m_plate_list[i]->is_locked())
                continue;
            else
                break;
        }
    }
    return ret;
}
```

**Critical translation notes for the Qt6 port:**
- **Sort key:** `arrange_order` is a libslic3r `ModelInstance` field set by `arrange_objects`. The Qt6 port reads `model_->objects[i]->instances[0]->arrange_order` for the sort. If `arrange_order` is uniform/zero (e.g., when arrange didn't fully populate it), the sort is stable and harmless. **Confirm `arrange_order` is populated** — see subtlety in §2.3.
- **Recycling condition:** upstream uses `plate->empty() || !plate->has_printable_instances()`. The Qt6 `PartPlate` has `objToInstanceSet()` but **does NOT yet have `empty()` or `hasPrintableInstances()`** (confirmed by grep — `PartPlate.h` has neither). The planner must either (a) add a lightweight `bool empty() const { return m_obj_to_instance_set.empty(); }` (mirrors upstream `PartPlate.hpp:387`), or (b) inline the check as `plate->objToInstanceSet().empty()` in the rebuild. `has_printable_instances` upstream (`PartPlate.cpp:2999-3019`) walks instances checking `instance->printable && not in instance_outside_set` — for Phase 29 scope, the membership-empty check (`empty()`) is the load-bearing half; the printable-instance check requires a model pointer Qt6's `PartPlate` doesn't hold. **Recommendation:** add `bool empty() const` to `PartPlate` and use only `empty()` for recycling (the printable half is a refinement that needs the model backref; document as carry-forward).
- **Iteration direction:** `for i = size-1; i > 0; --i` — stops at i==1, NEVER deletes plate 0 (keeps ≥1 invariant). The `break` on first non-empty/non-locked plate means only a CONTIGUOUS trailing run is recycled.
- **`except_locked` flows into `reload_all_objects`** to preserve locked-plate memberships.

### 2.2 `reload_all_objects` — `PartPlate.cpp:5260-5307`

```cpp
int PartPlateList::reload_all_objects(bool except_locked, int plate_index) {
    clear(false, false, except_locked, plate_index);  // wipes instance memberships (except locked)
    for (i in m_model->objects) {
        for (j in object->instances) {
            BoundingBoxf3 bbox = object->instance_convex_hull_bounding_box(j);
            for (k in m_plate_list) {
                if (plate->intersect_instance(i, j, &bbox)) {
                    plate->add_instance(i, j, false, &bbox);
                    break;
                }
            }
            if (k == m_plate_list.size() && unprintable_plate.intersect_instance(i,j,&bbox))
                unprintable_plate.add_instance(i, j, false, &bbox);
        }
    }
}
```

**Upstream uses BOUNDING-BOX INTERSECTION, not `compute_plate_index`.** This is a divergence between the CONTEXT (D-29-5/D-29-7) and the literal upstream. CONTEXT D-29-5 explicitly chose to use `compute_plate_index` on the post-arrange translation instead of bounding-box intersection. **Both approaches are valid** and converge for objects fully inside one plate cell; the CONTEXT chose the translation-decode path because:
1. It's simpler (no per-plate bounding-box test).
2. It avoids needing `intersect_instance` (which requires the plate to know its bbox + a model backref).
3. It directly inverts the geometry that `compute_origin` produced.

**Planner decision point (confirm with D-29-5):** the Qt6 `rebuildPlatesAfterArrangement` should:
1. `clear(false, false, exceptLocked, -1)`-equivalent: clear non-locked plates' instance sets (preserve locked).
2. For each `model_->objects[objIdx]->instances[instIdx]`: read `get_offset()` → (x, y) → `compute_plate_index(x, y)` → if plate index ≥ current count, `createPlate()` up to that index → `plate(idx)->addInstance(objIdx, instIdx)`.
3. Recycle trailing empty non-locked plates (per §2.1).

The Qt6 `PartPlate` lacks a model backref, so the bounding-box-intersection path is NOT available without adding one — `compute_plate_index` is the natural fit. This is a justified, documented divergence from the literal upstream `reload_all_objects` (CONTEXT D-29-5/D-29-7 authorize it).

### 2.3 `apply_arrange_polys` and the `bed_idx = 0` reset

**`third_party/OrcaSlicer/src/libslic3r/ModelArrange.cpp:29-41`** (`apply_arrange_polys`):
```cpp
for(size_t i = 0; i < input.size(); ++i) {
    if (input[i].bed_idx != 0) { ret = false; if (vfn) vfn(input[i]); }  // out-of-bed → vfn
    if (input[i].bed_idx >= 0)
        instances[i]->apply_arrange_result(input[i].translation.cast<double>(), input[i].rotation);
}
```
This writes the arranged translation back to the `ModelInstance`. The tolerant vfn in the current `arrangeObjects` (ProjectServiceMock.cpp:2438) makes `bed_idx != 0` items survive (no throw), so the translation is still applied. **The result: every instance ends up with a real world translation, but `bed_idx` is meaningless (reset to 0 by `get_arrange_poly`).**

**`ModelArrange.cpp:90-98`** (`get_arrange_poly` template specialization):
```cpp
ap.bed_idx = 0;   // BBS: always reset to 0; per-bed split is reconstructed in PartPlateList
ap.setter = [obj](const ArrangePolygon& p) {
    if (p.is_arranged()) {
        Vec2d t = p.translation.cast<double>();
        T{obj}.apply_arrange_result(t, p.rotation, p.itemid);
    }
};
```
**This is the reason D-29-5 mandates reconstructing the per-plate split from translation, not `bed_idx`.** `bed_idx` is always 0 entering arrange, and `arrange_objects` does NOT populate per-bed indices in this code path — the sudoku-style grid decode happens entirely in `PartPlateList::compute_plate_index`. Confirmed.

### 2.4 Current Qt6 `arrangeObjects` — `ProjectServiceMock.cpp:2416-2479`

Current implementation (the rewiring target):
- Builds `Slic3r::ArrangeParams` (min_obj_distance in μm, allow_rotations, align_to_y_axis, `parallel=false`, no-op progress).
- Defines `tolerantVfn` as a no-op lambda (replaces default `throw_if_out_of_bed`).
- Parses `printableArea` string `"x1,y1,x2,y2,..."` → `Slic3r::Points` (mm×1000 = μm) → `Slic3r::BoundingBox bed_bb`.
- Calls `Slic3r::arrange_objects(*model_, bed_bb, params, tolerantVfn)` and returns its bool.
- **Gap (ARRANGE-02):** never translates the arranged result back into `PartPlate` membership. After arrange, all instances have world translations but plate membership is stale (objects stay on whatever plate they were on, or plate 0).

**What the rewire adds (per D-29-5/D-29-6/D-29-10):**
1. BEFORE arrange: for each locked plate, for each `(objIdx, instIdx)` in its membership, build the ArrangePolygon with `bed_idx = lockedPlateIndex` (minimal locked-preprocessing per §3.1). Since `arrangeObjects` operates on the whole model (not a per-instance polygon list it builds), the cleanest implementation is to use `Slic3r::arrange_objects`'s `selected`-less overload and rely on the locked items being treated as fixed via a custom `VirtualBedFn` that pins locked-plate items. **Alternative simpler path (recommended for D-29-11 determinism):** because the current code calls `arrange_objects` on the entire model and the tolerant vfn swallows out-of-bed, the locked-preprocessing can be deferred to post-arrange: locked plates are simply NOT touched by `rebuildPlatesAfterArrangement` (their membership is preserved), and new objects get arranged into world positions that `compute_plate_index` maps to non-locked plate cells. The risk: arrange might place a new object ON TOP of a locked plate's footprint. CONTEXT D-29-10 wants the locked-bed_idx preprocessing to prevent that. **Recommend implementing D-29-10's minimal subset** (§3.1) to make ARRANGE-03's test deterministic.
2. AFTER arrange succeeds: call `partPlateList_->rebuildPlatesAfterArrangement(exceptLocked=true, recyclePlates=true)` → re-distributes instance membership.
3. Emit `plateDataLoaded(newCount)` if count changed.

**Signature is unchanged** (ProjectServiceMock.h:174) — `Q_INVOKABLE bool arrangeObjects(float, bool, bool, const QString&)`. EditorViewModel proxy (EditorViewModel.cpp:3859-3868) needs no change; it already calls `syncTransformsFromModel()` + `emit stateChanged()` on success. The new plate distribution flows through those.

## 3. Locked-Plate Exclusion (ARRANGE-03)

### 3.1 `preprocess_arrange_polygon` — `PartPlate.cpp:5378-5422`

```cpp
bool PartPlateList::preprocess_arrange_polygon(int obj_index, int instance_index,
                                               arrangement::ArrangePolygon& arrange_polygon, bool selected) {
    bool locked = false; int lockplate_cnt = 0;
    for (unsigned int i = 0; i < m_plate_list.size(); ++i) {
        if (m_plate_list[i]->contain_instance(obj_index, instance_index)) {
            if (m_plate_list[i]->is_locked()) {
                locked = true;
                arrange_polygon.bed_idx = i;                       // PIN to locked plate index
                arrange_polygon.row = i / m_plate_cols;
                arrange_polygon.col = i % m_plate_cols;
                arrange_polygon.translation(X) -= scaled<double>(plate_stride_x() * arrange_polygon.col);
                arrange_polygon.translation(Y) += scaled<double>(plate_stride_y() * arrange_polygon.row);
            } else if (!selected) {
                // treated as fixed item later; bed_idx = i - lockplate_cnt
                arrange_polygon.bed_idx = i - lockplate_cnt;
                ...
            }
            return locked;
        }
        if (m_plate_list[i]->is_locked()) lockplate_cnt++;
    }
    if (!selected) arrange_polygon.bed_idx = MAX_PLATES_COUNT;  // not in any plate
    return locked;
}
```

**Minimal locked subset for Phase 29 (CONTEXT D-29-10 / Claude discretion):** the ONLY branch the plan implements is the `is_locked()` branch (lines 5388-5396): pin `bed_idx` to the locked plate index, and convert the world translation to plate-local (subtract `col*stride_x`, add `row*stride_y`). This marks the instance as fixed-at-its-bed so the arranger leaves it in place and routes new objects elsewhere.

**Branches to DEFER (per CONTEXT `deferred`):**
- The `!selected` non-locked "fixed item" branch (lines 5399-5407) — upstream-coupled to selection state Qt6 doesn't fully mirror.
- `preprocess_arrange_polygon_other_locked`, `preprocess_exclude_areas`, `preprocess_nonprefered_areas`, `postprocess_*` (PartPlate.hpp:824-830, 834) — all out of scope.

**Translation-offset inversion (D-29-7):** the `translation(X) -= stride_x * col` / `translation(Y) += stride_y * row` converts the instance's WORLD position to its PLATE-LOCAL position (inverse of `compute_origin`). When decoding back via `compute_plate_index`, you must NOT apply this offset again — `compute_plate_index` reads the world translation directly. This is the round-trip the planner must keep straight.

### 3.2 Qt6 membership API for ARRANGE-03 — `PartPlate.h`

Available and ready (no additions needed for membership):
- `isLocked() const` → `bool` (PartPlate.h:111)
- `objToInstanceSet() const` → `const std::set<std::pair<int,int>>&` (PartPlate.h:138) — iterate to find locked-plate instances
- `addInstance(int, int)` / `removeInstance(int, int)` (PartPlate.h:143, 148)
- `clearInstances()` (PartPlate.h:156) — used by `rebuildPlatesAfterArrangement` to wipe non-locked membership before re-distributing

**To enumerate locked-plate instances for preprocessing (D-29-10):** iterate `partPlateList_->plate(i)` for each locked `i`, walk `plate->objToInstanceSet()`, for each `(objIdx, instIdx)` access `model_->objects[objIdx]->instances[instIdx]` to set its arrange polygon's bed_idx. **Missing helper:** Qt6 `PartPlate` has no `contain_instance(obj, inst)` — but it's trivially `objToInstanceSet().count({obj,inst}) > 0`. The planner can add a convenience `bool containsInstance(int,int) const` mirroring upstream `contain_instance` (PartPlate.hpp:351) or inline the set lookup.

### 3.3 Edge case — all plates locked (D-29-12)

Upstream: when every plate is locked and there's no movable slot, `arrange_objects` returns false (apply_arrange_polys sees bed_idx != 0 on all items, tolerant vfn swallows, ret=false). **Qt6 must mirror this:** `arrangeObjects` returns false, instances unchanged, no rebuild. The current tolerant-vfn path already produces this naturally — the planner just must NOT call `rebuildPlatesAfterArrangement` when `arrange_objects` returns false (guard the rebuild behind the bool return).

## 4. Existing Qt6 Integration Points (verified)

### 4.1 `PartPlateList` current state — `src/core/model/PartPlateList.h` / `.cpp`

- **Owns** `std::vector<std::unique_ptr<PartPlate>> m_plate_list` + `int m_current_plate = 0` (PartPlateList.h:88-89).
- Has `createPlate()` / `deletePlate(int)` / `movePlate(int,int)` / `reindex()` / `resetToSinglePlate()` / `setPlateLocked` / `setPlatePrintable` / `plateIndexForObject` / `objectIndicesOnPlate`.
- `kMaxPlateCount = 36` constexpr (PartPlateList.h:26) — mirrors upstream `MAX_PLATE_COUNT` (PartPlate.hpp:36).
- **MISSING (Phase 29 additions):** `m_plate_cols`, `m_plate_width`, `m_plate_depth`, `m_plate_height`, `compute_colum_count` (file-scope or static), `plate_stride_x/y`, `compute_shape_position`, `compute_origin`, `compute_plate_index`, `update_plate_cols`, `updatePlateOrigins`, `rebuildPlatesAfterArrangement`.
- `movePlate` comment (PartPlateList.cpp:67-68) explicitly says "No geometry recompute (Qt6 doesn't compute plate origins yet — deferred per Phase 17 CONTEXT)." — Phase 29 closes this (D-29-14).
- `createPlate`/`deletePlate`/`movePlate` must gain `update_plate_cols()` + `updatePlateOrigins()` calls after the structural change + reindex (D-29-4, D-29-14).

### 4.2 `PartPlate` origin API — `src/core/model/PartPlate.h`

- `m_origin` is `Slic3r::Vec3d` under HAS_LIBSLIC3R, zero-initialized (PartPlate.h:206); 3-double fallback otherwise.
- `origin()` / `setOrigin(const Vec3d&)` / `setOrigin(double,double,double)` exist (PartPlate.h:91-101).
- `m_width/depth/height` are `int` (PartPlate.h:210-212), with `setSize(int,int,int)`.

### 4.3 ProjectServiceMock — `src/core/services/ProjectServiceMock.h:174` / `.cpp:2416-2479`

- `Q_INVOKABLE bool arrangeObjects(float spacing, bool allowRotation, bool alignY, const QString &printableArea)` — signature UNCHANGED in Phase 29.
- Holds `Slic3r::Model* model_` (referenced via `rawModel()`, ProjectServiceMock.h:72) and `std::unique_ptr<OWzx::PartPlateList> m_plateList` (ProjectServiceMock.h:364).
- **MISSING for D-29-15:** `plateCols`, `plateStrideX`, `plateStrideY` Q_PROPERTY (read-only) on ProjectServiceMock. Add as Q_PROPERTY with READ + NOTIFY `plateDataLoaded` (or a new `plateGeometryChanged` if you don't want to overload the existing signal — `plateDataLoaded` fires on count change, which is the right trigger for cols change too since cols derives from count).
- `arrangeObjects` is called from TWO load paths (ProjectServiceMock.cpp:848 and :5334) with a hardcoded `"0,0,220,0,220,220,0,220"` printable area — these would also benefit from the new distribution (objects spread across plates), but the CONTEXT scopes the rewire to the public `arrangeObjects` method, which both paths call.

### 4.4 EditorViewModel proxy — `src/core/viewmodels/EditorViewModel.cpp:3855-3876`

`arrangeAllObjects()` calls `projectService_->arrangeObjects(spacing, rotation, alignY, printableArea)`, then `syncTransformsFromModel()` + `emit stateChanged()` on success. **No change needed** — it benefits transparently from the new distribution. The `printableArea` is sourced from `configViewModel_->mergedConfigValues().value("printable_area")` (EditorViewModel.cpp:3865-3866).

## 5. Bed-Geometry Source for Plate Size (D-29-3) — RECOMMENDATION

**Where the bed geometry lives today:**
- `EditorViewModel` has the canonical Q_PROPERTY bed dimensions: `bedWidth` (default 220), `bedDepth` (default 220), `bedMaxHeight` (default 300), `bedOriginX/Y`, `bedShapeType`, `bedDiameter` (EditorViewModel.h:566-572, 799-805). Persisted to `QSettings` under `bed/width`, `bed/depth`, `bed/maxHeight` (EditorViewModel.cpp:1539-1541).
- The `printable_area` string passed to `arrangeObjects` originates from `ConfigViewModel::mergedConfigValues()` (ConfigViewModel.cpp:1230-1233 → returns `globalOptionValues_`), keyed `"printable_area"`.
- ProjectServiceMock does NOT hold a printer preset; the load-path calls to `arrangeObjects` hardcode `"0,0,220,0,220,220,0,220"` (ProjectServiceMock.cpp:849, 5335).

**The drift risk (D-29-3 concern):** if `m_plate_width`/`m_plate_depth` on `PartPlateList` are set from a different source than the bed `arrangeObjects` uses, the grid decode (`compute_plate_index`) won't match the arrange output's coordinate space → objects mis-assigned to plates.

**RECOMMENDED single source of truth:** `ProjectServiceMock::arrangeObjects` ALREADY parses `printableArea` into a `Slic3r::BoundingBox bed_bb` (ProjectServiceMock.cpp:2444-2455). **Have `arrangeObjects` compute `m_plate_width = bed_bb.size().x()/1000.0` (μm→mm) and `m_plate_depth = bed_bb.size().y()/1000.0` from the SAME `bed_bb` it passes to `arrange_objects`, and write them onto `m_plateList` before calling `rebuildPlatesAfterArrangement`.** This guarantees the grid and the arrange coordinate space are identical by construction — no separate config-source plumbing, no drift. For the two load-path calls (which hardcode the 220×220 string), the same parse produces 220×220 automatically.

**Alternative (rejected):** threading bed width/depth as parameters from `EditorViewModel::bedWidth()` adds a cross-layer dependency (viewmodel → service) and duplicates the value. The bed_bb-parse approach is self-contained inside `arrangeObjects` and is the minimal-drift path. The planner should pick this unless a future phase needs plate geometry outside arrange context (then promote to a `PartPlateList::setPlateSize(w,d,h)` called from a bed-change handler).

**For unit tests (ARRANGE-01):** `PartPlateList` unit tests must set `m_plate_width`/`m_plate_depth` directly via a test seam (a `setPlateSize(int,int,int)` setter, or friend access). The geometry math (`compute_origin`, `compute_plate_index`) is independent of HOW the size was set, so tests can use any fixed size (e.g., 200×200 → stride 240).

## 6. 3MF Load Path Consistency — RECOMMENDATION

**Read:** `ProjectServiceMock.cpp:5066` (`Model::read_from_archive`) → plate-data extraction `:5110-5159` → main-thread rebuild lambda `:5237-5342`. The rebuild lambda (`:5302-5328`) constructs a fresh `PartPlateList`, calls `resetToSinglePlate()`, then `createPlate()` per loaded plate, sets name/membership/locked/bedType. **It does NOT call any origin-recompute** — all plates get the default zero origin.

**Then `:5334` auto-arranges** with the hardcoded 220×220 bed. After Phase 29's rewire, this `arrangeObjects` call will (a) parse the bed → set `m_plate_width/depth`, (b) arrange, (c) call `rebuildPlatesAfterArrangement` → which calls `updatePlateOrigins` → origins become real. **So the load path gets origin consistency for free** via the post-arrange rebuild, AS LONG AS `rebuildPlatesAfterArrangement` is wired into `arrangeObjects`.

**Defensive recommendation (CONTEXT code_context §Integration Points flagged this as planner's discretion):** ALSO call `m_plateList->updatePlateOrigins()` (or the equivalent `update_plate_cols()` + `updatePlateOrigins()`) at the end of the rebuild lambda (`:5319-5328`, after the `createPlate` loop). This is cheap and guarantees origins are correct even if the auto-arrange is later removed/conditioned out. **Recommended: YES, add it** — it's defensive, costs one loop, and protects against the auto-arrange being made conditional (e.g., user disabled auto-arrange-on-load). Place it right before `setCurrentPlateIndex(0)` at `:5323`.

**Note:** `m_plate_width`/`m_plate_depth` won't be set at load time yet (no bed parse before arrange). If you add `updatePlateOrigins()` to the load path, set a sane default size first (mirror the 220×220 the auto-arrange will use, or read from the loaded config's `printable_area` if present in `loadedConfigMap`). Simplest: since the auto-arrange immediately follows and re-derives everything, the load-path `updatePlateOrigins()` call is belt-and-suspenders — it can use whatever size is current (default 0 → all origins 0,0,0, which is correct for a 1-plate-loaded state anyway since `compute_origin(0, cols=1) = (0,0,0)`).

## 7. Validation Architecture

**Build command (ONLY):** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (AGENTS.md:15, build-rules). This runs cmake configure (3 retries for AUTOMOC recompaction) + ninja + ctest. Sets up vcvars64, Qt6.10 env, `/Zm300 /bigobj`.

**Test home:** `tests/PartPlateTests.cpp` **DOES NOT EXIST YET** (confirmed via glob — only `QmlUiAuditTests.cpp`, `ViewModelSmokeTests.cpp`, `PrepareSceneDataTests.cpp`, `E2EWorkflowTests.cpp`, `CliTests.cpp` exist). The CONTEXT references it as existing ("Existing `tests/PartPlateTests.cpp` is the home") — this is INACCURATE. **The planner must CREATE `tests/PartPlateTests.cpp`** and register it in `CMakeLists.txt` following the `PrepareSceneDataTests` pattern (CMakeLists.txt:372-382): `qt_add_executable`, `target_include_directories(... PRIVATE src)`, `target_link_libraries(... PRIVATE Qt6::Test)` + the `owzx_app_core` (or directly the PartPlate/PartPlateList sources), `add_test`, `set_tests_properties(... ENVIRONMENT PATH=Qt bin)`.

**Test framework:** `QtTest` with `private slots` (matches PrepareSceneDataTests.cpp:45-60 and ViewModelSmokeTests.cpp). Include `<QtTest>`, class inherits `QObject`, `Q_OBJECT`, `private slots:` test methods. ViewModelSmokeTests.cpp:17-19 already includes `PartPlate.h` + `PartPlateList.h` — so the include path and link setup are proven.

**AUTOMOC warning (CMakeLists.txt:344-352):** single-file tests with cpp-internal `Q_OBJECT` have weak moc dependency tracking. After adding a new slot to the test class, delete `build/PartPlateTests_autogen/timestamp` before incremental builds, OR rely on the canonical verify script which re-runs cmake configure. The verify script handles this.

### 7.1 ARRANGE-01 unit tests (deterministic, no libslic3r model needed)

In `tests/PartPlateTests.cpp` (new file). These test the pure-domain geometry on `PartPlateList` directly:

1. **`compute_colum_count` parity table (1..36):** encode the full table from §1.2 as a data-driven test (`QTest::addColumn<int>` / `QTest::newRow`). Assert each count → expected cols. Cover ALL perfect squares (1,4,9,16,25,36) and all +1 transition points (2,5,10,17,26). This is the highest-drift-risk surface.
2. **`compute_origin` grid math:** with `m_plate_width = m_plate_depth = 200` (stride = 240), `m_plate_cols` set per count, assert:
   - `compute_origin(0, cols)` = (0, 0, 0)
   - `compute_origin(1, cols=3)` = (240, 0, 0)  (col 1)
   - `compute_origin(3, cols=3)` = (0, -240, 0)  (row 1, col 0)
   - `compute_origin(4, cols=3)` = (240, -240, 0)  (row 1, col 1)
   - Sign-flip on Y verified (rows go negative).
3. **`compute_plate_index` round-trip:** for a set of (col, row) → translation pairs, assert `compute_plate_index(translationX, translationY)` returns `row*cols + col`. Include boundary cases:
   - translation exactly on a grid line (e.g., x=240 exactly) — `round()` snaps to nearest, so x=240.0 → col 1, but x=120.0 (midway) → col 0 or 1 depending on float round (round-half-to-even vs round-half-up). **Test both x=240 (clean) and x=239.9 / x=240.1** to characterize the `round()` behavior. Upstream uses C `round()` (round-half-away-from-zero), so 120.0 → 0 (round-half-up... actually round(0.5)=1). Document the exact behavior in the test comment.
4. **`updatePlateOrigins` writes to plates:** after `createPlate` × 4 + `updatePlateOrigins()`, assert `plate(0)->origin() == (0,0,0)`, `plate(1)->origin()` has x>0, etc. Confirms D-29-13.

### 7.2 ARRANGE-02 integration test (through `arrangeObjects`)

**Challenge:** needs a real `Slic3r::Model` with multiple objects (HAS_LIBSLIC3R). The `tests/data/` directory exists but contains only `baseline/` (slice timing baseline JSON) — **no STL/3MF fixture** (FIXTURE-01 is deferred to Phase 32). So the test must construct a model **in-memory**.

**Pattern (mirror ViewModelSmokeTests.cpp which already exercises ProjectServiceMock + libslic3r):**
1. Create a `ProjectServiceMock`, load a tiny STL (ViewModelSmokeTests.cpp:41 already references `third_party/OrcaSlicer/resources/profiles/hotend.stl` as `kStlPath` — reuse this, or `addPrimitive`/load a small mesh).
2. Add/duplicate N objects (N large enough that one plate can't hold them — e.g., 8-10 objects on a 100×100 bed, or arrange with large spacing).
3. Call `arrangeObjects(spacing=large, false, false, "0,0,100,0,100,100,0,100")` (small bed forces overflow).
4. Assert: `plateCount() >= 2` AND objects are distributed (not all instances on plate 0). Query via `objectIndicesOnPlate(0)` vs `objectIndicesOnPlate(1)` — at least one object should be on plate ≥ 1.
5. Assert: `currentPlateIndex()` valid, `plateDataLoaded` signal fired (QSignalSpy).

**Determinism note:** arrange output can vary slightly by spacing/bed. The assertion should be "≥ 2 plates used AND ≥ 1 object off plate 0", NOT exact plate counts (which depend on libnest2d's packing). The CONTEXT D-29-11 demands determinism — use fixed spacing + fixed bed + fixed object count so the result is reproducible on the build machine.

### 7.3 ARRANGE-03 locked-exclusion test

1. Setup as ARRANGE-02 but FIRST place an object on plate 0 and `setPlateLocked(0, true)` with a known instance membership.
2. Add 2-3 MORE objects.
3. Call `arrangeObjects(...)`.
4. Assert: plate 0's `objToInstanceSet()` is UNCHANGED (locked preserved — capture before/after).
5. Assert: new objects land on plate ≥ 1 (their instances are NOT on plate 0).
6. **Edge case (D-29-12):** lock ALL plates → `arrangeObjects` returns false, no membership changes, no crash.

### 7.4 Build verification

Run the canonical script. Expected: cmake configure succeeds (AUTOMOC picks up the new `PartPlateTests.cpp` Q_OBJECT), ninja builds `PartPlateTests` target, ctest runs all 6 test executables green. If the planner adds the test to `CMakeLists.txt` but the script's `ninja -t targets` check doesn't auto-build it, verify the script builds test targets explicitly (read `scripts/auto_verify_with_vcvars.ps1` fully — it has an `Invoke-NinjaTarget` helper; the planner may need to ensure `PartPlateTests` is in the built-target list).

## 8. Subtleties & Gotchas Summary (planner must encode these)

1. **`compute_colum_count` uses FLOAT `sqrt` + `round` + strict `>`** — NOT integer ceil. Replicate the exact form. Perfect squares are the boundary; encode all 6 (1,4,9,16,25,36) in the test.
2. **`compute_plate_index` Y-axis sign flip:** `row_value = (stride_y - translation_y) / stride_y` — decodes the negative-Y-row encoding. Getting this backwards clusters everything on row 0.
3. **`round()` not floor:** translations near boundaries snap to nearest grid line. Characterize 239.9 vs 240.0 vs 240.1 in tests.
4. **`bed_idx` is reset to 0 by `get_arrange_poly`** (ModelArrange.cpp:98) — you CANNOT read per-bed indices from arrange output. Reconstruct from translation via `compute_plate_index` (D-29-5). This is the core architectural reason for Phase 29.
5. **`tolerantVfn` swallows out-of-bed items** — `arrange_objects` returns false but partial translations are still applied. Guard the rebuild behind the bool return, but note that even on `false` some instances may have moved (mirrors upstream behavior — the rebuild still runs and re-derives membership from current translations).
6. **`PartPlate` lacks `empty()` and `has_printable_instances()`** — add `bool empty() const` (mirrors upstream PartPlate.hpp:387) for the recycling loop. `has_printable_instances` needs a model backref Qt6's PartPlate doesn't have — defer, use `empty()` only.
7. **`PartPlate` lacks `contain_instance(obj,inst)`** — use `objToInstanceSet().count({obj,inst})` inline, or add a convenience `containsInstance`.
8. **`tests/PartPlateTests.cpp` does NOT exist** — create it + register in CMakeLists.txt (CONTEXT incorrectly assumes it exists).
9. **`tests/data/` has NO model fixture** (only `baseline/`) — ARRANGE-02/03 tests must build models in-memory (reuse `hotend.stl` path from ViewModelSmokeTests or `addPrimitive`). Don't depend on FIXTURE-01 (Phase 32).
10. **D-29-13's claim "GLViewport already reads `PartPlate::origin()`" is INACCURATE** — grep confirms `origin()`/`setOrigin()` are defined but NEVER called in `src/` (only the PartPlate.h definitions). Writing real origins (D-29-13) is correct and necessary, but multi-plate RENDERING will NOT follow automatically — a future GL phase must consume `origin()`. Phase 29's scope is the DATA layer only; do NOT claim rendering benefits in VERIFICATION. Document this so the planner doesn't over-promise.
11. **`m_plate_width/depth` are `int` upstream** (PartPlate.hpp:574-575) and Qt6 PartPlate already uses `int m_width/depth` — keep `int` on PartPlateList to match. Stride/origin math produces `double`.
12. **`update_plate_cols` refreshes BOTH `m_plate_count` and `m_plate_cols`** from list size — call it (not just `compute_colum_count` directly) after structural changes, so `m_plate_count` stays in sync.
13. **Load-path auto-arrange (ProjectServiceMock.cpp:5334) will trigger the new distribution** — objects loaded from a multi-plate 3MF may get re-distributed differently than their saved plate assignment. This is upstream-faithful (upstream also re-arranges on load) but the planner should verify the PLATE-09 round-trip test (when written in Phase 32) still passes, OR gate the load-path arrange to skip when the project already has explicit plate assignments. Flag for Phase 32.

## 9. Files to Touch (planner's edit map)

| File | Change |
|------|--------|
| `src/core/model/PartPlateList.h` | Add `m_plate_cols`, `m_plate_width/depth/height`, `compute_colum_count`, `plate_stride_x/y`, `compute_shape_position`, `compute_origin`, `compute_plate_index`, `update_plate_cols`, `updatePlateOrigins`, `rebuildPlatesAfterArrangement` declarations. |
| `src/core/model/PartPlateList.cpp` | Implement above; add `LOGICAL_PART_PLATE_GAP` constexpr; call `update_plate_cols`+`updatePlateOrigins` in `createPlate`/`deletePlate`/`movePlate`. |
| `src/core/model/PartPlate.h` | Add `bool empty() const`, optionally `bool containsInstance(int,int) const`. |
| `src/core/services/ProjectServiceMock.h` | Add `plateCols`/`plateStrideX`/`plateStrideY` Q_PROPERTY (D-29-15). |
| `src/core/services/ProjectServiceMock.cpp` | Rewire `arrangeObjects` (2416-2479): parse bed → set plate size → arrange → rebuild. Optionally call `updatePlateOrigins` in load path (§6). |
| `tests/PartPlateTests.cpp` | **NEW FILE** — ARRANGE-01 unit tests (geometry parity). |
| `CMakeLists.txt` | Register `PartPlateTests` (pattern: CMakeLists.txt:372-382). |
| No change | `EditorViewModel.cpp` (proxy benefits transparently), `Renderer/*` (data layer only — rendering follows in a future phase, NOT Phase 29). |

## RESEARCH COMPLETE
