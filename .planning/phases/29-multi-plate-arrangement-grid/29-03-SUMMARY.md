---
plan: 29-03
phase: 29
status: complete
requirements: [ARRANGE-02]
---

# Plan 29-03 Summary: rebuildPlatesAfterArrangement + Model backref

## What was built

Added `PartPlateList::rebuildPlatesAfterArrangement(bool exceptLocked, bool recyclePlates)` — the ARRANGE-02 domain logic that reconstructs per-plate membership from post-arrange world translations via `computePlateIndex`. Mirrors upstream `rebuild_plates_after_arrangement` (`PartPlate.cpp:6096-6139`).

Flow:
1. **CLEAR** non-locked memberships (preserve locked when `exceptLocked=true`).
2. **RE-DISTRIBUTE** via `computePlateIndex` on each instance's `get_offset()` (creates plates up to `kMaxPlateCount` as needed). Diverges from upstream's bbox-intersection `reload_all_objects` because the Qt6 `PartPlate` lacks a model backref — `computePlateIndex` is the natural fit (D-29-5/D-29-7).
3. **RECYCLE** trailing empty non-locked plates (reverse iteration, `i > 0` so plate 0 NEVER deleted, locked never deleted, `break` on first non-empty/non-locked).
4. **Refresh origins** for all surviving plates.

Added a `Slic3r::Model* m_model` backref + `setModel()` setter (HAS_LIBSLIC3R) so the rebuild can enumerate `model->objects[*].instances[*]` without taking libslic3r types in the public `rebuildPlatesAfterArrangement` signature (keeps PartPlateList includable without libslic3r).

## Key decisions / deviations

- **`bed_idx = 0` reset (ModelArrange.cpp:98)** is the core architectural reason: `arrange_objects` resets per-bed indices, so the per-plate split MUST be reconstructed from translations via `computePlateIndex` (D-29-5).
- The upstream `sort by arrange_order` (PartPlate.cpp:6103) is intentionally omitted — `computePlateIndex` decodes each instance's plate directly, so ordering is irrelevant (comment explains the divergence).
- `has_printable_instances` deferred (needs model backref on PartPlate); uses `empty()` (Plan 01) only.

## Verification

- `owzx_app_core` compiles green.
- Exercised end-to-end by Plan 05's integration tests (44 passed, 0 failed).

## Files changed

- `src/core/model/PartPlateList.h` — `rebuildPlatesAfterArrangement` declaration + `setModel` + `m_model` + `<libslic3r/Model.hpp>`.
- `src/core/model/PartPlateList.cpp` — implementation (clear/distribute/recycle/refresh).
