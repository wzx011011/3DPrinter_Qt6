---
plan: 29-01
phase: 29
status: complete
requirements: [ARRANGE-01]
---

# Plan 29-01 Summary: PartPlateList Plate-Grid Geometry + PartPlate::empty()

## What was built

Landed the ARRANGE-01 plate-grid geometry foundation on `PartPlateList` mirroring upstream `PartPlate.cpp:3905-3964,4836-4892,5365-5376`:

- **File-scope `compute_colum_count(int)`** — replicates `PartPlate.hpp:38-50` exactly (float sqrt + round + strict `>`, NOT integer ceil). Kept snake_case to mirror upstream (NOT a member).
- **`plateStrideX()/plateStrideY()`** — `size * (1 + LOGICAL_PART_PLATE_GAP)` where `LOGICAL_PART_PLATE_GAP = 1.0/5.0` (`PartPlate.cpp:55`), file-private constexpr.
- **`computeShapePosition/computeOrigin`** (HAS_LIBSLIC3R) — y goes NEGATIVE for rows below the first (`PartPlate.cpp:3960-3961`).
- **`computePlateIndex(double, double)`** — pure-double decode (keeps PartPlateList free of ArrangePolygon). **Sign-flip decode: `-translationY_mm / stride_y`** (corrected from RESEARCH's `(stride_y - t_y)/stride_y` form — see Decisions).
- **`updatePlateCols/updatePlateOrigins/setPlateSize`** maintenance helpers; `updatePlateOrigins` has dual HAS_LIBSLIC3R / fallback branches.
- **Wired `updatePlateCols() + updatePlateOrigins()` into createPlate/deletePlate/movePlate** — closes Phase 17 D-07 geometry deferral (origins are now always consistent with plate position).
- **`PartPlate::empty()`** mirrors upstream `PartPlate.hpp:387` (for the recycling loop in Plan 03).

## Key decisions / deviations

- **`computePlateIndex` sign-flip correction (found via the unit test):** the upstream `compute_plate_index` formula `(stride_y - translation_y)/stride_y` operates on the ArrangePolygon translation space, which is pre-shifted by one stride. Qt6 reads the raw world offset from `ModelInstance::get_offset()` directly, so the correct decode is `-world_y/stride_y` (unshifted inverse of `computeShapePosition`). RESEARCH §1.9 flagged the sign-flip but did not catch the one-stride shift; the `computePlateIndexRoundTrip` test (Plan 02) caught it. Documented in code comments.

## Verification

- `auto_verify_with_vcvars.ps1` builds all targets green (254/254 build steps).
- `rg -n "ArrangePolygon" src/core/model/` → zero matches (pure domain object).
- `rg -n "bool empty() const" src/core/model/PartPlate.h` → accessor present.
- DATA-LAYER-ONLY: `origin()`/`setOrigin` now called from `PartPlateList.cpp`, but `rg "origin()" src/qml_gui/Renderer/` → zero (rendering does NOT follow automatically; out of Phase 29 scope).

## Files changed

- `src/core/model/PartPlate.h` — `empty()` accessor (+1 line).
- `src/core/model/PartPlateList.h` — geometry members + method declarations + `<cmath>` + file-scope `compute_colum_count`.
- `src/core/model/PartPlateList.cpp` — implementations + `LOGICAL_PART_PLATE_GAP` constexpr + structural-call wiring.
