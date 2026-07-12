---
phase: 106-d3d12-crash-root-cause-and-backend-readiness
plan: 01
task: 106-01-01
type: investigation
status: time-boxed (DR-04 acceptable outcome)
requirements: [D3D12-02, D3D12-03]
created: 2026-07-12
---

# D3D12 Crash Root-Cause Report

**Status:** TIME-BOXED (DR-04). The crash could NOT be reproduced in the test
environment. This report documents the repro attempts, the leading hypothesis
with evidence, what additional tooling would be needed for a confirmed root
cause, and explicit acceptance that D3D12 stays opt-in per D3D12-03.

This is the documented acceptable outcome for D3D12-02, NOT a fabricated root
cause. A confirmed isolation requires a live display + GPU debugger on the
machine that originally produced the `0xc0000005` dumps (see "Tooling Gap").

---

## 1. Repro Procedure (DR-01)

### 1.1 Canonical repro command

PowerShell, from the repo root:

```powershell
$env:OWZX_RHI_RENDERER = 'd3d12'
$env:OWZX_D3D12_DEBUG  = '1'   # Phase 105: enables D3D12 debug layer + QSG_RHI_DEBUG
Start-Process build/OWzxSlicer.exe -PassThru
```

### 1.2 Historical crash signature (the target)

From `.planning/debug/qrhi-d3d12-crash.md` (the prior debug session, status:
"resolved" by policy, NOT by root cause):

- **Exit code:** `-1073741819` = `0xC0000005` (STATUS_ACCESS_VIOLATION).
- **Original trigger:** `OWZX_RHI_RENDERER=1` (which normalized to `auto`, which
  selected `d3d12` first under the OLD candidate order).
- **Startup log (2026-06-27):** `QRhi backend selection: enabled=true requested=auto selected=d3d12 attempts=[d3d12:ok]`.
- **Crash (2026-06-27T22:36:22):** `Unhandled exception code=0xc0000005`.
- **Comparison runs (2026-06-27):** software + explicit `d3d11` stayed alive to
  the timeout; explicit `d3d12` exited `-1073741819` and wrote a new minidump.
- **Original env:** the developer's workstation with a real GPU + display
  (the `crash_dumps/FramelessDialogDemo_2026*_*.dmp` files dated June 27/28).

### 1.3 Repro attempt in THIS test environment (2026-07-12)

Two repro attempts were made in the test environment where this investigation
ran (no interactive display, headless build/run box):

**Attempt 1** (12s wait, debug layer + QT_LOGGING_RULES for qt.rhi):

```
$env:OWZX_RHI_RENDERER = 'd3d12'
$env:OWZX_D3D12_DEBUG  = '1'
$env:QT_LOGGING_RULES  = 'qt.qpa.*.debug=true;qt.rhi.*=true'
Start-Process ... -PassThru
```

Result: **`STILL RUNNING after 12s - stopping`.** No exit, no minidump.

**Attempt 2** (20s polled wait, QSG_INFO=1):

```
$env:OWZX_RHI_RENDERER = 'd3d12'
$env:OWZX_D3D12_DEBUG  = '1'
$env:QSG_INFO          = '1'
```

Result: **`SURVIVED 20s - D3D12 did NOT crash in headless test env`.** No exit,
no new minidump in `build/crash_dumps/` (the newest `.dmp` is dated
2026-07-04; the only July 12 artifact is `crash_stack.log`, which predates the
repro runs and is rewritten by the crash handler on each process).

### 1.4 Repro verdict

**The D3D12 access violation does NOT reproduce in the test environment.** The
app survives 20 seconds with the D3D12 backend + debug layer both active, which
is well past the sub-second-to-3-second window in which the historical crash
fired (2026-06-27 logs: launched 22:36:19, crashed 22:36:22 — ~3s). Per the
phase plan and DR-04, this is documented rather than fabricated.

### 1.5 Why the repro does not converge here

Three concrete reasons, in order of likelihood:

1. **No interactive display / GPU surface.** The test environment is headless.
   The crash historically fired inside the LIVE `QQuickRhiItem` render path
   (`RhiViewportRenderer::render` → `cb->beginPass` at `:298`), which requires
   the Qt Quick scene graph to drive a real swapchain. On Windows, a
   QQuickRhiItem without a visible, composited window may take a degraded
   render path (or the scene graph may not advance frames at all), so the
   exact frame that triggers the violation is never reached.
2. **Different GPU / driver than the original machine.** D3D12 behavior is
   driver-sensitive; the validation layer's strictness and the underlying
   resource-binding behavior differ across IHVs. The historical dumps came
   from a specific dev workstation; this box is not byte-identical.
