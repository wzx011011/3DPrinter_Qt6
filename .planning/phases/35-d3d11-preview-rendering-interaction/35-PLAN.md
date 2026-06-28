---
phase: 35
plan: 01
type: tdd
wave: 1
depends_on: []
files_modified:
  - tests/QmlUiAuditTests.cpp
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
autonomous: true
requirements:
  - RENDER-01
  - RENDER-02
  - RENDER-03
  - TEST-03
---

# D3D11 Preview Rendering Interaction Implementation Plan

<objective>
Harden the Qt RHI Preview renderer so D3D11 normal-path Preview renders the Phase 34 G-code payload with exact layer and move range behavior, and add audit coverage that prevents silent fallback or proportional playback regressions.
</objective>

<tasks>

## Task 1: RED audit coverage for Preview RHI interaction

**Files:** `tests/QmlUiAuditTests.cpp`

**Action:**
- Add a source/audit regression covering the Preview RHI path:
  - `PreviewPage.qml` must bind `previewData`, `layerMin`, `layerMax`, `moveEnd`, `showTravelMoves`, and `gcodeViewMode` into `GLViewport`.
  - `main_qml.cpp` must keep `RhiViewport` as the default `GLViewport` registration on the normal path.
  - `RhiViewportRenderer` must expose/use an explicit per-segment draw span keyed by layer and move.
  - `RhiViewportRenderer` must not contain the old proportional `moveEnd` vertex-count clipping formula.
  - `RhiViewportRenderer` must not treat `move == 0` as the travel marker.

**Verify:**
- Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Confirm the new audit fails before renderer production changes.

## Task 2: Exact layer/move draw range in RHI renderer

**Files:** `src/qml_gui/Renderer/RhiViewportRenderer.h`, `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

**Action:**
- Parse each packed `GCV1` segment into QRhi line vertices and a matching draw span containing:
  - layer index,
  - move index,
  - vertex offset,
  - vertex count.
- Rework `computePreviewDrawRange` to select vertices by exact layer range and `moveEnd` cutoff.
- Draw zero segments when `moveEnd <= 0`.
- Draw all eligible segments when `moveEnd` is greater than the available move indices.
- Remove renderer-side travel classification based on move index.

**Verify:**
- Re-run the canonical verification command and confirm the RED audit passes.

## Task 3: Normal-path fallback guard and review

**Files:** `tests/QmlUiAuditTests.cpp`, `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

**Action:**
- Ensure the audit still guards that `SoftwareViewport` is not the normal Preview path.
- Review the final renderer diff for stale range state, buffer upload churn, and mismatched `GCV1` struct assumptions.
- Keep changes scoped to Preview RHI interaction and tests.

**Verify:**
- Re-run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Confirm PrepareScene, PartPlate, QML UI audit, and E2E pipeline suites pass.

</tasks>

<verification>
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Confirm QML/UI audit coverage detects exact Preview RHI range handling.
- Confirm existing Phase 33 and Phase 34 workflow/parser tests still pass.
</verification>

<success_criteria>
- `RENDER-01`: default Windows Preview remains on the D3D11-capable QRhi viewport and renders non-empty `GCV1` segment buffers.
- `RENDER-02`: layer and move controls drive exact renderer range selection without proportional clipping.
- `RENDER-03`: renderer keeps a stable uploaded segment buffer and only changes draw range for slider movement.
- `TEST-03`: source/audit coverage guards default Preview bindings and prevents normal-path `SoftwareViewport` fallback.
</success_criteria>
