---
phase: 108-filament-map-auto-recommendation-readback
status: clean
base: 461d4f2
head: HEAD
files_reviewed:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
counts: {critical: 0, warning: 0, info: 2, total: 2}
---

# Phase 108 Code Review — Filament-Map Auto Recommendation Readback

## Verdict: APPROVE (clean)

Phase 108 faithfully mirrors the v4.4 WipeTowerGeometry readback pattern for filament-map auto-recommendation. All five review focus areas pass.

## Verified

1. **Capture-by-value invariant (Frozen Decision 1):** `FilamentMapResult` is a pure value POD (bool + enum + std::vector<int>). `capturedFilamentMap` is a worker-local stack var (SliceService.cpp:431), captured BY VALUE into the GUI-thread delivery lambda (:756). The `std::vector<int> maps` is deep-copied. No `Print*` or libslic3r reference type stored on the signal path.

2. **Success-branch-only gate:** `emit receiver->filamentMapReady(...)` at SliceService.cpp:855 sits on the success branch AFTER the cancel (:764-779) and error (:781-796) early-returns. Same gate as `wipeTowerGeometryReady` (:842). Cancel/error do not emit.

3. **Thread safety:** QueuedConnection at :756/:856. The only cross-thread handoff copies the POD by value. Signal emitted from the GUI-thread lambda; slot runs synchronously on GUI thread.

4. **`OWzx::FilamentMapMode` ↔ `int` static_cast lossless:** Upstream `PrintConfig.hpp:424-429` and Qt6 `PartPlate.h:96-101` declare identical numeric values (0/1/2/3). Round-trips cleanly.

5. **`mode < fmmManual` valid gate matches upstream:** SliceService.cpp:687 `mapModeInt < 2` == upstream Print.cpp:2487 `map_mode < fmmManual`. The **1-based group id transform verified**: upstream Print.cpp:2488-2490 applies `+1` then `update_filament_maps_to_config` writes 1-based values; `get_filament_maps()` (:3051-3053) returns 1-based. No off-by-one.

## Findings (2 nits, no behavior impact)

| # | Severity | Finding |
|---|----------|---------|
| F6 | info | SliceService.h:90-93 doc overclaims the worker "resolves fmmDefault against the resolved mode" — the worker reads `print.get_filament_map_mode()` RAW and static_casts unconditionally; there is NO fmmDefault resolution logic in the worker. Harmless: the valid gate (`mode < fmmManual`) excludes fmmDefault, so it never reaches the autoFilamentMapMode Q_PROPERTY (the slot only writes mode when valid=true). Recommend rewording to "mode is read raw; fmmDefault never surfaces because the valid gate excludes it." |
| F7 | info | Worker writes `mode` unconditionally before the valid gate (SliceService.cpp:685-686), so the field is populated even for the valid=false (Manual/Default) branches. The slot ignores mode when valid=false, so behavior is correct, but the assignment could move inside the `if (mapModeInt < fmmManual)` block for symmetry with `maps`/`valid`. Cosmetic only. |

## Test Quality

`filamentMapAutoRecommendationReadbackWired` (ViewModelSmokeTests.cpp:4071-4137) registers FilamentMapResult as a metatype, asserts the default inert state, drives the signal via by-name invokeMethod (exercising the connect wiring), asserts valid path surfaces mode+maps through all 3 Q_PROPERTYs, asserts invalid path forces hasAuto=false while prior maps/mode persist (the FMAP-01 gate guarantee). QSignalSpy locks the NOTIFY. `filamentMapAutoRecommendationReadbackPresent` source-audits all 4 anchors. Appropriate coverage for a readback-wiring phase. Uses DirectConnection in invokeMethod (not the production QueuedConnection) — same acceptable simplification as the v4.4 wipe-tower test.

## Conclusion

Clean execution. FMAP-01 closed. The readback contract (capture-by-value, success-branch-only, lossless enum cast, 1-based group ids, mode<fmmManual valid gate) is locked and ready for Phase 110 (FilamentGroupPopup UI) to bind the 3 Q_PROPERTYs.

Regression ctest all pass (PrepareSceneData, PartPlate, ViewModelSmoke incl. new slot, QmlUiAudit incl. new slot, PreviewParser). OWzxSlicer.exe links clean. `git diff --check` clean; encoding guard clean.