3. **Debug-layer capture channel.** The D3D12 debug layer (and
   `QSG_RHI_DEBUG`) write via `OutputDebugString`, NOT to stdout/stderr. The
   captured `d3d12_*_err.txt` / `d3d12_*_out.txt` files are all 0 bytes. To
   read the validation messages you need an attached debugger
   (Visual Studio / WinDbg / DebugView) or a WER minidump with frames — none
   of which are usable in this headless environment.

---

## 2. Debug-Layer Output Captured (DR-02)

**None captured.** Per Section 1.5 #3, the D3D12 debug layer + `QSG_RHI_DEBUG`
emit to `OutputDebugString`. No debugger was attached to the repro process and
no new minidump was produced (the process did not crash), so there is no
GPU-validation text and no stack to cite.

This is the explicit "could not capture + reason" case the acceptance criteria
allow. The triage therefore falls back to static code analysis + the historical
evidence below.

---

## 3. Static Triage of the Render Path (DR-02)

Because the live repro did not crash, the failure point is isolated by reading
the code rather than by a debugger. The renderer's per-frame
`render(QRhiCommandBuffer *cb)` at `src/qml_gui/Renderer/RhiViewportRenderer.cpp:225`
issues, in order:

1. **Resource-update batch built** (lines 242-296): scene buffers, gizmo
   buffer, cut-plane/wipe-tower buffers, and (for `CanvasPreview`) the preview
   segment buffer + camera uniform. All `uploadStaticBuffer` /
   `updateDynamicBuffer` calls (lines 912, 918, 947, 976, 1049, 1054, 1090,
   1176, 1351-1357, 1434, 1449, 1451, 1700, 1993) go onto this pre-pass batch.
2. **`cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates)` at `:298`.**
   The `updates` batch is passed AS the 4th argument — the QRhi-correct way to
   fold resource updates into a pass.
3. Draw commands (`setViewport`, `setShaderResources`, `setGraphicsPipeline`,
   `setVertexInput`, `draw`) at `:303-388`.
4. **`cb->endPass()` at `:389`.**
5. (Thumbnail path, only when a capture is pending) `renderThumbnailPass(cb)`
   at `:402` → its own `beginPass`/`endPass` pair at `:657`/`:684` on the
   offscreen RT, then **`cb->resourceUpdate(readbackUpdates)` at `:414`** to
   queue the readback.

### 3.1 The BUG-V31-1 comment and whether its fix is in place

The comment at `:281-285` reads (verbatim):

> BUG-V31-1 fix: camera uniform MUST be uploaded before beginPass, not after.
> beginPass-after-resourceUpdate is undefined in QRhi; D3D12 strictly enforces
> command buffer ordering and segfaults on this pattern (root cause of the
> D3D12 crash that was worked around with D3D11-first in RhiBackendSelector).

**The fix the comment describes IS present in the current code.** At `:298`
the `updates` batch (which carries `uploadCameraUniform` at `:295` and the
preview-segment upload at `:291-292`) is passed INTO `beginPass` as its 4th
argument. There is no `cb->beginPass(...)` followed by a bare
`cb->resourceUpdate(...)` on the on-screen pass. So whatever crash the V31-1
comment was originally written against, the cited ordering violation is no
longer in the code.

### 3.2 BUG-V31-1 verdict

**SUPERSEDED for the purpose of confirming the CURRENT crash, INCONCLUSIVE as
a confirmed root cause.** Precisely:

- The *symptom* the comment describes (access violation in `beginPass` under
  D3D12) matches the historical crash signature (`0xc0000005`).
- But the *cause* the comment names (camera uniform uploaded AFTER beginPass)
  is already corrected at `:281-298`. PITFALLS.md pitfall 5 warned that V31-1
  is a hypothesis, not a confirmed root cause, and that "a NEW crash needs NEW
  isolation." The static triage confirms the cited fix is in place, so V31-1
  cannot be the active cause of any crash observed today.
- Because the live repro does not crash in this environment, there is no NEW
  isolation to confirm. V31-1 is therefore neither re-confirmed nor refuted as
  the historical cause; it is **superseded as a live explanation** — the
  pattern it blamed no longer exists in the source.

---

## 4. Hypothesized Root Cause (leading explanation) (DR-02)

Since V31-1's cited pattern is fixed and the repro does not converge, the
leading hypothesis for the ORIGINAL (historical) crash is a **combination of
D3D12 command-list ordering sensitivity and a non-reproducing
machine/driver-specific trigger**, with three candidate code-level seams that
D3D12 is stricter about than D3D11. None is confirmed; each is testable.

