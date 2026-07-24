# Phase 211 Plan: D3D12 Promotion with Automatic Fallback + Slot Rewrite

**Requirement:** PROMO-01
**Goal:** Promote D3D12 to the default QRhi backend on Windows (D3D12-first
candidate order), with D3D11 as automatic probe fallback, and rewrite the
regression slots to assert the new policy.

## Files

- Modify: `src/qml_gui/Renderer/RhiBackendSelector.cpp` (D3D12-first order + comment)
- Modify: `src/qml_gui/main_qml.cpp` (comment update)
- Modify: `tests/QmlUiAuditTests.cpp` (5 slot rewrites + 3 verify-script assertion updates)
- Modify: `tools/render_bench/main.cpp` (stableAuto D3D12-first alignment)
- Modify: `scripts/auto_verify_with_vcvars.ps1` (OWZX_VERIFY_RHI_BACKEND override for IDD hosts)
- Create: `.planning/milestones/v5.7-MILESTONE-AUDIT.md`

## Steps

- [x] `defaultWindowsCandidates()` swapped to D3D12-first; comment documents the
      promotion + the IDD/virtual-display limitation + the D3D11 fallback.
- [x] `main_qml.cpp` RhiBackendSelector policy comment updated.
- [x] 5 regression slots rewritten: `d3d12StaysOptInBehindEnvFlag` renamed to
      `d3d12PromotedWithD3d11Fallback` (D3D12 < D3D11 assertions, D3D12-first
      comment check, D3D11 fallback-exists assertion, no Vulkan retained);
      `mainRegistersRhiViewportByDefaultWithSoftwareFallback`,
      `mainRegistersRhiViewportOnlyBehindExplicitGate`,
      `previewNormalPathCoversFullWorkflowBindingsAndDiagnostics` order
      assertions flipped; WS4 soft reference + GATE-01 block updated.
- [x] 3 verify-script assertions relaxed: allow conditional
      `OWZX_VERIFY_RHI_BACKEND` override but still forbid unconditional
      `OWZX_RHI_RENDERER = "..."` hardcoding.
- [x] `render_bench/main.cpp` stableAuto aligned to D3D12-first.
- [x] `auto_verify_with_vcvars.ps1` launch gate honors
      `OWZX_VERIFY_RHI_BACKEND` (set to `d3d11` on IDD/virtual-display hosts so
      the launch liveness gate can verify despite the D3D12 swapchain crash).

## Verify (this environment = virtual display, D3D12 crashes at swapchain init)

- [x] Canonical build with `OWZX_VERIFY_RHI_BACKEND=d3d11` exited 0.
- [x] All ctest targets PASS (PrepareScene, PartPlate, ViewModel, QML UI audit
      incl. `d3d12PromotedWithD3d11Fallback`, PreviewParser).
- [x] `APP_RUNNING_PID=64080` (D3D11 launch liveness via the override).
- [x] E2E "All pipeline tests passed".

## Pending Machine Verification (out of this environment)

The promoted D3D12 default cannot be launch-verified here because the host uses
virtual displays (Todesk/Oray IDD) where D3D12's flip-model swapchain crashes
before render() (see Phase 207 REPRODUCTION-REPORT.md — a different root cause
from the historical seam-A/B/C crash). Confirmation that D3D12 launches cleanly
requires a host with a real GPU + real display. The three seam mitigations
(Phase 208/209/210) address the historical render()-internal crash but cannot
be validated in this environment.

## Known Limitation

On IDD/virtual-display hosts, users (or the canonical verify script) must set
`OWZX_RHI_RENDERER=d3d11` (or the launcher sets `OWZX_VERIFY_RHI_BACKEND=d3d11`).
This is documented in the `defaultWindowsCandidates()` comment and the v5.7
milestone audit. A future enhancement could auto-detect IDD adapters and
auto-fallback, but that is out of v5.7 scope.
