# Phase 156: Runtime Plate Thumbnail Capture Scheduler

**Status:** Ready to execute
**Workstream:** CLOS (PLATE-05 closure)
**Requirement:** CLOS-03

## Goal

Add the session-capture loop that Phase 151 shipped without. Phase 151 ships
persisted-plate thumbnails only (the read accessor `plateThumbnailBase64` + the
3MF save/load round-trip). The runtime gap: a non-current plate created or
modified in-session has a blank card because (a) the only capture trigger is
plate-switch, and (b) `RhiViewport::deliverThumbnail` discards the `plateIndex`
so captured bytes never reach `PartPlate::setThumbnail` for non-current plates.

## Source-truth mapping

Upstream `GLCanvas3D` + `Plater::update_thumbnails` captures thumbnails on
content-change + before-save, one per plate, routed back into
`PartPlateList::plate(i)->thumbnail_data`. We mirror the same shape: capture
trigger + per-plate routing into `PartPlate::setThumbnail(QImage)`.

## Plan

### Wave 1 — Carry plateIndex through thumbnailCaptured

1. `src/qml_gui/Renderer/RhiViewport.h/.cpp`:
   - Change `thumbnailCaptured()` signal to `thumbnailCaptured(int plateIndex, const QString &data)`.
   - Remove the `Q_UNUSED(plateIndex)` in `deliverThumbnail` — emit the real plateIndex.
   - Keep `lastThumbnailData` Q_PROPERTY as-is (back-compat for QML bindings that
     still read it for the current plate's live preview).

### Wave 2 — setPlateThumbnailFromBase64 write path

2. `src/core/services/ProjectServiceMock.h/.cpp`: add
   `Q_INVOKABLE bool setPlateThumbnailFromBase64(int plateIndex, const QString &b64)`.
   Decode `QByteArray::fromBase64` → `QImage::fromData(bytes, "PNG")` →
   `m_plateList->plate(idx)->setThumbnail(img)`. Null-guard + bounds-check.
   Strips a `data:image/png;base64,` prefix if present.

3. `src/core/viewmodels/EditorViewModel.h/.cpp`: add
   `Q_INVOKABLE bool setPlateThumbnailFromBase64(int, const QString&)` proxy
   mirroring the existing `plateThumbnailBase64` read accessor proxy.

### Wave 3 — QML wiring: per-plate capture + scheduler trigger

4. `src/qml_gui/pages/PreparePage.qml`:
   - Update `onThumbnailCaptured(plateIndex, data)` to call
     `root.editorVm.setPlateThumbnailFromBase64(plateIndex, data)` so the
     captured bytes persist into the PartPlate thumbnail cache (and thus into
     the next 3MF save). This closes the per-plate routing gap.
   - Add a small session-capture scheduler: a Timer that fires on
     `editorVm.projectChanged` (or before-save via a `Connections` handler on
     the existing `backend.canSave` state) and iterates plates calling
     `viewport3d.requestThumbnailCapture(idx, 128)` for each plate that lacks
     a thumbnail. The renderer's queued delivery means captures serialize
     naturally; we just enqueue one per plate.

### Wave 4 — Test anchor

5. `tests/QmlUiAuditTests.cpp`: add `v51PartPlateSessionThumbnailWired` slot
   asserting:
   - `setPlateThumbnailFromBase64` exists on ProjectServiceMock + EditorViewModel
   - `RhiViewport.h` declares `thumbnailCaptured(int plateIndex, const QString &data)`
     (i.e. the signal carries the plate index, not a no-arg legacy signal)
   - `PreparePage.qml` calls `setPlateThumbnailFromBase64` from the capture handler

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- 5/5 ctest groups PASS
- 12 v5.0 regression slots still pass
- New `v51PartPlateSessionThumbnailWired` slot passes