### 4.1 Candidate seam A — `cb->resourceUpdate()` outside a pass (`:414`)

`renderThumbnailPass` ends at `:684`; immediately after, `:414` calls
`cb->resourceUpdate(readbackUpdates)` to queue the thumbnail readback. This is
LEGAL QRhi usage (resource updates may be queued outside a pass between
beginFrame/endFrame), BUT it is the only place in the renderer that calls
`resourceUpdate()` directly rather than folding the batch into the next
`beginPass`. A thumbnail capture during the FIRST frame after startup (the
3MF thumbnail-request flow, Phase 95) would exercise this seam on the same
command buffer that is about to drive the live on-screen swapchain. D3D11
tolerates the interleaving; D3D12's command-list recording is stricter.

**Evidence for:** the only non-`beginPass`-folded resource update in the whole
renderer; it sits in the capture path that fires shortly after startup
(thumbnail generation); D3D12 is documented to be stricter than D3D11 on
command-list ordering (PITFALLS.md integration-gotchas row).
**Evidence against:** legal per QRhi API; thumbnail capture is gated and does
not fire on every launch; the historical crash fired at startup before any 3MF
thumbnail would be requested.
**Testable:** gate the readback onto the NEXT on-screen `beginPass` batch
instead of calling `cb->resourceUpdate()` directly, rebuild, repro on the
original machine.

### 4.2 Candidate seam B — dynamic UBO sub-range writes (`:1434`, `:1449`, `:1451`)

`uploadCameraUniform` allocates a 256-byte buffer (`:1430`, correct for the
D3D12 256-byte cbuffer alignment — PITFALLS.md performance-traps row) but then
issues THREE separate `updateDynamicBuffer` calls into it (64-byte MVP at
offset 0; 12-byte `gizmoCenter` at offset 64; 4-byte `gizmoScale` at offset
76). D3D11 merges overlapping/coherent sub-writes transparently; on D3D12 the
three writes become separate copy operations into the same dynamic buffer,
and a `setShaderResources` issued between them (if any re-ordering occurred)
could expose an uninitialized tail. The 256-byte allocation is correct, so
this seam is about WRITE GRANULARITY, not alignment.

**Evidence for:** three sub-range writes where one packed write would do; the
gizmo-tail fields are only consumed by the gizmo shader, so a partial-write
read would manifest only under D3D12.
**Evidence against:** all three writes go onto the SAME pre-`beginPass` batch
before `setShaderResources`, so QRhi should batch them coherently; the 256-byte
buffer is larger than the 80 bytes written, so an over-read lands in padding,
not past the allocation.
**Testable:** coalesce the three `updateDynamicBuffer` calls into a single
write of an 80-byte packed struct, rebuild, repro.

### 4.3 Candidate seam C — first-frame pipeline/buffer readiness race

`render()` guards draws on `ensurePipelines()` (`:303`) and per-buffer
`...BufferUploaded` flags, but `ensurePipelines()` returns true based on
pipeline-object creation, not on whether the SRB's referenced buffers are all
in the uploaded state on the very first frame. The D3D11 backend is lenient
about binding a partially-populated SRB; D3D12's root-signature binding is
not. A startup race where the first `setShaderResources(m_srb.get())` runs
before the camera UBO is uploaded (e.g. on a `CanvasView3D` where the
`m_canvasType != CanvasPreview` branch at `:250` builds the scene batch but the
camera upload only happens in the `CanvasPreview` branch at `:286-296`) would
bind an empty/uninitialized UBO.

**Evidence for:** the camera-uniform upload at `:295` is INSIDE the
`if (m_canvasType == RhiViewport::CanvasPreview)` block; for
`CanvasView3D`/`CanvasAssembleView` the camera uniform is uploaded only via
`uploadSceneBuffers` (`:259`) → `uploadCameraUniform` (`:864`), and only when
`sceneDirty || !m_sceneBuffersUploaded`. On the very first frame after backend
hand-off, the ordering between "scene batch built" and "SRB bound" is
backend-sensitive.
**Evidence against:** `m_sceneBuffersUploaded` starts false so the first frame
should build + upload the scene (including camera) before any draw; the
crash historically fired ~3s after launch, not on frame 0.
**Testable:** force an explicit `uploadCameraUniform` on every frame until the
first successful `beginPass` completes under D3D12, rebuild, repro.

### 4.4 Most-likely synthesis

