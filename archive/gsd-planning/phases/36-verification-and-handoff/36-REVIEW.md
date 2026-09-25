---
phase: 36
status: clean
depth: standard
files_reviewed: 7
findings:
  critical: 0
  warning: 0
  info: 1
total_findings: 1
reviewed_at: 2026-06-28
---

# Phase 36 Code Review

## Scope

Reviewed the v3.3 main-flow code path across the Phase 33-35 changes:

- `src/qml_gui/BackendContext.cpp`
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `src/qml_gui/pages/PreviewPage.qml`
- `src/qml_gui/Renderer/RhiViewportRenderer.h`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `tests/E2EWorkflowTests.cpp`
- `tests/QmlUiAuditTests.cpp`

## Findings

No critical or warning-level issues were found in the v3.3 MVP path.

### Info: End-position marker polish

`PreviewViewModel::updateToolPositionData()` clears `hasToolPosition_` when `currentMove_ == moveCount_`, so the tool marker may be hidden when the move slider is at the full/end position. This does not block the v3.3 requirements because the non-empty D3D11 preview, layer/move range filtering, travel toggle, and color modes still work. Track this as preview-interaction polish for the next milestone.

## Checks

- Slice completion posts the existing completion notification and switches to Preview.
- `PreviewViewModel` loads sliced or fixture G-code, handles absolute/relative extrusion, `G92 E`, layer Z, tool changes, and travel filtering.
- `PreviewPage.qml` binds the normal `GLViewport` path to preview data/range/color state.
- The normal Preview path remains QRhi/D3D11 through `RhiViewport`; `SoftwareViewport` remains only the guarded fallback registration path.
- `RhiViewportRenderer` computes draw ranges from exact per-segment layer/move spans instead of proportional vertex slicing.
