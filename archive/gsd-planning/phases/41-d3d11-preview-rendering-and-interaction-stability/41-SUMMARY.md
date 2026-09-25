---
phase: 41-d3d11-preview-rendering-and-interaction-stability
plan: 01
subsystem: preview-rendering
tags: [qt, qml, qrhi, d3d11, preview, renderer]
requires:
  - phase: 40-complete-preview-data-and-upstream-view-semantics
    provides: coherent active-result GCV1 Preview payload
provides:
  - QRhi Preview GPU state reset after resource release
  - Preview buffer reupload after resize/resource rebuild
  - defensive layer-range draw normalization
  - audit guards for payload-preserving interactions
affects: [phase-42-gcode-export, phase-43-e2e-verification]
key-files:
  created:
    - .planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-REVIEW.md
    - .planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-SUMMARY.md
    - .planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-VERIFICATION.md
  modified:
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - tests/QmlUiAuditTests.cpp
key-decisions:
  - "Keep D3D11 QRhi as the normal capable-system Preview path."
  - "Preserve CPU staging across QRhi resource release and force GPU reupload."
  - "Treat layer/move/camera/toggle interactions as view-state changes over the same payload."
patterns-established:
  - "Preview GPU-only state is reset through resetPreviewGpuState(true); payload replacement uses resetPreviewGpuState(false)."
requirements_completed: [PREVIEW-05, PREVIEW-06, PREVIEW-07, PREVIEW-08]
completed: 2026-06-29
---

# Phase 41: D3D11 Preview Rendering and Interaction Stability Summary

Phase 41 stabilizes the Preview renderer's QRhi/D3D11 interaction path.

## Accomplishments

- Added RED audit tests for Preview QRhi resource lifecycle, payload-preserving interaction setters, and defensive draw-range handling.
- Fixed `RhiViewportRenderer::releaseResources()` so QRhi resource rebuilds reset Preview GPU upload state without discarding CPU staging data.
- Added `resetPreviewGpuState(bool keepCpuStaging)` to centralize Preview buffer, upload-flag, byte-count, vertex-count, and timing reset behavior.
- Ensured payload replacement clears stale CPU/GPU state before parsing new `GCV1` data.
- Normalized transient inverted layer ranges in `computePreviewDrawRange()` so slider drag edge cases do not blank otherwise valid data.
- Verified the normal Preview path remains `RhiViewport`/QRhi with D3D11-first backend policy and no direct `SoftwareViewport` in `PreviewPage.qml`.

## Task Commits

1. **Planning** - `7b3e2da` (`docs(41)`)
2. **RED tests** - `d2c4057` (`test(41-01)`)
3. **Renderer fix** - `a7e177d` (`fix(41-01)`)

## Files Created/Modified

- `tests/QmlUiAuditTests.cpp` - adds regression coverage for Preview QRhi lifecycle and interaction payload stability.
- `src/qml_gui/Renderer/RhiViewportRenderer.h` - declares the Preview GPU state reset helper.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - implements resource-release reupload behavior and layer-range normalization.
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-REVIEW.md` - review outcome.
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-VERIFICATION.md` - verification evidence.

## Deviations from Plan

- No new runtime pixel-capture test was added. The deterministic coverage is source/audit based, and canonical app smoke/E2E passed.
- No new visible marker or bed rendering was added. Existing toggles are stability-safe, but full marker/bed visual parity is not expanded in this phase.

## Residual Risk

- Manual UAT is still needed for the exact user-reported workflow: enter Preview, drag layer height/move controls, orbit/pan/zoom, and confirm the toolpath no longer disappears.
- D3D12/Vulkan promotion remains future backend work. D3D11 remains the normal stable path.

## Next Phase Readiness

Phase 42 can proceed to local G-code export/finalization. Phase 43 should include manual Preview interaction UAT as part of the full import-to-export verification.
