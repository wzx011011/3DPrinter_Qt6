---
phase: 41
plan: 01
type: implementation
wave: 1
depends_on: [40]
files_modified:
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/qml_gui/Renderer/RhiViewport.cpp
  - src/qml_gui/main_qml.cpp
  - tests/QmlUiAuditTests.cpp
autonomous: true
requirements_addressed: [PREVIEW-05, PREVIEW-06, PREVIEW-07, PREVIEW-08]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp
---

# Phase 41 Plan: D3D11 Preview Rendering and Interaction Stability

<objective>
Make the Preview page's Qt QRhi/D3D11 renderer stable and performant for the complete local workflow: valid G-code remains visible while the user drags layer/move sliders, changes view modes/toggles, plays preview, orbits/pans/zooms/fits the camera, resizes, switches plates, and returns to Preview.
</objective>

<truths>

- D-41-01: `RhiViewport` through Qt QRhi/D3D11 is the normal Windows capable-system Preview path; `SoftwareViewport` is fallback only.
- D-41-02: Upstream Preview behavior treats sliders and camera as view state over loaded G-code data. Qt must preserve that behavior even though the renderer implementation is QRhi rather than upstream OpenGL/libvgcode.
- D-41-03: Phase 40's `GCV1` payload is the renderer data contract and should remain stable unless a blocker appears.
- D-41-04: QRhi resource release must never leave upload flags saying a released Preview buffer is live.
- D-41-05: Layer/move/camera/toggle interactions must not reparse or rebuild the Preview payload; they should redraw or update uniforms/ranges over the same CPU/GPU segment data.
- D-41-06: Tests are RED-first and must capture the resource lifecycle defect before production renderer changes.
- D-41-07: Performance is a primary requirement: avoid full buffer rebuilds on pure range/camera changes and keep large payload interactions responsive.

</truths>

<tasks>

## Task 1 - Add RED Tests for Preview QRhi Lifecycle and Normal Path

type: tdd
files:
- `tests/QmlUiAuditTests.cpp`

action:
- Add failing audit coverage for the Phase 41 regression surface:
  - `releaseResources()` or a dedicated helper resets Preview GPU upload state after QRhi resource release, including upload flag, buffer byte size, vertex count, and first-frame timing flags.
  - Payload changes reset Preview GPU upload state before parsing, so empty/invalid/new data cannot keep stale buffers.
  - `computePreviewDrawRange()` handles inverted/transient layer ranges defensively instead of blanking solely because `layerMin > layerMax`.
  - Preview range/camera/toggle setters call `update()` and do not mutate `m_previewData`.
  - Backend policy continues to register `RhiViewport` as the normal path and does not instantiate `SoftwareViewport` in `PreviewPage.qml`.

verify:
- Run focused audit test and capture RED:
  - `.\build\QmlUiAuditTests.exe -o build\qml_phase41_red.txt,txt`
- If the binary is stale or unavailable, run canonical script to rebuild and capture the expected failure:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

acceptance_criteria:
- Tests fail before renderer changes for the missing Preview resource reset behavior.
- Tests remain deterministic and do not require GPU pixel capture.

## Task 2 - Harden Preview QRhi Buffer Lifecycle

type: implementation
files:
- `src/qml_gui/Renderer/RhiViewportRenderer.h`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

action:
- Add a private helper such as `resetPreviewGpuState(bool keepCpuStaging)` that can reset GPU-only state without discarding CPU `GCV1` staging data.
- Call the helper from:
  - `releaseResources()`;
  - Preview payload-change path in `synchronize()`;
  - parse invalid/empty payload paths where stale GPU state must be cleared.
- Reset at minimum:
  - `m_previewSegmentBuffer`;
  - `m_previewSegmentBufferBytes`;
  - `m_previewSegmentVertexCount` when CPU staging is not kept;
  - `m_previewSegmentBufferUploaded`;
  - first-frame and upload/frame timing fields.
- Ensure CPU staging is preserved across QRhi resource release so the next Preview frame reuploads the same visible data.
- Only mark `m_previewSegmentBufferUploaded = true` after a live buffer upload has been queued.

verify:
- Run focused `QmlUiAuditTests`.

acceptance_criteria:
- After `initialize()`/`releaseResources()`, a valid existing `m_previewVertices` payload is ready for reupload.
- Empty or invalid payload cannot leave a stale buffer or stale uploaded flag.

