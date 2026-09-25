---
task: 260822-x4n
status: complete
completed: 2026-08-23
---

# Quick Task 260822-x4n: Fix multi-plate P1 gaps

Plate-switch slicing guard (B1), delete-plate instance migration (B2), and
all-plates print/export readiness gates (B3), plus landing the pre-existing
uncommitted P0.5.5b slice-readiness work as its own commit.

## What was done

### Task 1 — Land pending P0.5.5b work (commit `2834499`)
- Verified the 8-file uncommitted slice-readiness change-set via the canonical
  build, then committed it alone by explicit paths:
  `feat(slicing): P0.5.5b slice-readiness gating (per-instance outside set + pre-flight slice gates)`.
- Scope: PartPlate instanceOutsideSet + updateSliceReadiness, PartPlateList
  findInstance/findInstanceBelongs/isAllPlatesReadyForSlice,
  ProjectServiceMock isPlateReadyForSlice/currentPlateCanSlice,
  SliceService startSlice pre-flight + validated startSlicePlate,
  EditorViewModel readiness gating, 3 new PartPlateTests slots.

### Task 2 — B2 delete-plate instance migration (commit `7ac990f`)
- `PartPlateList::deletePlate` now migrates the deleted plate's instance
  memberships to the next plate (previous plate when deleting the last one),
  matching upstream PartPlate.cpp:3710-3761 ("add them into next plate if
  have") instead of dropping them.
- Tests: `deletePlateMigratesInstancesToNeighbor` locks the migration and
  rejects the old orphaning behavior.

### Task 3 — B1 switch guard + B3 readiness gates (commit `8526271`)
- **B1**: `SliceService::canSwitchPlate()` (`!slicing()`) mirrors upstream
  BackgroundSlicingProcess::can_switch_print (BackgroundSlicingProcess.cpp:140-155);
  `EditorViewModel::setCurrentPlateIndex` refuses plate switches while a slice
  is RUNNING and reports a status message (upstream Plater.cpp:13879 gate).
  Programmatic service-level call sites stay unguarded so mid/post-slice
  internal flows keep working.
- **B3**: `PartPlateList::isAllSliceResultsValid` /
  `isAllSliceResultsReadyForPrint` / `isAllSliceResultReadyForExport` mirror
  upstream PartPlate.cpp:4989-5044 loop-for-loop; per-plate predicates live on
  PartPlate (isSliceResultReadyForPrint / hasPrintableInstances /
  isAllInstancesUnprintable proxy / isSliceResultReadyForExport).
  SliceService keeps the domain validity flag in sync with its result store at
  every mutation point and gates `exportAllPlateGCodeToDirectory`;
  `ProjectServiceMock::exportGcode3mf` requires an export-ready plate
  (upstream PartPlate.hpp:437). Existing `isAllPlatesReadyForSlice` untouched —
  its any-plate semantics already match upstream PartPlate.cpp:5047-5055.
- Tests: `allPlatesAggregateReadinessTruthTables` (PartPlateTests),
  `editorPlateSwitchRefusedWhileSlicing` (ViewModelSmokeTests, drives a real
  slice end to end incl. refusal + recovery), E2EWorkflowTests
  slice-all updated for the new fail-closed export gate.

## Deviation fixed during verification

First canonical run: E2EWorkflowTests 28/1 —
`test_slice_all_stores_outputs_for_printable_unlocked_plates_only` failed on
`setCurrentPlateIndex(2)`. Root cause was NOT the new guard: the executor's
inserted `removeAllOnPlate(2)` triggers pre-existing empty-plate pruning in
`ProjectServiceMock::deleteObject` (commit 630e6ea), so plate 2 stopped
existing before the later switch assertions. Fix: moved the per-plate
activation checks before the removal and added an explicit assertion that the
pruned index now refuses switching.

## Verification

Canonical command `scripts/auto_verify_with_vcvars.ps1`, final run green:

| Suite | Result |
|-------|--------|
| PartPlateTests | 60 passed, 0 failed |
| ViewModelSmokeTests | 161 passed, 0 failed, 1 skipped (pre-existing conditional) |
| E2EWorkflowTests | 29 passed, 0 failed |

## Files touched (Task 3)

src/core/model/PartPlate.h, src/core/model/PartPlateList.{h,cpp},
src/core/services/ProjectServiceMock.{h,cpp},
src/core/services/SliceService.{h,cpp},
src/core/viewmodels/EditorViewModel.cpp,
tests/{PartPlateTests,ViewModelSmokeTests,E2EWorkflowTests}.cpp

## Commits

| Task | Commit |
|------|--------|
| P0.5.5b landing | `2834499` |
| B2 delete migration | `7ac990f` |
| B1 guard + B3 gates | `8526271` |

## Known follow-ups (out of scope)

- Empty-plate auto-pruning on object delete (630e6ea) contradicts upstream,
  where plates persist until explicitly deleted — candidate source-truth fix.
- Remaining multi-plate gaps from the gap analysis: slice-all auto-advance UX,
  Preview "Select Plate" strip / All Plates Stats, plates_custom_gcodes
  persistence, single .gcode.3mf all-plates export, per-plate Print pool.
