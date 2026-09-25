# Phase 41 Code Review: D3D11 Preview Rendering and Interaction Stability

**Date:** 2026-06-29
**Reviewer:** Codex
**Scope:** Commits `d2c4057` and `a7e177d`
**Result:** Pass

## Findings

No P0/P1 issues found.

## Review Notes

- `RhiViewportRenderer::releaseResources()` now resets Preview GPU upload state through `resetPreviewGpuState(true)` while preserving CPU staging data. This addresses the QRhi resource-rebuild path that could leave `m_previewSegmentBufferUploaded == true` after the buffer had been released.
- Preview payload changes reset GPU state and CPU staging before parsing. This prevents stale vertices, stale byte counts, or stale timing data from leaking across active G-code results.
- `computePreviewDrawRange()` now normalizes transient inverted layer bounds with `std::min`/`std::max`, preventing valid payloads from blanking during slider drag edge cases.
- Interaction setters are guarded by audit coverage to schedule redraws without mutating `m_previewData` or refitting the camera on pure layer/move/toggle updates.
- The backend normal path remains QRhi/RhiViewport with D3D11-first policy; `SoftwareViewport` remains a fallback only.

## Residual Risk

- The new test coverage is source/audit based rather than pixel capture. This is intentional because reliable QRhi pixel capture in the headless test environment is not established yet.
- `showBed` and `showMarker` are verified as stability-safe toggles for Preview interaction, but this phase did not add new marker/bed drawing features beyond preserving the main toolpath. Full visual parity can be expanded after the import-to-export main flow is complete if needed.
- Payload changes invoke the reset helper before and inside `parsePreviewSegments()`. This is redundant but not on the interaction hot path, and it keeps parse entry independently safe for future callers.

## Verification

- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed.
- `.\build\QmlUiAuditTests.exe -o build\qml_phase41_green.txt,txt` passed: 21 passed, 0 failed.
- `git diff --check` passed.
