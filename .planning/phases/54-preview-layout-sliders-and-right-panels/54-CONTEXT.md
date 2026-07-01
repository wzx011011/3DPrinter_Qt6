# Phase 54 Context - Preview Layout, Sliders, and Right Panels

## Goal

Restore the Preview page structure shown in `shotScreen/预览页.png` while keeping the high-performance QRhi/D3D11 renderer path intact.

The phase closes `PREVLAY-01` through `PREVLAY-05`:

- Preview layout has a left summary/sidebar, center G-code viewport, vertical layer slider, bottom move slider, and right legend/statistics/G-code panels.
- Layer and move slider interaction updates draw ranges without clearing the packed G-code payload or resetting camera state.
- Mouse orbit, pan, zoom, and view preset actions keep the Preview payload visible.
- Plate/summary, current layer, current move, print time, filament usage, and warnings are visible when data exists.
- Side panels can collapse or resize without overlapping the viewport or clipping core text.

## Source Truth

- Visual truth: `shotScreen/预览页.png`.
- Inventory truth: `docs/v3.6-ui-inventory.md` section `PREV-*`.
- Upstream behavior truth:
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.*`
  - `third_party/OrcaSlicer/src/libslic3r/GCode/*`

## Current Qt State

- `src/qml_gui/pages/PreviewPage.qml` already uses the registered `GLViewport` QML type. In the default app path this resolves to `RhiViewport`, not `SoftwareViewport`.
- The important renderer bindings already exist and must be preserved:
  - `previewData: root.previewVm.gcodePreviewData`
  - `layerMin: root.previewVm.currentLayerMin`
  - `layerMax: root.previewVm.currentLayerMax`
  - `moveEnd: root.previewVm.currentMove`
  - `gcodeViewMode: root.previewVm.viewModeIndex`
- `RhiViewport` interaction setters only call `update()` and do not mutate `m_previewData`, so UI changes should not rebuild or clear Preview payloads.
- Existing Preview QML files contain historical mojibake in visible labels and comments. Phase 54 will repair the touched Preview files. New comments must be English.

## Decisions

- Keep QRhi/D3D11 as the normal rendering path. Do not instantiate `SoftwareViewport` from `PreviewPage.qml`.
- Treat `GLViewport` as the stable QML compatibility name, because startup chooses the concrete renderer backend.
- Keep business logic in `PreviewViewModel`. QML may compose text from exposed properties but must not parse G-code or own draw-range logic.
- Build missing G-code text preview as a `PreviewViewModel` data surface backed by the active G-code file, not a QML file reader.
- Use local panel collapse state in QML for layout ergonomics only. Renderer state remains in C++/ViewModel.

## Risks

- Recoloring or travel visibility changes repack `gcodePreviewData`; camera fit must remain stable via `RhiViewport::m_previewFitHint`.
- A vertical slider implemented as a different control could accidentally bypass existing `LayerSlider.qml`. Use the existing component unless a targeted wrapper is needed.
- G-code text preview can be large. Expose a bounded window of lines around the current move to keep QML lightweight.

## Verification Targets

- Source audit: PreviewPage contains all five screenshot regions and never references `SoftwareViewport`.
- Unit/smoke: `PreviewViewModel` exposes non-empty G-code text window after loading a fixture and keeps payload across layer/move/view interactions.
- Canonical verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Manual UAT: import or reuse a sliced model, switch to Preview, drag layer slider, drag bottom move slider, orbit/zoom, collapse/expand side panels, inspect G-code text.
