# Phase 81: Preview Layer Move And Playback Controls - Context

**Gathered:** 2026-07-07
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 81 restores the Preview controls that directly mutate the visible G-code
window: layer range/current-layer controls, move stepping/playback controls,
and Preview camera controls. The phase must preserve the existing GCV1 payload,
role-mask, view-mode, and RHI rendering pipeline from earlier phases.

</domain>

<decisions>
## Implementation Decisions

### Layer Rail
- Replace the simple one-value vertical Slider in `PreviewPage.qml` with a
  compact Preview-specific vertical range rail component.
- The rail must expose both visible layer bounds and current-layer movement
  through `PreviewViewModel::setLayerRange`, `jumpToLayer`, and
  `moveLayerRange`.
- The rail must remain narrow enough for the screenshot right edge and avoid
  importing the large horizontal `LayerSlider.qml` editor surface.
- The renderer binding remains `layerMin: root.previewVm.currentLayerMin` and
  `layerMax: root.previewVm.currentLayerMax`.

### Move And Playback
- Bottom playback controls must keep the existing `MoveSlider.qml` data path
  but add explicit previous/next controls for single-step and larger jump
  semantics.
- Move stepping belongs in `PreviewViewModel`, not QML arithmetic, so keyboard
  shortcuts and buttons share the same clamping behavior.
- Playback must keep `togglePlayPause`, `playAnimation`, and `pauseAnimation`
  as the only timer controls.
- Current move changes must continue to update tool marker data and the
  bounded G-code source-line window atomically.

### Camera Stability
- Preserve the existing automatic Preview camera fit when GCV1 data enters the
  viewport.
- Add an explicit Preview fit control that reuses the cached Preview fit bounds
  instead of fabricating QML-side camera coordinates.
- Orbit, pan, zoom, and view preset controls must update only camera state and
  must not clear or repack `previewData`.
- D3D12/Vulkan promotion is out of scope; Phase 81 stays on the verified QRhi
  auto/D3D11 path.

### the agent's Discretion
- The agent may use text glyphs for compact playback buttons if no existing
  icon asset matches the control and the result stays readable.
- The agent may add focused source-audit tests where full QML interaction tests
  are too heavy for this phase; Phase 83 owns runtime screenshot evidence.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/pages/PreviewPage.qml` owns the Preview layout and current
  bindings into `GLViewport`.
- `src/qml_gui/components/MoveSlider.qml` owns playback, move slider, tool
  change color bands, time labels, and hover time.
- `src/core/viewmodels/PreviewViewModel.*` owns layer range, current move,
  playback timer, current G-code line window, tool marker data, and labels.
- `src/qml_gui/Renderer/RhiViewport.*` owns Preview camera fit, orbit, pan,
  zoom, and view presets.
- `tests/ViewModelSmokeTests.cpp` already tests current-move atomic updates and
  layer/legend stability.
- `tests/QmlUiAuditTests.cpp` already audits Preview bindings and renderer
  interaction setters.

### Established Patterns
- QML calls C++ `Q_INVOKABLE` methods for behavior; QML does not own Preview
  business logic or parse G-code.
- Renderer interaction setters schedule redraws but must not mutate
  `m_previewData` or call `fitPreviewCameraToData()` except when new Preview
  data enters the viewport or the user explicitly asks to fit.
- Source-audit Qt tests are acceptable for QML wiring and stale path guards;
  final runtime evidence belongs in Phase 83.

### Integration Points
- `PreviewPage.qml` binds `layerMin`, `layerMax`, and `moveEnd` to
  `PreviewViewModel` properties.
- `MoveSlider.qml` currently calls `setCurrentMove(Math.round(value))`.
- Keyboard shortcuts in `PreviewPage.qml` currently perform move-step
  arithmetic inline; these should route through the same ViewModel step method
  as the new playback buttons.
- `RhiViewport::fitPreviewCameraToData()` computes a `m_previewFitHint` from
  the same GCV1 payload consumed by the renderer.

</code_context>

<specifics>
## Specific Ideas

Use a new compact `PreviewLayerRail.qml` for the screenshot-visible far-right
rail. Keep the older `LayerSlider.qml` available for future richer layer-edit
work, but do not insert its large horizontal menu-heavy UI into the Preview
right edge.

</specifics>

<deferred>
## Deferred Ideas

- Runtime pixel comparison and final Preview screenshot capture are Phase 83.
- Full color-mode/role semantics are Phase 82.
- Full D3D12/Vulkan backend investigation is future backend work.

</deferred>
