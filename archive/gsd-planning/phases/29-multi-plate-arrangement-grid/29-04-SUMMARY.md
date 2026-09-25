---
plan: 29-04
phase: 29
status: complete
requirements: [ARRANGE-02, ARRANGE-03]
---

# Plan 29-04 Summary: Rewire arrangeObjects + plate-grid Q_PROPERTY

## What was built

Rewired `ProjectServiceMock::arrangeObjects` (ProjectServiceMock.cpp:2416+) to distribute models across plates:
1. Derives `m_plate_width/depth` from the SAME `Slic3r::BoundingBox bed_bb` it passes to `arrange_objects` (μm→mm via `/1000.0`) — D-29-3 minimal-drift source (RESEARCH §5). Calls `setPlateSize` + `setModel` before arrange.
2. Captures the arrange bool; on success, calls `rebuildPlatesAfterArrangement(exceptLocked=true, recyclePlates=true)` + emits `plateDataLoaded(newCount)`.
3. **D-29-12:** the rebuild is guarded behind the bool return (all-locked returns false → no rebuild, no membership changes).
4. **Locked exclusion (ARRANGE-03):** `exceptLocked=true` skips clearing locked-plate membership. Minimal-determinism path (full `preprocess_arrange_polygon` deferred per CONTEXT — Qt6's `arrange_objects` overload doesn't expose the ArrangePolygon list).

Added the `plateCols`/`plateStrideX`/`plateStrideY` Q_PROPERTY (D-29-15) on ProjectServiceMock, NOTIFY `plateDataLoaded` (cols/stride derive from count).

## Key decisions / deviations

- **Q_INVOKABLE signature UNCHANGED** — `bool arrangeObjects(float, bool, bool, const QString&)`. `EditorViewModel::arrangeAllObjects` proxy (EditorViewModel.cpp:3855+) needs NO change; benefits transparently.
- The tolerant VirtualBedFn already in `arrangeObjects` is compatible — `bed_idx != 0` items don't throw, `arrange_objects` returns false.
- DATA-LAYER-ONLY — multi-plate rendering does NOT follow (GLViewport doesn't read `origin()`).

## Verification

- `owzx_app_core` compiles green.
- `arrangeObjects` signature unchanged; `EditorViewModel` proxy untouched.
- Exercised by Plan 05's `allLockedReturnsFalse` test (D-29-12).

## Files changed

- `src/core/services/ProjectServiceMock.h` — plateCols/plateStrideX/plateStrideY Q_PROPERTY + accessors.
- `src/core/services/ProjectServiceMock.cpp` — `arrangeObjects` rewired (bed_bb→setPlateSize, arrange, rebuild guarded by bool return).
