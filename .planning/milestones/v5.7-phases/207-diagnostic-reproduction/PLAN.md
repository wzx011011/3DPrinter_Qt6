# Phase 207 Plan: Diagnostic Hardening + In-Environment Reproduction

**Requirement:** REPRO-01
**Goal:** Add structured render/startup diagnostics and reproduce the D3D12
crash in this environment to locate the root cause as far as possible.

## Files

- Modify: `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (OWZX_RHI_TRACE milestones)
- Modify: `src/qml_gui/main_qml.cpp` (startup milestones)
- Create: `REPRODUCTION-REPORT.md` (this directory)

## Steps

- [x] Added env-gated `rhiTrace()` helper + milestones in RhiViewportRenderer
  (render/initialize/upload/beginPass/endPass/thumbnail).
- [x] Added startup milestones in main_qml (BackendContext/load/app.exec).
- [x] Ran 5 D3D12 reproduction attempts: 5/5 crashed with 0xC0000005.
- [x] Ran 5 D3D11 attempts: 5/5 survived (control).
- [x] Analyzed milestone traces + GPU/display info.
- [x] Located root cause for THIS environment (virtual display IDD + D3D12
  flip-model swapchain), distinct from the historical original-machine crash.
- [x] Produced REPRODUCTION-REPORT.md.

## Key Result

The crash reproduces 100% here but is a DIFFERENT root cause from the
historical one (see REPRODUCTION-REPORT.md). This environment's crash dies in
QRhi D3D12 swapchain init (before render()), caused by Todesk/Oray virtual
display IDD drivers. The historical crash reached render() (seam A/B/C remain
valid for it). seam A/B/C mitigations cannot be validated here. Proceed to
mitigate (208-210) as preventive improvements, then promote with robust
fallback (211).

## Verify

- [x] D3D12 crash reproduced deterministically (5/5).
- [x] D3D11 control survived (5/5).
- [x] Crash localized to QRhi scene-graph startup, before render().
- [x] REPRODUCTION-REPORT.md documents the two distinct root causes.
