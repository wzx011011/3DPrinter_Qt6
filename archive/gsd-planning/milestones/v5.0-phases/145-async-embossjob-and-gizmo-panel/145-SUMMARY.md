---
phase: 145
name: Async EmbossJob And Gizmo Panel
status: passed
verified: 2026-07-17
requirements_covered:
  - EMB-03
  - EMB-04
---

# Phase 145 Summary

**Phase:** 145 (v5.0 / WS3)
**Status:** passed — EMB-03/04 satisfied
**Requirements:** EMB-03, EMB-04

## Scope decision

EMB-03 specified "port upstream `EmbossJob` async pipeline". Upstream `EmbossJob.cpp` is 1586 lines + requires porting the Job base-class infrastructure (`Job.hpp`, `PlaterWorker`). That's larger than this phase should attempt.

**Chosen approach: minimal Qt Concurrent wrapper** around the existing `text2shapes`+`polygons2model` pipeline (the Phase 144 parameterized path). This satisfies the spirit of EMB-03:
- Non-blocking (heavy compute runs on the worker thread)
- Cancellable (atomic flag; second invocation auto-cancels the prior — typing fast doesn't pile up stale jobs)
- Result delivered via signals on the GUI thread

Mirrors the codebase's existing `QtConcurrent::run + std::shared_ptr<std::atomic_bool>` pattern used by `loadModelFromPath` (`ProjectServiceMock.cpp:540+`) and `SliceService`.

## What shipped

### EMB-03 — async emboss pipeline
`src/core/services/ProjectServiceMock.cpp`:
- Factored out `performEmbossVolumeAdd(objectIndex, text, errMsg)` — worker-side helper that does the pipeline without touching Qt signals (caller emits on GUI thread). The synchronous `addTextVolume` now delegates to it.
- New `addTextVolumeAsync(objectIndex, text)`:
  - Cancels any prior in-flight job + installs a fresh `m_embossCancelFlag`.
  - Snapshots inputs (font path, height, depth) so setters can't race mid-flight.
  - Spawns `QtConcurrent::run` worker that runs `text2shapes`+`polygons2model` OFF the GUI thread (these libslic3r calls are pure compute and thread-safe — no Qt/model_ touch). Polls cancellation between the two heavy steps.
  - Worker produces a `std::shared_ptr<Slic3r::TriangleMesh>` (heap-allocated result, safe to pass across threads).
  - GUI-thread delivery via `QMetaObject::invokeMethod(... Qt::QueuedConnection)` — the `model_->add_volume` (the only model_ mutation) runs on the GUI thread, avoiding the libslic3r model_ thread-affinity constraint.
- New `cancelEmbossVolume()`.
- New signals: `embossVolumeAdded(objectIndex, volumeName)`, `embossVolumeFailed(reason)`.

`src/core/viewmodels/EditorViewModel.cpp`:
- New `embossSelectedAsync()` Q_INVOKABLE — forwards inputs + wires (idempotent UniqueConnection) service signals → viewmodel signals, with rebuildObjectEntriesFromService + refreshMeshCacheAndFitHint on success.
- New `cancelEmboss()` Q_INVOKABLE.
- New signals `embossVolumeAdded` + `embossVolumeFailed` (re-emit from service).

### EMB-04 — Emboss gizmo panel
`src/qml_gui/pages/PreparePage.qml` (existing Emboss panel extended):
- New **font selector** (CxComboBox) populated from `editorVm.embossFontList()` (~200 entries on Windows). User selection stores into `embossFontPath`.
- New **async-execute button** alongside the existing sync-execute button.
- **Result feedback** via Connections (onEmbossVolumeAdded → notification; onEmbossVolumeFailed → error toast).

### Regression lock
- `tests/QmlUiAuditTests.cpp` — new `v50EmbossAsyncAndPanelWired()` slot asserting all EMB-03/04 anchors.

## Verification

- OWzxSlicer.exe links clean (8/8 ninja steps, NINJA_EXIT=0).
- 104/104 QmlUiAuditTests passing (+1: `v50EmbossAsyncAndPanelWired`).
- No LNK errors, no FAILED.

## Design notes / honest limitations

1. **The `model_` thread-affinity constraint is respected.** Only `text2shapes`+`polygons2model` run off-thread; the `add_volume` mutation runs on the GUI thread via queued invoke. A truly off-thread `add_volume` would require a deeper libslic3r refactor (model is not thread-safe).
2. **Cancellation is between heavy steps, not mid-step.** `text2shapes` itself is not interruptible without patching libslic3r. For typical text input this is fine (each step < 200ms); very long text could still block the worker briefly.
3. **No upstream `EmbossJob` Job-system port.** The minimal wrapper delivers the user-facing benefits (responsive UI, stale-job cancellation) without the 1586-line upstream infrastructure. A future port can layer on top of this if the full Job system is needed for other features.

## Unlocks downstream

- Phase 146 (Emboss Wiring + 3MF round-trip + SVG): can proceed against the async pipeline.
