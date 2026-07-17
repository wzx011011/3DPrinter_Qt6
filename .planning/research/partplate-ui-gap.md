# PartPlate UI Gap Analysis — Phase 150 (PLATE-01)

**Analyzed:** 2026-07-17
**Scope:** read-only comparison of Qt6 PlateBar UI vs upstream OrcaSlicer PartPlate user-facing behavior for Phase 151 planning.

## Summary

Of PLATE-02..05: **0 greenfield implement, 2 verify-existing, 1 verify + small UI add, 1 real implement**. Phase 151 is ~1 real feature (PLATE-05 thumbnails) + 1 small UI enhancement (PLATE-02 drag-reorder); the rest is verification + Phase 152 ctest locks.

## PLATE-02 — Plate reorder UI

**Verdict: Verify existing + small drag-to-reorder add.**

- Backend real: `ProjectServiceMock::movePlate(oldIndex, newIndex)` → `PartPlateList::movePlate` (`ProjectServiceMock.cpp:1317-1321`).
- UI affordance: right-click menu "左移平板/右移平板" in `PreparePage.qml:678-695` with correct enable guards.
- GAP: no `DragHandler` / `drag.target` on plate card delegate (line 3588-3707) — only DropArea for object-drag (line 3695-3706). User cannot drag a plate card to reorder.
- **Work to close**: add DragHandler + DropArea keyed `["plate-reorder"]` on the plate card delegate; on drop call `editorVm.movePlate(sourceIndex, dropIndex)`. Backend already exists. UI-only.

## PLATE-03 — Per-plate print sequence dialog

**Verdict: Verify existing — fully implemented, beyond upstream parity.**

- PlateSettingsDialog at `PreparePage.qml:824-1393` opened from plate context menu "平板设置".
- Print sequence combo at `PreparePage.qml:893-912` bound to `PlatePrintSequence` enum via `editorVm.platePrintSequence(idx)` + `setPlatePrintSequence(idx, value)`.
- Bonus: full per-layer-range extruder sequence editor at `PreparePage.qml:935-1393` (drag-to-reorder pills, add/remove, complete backend).
- **Work to close**: none for UI. Round-trip ctest belongs to Phase 152.

## PLATE-04 — Per-plate config override editor (scope=Plate)

**Verdict: Verify existing — end-to-end wiring complete, writes to `PartPlate::m_config`.**

- Scope switcher UI: "盘" segment button `LeftSidebar.qml:300-309` → `configVm.requestPlateScope(plateIndex)`.
- ViewModel: `ConfigViewModel::activatePlateScope`/`requestPlateScope` set settingsScope="plate"; `handleOptionValueChanged` branches on `settingsScope_=="plate"` → `projectService_->setPlateScopedOptionValue(plateIdx, key, value)`.
- Backend (HAS_LIBSLIC3R): `ProjectServiceMock::setPlateScopedOptionValue` writes to `PartPlate::config()` via `cfg.option(k, true)` with QVariant-type dispatch. Reads symmetric.
- **Work to close for UI**: none. SettingsDialog + OptionRow + scope segment cover it.
- Known audit flag (v3.0): "setPlateScopedOptionValue exotic-type dispatch lossy" — a verify-existing check, not new UI.

## PLATE-05 — Non-current-plate thumbnails

**Verdict: Refined scope — partially implemented; real gap for session-created plates.**

- QML binding at `PreparePage.qml:3625-3631` correctly requests `editorVm.plateThumbnailBase64(index)` for non-current plates (current plate uses `viewport3d.lastThumbnailData`).
- Backend `ProjectServiceMock::plateThumbnailBase64` returns real bytes when plate has a cached thumbnail; empty when `!p->hasThumbnail()`.
- GAP: `PartPlate::setThumbnail` is called in only 2 places (`ProjectServiceMock.cpp:1007, 6373`), both on the 3MF LOAD path (restoring `pendingPlateThumbnails_`). No scheduler captures thumbnails for plates added/modified during a session. Live capture path `requestGLThumbnail()` is fired only by `onCurrentPlateIndexChanged` for `currentPlateIndex`.
- No Q_INVOKABLE write path from QML → setThumbnail exists.
- **Work to close** (real implement):
  1. Capture scheduler: on plate-content change (or before save), iterate plates, capture thumbnail for each via `requestThumbnailCapture(plateIndex, 128)`.
  2. New `Q_INVOKABLE setPlateThumbnailFromBase64(plateIndex, b64)` on ProjectServiceMock → `PartPlate::setThumbnail(QImage)`.
  3. Or accept "thumbnails only for persisted plates" as documented limitation (lighter scope).

## Phase 151 scope (refined)

- PLATE-02: ~30 lines QML (DragHandler + DropArea on plate card). Real but small.
- PLATE-03: verify + lock (no work).
- PLATE-04: verify + lock (no work).
- PLATE-05: real work — either (a) capture scheduler + Q_INVOKABLE write path (~150 lines), or (b) document-and-defer.

**Recommendation**: Phase 151 = PLATE-02 drag-reorder + PLATE-05 option (a) scheduler + verify-locks for PLATE-03/04. Phase 152 = ctest round-trip locks for PLATE-02..06.
