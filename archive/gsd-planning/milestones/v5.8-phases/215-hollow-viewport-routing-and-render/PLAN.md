# Phase 215 Plan: Hollow Viewport Routing and Render

**Requirement:** HOLLOW-04, HOLLOW-05, VER-01..04
**Goal:** Complete the Hollow gizmo interactive loop (place/clear/render) by
wiring the viewport mouse routing, renderer marker upload/draw, and QML
bindings; re-verify the regression gates.
**Status:** Complete · **Commit:** `5b7b8b4`

## Files

- Modify: `src/qml_gui/Renderer/RhiViewport.h`, `.cpp` (`hollowPickRequested`
  signal, `emitHollowPickIfActive`, mousePressEvent GizmoHollow branch,
  `hollowMarkerData` Q_PROPERTY + setter)
- Modify: `src/qml_gui/Renderer/RhiViewportRenderer.h`, `.cpp`
  (`uploadHollowMarkerBuffer`, `renderHollowMarkers`, buffer/flag/bytes
  members, synchronize + releaseResources cleanup)
- Modify: `src/qml_gui/pages/PreparePage.qml` (`hollowMarkerData` binding,
  `onHollowPickRequested` Connections)

## Steps

- [x] `RhiViewport::hollowPickRequested(worldOrigin, worldDirection,
  pickedSourceIndex)` signal + `emitHollowPickIfActive` (stage-1 AABB pick +
  GizmoMath::computeRay world ray; mirrors emitMeasurePickIfActive).
- [x] mousePressEvent `GizmoHollow` branch routes left-click to the emitter.
- [x] `RhiViewport::hollowMarkerData` Q_PROPERTY + setter (byte-pipe from
  EditorViewModel, same pattern as paintOverlayData).
- [x] `RhiViewportRenderer::uploadHollowMarkerBuffer` — parses uint32 count +
  GizmoVertex stream, swaps libslic3r Y/Z → scene space, uploads to
  `m_hollowMarkerBuffer`.
- [x] `renderHollowMarkers` — translucent fill pipeline (depth-tested, no
  depth write), gated to GizmoHollow (mode 8); drawn after paint overlay.
- [x] synchronize + releaseResources + buffer-bytes cleanup wired (mirrors
  paint overlay's three cleanup sites).
- [x] PreparePage.qml: `viewport3d.hollowMarkerData` bound to
  `editorVm.hollowMarkerData`; `onHollowPickRequested` forwards to
  `editorVm.placeHollowPoint`.

## Verification (VER-01..04)

- owzx_app_core + ViewModelSmokeTests + QmlUiAuditTests build clean.
- `multiPlateFullStateRoundTrip` PASS (3 passed, 0 failed) [VER-01].
- `E2EWorkflowTests::test_slice_produces_gcode_file` PASS (cereal StaticObject
  resolved) [VER-02].
- v5.0 dialog regression locks all PASS: `v50PresetIniAndCreateDialogWired`,
  `v50UnsavedChangesAndFilterWired`, `v50CompareDiffAndRoundTripWired`
  (3 passed, 0 failed each) [VER-03].
- `mmuSegmentationPaintFeedsSlice` PASS (3 passed, 0 failed) [VER-04] — the
  Hollow additions introduce no regression in the existing MMU/paint paths.
