# Phase 207 Reproduction Report: D3D12 0xC0000005 Crash

**Date:** 2026-07-24
**Status:** Root cause located (environment-specific); historical root cause still unconfirmed on original machine.

## Key Finding: The Crash Reproduces 100% In This Environment

Unlike v4.5 Phase 106 (which reported "does not reproduce"), the v5.7 Phase 207
reproduction **reproduced the `0xC0000005` crash deterministically** in this
environment: 5/5 attempts under `OWZX_RHI_RENDERER=d3d12` crashed; 0/5 under
`d3d11` survived.

## Root Cause (This Environment): Virtual Display + D3D12 Swapchain Incompatibility

**The crash is NOT seam A/B/C.** Structured milestone tracing (env
`OWZX_RHI_TRACE=1`) shows the crash happens:
- AFTER `engine->load(main.qml)` returns successfully,
- AFTER `app.exec()` event loop is entered,
- BEFORE `RhiViewportRenderer::initialize()` is ever called (its trace never
  appears).

This localizes the crash to **QRhi D3D12 scene-graph startup / first swapchain
creation** — not to any application render code.

**Environment evidence:**
```
GPU: AMD Radeon(TM) Graphics (real, driver 32.0.12033.1030)
Display adapters (active): Todesk Virtual Display Adapter, OrayIddDriver Device
```
This machine renders to **virtual displays driven by Indirect Display Driver
(IDD)** remote-desktop software (Todesk / Sunlogin/Oray). D3D12's DXGI flip-model
swapchain is known to fail or crash on IDD virtual displays, while D3D11's
legacy BitBlt swapchain tolerates them. This matches the observed symptom
exactly: D3D11 survives, D3D12 crashes at swapchain init, before any app render
frame.

## Two Distinct Root Causes (Important Clarification)

| Crash | Environment | Timing | Reached render()? | Likely cause |
|---|---|---|---|---|
| Historical (2026-06-27) | Original dev workstation, real display + real GPU | ~3s after launch | YES (gizmo upload logged) | Seam A/B/C (unconfirmed) |
| This environment (2026-07-24) | Virtual display (Todesk/Oray IDD) + AMD GPU | <1s after app.exec() | NO (initialize() never called) | D3D12 flip-model swapchain vs IDD |

**They are different crashes.** The historical crash advanced into render() (seam
A/B/C are still valid hypotheses for it). This environment's crash dies earlier
in the QRhi/scene-graph swapchain setup, before any application renderer code
runs. Mitigating seam A/B/C will NOT fix this environment's crash.

## What This Means For The Promotion

1. **This environment cannot validate seam A/B/C mitigations** — the crash
   happens before render() is reached, so the mitigated code never executes.
2. **The original-machine crash (seam A/B/C territory) remains unconfirmed** —
   still needs the original workstation with a real display.
3. **The D3D12 default promotion is still risky for IDD/virtual-display users**
   (remote-desktop scenarios) even if seam A/B/C are fixed — a fallback to D3D11
   is essential, not just for probe failure but for swapchain-init failure.

## Diagnostic Hardening Delivered (retained regardless)

- `RhiViewportRenderer` gains env-gated (`OWZX_RHI_TRACE=1`) structured milestone
  tracing at: render-enter, ensurePipelines-enter, uploadSceneBuffers-enter,
  uploadCameraUniform-enter, initialize-enter (+ call# + backend name),
  beginPass-done, setShaderResources-done, endPass-done, thumbnail-pass-begin/done,
  thumbnail-readback-issued, render-exit. Zero overhead when env unset.
- `main_qml.cpp` gains startup milestones: BackendContext constructed, context
  property set, engine->load calling/returned, app.exec entering.

These remain useful for the original-machine verification step (Phase 211
pending_machine_verification).

## Recommendation For Phase 211

The automatic fallback (D3D12 probe/init fails -> D3D11) is the critical safety
net for BOTH root causes:
- IDD/virtual-display users: D3D12 swapchain fails -> fall back to D3D11.
- Original-machine seam A/B/C: if a residual crash appears, the user can set
  `OWZX_RHI_RENDERER=d3d11` (or the app could detect repeated startup crashes
  and auto-fallback, but that is out of v5.7 scope).

Proceed with seam A/B/C mitigations (Phase 208/209/210) as preventive correct-
usage improvements, then promote with the robust fallback (Phase 211). The
historical root cause will only be confirmed on the original machine.
