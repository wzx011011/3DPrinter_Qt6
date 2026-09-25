# D3D12 Root-Cause Confirmation Report (v5.6, Phase 203)

**Phase:** 203 (D3D12 Root-Cause Confirmation)
**Milestone:** v5.6
**Type:** Documentation-only re-confirmation. No code change. No default
promotion.
**Predecessor:** `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` (v4.5,
phase 106-01-01, status: time-boxed per DR-04).
**Status:** RE-CONFIRMED INCONCLUSIVE. The root cause of the historical
`0xC0000005` access violation is **still not confirmed**. D3D12 stays
opt-in. This is the correct and honest deliverable for this phase.

---

## 0. TL;DR

- The D3D12 startup crash (`0xC0000005` / STATUS_ACCESS_VIOLATION) that fired
  ~3s after launch on the developer's original workstation on 2026-06-27
  **still cannot be reproduced** in the headless test environment.
- The BUG-V31-1 fix (camera uniform uploaded before `beginPass`, folded into
  `beginPass`'s resource-update batch) **is still present** in v5.6.
- The three candidate seams A/B/C from v4.5 **are still present** at drifted
  v5.6 line offsets (A: `:526`; B: `:1585/1600/1602`; C: first-frame readiness,
  `render()` draw guard at `:407`).
- D3D11-first ordering in `defaultWindowsCandidates()`
  (`RhiBackendSelector.cpp:56-65`) **is intact** and the
  `d3d12StaysOptInBehindEnvFlag` regression slot still locks it.
- **Per the v5.6 user decision, D3D12 is NOT promoted to default.** A confirmed
  root cause requires the original machine + a native debugger (VS/WinDbg/PIX)
  + symbols + the historical minidumps + an A/B/C mitigation-probe repro. None
  of that is available in this phase, so the conclusion is "not confirmed;
  D3D11-first retained."

This report does NOT fabricate a root cause. "Inconclusive, D3D12 stays
opt-in" is the documented acceptable outcome, exactly as it was in v4.5.

---

## 1. Reference to the v4.5 original conclusions

The full v4.5 investigation lives at
`.planning/research/D3D12-CRASH-ROOT-CAUSE.md` (phase 106-01-01). Its
load-bearing conclusions, which this v5.6 report re-confirms rather than
replaces:

1. **Repro verdict (v4.5 Section 1.4):** the D3D12 access violation does NOT
   reproduce in the test environment. Two attempts (12s and 20s waits, debug
   layer + QSG_RHI_DEBUG both on) survived past the historical ~3s crash
   window. No new minidump was produced.
2. **BUG-V31-1 verdict (v4.5 Section 3.2):** SUPERSEDED as a live
   explanation. The symptom (access violation in `beginPass` under D3D12)
   matches, but the cause the V31-1 comment names (camera uniform uploaded
   AFTER `beginPass`) was already corrected in v4.5. V31-1 is neither
   re-confirmed nor refuted as the historical cause; it is superseded because
   the pattern it blamed no longer exists in source.
3. **Leading hypothesis (v4.5 Section 4):** a combination of D3D12 command-list
   ordering sensitivity and a non-reproducing machine/driver-specific trigger,
   with three candidate seams:
   - **Candidate A:** `cb->resourceUpdate()` outside a pass (thumbnail
     readback) -- the only non-`beginPass`-folded resource update in the
     renderer.
   - **Candidate B:** three sub-range `updateDynamicBuffer` writes into the
     256-byte camera UBO where one packed write would do.
   - **Candidate C:** first-frame SRB/buffer readiness race.
4. **Tooling gap (v4.5 Section 6):** a confirmed isolation needs (a) an
   interactive display + composited window, (b) a GPU/native debugger, (c) the
   original machine's GPU/driver, (d) WinDbg + symbols on the existing
   `crash_dumps/*.dmp`. None were available in v4.5.
5. **Acceptance (v4.5 Section 7):** D3D12 stays opt-in via
   `OWZX_RHI_RENDERER=d3d12`; `defaultWindowsCandidates()` keeps D3D11 first;
   default promotion is out of scope until a confirmed root cause + clean
   repro on the original machine.

Phase 203 accepts the v4.5 report as the historical record and adds only a
v5.6 re-confirmation layer below. Nothing in v5.6 contradicts the v4.5
conclusions.

---

## 2. v5.6 code-state re-confirmation

The line numbers cited in v4.5 have drifted because Phase 95 (THUMBCAP) and
later view-renderer additions inserted code above the render path. The
**patterns** are unchanged; only the offsets moved. This section pins the v5.6
offsets so a future debugger session can find the seams without re-searching.

### 2.1 BUG-V31-1 fix: still present

File: `src/qml_gui/Renderer/RhiViewportRenderer.cpp`.

The BUG-V31-1 comment is at `:386-389`:

```
// BUG-V31-1 fix: camera uniform MUST be uploaded before beginPass, not after.
// beginPass-after-resourceUpdate is undefined in QRhi; D3D12 strictly enforces
// command buffer ordering and segfaults on this pattern (root cause of the
// D3D12 crash that was worked around with D3D11-first in RhiBackendSelector).
```

The fix the comment describes is in force at the v5.6 offsets:

- The pre-`beginPass` resource-update batch is built and, for
  `CanvasPreview`, the camera uniform is merged into it via
  `uploadCameraUniform(updates, ...)` at `:399` (inside the
  `if (m_canvasType == RhiViewport::CanvasPreview)` block at `:390`).
- `cb->beginPass(renderTarget(), m_clearColor, {1.0f, 0}, updates)` at `:402`
  receives `updates` as its 4th argument. There is NO bare
  `cb->beginPass(...)` followed by `cb->resourceUpdate(...)` on the on-screen
  pass.
- A trailing comment at `:474` ("Camera uniform was already uploaded before
  beginPass (BUG-V31-1 fix).") reinforces the invariant inside the draw block.

**Verdict (unchanged from v4.5 Section 3.2):** the V31-1 cited ordering
violation is NOT present in the v5.6 source. Whatever crash V31-1 was written
against, the pattern it blamed no longer exists. V31-1 remains
SUPERSEDED/inconclusive as a confirmed root cause; it cannot be the active
cause of any crash observed today.

### 2.2 Candidate seam A -- `cb->resourceUpdate()` outside a pass: still present

v4.6 location (drifted from v4.5's `:414`): `RhiViewportRenderer.cpp:526`.

After the on-screen pass's `endPass()` at `:501`, the thumbnail path
(`:508-534`, gated on `m_thumbnailRequestPending && !m_thumbnailReadbackInFlight
&& m_canvasType != CanvasPreview && ...`) calls `renderThumbnailPass(cb)` at
`:514` (its own `beginPass`/`endPass` pair at `:777`/`:804` on the offscreen
RT), then builds a fresh readback batch and issues:

```
QRhiResourceUpdateBatch *readbackUpdates = rhi()->nextResourceUpdateBatch();
issueThumbnailReadback(readbackUpdates);
...
cb->resourceUpdate(readbackUpdates);   // <-- seam A, :526
```

This is still the ONLY place in the renderer that calls `cb->resourceUpdate()`
directly rather than folding the batch into a `beginPass`'s 4th argument. It is
legal QRhi usage (resource updates may be queued outside a pass between
beginFrame/endFrame), but it is the unusual API-usage seam D3D12 is stricter
about than D3D11. The seam is now explicitly inside the View3D/AssembleView
thumbnail path (Preview thumbnails are out of scope per the `:509` guard).

### 2.3 Candidate seam B -- three sub-range UBO writes: still present

v5.6 location (drifted from v4.5's `:1434/1449/1451`):
`RhiViewportRenderer.cpp`, `uploadCameraUniform()` at `:1559-1607`.

The 256-byte buffer is still allocated (`:1581`, correct for the D3D12
256-byte cbuffer alignment) and the THREE sub-range writes are still issued
onto the same batch:

- `:1585` -- `updateDynamicBuffer(..., 0, 64, corrected.constData())` (MVP)
- `:1600` -- `updateDynamicBuffer(..., 64, 12, &m_gizmoCenter[0])` (gizmoCenter)
- `:1602` -- `updateDynamicBuffer(..., 76, 4, &gizmoScale)` (gizmoScale)

Total 80 bytes written into a 256-byte allocation. The seam is about WRITE
GRANULARITY (three writes where one packed 80-byte write would do), not
alignment (the 256-byte allocation is correct). D3D11 merges coherent
sub-writes transparently; D3D12's stricter command-list recording is the
concern.

### 2.4 Candidate seam C -- first-frame readiness race: still present

v5.6 location: `render()` draw guard at `:407`
(`if (m_canvasType != RhiViewport::CanvasPreview && ensurePipelines())`).

`ensurePipelines()` returns true based on pipeline-object creation, not on
whether every buffer the SRB references is in the uploaded state on the very
first frame. The on-screen draw at `:410` calls `cb->setShaderResources(m_srb.get())`
which binds the SRB regardless of whether the camera UBO has been uploaded yet
this frame. For `CanvasView3D`/`CanvasAssembleView`, the camera upload happens
via `uploadSceneBuffers -> uploadCameraUniform` only when
`sceneDirty || !m_sceneBuffersUploaded`; the first-frame ordering between
"scene batch built" and "SRB bound" is backend-sensitive. D3D11 tolerates a
partially-populated SRB; D3D12's root-signature binding is not lenient.

This seam still fits the historical "~3s after launch" timing (startup scene
build completes, first D3D12 draw binds the SRB).

### 2.5 Default backend policy: D3D11-first, intact

File: `src/qml_gui/Renderer/RhiBackendSelector.cpp:56-65`.

`defaultWindowsCandidates()` returns D3D11 first, D3D12 second. The load-bearing
rationale comment (`:58-60`, "D3D11-first: D3D12 has a rendering crash on
startup...") is present. `probeBackend()` (`:82-117`) still gates the D3D12
debug layer on `OWZX_D3D12_DEBUG` (Phase 105, `:97-100`). D3D12 is reachable
only via the explicit `OWZX_RHI_RENDERER=d3d12` opt-in
(`candidatesForRequest()` exact-match at `:67-80`).

`main_qml.cpp:271-296` still defaults `OWZX_RHI_RENDERER=auto` when unset
(`:271-272`), forwards `OWZX_D3D12_DEBUG` to `QSG_RHI_DEBUG` before
`QGuiApplication` (`:285-286`), and applies the selector's chosen graphics API
(`:291-292`), falling back to the software backend otherwise (`:293-297`).

**No phase between 106 and 203 (v4.5 -> v5.6) altered the D3D11-first order.**
Confirmed by direct re-read of `:56-65` (D3D11 at `:62`, D3D12 at `:63`) and
by the regression-slot verification in Section 5.

---

## 3. Honest conclusion: root cause STILL NOT confirmed

This is the load-bearing section. It states the outcome plainly.

**The root cause of the historical `0xC0000005` D3D12 access violation is
STILL NOT confirmed as of v5.6 / Phase 203.**

Reasons, unchanged from v4.5:

1. **No reproduction.** The crash does NOT reproduce in the headless test
   environment. The v4.5 attempts (12s/20s, both with the D3D12 debug layer
   and QSG_RHI_DEBUG active) survived well past the historical ~3s crash
   window. Phase 203 does not run a new repro attempt because the environment
   that could not reach the crashing frame in v4.5 is the same environment
   now; the v4.5 repro verdict stands. Re-running an identical headless launch
   would not change the outcome and would only consume time.
2. **No debugger capture.** No native debugger (Visual Studio / WinDbg / PIX)
   was attached to a crashing process in v4.5, and none is available in this
   phase. The D3D12 debug layer writes via `OutputDebugString`, not
   stdout/stderr, so without an attached debugger there is no validation text
   and no faulting frame.
3. **No minidump analysis.** The historical `crash_dumps/*.dmp` files (dated
   2026-06-27/28) were not opened in WinDbg with symbols in v4.5, and are not
   opened in this phase either (no WinDbg session in this environment). They
   likely contain the faulting frame but it has not been extracted.
4. **Different hardware.** The original dumps came from a specific developer
   workstation with a specific GPU/driver. D3D12 is driver-sensitive. The
   test box is not byte-identical.

**Decision (per the v5.6 user decision):** D3D12 is NOT promoted to default.
D3D11 stays first in `defaultWindowsCandidates()`. The
`d3d12StaysOptInBehindEnvFlag` regression slot remains in force. This is the
D3D12-03 / DR-05 hard rule carried forward from v4.5.

Phase 203 explicitly does NOT invent a confirmed root cause. The three
candidate seams (A/B/C) remain HYPOTHESES, ranked only by plausibility:

- **C** (first-frame readiness race) best fits the "~3s after launch" timing
  and the fact that D3D11 is unaffected.
- **A** (`resourceUpdate` outside pass) is the most unusual API usage and the
  first thing to probe.
- **B** (sub-range UBO writes) is the lowest-probability but cheapest-to-refactor
  seam.

None of A/B/C is confirmed. Confirming any one of them requires the tooling in
Section 4.

---

## 4. Prerequisites for a confirmed root-cause isolation

To convert Section 3 from "inconclusive" to "confirmed," the following would be
required. This list is the same as v4.5 Section 6, restated so a future session
has a concrete checklist.

| Needed | Why | Available in v5.6? |
|---|---|---|
| Interactive display + composited `QQuickRhiItem` window | The crash fires in the live render path (`render()` -> `cb->beginPass` at `:402`) which needs a real swapchain driven by the Qt Quick scene graph. A headless launch does not advance frames the same way and does not reach the crashing frame. | **No** -- headless test box. |
| Native debugger (Visual Studio, WinDbg, or PIX) attached to the crashing process | The D3D12 debug layer + `QSG_RHI_DEBUG` emit via `OutputDebugString`. An attached debugger captures the GPU-validation text AND the exact faulting instruction/register state at the `0xC0000005`. | **No** -- no interactive debugger in this environment. |
| The original machine's GPU + driver | D3D12 behavior is IHV/driver-sensitive; the historical dumps came from a specific workstation. A different box may simply not reproduce even with identical code. | **No** -- different hardware. |
| WinDbg + `RhiViewportRenderer.pdb` symbols opened on the historical `crash_dumps/*.dmp` (2026-06-27/28) | The existing minidumps most likely contain the faulting frame. Loading them with symbols + the `RhiViewportRenderer.cpp` source would extract the failing instruction and pin which seam (A/B/C/none) was live. This is the single highest-value, lowest-cost step and can be done WITHOUT a new repro. | **Not run** -- minidumps present but no WinDbg session in this phase. |
| GPU debugger (PIX / RenderDoc / VS Graphics) for a live capture | If a live repro is achieved on the original machine, a GPU frame capture shows the exact D3D12 command-list state at the violation, disambiguating A vs B vs C. | **No** -- no GPU debugger available. |
| An A/B/C mitigation-probe build + repro on the original machine | Even with a debugger, confirming a seam requires demonstrating that the crash DISAPPEARS when that seam is mitigated (Section 5). | **No** -- no original-machine repro available. |

**Lowest-cost first step (recommended before any new repro attempt):** open
the existing June-27/28 `crash_dumps/*.dmp` in WinDbg with
`RhiViewportRenderer.pdb` symbols and extract the faulting frame. This needs
no live repro and no new build, and it alone might confirm or refute seam A/B/C.

---

## 5. Candidate-seam mitigation probes (run only on the original machine)

These are the procedures to execute IF and WHEN the original machine + a
native debugger become available. They are NOT v5.6 deliverables. They are
documented here so a future session does not have to re-derive them. Each
probe is a small, surgical code change followed by a repro attempt on the
original machine.

**Order (expected value):** A first (most unusual API usage), then C (best
timing fit), then B (cheapest refactor). Run them one at a time so a clean
repro unambiguously attributes the fix.

### 5.1 Candidate-A mitigation probe

**Hypothesis:** `cb->resourceUpdate(readbackUpdates)` at
`RhiViewportRenderer.cpp:526` (the thumbnail readback queued outside a pass,
right after the offscreen `endPass` at `:804` returns) is legal QRhi but is
the one place D3D12's stricter command-list recording diverges from D3D11.

**Probe:**
1. Stash the readback batch on a member (e.g. `m_pendingReadbackUpdates`)
   instead of issuing it directly at `:526`.
2. On the NEXT on-screen `beginPass` (`:402`), merge the stashed batch into
   that `beginPass`'s 4th-argument `updates` batch, then clear the member.
3. Rebuild with `OWZX_D3D12_DEBUG=1`, launch with
   `OWZX_RHI_RENDERER=d3d12`, and trigger a thumbnail capture (load a 3MF /
   View3D or AssembleView canvas).
4. Attach Visual Studio / WinDbg / PIX before launch; let it crash or survive
   past 5s.

**Confirmed-if:** the crash disappears AND the D3D12 debug layer reports no
command-list-ordering violation. **Refuted-if:** the crash still fires at the
same instruction with the same validation output.

### 5.2 Candidate-B mitigation probe

**Hypothesis:** the three `updateDynamicBuffer` calls in `uploadCameraUniform()`
(`:1585`, `:1600`, `:1602`) write three sub-ranges into the 256-byte camera
UBO where a single packed 80-byte write would do; D3D12's handling of multiple
sub-writes into one dynamic buffer may expose uninitialized tail bytes to the
gizmo shader's `CameraBlock`.

**Probe:**
1. Define a packed `struct CameraBlockPacked { QMatrix4x4 mvp; QVector3D
   gizmoCenter; float gizmoScale; }` (16 bytes of padding handled by the
   std140 layout already documented at `:1592-1596`).
2. Replace the three `updateDynamicBuffer` calls with a single
   `updateDynamicBuffer(m_cameraUniformBuffer.get(), 0, 80, &packed)`.
3. Rebuild + repro on the original machine with the debugger attached.

**Confirmed-if:** the crash disappears. **Refuted-if:** the crash still fires.
Note: this is the cheapest refactor but the lowest-probability seam; it is
worth running last to avoid masking A/C.

### 5.3 Candidate-C mitigation probe

**Hypothesis:** on the first frame after backend hand-off, the on-screen draw
guard at `:407` (`m_canvasType != CanvasPreview && ensurePipelines()`) binds
the SRB (`cb->setShaderResources(m_srb.get())` at `:410`) before the camera
UBO is guaranteed uploaded for `CanvasView3D`/`CanvasAssembleView`. D3D11 is
lenient about a partially-populated SRB; D3D12 root-signature binding is not.

**Probe:**
1. For the first N frames (e.g. N=3) after `ensurePipelines()` first succeeds,
   force `uploadCameraUniform(updates, PrepareSceneData::DirtyCamera |
   PrepareSceneData::DirtyGpu)` unconditionally before `beginPass`, regardless
   of dirty flags, for ALL canvas types (not just `CanvasPreview`).
2. Rebuild + repro on the original machine with the debugger attached.

**Confirmed-if:** the crash disappears on the forced-upload build. **Refuted-if:**
the crash still fires at the same frame.

### 5.4 What "confirmed" would unlock

If exactly one of A/B/C reproduces clean (crash disappears) on the original
machine after its probe, AND the D3D12 debug layer reports no remaining
violation, that seam is the confirmed root cause. ONLY THEN does the
D3D12-03/DR-05 hard rule allow revisiting D3D12 default promotion -- and even
then, promotion is a SEPARATE decision with its own phase, not an automatic
consequence. Phase 203 takes no position on a future promotion; it only
documents the prerequisites.

---

## 6. Regression-slot verification (d3d12StaysOptInBehindEnvFlag)

The regression slot at `tests/QmlUiAuditTests.cpp:4425-4488` was re-read at
the end of this phase. It still asserts, with D3D12-03-named `QVERIFY2`
messages:

1. **D3D11-first order** (`:4454-4465`): inside the
   `defaultWindowsCandidates()` body slice, `Direct3D11` appears before
   `Direct3D12`, and `QRhi::D3D11` appears before `QRhi::D3D12`.
2. **Rationale comment present** (`:4472-4473`): the slice contains the
   literal `D3D11-first`, so a refactor cannot silently flip the order without
   understanding why it exists.
3. **Opt-in gate** (`:4479-4480`): the selector source still contains
   `OWZX_RHI_RENDERER`, locking D3D12 behind the explicit env opt-in.
4. **No Vulkan** (`:4486-4487`): the default candidate slice does NOT contain
   `QRhi::Vulkan`, locking the SDK-blocked-Vulkan part of D3D12-03.

**Direct re-read of `RhiBackendSelector.cpp:56-65` confirms the slot's
expectations are met in v5.6 source:** D3D11 at `:62`, D3D12 at `:63`, the
`D3D11-first` rationale comment at `:58-60`, `OWZX_RHI_RENDERER` read at
`:147`/`:150`, no Vulkan anywhere in the default candidate list.

**Phase 203 does not edit this slot.** It is verified intact and un-weakened.
No code change in this phase can have broken it because no code change
occurred.

---

## 7. Acceptance summary

- ROOT-CAUSE-REPORT.md (this file) and PLAN.md produced, ASCII-only English.
- `RhiBackendSelector.cpp`, `RhiViewportRenderer.cpp`, `main_qml.cpp`, and
  `tests/QmlUiAuditTests.cpp` are **byte-identical** before and after this
  phase (no edit occurred).
- BUG-V31-1 fix re-confirmed present at `:386-402`; superseded/inconclusive as
  a confirmed root cause, as in v4.5.
- Candidate seams A (`:526`), B (`:1585/1600/1602`), C (`:407/410`) re-confirmed
  present at drifted v5.6 offsets; all remain UNCONFIRMED hypotheses.
- `d3d12StaysOptInBehindEnvFlag` (`tests/QmlUiAuditTests.cpp:4425`) verified
  intact; D3D11-first order, rationale comment, opt-in gate, and no-Vulkan
  constraint all still asserted.
- **Root cause: NOT CONFIRMED.** D3D12 stays opt-in. D3D11 stays default.
  This is the documented acceptable outcome for Phase 203 per the v5.6 user
  decision ("root-cause confirmation only, no default promotion").

---

*Phase 203 (v5.6) D3D12 Root-Cause Confirmation report. Inherits and
re-confirms `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` (v4.5 / phase
106-01-01). No code changed. No default promoted.*
