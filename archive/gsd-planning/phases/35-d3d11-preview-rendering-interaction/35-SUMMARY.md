---
phase: 35
plan: 01
subsystem: preview-rhi-rendering
tags:
  - rendering
  - preview
  - qrhi
  - tests
key-files:
  modified:
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - tests/QmlUiAuditTests.cpp
metrics:
  tests_added: 1
---

# Phase 35 Summary: D3D11 Preview Rendering Interaction

## Result

Complete.

## Delivered

- Added QML/UI audit coverage for the Preview RHI normal path, including:
  - `PreviewPage.qml` binding `GCV1` preview data, layer range, move range, travel visibility, and color mode into `GLViewport`.
  - `main_qml.cpp` keeping `RhiViewport` registered as the normal `GLViewport` path when QRhi initializes.
  - `PreviewPage.qml` not instantiating `SoftwareViewport` directly.
- Replaced renderer layer/travel ranges with per-segment `PreviewDrawSpan` records containing packed layer, move, vertex offset, and vertex count.
- Reworked `computePreviewDrawRange` to select vertices by exact layer range and move cutoff.
- Removed renderer-side `move == 0` travel classification and proportional vertex-count playback clipping.

## Commits

| Commit | Description |
|---|---|
| `3c34615` | Add failing audit coverage for Preview RHI binding and exact draw span requirements. |
| `f8af356` | Implement exact Preview draw spans in `RhiViewportRenderer`. |

## Deviations

- None. Phase 34 already repacks the `GCV1` payload for travel visibility and color mode, so Phase 35 kept the payload format stable and made renderer range selection exact.

## Self-Check

PASSED.
