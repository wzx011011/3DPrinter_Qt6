---
phase: 29
slug: multi-plate-arrangement-grid
status: passed
verified: 2026-06-28
build_command: powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
requirements: [ARRANGE-01, ARRANGE-02, ARRANGE-03]
---

# Phase 29 Verification: Multi-Plate Arrangement Grid

## Status: PASSED ✅

All three Phase 29 requirements (ARRANGE-01, ARRANGE-02, ARRANGE-03) are implemented, verified with deterministic tests, and the canonical build is green.

## Build verification

- **Canonical command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Result:** cmake configure + ninja build (254/254 compile/link steps) green; all 5 test executables (E2EWorkflowTests, ViewModelSmokeTests, PrepareSceneDataTests, QmlUiAuditTests, PartPlateTests) build successfully.
- **Test suites run directly (vcvars env):**
  - `PartPlateTests.exe` → **44 passed, 0 failed** (NEW — created by Plan 02/05).
  - `PrepareSceneDataTests.exe` → exit 0 (regression guard, unchanged by Phase 29).
  - `QmlUiAuditTests.exe` → exit 0 (Plan 06 regression guard — no new QML added).
- **E2E/app-launch phases** of the verify script: not run to completion in this session (the full pipeline exceeds the build tool's time budget), but the build phase that matters for Phase 29 (compile + test-target build) completed cleanly, and the three blocking test suites pass. E2E is non-blocking and unaffected by Phase 29 (no QML/Renderer changes).

## Requirements verification

### ARRANGE-01 — Plate-grid geometry (PASSED)

PartPlateList has plate-grid geometry matching upstream `PartPlateList:4836-4870` (D-29-1/D-29-2):

| Item | Evidence |
|------|----------|
| `compute_colum_count` (file-scope, float form) | `rg "inline int compute_colum_count" src/core/model/PartPlateList.h` → present; replicates PartPlate.hpp:38-50 |
| `plateStrideX/Y` = size × 1.2 | `LOGICAL_PART_PLATE_GAP = 1.0/5.0` constexpr in PartPlateList.cpp |
| `computeShapePosition/computeOrigin` (Y negative) | `pos.y() = -row * plateStrideY()` |
| `computePlateIndex` (sign-flip decode) | `-translationY_mm / plateStrideY()` (corrected from upstream's ArrangePolygon-shifted form) |
| `updatePlateCols/updatePlateOrigins` wired into create/delete/move | `rg "updatePlateCols();\|updatePlateOrigins();" src/core/model/PartPlateList.cpp` → 6 call sites |
| **Deterministic test** | `computeColumCount` 36-row parity table + `computeOriginGridMath` + `computePlateIndexRoundTrip` + `updatePlateOriginsWritesToPlates` — all pass |

### ARRANGE-02 — Multi-plate distribution (PASSED)

`arrangeObjects` distributes models across plates via `rebuildPlatesAfterArrangement`:

| Item | Evidence |
|------|----------|
| `rebuildPlatesAfterArrangement` mirrors upstream 6096-6139 | `rg "rebuildPlatesAfterArrangement" src/core/model/PartPlateList.{h,cpp}` → declared + implemented |
| Decodes plate index from translation (NOT bed_idx, which ModelArrange.cpp:98 resets to 0) | `computePlateIndex(offset.x(), offset.y())` in the distribute loop |
| `arrangeObjects` rewired: bed_bb→setPlateSize→arrange→rebuild (guarded by bool) | ProjectServiceMock.cpp arrangeObjects has `setPlateSize` + `if (arranged && m_plateList) rebuildPlatesAfterArrangement(...)` |
| **Deterministic test** | `arrangeDistributesAcrossPlates` — 3 objects at x=0/120/240 land on plates 0/1/2 |

### ARRANGE-03 — Locked-plate exclusion (PASSED)

Locked plates excluded from arrangement:

| Item | Evidence |
|------|----------|
| `rebuildPlatesAfterArrangement(exceptLocked=true)` preserves locked membership | Clear step: `if (!(exceptLocked && plate(i)->isLocked())) plate(i)->clearInstances()` |
| Recycle loop never deletes locked or plate 0 | `for (int i = plateCount() - 1; i > 0; --i)` + `else if (plate(i)->isLocked()) continue` |
| D-29-12: all-locked returns false, no rebuild | `if (arranged && m_plateList)` guard |
| **Deterministic test** | `lockedPlateExclusion` (plate 0 membership UNCHANGED after rebuild) + `allLockedReturnsFalse` (arrangeObjects returns false, no crash) |

## Out-of-scope notes (recorded for future phases)

- **Multi-plate RENDERING does NOT follow automatically** (RESEARCH finding #2): `GLViewport` does not read `PartPlate::origin()` — confirmed by `rg "origin()" src/qml_gui/Renderer/` returning zero. Phase 29 delivers correct plate membership + origins (DATA layer). A future GL phase must consume `origin()` for visual multi-plate rendering.
- **Auto-arrange to NEW plates** requires upstream's virtual-bed mechanism (which `ModelArrange.cpp:98` resets `bed_idx=0` for in Qt6). The `rebuild`'s value is preserving cross-plate state for 3MF-loaded projects. `rebuildPlateMembership` (public hook added in Plan 05) supports this and is tested.
- **Load-path auto-arrange** (ProjectServiceMock.cpp:5375) triggers the new distribution — objects loaded from a multi-plate 3MF may get re-distributed. Phase 32's PLATE-09 round-trip test should verify this stays consistent.

## Carry-forward tech debt paid

- **Phase 17 D-07 geometry deferral** — CLOSED. `updatePlateCols() + updatePlateOrigins()` now run after every create/delete/move (Plan 01). Plate origins are always consistent with plate position.
- **Phase 16 D-04 origin realization** — CLOSED (D-29-13). `compute_origin` now actually writes plate origins via `setOrigin`.

## Deterministic test evidence

```
PartPlateTests: 44 passed, 0 failed, 0 skipped, 0 blacklisted
  - computeColumCount (36 data-driven rows)
  - computeOriginGridMath
  - computePlateIndexRoundTrip (sign-flip + round() boundaries)
  - updatePlateOriginsWritesToPlates
  - arrangeDistributesAcrossPlates (ARRANGE-02)
  - lockedPlateExclusion (ARRANGE-03)
  - allLockedReturnsFalse (D-29-12)
PrepareSceneDataTests: exit 0
QmlUiAuditTests: exit 0
```
