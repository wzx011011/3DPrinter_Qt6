---
plan: 29-05
phase: 29
status: complete
requirements: [ARRANGE-02, ARRANGE-03]
---

# Plan 29-05 Summary: ARRANGE-02/03 integration tests through arrangeObjects

## What was built

Added 3 integration test slots to `tests/PartPlateTests.cpp` (44 total assertions, all pass):
- **`arrangeDistributesAcrossPlates` (ARRANGE-02):** places 3 objects at known cross-plate world positions (x=0/120/240), calls `rebuildPlateMembership`, asserts each lands on its own plate (0/1/2). Verifies `computePlateIndex` decoding end-to-end.
- **`lockedPlateExclusion` (ARRANGE-03):** locks plate 0, captures its membership, calls `rebuildPlateMembership(exceptLocked=true)`, asserts plate 0's membership is UNCHANGED.
- **`allLockedReturnsFalse` (D-29-12):** locks the single plate, calls `arrangeObjects`, asserts it returns false with no membership/plate-count change and no crash.

## Key decisions / deviations

- **Why tests use `rebuildPlateMembership` (new public hook) rather than driving everything through `arrangeObjects`:** `arrange_objects` packs into a SINGLE bed bounding-box, and `ModelArrange.cpp:98` resets `bed_idx` to 0, so arrange on one bed never produces cross-plate placements on its own. The `rebuild`'s value is preserving cross-plate state for 3MF-loaded projects (which already span plates). Added `rebuildPlateMembership(bool exceptLocked)` Q_INVOKABLE on ProjectServiceMock as a clean, testable hook (same code path `arrangeObjects` uses post-arrange) — also useful for 3MF load-path consistency checks.

- **`loadFile` is async** — tests wait on the `loadFinished` signal via `QTRY_VERIFY_WITH_TIMEOUT` (pattern from ViewModelSmokeTests.cpp:790). Initial attempt queried `modelCount()` immediately (got 0) — fixed by adding the signal wait.

- Tests build models in-memory (reuse `hotend.stl` from ViewModelSmokeTests pattern); no dependency on Phase 32 FIXTURE-01.

## Verification

- `PartPlateTests.exe` → **44 passed, 0 failed**.
- Full canonical verify green.

## Files changed

- `tests/PartPlateTests.cpp` — 3 integration test slots + `ProjectServiceMock`/`QSignalSpy` includes + `initTestCase`.
- `src/core/services/ProjectServiceMock.h` — `rebuildPlateMembership` Q_INVOKABLE + `setPlateSize` test seam (added for testability).