## Task 3 - Stabilize Layer/Move Range, Camera, Resize, and Toggle Interaction

type: implementation
files:
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `tests/QmlUiAuditTests.cpp`

action:
- Make `computePreviewDrawRange()` normalize or clamp transient layer bounds defensively while preserving upstream slider semantics.
- Keep `moveEnd <= 0` drawing zero moves, and valid `moveEnd` drawing exact packed spans.
- Confirm layer/move/camera/toggle setters schedule redraws without mutating preview payload or forcing buffer rebuild.
- Ensure camera uniform upload remains before `beginPass()` and is updated on orbit, pan, zoom, fit, preset, and resize paths.
- Keep `showTravelMoves` payload-driven through `PreviewViewModel`; renderer should not infer travel from move id.
- Ensure `showBed` and `showMarker` toggles do not disturb Preview segment buffers. If visible marker/bed rendering is out of scope for this slice, document that they are stability-safe but visual parity is bounded.

verify:
- Run focused audit tests and canonical script.

acceptance_criteria:
- Dragging layer/move controls cannot blank a valid payload due to transient range values.
- Orbit/pan/zoom/fit/resize preserves the same uploaded Preview segment data.
- Pure interaction changes do not reparse `GCV1` or reset segment upload unless QRhi resources were actually released.

## Task 4 - Add Diagnostics and Performance Guardrails

type: implementation
files:
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `src/qml_gui/main_qml.cpp`
- `tests/QmlUiAuditTests.cpp`

action:
- Keep startup diagnostics for selected QRhi backend and fallback reason.
- Add or refine concise renderer logs for Preview upload/draw behavior:
  - payload bytes;
  - segment count;
  - uploaded buffer bytes;
  - draw first/count;
  - first-frame/frame/upload timings.
- Guard with existing logging style so normal logs are useful but not spammed on every tiny interaction if possible.
- Add audit coverage that normal app startup is not forced to `SoftwareViewport` by canonical smoke verification and that D3D11 remains ahead of D3D12 in auto policy.

verify:
- Run `QmlUiAuditTests`.
- Inspect generated log behavior during app smoke if available.

acceptance_criteria:
- A future blank-preview report has enough diagnostics to distinguish backend fallback, empty payload, zero draw range, and lost GPU upload.
- Large payload range/camera interaction avoids avoidable full-buffer rebuilds.

## Task 5 - Verification, Review, and Phase Closeout

type: verification
files:
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-REVIEW.md`
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-SUMMARY.md`
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-VERIFICATION.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/STATE.md`

action:
- Run:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - `git diff --check`
- Review changes for:
  - stale Preview upload flags;
  - accidental software fallback as normal path;
  - buffer rebuilds on pure range/camera changes;
  - invalid range blanking;
  - QRhi command ordering regressions;
  - tests that overfit unrelated formatting.
- Write review, summary, and verification files.
- Mark `PREVIEW-05` through `PREVIEW-08` satisfied only if tests pass and renderer review has no P0/P1 findings.

acceptance_criteria:
- Canonical verification passes.
- Phase artifacts clearly state whether visible marker/bed rendering is completed or bounded to stability-safe toggles.
- Phase 42 can proceed to export finalization without an open P0/P1 Preview blanking defect.

</tasks>

<verification>

1. Add RED tests before production changes.
2. Capture expected failing output.
3. Implement QRhi lifecycle reset and interaction hardening.
4. Run focused `QmlUiAuditTests`.
5. Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
6. Run `git diff --check`.
7. Perform code review and write `41-REVIEW.md`.
8. Write `41-SUMMARY.md` and `41-VERIFICATION.md`.
9. Update requirements/roadmap/state only after verification passes.

</verification>

<success_criteria>

- Preview renders through `RhiViewport`/QRhi/D3D11 as the normal capable-system path.
- Dragging layer/move controls never blanks a valid active Preview payload.
- Orbit, pan, zoom, fit, view preset, and resize preserve visible G-code.
- Plate/result switching uses Phase 40 data and causes only valid payload replacement or reset.
- Pure interaction changes do not trigger avoidable full segment-buffer rebuilds.
- Diagnostics identify backend, upload, and draw-range state enough to debug future blank-preview regressions.
- Canonical verification passes.

</success_criteria>
