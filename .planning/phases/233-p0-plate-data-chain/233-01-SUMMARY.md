# Phase 233 Summary — Plate Data Chain

**Completed:** 2026-08-15
**Requirements:** PLATE-01..05, ENGN-04 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all 8 suites,
app launches). One intermediate run failed PreviewParserTests — the known
stale-object-ABI regression after the ProjectServiceMock.h member change; the
purge list had missed the PreviewParserTests/Gizmo*/InventoryAudit target
dirs. After purging those too, all green. (Lesson recorded: purge ALL test
target dirs after any ProjectServiceMock.h member change.)

## Changes (all in ProjectServiceMock.* / EditorViewModel.cpp / BackendContext.cpp / main.qml)

1. **PLATE-01 bed type → slicing**: `setPlateBedType` now syncs
   `curr_bed_type` into `p->config()` (coEnum setInt), mirroring the 3MF write
   side. Load rebuild paths (upstream-format + JSON) sync it too.
2. **PLATE-02 spiral → slicing**: `setPlateSpiralMode` writes `spiral_mode`
   via the typed Bool accessor; load rebuild paths sync.
3. **PLATE-03 filament sequences → slicing + 3MF**:
   - New `syncPlateOtherLayersSeqConfig` helper (flattened coInts + nums;
   "auto" erases both keys). All five other-layers mutators call it.
   - `setPlateFirstLayerSeqChoice/Order` write/erase
   `first_layer_print_sequence` (coInts).
   - 3MF upstream-format write (buildPlateDataList) persists both key groups
   (upstream bbs_3mf plate config block).
   - 3MF upstream-format read captures them into new
   `pendingPlateFirstLayerSeq_/pendingPlateOtherLayersSeq_[Nums]_` and the
   rebuild restores fields + config (other-layers split into N equal
   per-range orders, upstream reconstruction semantics).
   - JSON project restore path also syncs all keys.
4. **PLATE-04 exact membership on load**: loadFile now uses
   `rebuildPlateMembership` (geometry re-derive, no object moves, no plate
   recycling) when the file carried plate data (any 3MF); plain STL/OBJ/STEP
   imports keep auto-arrange. loadProject (project restore) never arranges —
   matches upstream load_project.
5. **PLATE-05 dirty state**: `ProjectViewModel::markDirty()` added;
   BackendContext wires `ProjectServiceMock::projectChanged` → markDirty
   (loads excluded via loading()). Guards: new-project dialog now only opens
   when dirty; open-project and window-close (onClosing) route through the
   same guard (pendingOpenAfterGuard / pendingQuitAfterGuard), confirm
   discards and proceeds, cancel aborts. Save paths already clear dirty.
6. **ENGN-04 empty-plate slice-all**: `requestSliceAll` skips plates with
   zero objects so one empty plate no longer aborts the whole queue.

## Tests

- `plateSettingsSyncIntoPlateConfig` (new): bed type/spiral/first+other-layer
  sequences verified via new `plateConfigValue()` readback API (int/bool/
  ints-list) — proves the keys land in the plate DynamicPrintConfig that
  SliceService merges; "auto" erases keys.
- `projectEditsDriveDirtyState` (new): markDirty → dirty; save/open clear.
- PartPlateTests existing multi-plate round-trips still green under the new
  no-arrange load semantics.

## Evidence

Canonical run 2026-08-15 08:13: PrepareScene/PartPlate/ObjectPicking/
ViewModelSmoke (incl. 2 new tests)/QmlUiAudit/ViewportContext/PreviewParser/
E2E pipeline all passed, APP_RUNNING_PID=41224, exit 0.
