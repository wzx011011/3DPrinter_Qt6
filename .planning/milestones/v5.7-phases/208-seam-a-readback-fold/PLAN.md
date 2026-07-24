# Phase 208 - Seam A Mitigation (MIT-01): Fold thumbnail readback into beginPass

## Context

Phase 207 traced the historical D3D12 0xC0000005 (access violation) to three
candidate seams in `RhiViewportRenderer.cpp`. This phase mitigates **Seam A**:
the only `cb->resourceUpdate(readbackUpdates)` call issued *outside* a
beginPass/endPass pair. All other `resourceUpdate` calls in this file are
folded into a `beginPass(...)` 4th argument; this lone exception issues a
readback batch on the bare command buffer after the offscreen thumbnail pass
has already ended.

In QRhi's D3D12 backend, a `resourceUpdate` issued between two passes (or
after the last pass of a frame) maps to a resource-barrier / copy-queue
operation that the swapchain's frame-close must reconcile. Issuing it directly
on the command buffer mid-frame is not the idiomatic QRhi usage and is a
plausible contributor to the historical render()-inside crash on D3D12. This
is also flagged in the BUG-V31-1 comment block (~cpp:411) as an undefined
pattern in QRhi that D3D12 strictly enforces.

## Problem location

`src/qml_gui/Renderer/RhiViewportRenderer.cpp`, inside `render()` at ~:557:

```cpp
QRhiResourceUpdateBatch *readbackUpdates = rhi()->nextResourceUpdateBatch();
issueThumbnailReadback(readbackUpdates);
cb->resourceUpdate(readbackUpdates);   // <-- Seam A: pass-external issue
```

The offscreen thumbnail pass ends at ~:544 (`renderThumbnailPass(cb)` ends its
own beginPass/endPass), and the readback is then handed to the bare command
buffer. This is the only `cb->resourceUpdate(...)` call in the whole file
that is not the 4th argument of `beginPass(...)`.

## Mitigation

Defer the readback batch to the **next** frame's on-screen `beginPass`. The
batch object survives across frames (it is just a queued list of resource
ops); we stash it on the renderer and merge it into the next frame's update
batch before `beginPass`.

### Design decisions

1. **API choice: `merge()` over re-issue.** Qt docs confirm
   `QRhiResourceUpdateBatch::merge(QRhiResourceUpdateBatch *other)` exists and
   "copies all queued operations from the other batch into this one." After
   `merge`, `other` becomes invalid for submission but must still be released.
   This is cleaner than re-issuing the readback (which would double-enqueue
   the texture copy / barriers) and preserves the exact batch the renderer
   built.

2. **Lifecycle: `delete` after `merge`.** This codebase uses `delete updates`
   to discard batches throughout (proven at cpp:379, cpp:396). `merge()` makes
   the pending batch inert; `delete` then returns it to QRhi's internal pool
   (the public destructor does this - see QRhiResourceUpdateBatch docs / the
   existing `delete updates` calls). Using `delete` keeps the lifecycle
   consistent with the rest of the file rather than mixing in `release()`.

3. **Deferral window is one frame.** The thumbnail capture flow already
   tolerates a multi-frame latency: `issueThumbnailReadback` zeroes
   `m_thumbnailReadbackResult`, the readback completes asynchronously, and
   `render()` polls `m_thumbnailReadbackResult.data` non-empty on a later
   frame (cpp:356). Adding one more frame of latency is invisible.

4. **Safety on rebuild / shutdown.** `releaseResources()` must release any
   stashed pending batch. A non-null `m_pendingReadbackUpdates` at teardown
   means the capture frame was the last frame; `delete` it.

### Changes

**`src/qml_gui/Renderer/RhiViewportRenderer.h`**
- Add member `QRhiResourceUpdateBatch *m_pendingReadbackUpdates = nullptr;`
  (raw pointer; render-thread owned, no ownership transfer to QRhi).

**`src/qml_gui/Renderer/RhiViewportRenderer.cpp`**
- `render()` head, before any batch construction: if
  `m_pendingReadbackUpdates != nullptr`, fold it. Because the existing code
  allocates `updates` lazily (only when scene is dirty), the fold path must
  allocate a fresh `updates` if none exists, then `merge`. After merge,
  `delete m_pendingReadbackUpdates; m_pendingReadbackUpdates = nullptr;`
- Replace the pass-external `cb->resourceUpdate(readbackUpdates)` with
  `m_pendingReadbackUpdates = readbackUpdates;` (stash, do not issue).
- `releaseResources()`: if non-null, `delete` and null it. Add the
  `rhiTrace("seamA-folded")` / `rhiTrace("seamA-deferred")` markers.

## Verification (static only)

- File builds in current Qt6.2/6.5+ (merge() present since Qt 6.0).
- D3D11 path: `beginPass(..., updates)` already handles merged batches
  identically; no behavioral change for D3D11 (just one extra frame of
  readback latency on capture, which the poll loop tolerates).
- Bracket balance: no new blocks added; only member add + statement swaps.

## Out of scope

- The offscreen thumbnail pass itself (its own beginPass/endPass) is correct
  QRhi and stays as-is.
- The `m_thumbnailReadbackInFlight` flag is still set immediately on capture
  (prevents duplicate captures during the deferral window); unchanged.
