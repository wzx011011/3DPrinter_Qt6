# Phase 196 Plan: XD-02 Emboss Spinner and SliceProgress States

**Requirement:** FEAT-01
**Goal:** Add async Emboss feedback (spinner) and SliceProgress Cancelled/Error
state coverage.

## Files

- Modify: `src/core/viewmodels/EditorViewModel.h` (add embossRunning Q_PROPERTY,
  getter, signal, member; add sliceState() Q_INVOKABLE)
- Modify: `src/core/viewmodels/EditorViewModel.cpp` (set running on async
  start/clear on added/failed/cancel; implement sliceState() + embossRunning())
- Modify: `src/core/services/SliceService.h` (add sliceState Q_PROPERTY +
  sliceStateChanged signal)
- Modify: `src/core/services/SliceService.cpp` (emit sliceStateChanged after
  every stateChanged)
- Modify: `src/qml_gui/pages/PreparePage.qml` (Emboss panel: spinner via
  CxBusyIndicator + dynamic status text)
- Modify: `src/qml_gui/panels/SliceProgress.qml` (Cancelled/Error banner
  bound to sliceState enum)

## Steps

- [x] EditorViewModel.h: add Q_PROPERTY(bool embossRunning), getter, signal
      embossRunningChanged, member m_embossRunning; add Q_INVOKABLE sliceState().
- [x] EditorViewModel.cpp: set m_embossRunning=true before
      addTextVolumeAsync; clear in embossVolumeAdded/Failed/cancelEmboss lambdas;
      implement embossRunning() + sliceState().
- [x] SliceService.h: add Q_PROPERTY(State sliceState) + sliceStateChanged
      signal.
- [x] SliceService.cpp: emit sliceStateChanged() after every emit
      stateChanged() (covers all state transitions including worker-thread
      QueuedConnection callbacks).
- [x] PreparePage.qml: Emboss async button gets a CxBusyIndicator neighbor;
      button disabled while running; status text shows 生成中 vs placeholder.
- [x] SliceProgress.qml: add sliceStateEnum/sliceCancelled/sliceError
      properties; add a Cancelled/Error banner using Theme.bgErrorSubtle/
      bgWarningSubtle + statusError/statusWarning.

## Verify

- [ ] `grep -n "embossRunning" src/core/viewmodels/EditorViewModel.{h,cpp}` shows
      the property + setter sites.
- [ ] `grep -n "sliceStateChanged" src/core/services/SliceService.{h,cpp}` shows
      signal + emit sites.
- [ ] Canonical build exits 0 (pending environment fix).
- [ ] ViewModelSmokeTests PASS (embossRunning + sliceState exposed).
- [ ] Launch liveness confirmed.
