# Phase 41 Research: D3D11 Preview Rendering and Interaction Stability

**Date:** 2026-06-29
**Status:** Complete

## Upstream Source Truth

### `GUI_Preview`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.cpp`

Relevant behavior:

- `Preview::update_gcode_result()` swaps the active `GCodeProcessorResult` pointer when the slice result changes.
- `Preview::update_layers_slider()` preserves or resets slider ranges based on the loaded Preview layer data and keeps slider ticks/time data tied to the current result.
- `Preview::update_layers_slider_from_canvas()` maps dirty layer slider and move slider state into the canvas/gcode viewer, not into a reparse/reload of the result.
- `Preview::load_print_as_fff()` calls `m_canvas->load_gcode_preview(...)` only when a valid result exists, then shows sliders and updates them from the loaded layer Zs.

Implication for Qt:

- Slider interaction should be draw-state over the already loaded Preview payload. It should never blank or discard the current toolpath unless the active result itself changes or becomes invalid.
- Qt can keep the Phase 40 `GCV1` contract, but the QRhi renderer must treat range/camera changes as cheap draw/uniform changes, not as payload lifetime changes.

### `GLCanvas3D`

Relevant file:

- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp`

Relevant behavior:

- In Preview mode, dirty layers slider state calls `m_gcode_viewer.set_layers_z_range(...)`.
- Dirty moves slider state calls `m_gcode_viewer.update_sequential_view_current(...)`.
- Camera mouse/wheel interaction updates camera state and schedules redraws while leaving loaded Preview data intact.
- `load_gcode_preview()` disables rendering while replacing Preview data and reenables it after the result is loaded.

Implication for Qt:

- `RhiViewport` property setters for layer/move/camera/toggles should only schedule `update()`.
- `RhiViewportRenderer` must keep CPU staging and GPU upload state separate: CPU `m_previewVertices` can remain valid, but any QRhi resource release must force a GPU reupload.

## Current Qt Implementation

### `RhiViewport`

Relevant files:

- `src/qml_gui/Renderer/RhiViewport.h`
- `src/qml_gui/Renderer/RhiViewport.cpp`

Strengths:

- Uses `QQuickRhiItem`, giving Qt-native QRhi integration instead of a software-painted path.
- `setPreviewData()` fits the camera from the packed `GCV1` payload and calls `update()`.
- `setLayerMin()`, `setLayerMax()`, `setMoveEnd()`, `setShowTravelMoves()`, `setShowBed()`, `setShowMarker()`, marker setters, and camera input paths call `update()`.
- `fitPreviewCameraToData()` parses the same `GCV1` axis mapping as the renderer and avoids resetting orbit when the fit bounds are unchanged.

Risks:

- The viewport does not expose renderer diagnostics to tests. Phase 41 should avoid over-instrumenting public QML API unless diagnostics are user-visible and useful.
- Mouse left-drag first attempts object picking even in Preview. With no Prepare model batches it falls through to orbit, which is acceptable, but a future combined shell overlay could need a Preview-specific selection policy.

### `RhiViewportRenderer`

Relevant files:

- `src/qml_gui/Renderer/RhiViewportRenderer.h`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

Strengths:

- Parses `GCV1` once when `m_previewData` changes.
- Uploads Preview segments into a static vertex buffer and draws lines through the QRhi line pipeline.
- Uses exact per-segment layer and move metadata for draw-range filtering.
- Uploads camera uniform before `beginPass()`, avoiding the prior QRhi command ordering bug.

Defects and risks:

- `releaseResources()` resets general buffers but omits Preview-specific upload state:
  - `m_previewSegmentBufferUploaded`
  - `m_previewSegmentBufferBytes`
  - `m_previewSegmentVertexCount`
  - `m_previewLastUploadMs`
  - `m_previewLastFrameMs`
  - `m_previewFirstFrameMs`
  - `m_previewFirstFrameDone`
- Because `initialize()` calls `releaseResources()`, a render-target or QRhi resource rebuild can leave CPU preview vertices valid but GPU upload state incorrectly marked as already uploaded. The render branch then sees no buffer or a reset buffer and draws zero segments. This matches the user report: layer/camera interaction or resize/page churn can make the model disappear.
- `parsePreviewSegments()` clears CPU staging and vertex count, but does not consistently reset GPU upload byte state/timing. Payload changes should mark the buffer dirty before parse and leave empty/invalid payloads with no stale GPU state.
- `computePreviewDrawRange()` returns zero when the layer selection is inverted. QML should clamp, but renderer-level defensive normalization avoids blanking during transient drag values.
- The `m_showTravelMoves` property is copied but not used in renderer. Phase 40 repacks payload on travel visibility changes, so this is acceptable, but tests should ensure it does not cause buffer churn or stale state.

### Backend Selection

Relevant files:

- `src/qml_gui/main_qml.cpp`
- `src/qml_gui/Renderer/RhiBackendSelector.{h,cpp}`
- `tests/QmlUiAuditTests.cpp`

Current behavior:

- Startup sets `OWZX_RHI_RENDERER=auto` unless the user explicitly requests `OWZX_OPENGL`.
- `RhiBackendSelector` owns the D3D11-first / D3D12 explicit opt-in policy.
- `main_qml.cpp` registers `RhiViewport` as `OWzxGL.GLViewport` when QRhi initializes, and registers `SoftwareViewport` only when QRhi cannot initialize.
- Existing audits guard this policy and ensure PreviewPage does not instantiate `SoftwareViewport` directly.

Implication:

- Phase 41 does not need to rewrite backend policy. It should add missing diagnostics/guards only if they help identify fallback regressions.

## Test Strategy

### RED Tests

- Extend `tests/QmlUiAuditTests.cpp` with deterministic source-level assertions:
  - Preview GPU upload state is reset when QRhi resources are released.
  - Preview payload parsing and resource release use a shared helper or explicit state reset that includes upload flag, bytes, vertex count, and first-frame timing.
  - Range/camera/toggle setters update the item without mutating `m_previewData`.
  - Renderer draw range defensively handles inverted layer ranges.

This is not a substitute for runtime UAT, but it captures the exact lifecycle bug and is stable under CI/headless constraints.

### Runtime Verification

- Run the canonical verification script.
- Run focused `QmlUiAuditTests` after the RED test and after implementation.
- If the app starts successfully, manually launch and exercise Preview after Phase 41 so the user can validate the visual behavior.

## Implementation Implications

- Add a private helper in `RhiViewportRenderer`, for example `resetPreviewGpuState(bool keepCpuStaging)`, and call it from:
  - `releaseResources()`
  - `initialize()`, through `releaseResources()`
  - payload-change path in `synchronize()`
  - parse failure/empty data paths as needed
- Keep CPU staging unless a payload change invalidates it. Resource release should not clear `m_previewData` or `m_previewVertices`; it should only force reupload.
- `uploadPreviewSegmentBuffer()` should set upload timing and upload state only through the render path after the QRhi buffer exists.
- `computePreviewDrawRange()` should normalize layer bounds locally and clamp move/layer filters without stale offsets.
- Keep logs concise and tagged:
  - backend selected in startup diagnostics already exists;
  - renderer can log upload/draw metrics with `[RHI-PREVIEW]` or adapt existing `[RHI-PERF]`.

## Verification Commands

Required:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
git diff --check
```

Focused optional checks:

```powershell
.\build\QmlUiAuditTests.exe -o build\qml_phase41.txt,txt
```
