---
phase: 29
status: clean
reviewed: 2026-06-28
findings_fixed: 2
---

# Phase 29 Code Review

## Status: CLEAN ✅ (after 2 fixes)

Inline review of the Phase 29 change surface (PartPlateList.{h,cpp}, ProjectServiceMock.{h,cpp}, tests/PartPlateTests.cpp, CMakeLists.txt, auto_verify_with_vcvars.ps1).

## Findings

### FIXED — P1: InfiniteBed divide-by-zero in computePlateIndex

**Location:** `ProjectServiceMock.cpp` arrangeObjects InfiniteBed fallback path.

**Issue:** The InfiniteBed fallback called `rebuildPlatesAfterArrangement` without first calling `setPlateSize`, so `m_plate_width/depth` could be 0 (or stale). With stride 0, `computePlateIndex` divides by zero → inf/nan → wrong plate-index decode (or NaN propagation).

**Fix:** InfiniteBed has no bounding box, so multi-plate distribution is not meaningful there (arrange packs onto one infinite bed). Removed the rebuild call from the InfiniteBed path (the bed_bb path handles multi-plate). Added a comment explaining why. Also added a defensive guard in `computePlateIndex` (`if (strideX <= 0.0 || strideY <= 0.0) return 0;`) so future callers that forget `setPlateSize` get plate 0 instead of NaN.

### FIXED — P2: computePlateIndex zero-stride defensive guard

**Location:** `PartPlateList.cpp` computePlateIndex.

**Issue:** No guard against `m_plate_cols == 0` or stride == 0 if a caller forgets `setPlateSize`/`updatePlateCols`.

**Fix:** Added `if (strideX <= 0.0 || strideY <= 0.0) return 0;` early-return. Cached `strideX`/`strideY` to avoid recomputation.

## No-action items (reviewed, acceptable)

- **Naming convention (camelCase methods + snake_case fields + snake_case file-scope `compute_colum_count`)** — documented in Plan 01's "Naming convention" section; matches CONVENTIONS.md and mirrors upstream for the free function. Acceptable.
- **`computePlateIndex` decode divergence from upstream** — the `-world_y/stride_y` form (vs upstream's `(stride_y - t_y)/stride_y`) is a justified, documented divergence: upstream operates on the pre-shifted ArrangePolygon translation space; Qt6 reads raw world offsets. Tested by `computePlateIndexRoundTrip`. Acceptable.
- **`rebuildPlateMembership` public Q_INVOKABLE added for testability** — minimal surface increase; useful for 3MF load-path consistency; documented in Plan 05 SUMMARY. Acceptable.
- **DATA-LAYER-ONLY (no rendering)** — correctly scoped; `origin()` is written but `GLViewport` doesn't read it (confirmed). Documented in VERIFICATION out-of-scope notes. Acceptable.

## Re-verification after fixes

- `owzx_app_core` + `PartPlateTests` compile green.
- `PartPlateTests.exe` → **44 passed, 0 failed** (unchanged from pre-review; the fixes are defensive and don't change tested behavior).
