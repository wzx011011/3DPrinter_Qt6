# Debug: Prepare viewport mouse crash (0xc0000409)

**Status:** RESOLVED (commit `3c116de`, main CI dispatched)
**Date:** 2026-08-22

## Symptom

Mouse interaction on the Prepare page center viewport crashed or froze.
Windows Event Log recorded two `OWzxSlicer.exe` faults (`0xc0000409`,
offset `0x1ca42fd`). cdb stack at crash:

```
Qt6Qml!QQmlData::isSignalConnected+0x9
Qt6Core!QObject::qt_static_metacall+0x10a6
OWzxSlicer!RhiViewport::mousePressEvent+0xd4   -> call RhiViewport::gizmoDragEnd
```

The crash happened while *emitting* a signal, meaning the QObject d-pointer
was already corrupted by an earlier writer (heap corruption, not a bad emit).

## Root cause

`RhiViewportRenderer::renderThumbnailPass()` dereferenced the GUI-owned item
from the render thread:

```cpp
m_viewportItem->cameraMvp(1.0f)   // render thread
```

while GUI-thread mouse/wheel handlers mutated the same `CameraController`
(`orbit` / `pan` / `zoom`) concurrently. `QPointer` only nulls on destroy; it
does not serialize access. Unsynchronized concurrent reads of live QObject /
QMatrix4x4 state can corrupt adjacent heap blocks — exactly the signature seen
(clobbered `QQmlData*` near a live element). Thumbnail capture runs during
normal Prepare use (per-plate thumbnails), so ordinary mouse movement could hit
the race window. The swapchain rebuild storm (initialize called every frame)
remains an environment-amplified symptom (RustDesk/Sunlogin/ToDesk virtual
display injection + Warpal), but the concrete in-product defect was this race.

## Fix (3c116de)

1. Camera snapshot copied only inside `synchronize()`
   (`m_thumbnailCameraMvp` + validity flag); `renderThumbnailPass()` reads the
   renderer-owned snapshot. No render-thread calls into `RhiViewport`.
2. Queued follow-up `update()` uses a local `QPointer` copy.
3. Teardown ordering: deferred readback batch deleted BEFORE thumbnail
   texture/render-target destruction; deterministic `delete` of the thumbnail
   render-pass descriptor; readback result state cleared.
4. Full release now resets previously omitted measure / flatten-hover /
   assembly-measure buffers, upload flags, byte counters, vertex counts.
5. Regression audit:
   `QmlUiAuditTests::rhiViewportThumbnailUsesSynchronizedCameraSnapshot`.

## Verification

- Canonical verifier (`scripts/auto_verify_with_vcvars.ps1`): exit 0;
  all suites green (QmlUiAudit 153, E2E 29, ViewModelSmoke 161, etc.).
- Runtime smoke on fresh build with Prusa.stl loaded, Prepare page:
  5-round sweep (orbit+pan) survived; 10-round heavier stress survived;
  no new CrashDumps entries after the runs (latest dump predates fix).
- Environment caveat kept: remote-desktop injection still amplifies
  swapchain rebuilds; if crashes reappear on THIS machine first close
  RustDesk/Sunlogin/ToDesk before re-diagnosing.
