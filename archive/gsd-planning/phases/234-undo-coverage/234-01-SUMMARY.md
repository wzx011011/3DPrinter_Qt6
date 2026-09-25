# Phase 234 Summary — Undo Coverage And Fidelity

**Completed:** 2026-08-15
**Requirements:** UNDO-01..06 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all 8 suites,
app launches; ViewModelSmokeTests 127 passed incl. 6 new Phase 234 tests).
Iterations: (1) private `ModelObject::set_model` → replaced with
`add_object(const ModelObject&)` deep-clone + rotate; (2) ModelConfig read via
`.get().option<T>()`; (3) two test fixtures needed `rebuildAndNotify()` after
service-level mutations and the volume fixture switched from assembleObjects
to `addPrimitive(objIdx, type)` (assembleObjects leaves the modelCount_ mirror
stale — noted as pre-existing); (4) one environment flake (PowerShell-pipe
test crash + an AV .obj lock) passed on rerun with the same binary.

## Changes

**New service APIs (ProjectServiceMock)** — upstream truth: whole-model
snapshots (Plater::_take_snapshot):
- `captureFullObjectSnapshot(i)` / `restoreFullObjectSnapshot(bytes, insertAt,
  name, printable, visible, plate)` — single-object 3MF round-trip via
  store_3mf/read_from_file + `add_object` deep clone; mirror arrays and plate
  membership resynced.
- `captureVolumeMeshSnapshot(o,v)` / `restoreVolumeSnapshot(o,v,...)` — volume
  its + type + name + 4x4 transform; restores into the original slot.
- `capturePlateListSnapshot(deep)` / `restorePlateListSnapshot` — QDataStream
  blob of the full plate list (names/locked/printable/bed/seq/spiral/filament
  sequences/maps/membership/config-serialized + optional per-member 3MF deep
  snapshots).
- `capturePaintSnapshot / restorePaintSnapshot / deserializePaintIntoSelector`
  — FacetsAnnotation bytes + PaintEngine selector resync.
- `syncLayerRangesToModel(i)` — bridges mock layer ranges into
  `ModelObject::layer_config_ranges` (Slicing.hpp:147) incl. upstream
  GUI_ObjectLayers overlap-trim (split/trim/remove); every layer-range mutator
  calls it → variable layer heights now reach slicing.

**Commands (UndoCommands.h/.cpp)**:
- DeleteObjectsCommand: full-3mf snapshots captured pre-delete; undo restores
  complete mesh+transforms+plate (records restoredIndex); redo re-deletes by
  captured identity, name-match fallback; skip-first-redo guard.
- AddObjectCommand: full snapshot; redo restores the real object (was an empty
  name shell).
- VolumeDeleteCommand: instantiated (was declared-never-used); wired into
  deleteVolume/deleteSelectedVolumesBySource (macro for multi).
- PlateCommand (new): before/after plate-list blobs; covers add/delete/move/
  clone/lock/printable — all six EditorViewModel plate ops now push.
- PaintCommand (new): per-stroke FacetsAnnotation before/after; mergeWith
  coalesces consecutive strokes on the same (object, volume, kind); undo/redo
  resyncs the paint selector + invalidates slicing.

**Paste fidelity (UNDO-05)**: copySelectedObjects captures per-object 3MF
snapshots; pasteObjects restores full objects with +5mm X anti-overlap and
plate assignment; old name-only path kept as no-snapshot fallback.

## Tests (ViewModelSmokeTests)

`deleteUndoRestoresFullObject`, `pasteSnapshotRestoresMeshFidelity`,
`volumeDeleteUndoRestoresMesh`, `layerRangesReachModelConfig`,
`plateOperationsUndoRestoresState`, `paintStrokeUndoRestoresFacets` —
including mesh-topology equality, insertAt ordering, overlap-trim invariants,
plate undo/redo cycles, and paint byte-identity round-trips.

## Noted pre-existing (not regressions)

- assembleObjects does not refresh the modelCount_/objectNames_ mirror until a
  VM rebuild (UI unaffected; noted for a future hygiene pass).
- save_object_mesh backup temp-file error lines (mojibake names) appear in
  logs across all recent green runs — BBS backup noise, benign.
