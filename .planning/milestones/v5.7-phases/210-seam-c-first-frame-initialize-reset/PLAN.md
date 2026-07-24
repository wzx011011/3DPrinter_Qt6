# Phase 210 - Seam C Mitigation (MIT-03): First-frame force upload + initialize reset

## Context

Phase 207 traced the historical D3D12 0xC0000005 to three candidate seams.
This phase mitigates **Seam C**: the on-screen `beginPass` / first
`setShaderResources` may bind the SRB (shader resource bindings) against a
camera UBO that has not yet been uploaded in this swapchain's lifetime.

Two sub-conditions form the seam:

1. **`initialize()` does not reset buffer-uploaded flags.** `QQuickRhiItem`
   calls `initialize()` on every swapchain rebuild (window resize, visibility
   toggle, dock/undock). On D3D12, a swapchain rebuild can invalidate the
   GPU-side backing memory of buffers created against the *previous*
   swapchain's device-context state. The current `initialize()`
   (~cpp:40-58) only resets `m_pipelineFailed` and `m_sceneBuffersUploaded`,
   leaving `m_cameraUniformBufferUploaded`, `m_modelVertexBufferUploaded`,
   `m_highlightVertexBufferUploaded`, etc. as `true`. On the first render()
   after a rebuild, the renderer skips re-uploading and binds stale buffers.

2. **First-frame race.** Even when `m_sceneBuffersUploaded` is reset, the
   per-canvas draw path at ~:429/505 binds the SRB and draws *before* the
   camera UBO is guaranteed populated for the *very first* frame. Under
   D3D12's strict null-descriptor rules, binding a uniform buffer whose
   contents were never written in the current device state is a plausible
   0xC0000005 source.

## Problem location

`src/qml_gui/Renderer/RhiViewportRenderer.cpp`:
- `initialize()` ~:40-58 (resets only 2 flags).
- `render()` scene-dirty guard ~:376, and the SRB bind at ~:429/505.

## Mitigation

### Design decisions

1. **Reset ALL buffer-uploaded flags in `initialize()`.** After a swapchain
   rebuild every buffer must be re-uploaded on the next render(). This is
   cheap (one extra upload per buffer, once per resize) and matches QRhi's
   stated contract: buffers survive swapchain rebuild but their backing
   memory semantics are backend-dependent. Re-uploading is the safe,
   backend-agnostic choice. This mirrors what `releaseResources()` already
   does (~cpp:603-613) but without destroying the buffer handles.

2. **Force camera upload on the first N frames.** Add `m_frameCount` and
   unconditionally call `uploadCameraUniform(updates, DirtyCamera|DirtyGpu)`
   for frames 1..3 across all canvas types. N=3 covers: (a) the frame that
   builds the SRB, (b) the next frame where the SRB is first bound, (c) one
   margin frame for drivers that defer descriptor binding. After N frames the
   normal dirty-flag path resumes.

3. **Guard location: inside the existing CanvasPreview block AND inside the
   scene-dirty guard block.** Both canvas branches already allocate `updates`
   lazily; the forcer hooks into both:
   - **CanvasPreview** (~cpp:416-426): the existing `uploadCameraUniform`
     call stays, but is made unconditional for the first N frames (already
     unconditional in current code - just needs the `updates` allocation to
     survive even when the segment buffer is already uploaded; verify it
     does).
   - **View3D/AssembleView** (~cpp:376-386): currently camera upload happens
     inside `uploadSceneBuffers` (via `uploadCameraUniform`). For the first N
     frames, even when `!sceneDirty && m_sceneBuffersUploaded`, we must
     allocate an `updates` batch and call `uploadCameraUniform` directly.
     The cleanest hook is a new `else if (m_frameCount <= 3)` arm that does
     only the camera upload.

4. **Counter increment point.** Increment `m_frameCount` at the very START of
   `render()` (just after the entry guard, before any batch construction), so
   the *value seen during the current frame's upload logic* is 1-based on the
   first frame. `m_frameCount <= 3` then covers exactly frames 1..3 (N=3).
   Reset `m_frameCount = 0` in `initialize()` (so a swapchain rebuild
   re-triggers the force window).

### Changes

**`src/qml_gui/Renderer/RhiViewportRenderer.h`**
- Add member `int m_frameCount = 0;`.

**`src/qml_gui/Renderer/RhiViewportRenderer.cpp`**
- `initialize()`: reset all `m_*BufferUploaded` flags (grep the current list
  from `releaseResources()` at ~cpp:603-613 - same set). Add explanatory
  comment about swapchain-rebuild GPU memory invalidation under D3D12. Reset
  `m_frameCount = 0`. Add `rhiTrace("seamC-initialize-reset")`.
- `render()`, CanvasPreview branch (~cpp:416-426): wrap the existing
  `uploadCameraUniform` so the `updates` batch is always allocated when
  `m_frameCount <= 3` (currently it can be skipped if both segment and
  camera are already uploaded - verify and force).
- `render()`, View3D/AssembleView branch (~cpp:376-409): add an
  `else if (!m_pipelineFailed && m_frameCount <= 3)` arm that allocates
  `updates = rhi()->nextResourceUpdateBatch()` and calls
  `uploadCameraUniform(updates, DirtyCamera | DirtyGpu)`. Add
  `rhiTrace("seamC-force-frame")`.
- `render()` end (~cpp:572): `++m_frameCount;` before `rhiTrace("render-exit")`.

## Verification (static only)

- All canvas types now have a guaranteed camera UBO upload in frames 1..3,
  even with zero dirty flags, so the first SRB bind never sees an
  uninitialized UBO.
- After a swapchain rebuild (`initialize()`), every buffer is re-uploaded
  on the next render, eliminating stale-backing-memory risk on D3D12.
- D3D11 path: extra uploads are a no-op cost (one-time); no behavioral
  change. The `m_sceneBuffersUploaded` reset in `initialize()` already
  exists for this reason - we extend the same safety to the other flags.
- Bracket balance: one new `else if` arm, balanced; counter increment is a
  single statement.

## Out of scope

- The pipelines themselves are NOT recreated in `initialize()` (the existing
  comment at cpp:50-55 explains why); we only reset upload flags + counter.
- N=3 is a heuristic; if a future frame trace shows the SRB bind happening
  later, bump it. It is intentionally generous.