Without a debugger the three candidates cannot be ranked by evidence, only by
plausibility. Candidate **C (first-frame readiness race)** best fits the
"~3s after launch" timing and the fact that D3D11 is unaffected; candidate
**A (resourceUpdate outside pass)** is the most unusual API usage and the
first thing to test; candidate **B (sub-range UBO writes)** is the lowest
probability but the cheapest to refactor. A confirmed root cause needs the
tooling in Section 6.

---

## 5. Recommended Next Steps (DR-02)

Ordered by expected value:

1. **Repro on the original machine WITH a debugger.** Attach Visual Studio or
   WinDbg to `OWzxSlicer.exe` with `OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1`,
   let it crash, and read the D3D12 debug-layer output from the Output window +
   the exact faulting instruction from the minidump. This is the single
   highest-value next step and the only one that can convert Section 4 from
   hypothesis to confirmed root cause. The historical June-27/28
   `crash_dumps/*.dmp` files should be opened in WinDbg with the symbols +
   `RhiViewportRenderer.cpp` source to extract the faulting frame.
2. **Candidate-A mitigation probe.** Refactor `:408-414` to fold the readback
   batch into the next on-screen `beginPass` (stash it on a member and pass it
   as the 4th arg on the next frame) instead of calling
   `cb->resourceUpdate()` directly. Rebuild, repro on the original machine.
   If the crash disappears, candidate A is confirmed.
3. **Candidate-B mitigation probe.** Coalesce the three `updateDynamicBuffer`
   calls in `uploadCameraUniform` (`:1434/1449/1451`) into a single packed
   80-byte write. Low-cost refactor; repro.
4. **Candidate-C mitigation probe.** Force `uploadCameraUniform` on the first
   N frames regardless of dirty flags, for `CanvasView3D`/`CanvasAssembleView`.
   Repro.
5. **Do NOT promote D3D12 to default** until step 1 returns a confirmed root
   cause AND the corresponding mitigation probe repros clean. This is the
   D3D12-03 hard rule (DR-05) and is locked by the source-audit slot added in
   task 106-01-03.

---

## 6. Tooling Gap (what this investigation needed but did not have) (DR-04)

Per DR-04, the explicit documentation of what additional tooling/time would be
needed:

| Needed | Why | Available here? |
|---|---|---|
| Interactive display + composited window | The crash fires in the live `QQuickRhiItem` render path, which needs a real swapchain driven by the scene graph. Headless launch does not reach the crashing frame. | **No** — headless test box. |
| GPU debugger (Visual Studio Graphics / PIX / RenderDoc) OR a native debugger attached | The D3D12 debug layer + `QSG_RHI_DEBUG` write to `OutputDebugString`, not stdout/stderr. Need an attached debugger to capture the validation text + the faulting frame. | **No** — no interactive debugger. |
| The original machine's GPU/driver | Historical dumps (June 27/28) came from a specific workstation. D3D12 is driver-sensitive; this box's GPU is different. | **No** — different hardware. |
| WinDbg + symbols on the existing `crash_dumps/*.dmp` | The June 27/28 minidumps likely contain the faulting frame; opening them with `RhiViewportRenderer.pdb` would extract the failing instruction. | **Not run** — minidumps present but no WinDbg session in this environment. |

**Time spent:** one focused session (repro attempts + static triage). The
investigation was time-boxed per the phase plan; rather than rabbit-hole on
blind refactors of candidates A/B/C without a repro signal, it ships this
documented hypothesis + the tooling list and accepts the D3D12-03 outcome.

---

## 7. Acceptance (DR-04 explicit)

- D3D12 stays **opt-in** via `OWZX_RHI_RENDERER=d3d12`. The
  `defaultWindowsCandidates()` order at
  `src/qml_gui/Renderer/RhiBackendSelector.cpp:56-65` keeps **D3D11 first**.
  This is locked by the `d3d12StaysOptInBehindEnvFlag` source-audit slot
  (task 106-01-03) and is the D3D12-03 hard rule.
- Vulkan is **SDK-blocked** (Qt disables the `vulkan` public feature per
  `PROJECT.md:143`) and is evaluation-only, not a v4.5 deliverable.
- D3D12 default-backend promotion stays **OUT OF SCOPE** until a confirmed
  root cause (step 1 above) AND a clean repro-on-the-original-machine after
  the mitigation probe.

This is the documented acceptable outcome for D3D12-02 per DR-04: inconclusive
repro with a leading hypothesis (Section 4), explicit next steps (Section 5),
and explicit tooling gap (Section 6). NOT a fabricated root cause.

---

*Phase 106-01-01 investigation report. D3D12-02 closed as time-boxed; D3D12-03 documented in STATE.md (task 106-01-02).*
