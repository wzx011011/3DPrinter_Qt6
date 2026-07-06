# Phase 79 Preview Gap Matrix

**Target evidence:** `shotScreen/预览页.png`
**Current evidence:** current source plus running `build/OWzxSlicer.exe`
process availability
**Scope:** Preview page only

## Summary

The current Preview implementation has real parser, renderer, and ViewModel
behavior from earlier phases, but the visible page is not yet screenshot-level
restored. The largest gaps are:

1. The Preview QML shell contains mojibake labels and does not yet match target
   density, right-panel table layout, and top/bottom control placement.
2. The target's far-right layer rail and bottom move rail require tighter,
   screenshot-aligned controls than the current generic sliders.
3. The right legend/statistics/G-code surfaces are structurally present but need
   upstream-like row density, role ordering, visibility affordances, and
   metadata grouping.
4. Current backend semantics are stronger than the UI: GCV1 payload, role mask,
   17 modes, current G-code line window, and camera fit exist and should be
   preserved rather than rewritten.
5. Final visual evidence was historically deferred; v4.0 must not close without
   a current Preview screenshot comparison.

## Canonical Region Matrix

| Region | Target Observation | Current Evidence | Qt Targets | Upstream Source | Decision | Gap | Severity | Owner | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|---|
| PV-TOP | Preview tab selected, compact dark top shell, single-plate/slice/export G-code controls on the top-right. | `main.qml` routes Prepare and Preview through shared `Plater`; `PreviewPage.qml` adds its own `previewHeader` with title/view metrics that do not match the screenshot top action placement. | `src/qml_gui/main.qml`; `src/qml_gui/BBLTopbar.qml`; `src/qml_gui/pages/PreviewPage.qml` | `GUI_Preview.cpp:251-283`; `Plater.cpp` top action wiring | Modify shared shell and remove redundant/off-target Preview header surfaces if the screenshot does not show them. | Header density/action placement mismatch and mojibake labels. | High | Phase 80 | PVLAYOUT-01, PVLAYOUT-03 | QML audit for top action placement plus runtime screenshot. |
| PV-LEFT | Prepare-like printer/filament/process sidebar remains visible on Preview, matching target width and density. | `PreviewPage.qml` embeds `LeftSidebar` at fixed 390px, preserving Prepare functionality but likely too wide for screenshot-level Preview parity. | `src/qml_gui/pages/PreviewPage.qml`; `src/qml_gui/panels/LeftSidebar.qml`; `src/qml_gui/panels/DockableSidebar.qml` | `Plater.cpp`; `GUI_Preview.cpp` shared Plater/Preview layout | Modify/reuse v3.9 restored sidebar with Preview-specific width constraints; do not fork duplicate sidebar logic. | Width/density may differ from target; Preview still inherits all Prepare sidebar complexity. | High | Phase 80 | PVLAYOUT-01, PVLAYOUT-03 | Runtime screenshot and QML audit for no raw labels/placeholders. |
| PV-VIEWPORT | Large grey workbench with bed-centered toolpath/model, stable camera, and empty overlay only when no data exists. | `PreviewPage.qml` binds `GLViewport` in `CanvasPreview` to `gcodePreviewData`; `RhiViewport` has Preview-data camera fit from the orbit-disappears fix. | `src/qml_gui/pages/PreviewPage.qml`; `src/qml_gui/Renderer/RhiViewport.*`; `src/qml_gui/Renderer/RhiViewportRenderer.*`; `src/core/viewmodels/PreviewViewModel.*` | `GUI_Preview.cpp:628-744`; `GCodeViewer.*`; `GLCanvas3D.*` | Preserve renderer/ViewModel path; modify visual framing, palette, overlay placement, and empty state. | Behavior exists, but visual framing and final runtime proof are incomplete. | High | Phase 80, Phase 81 | PVLAYOUT-01, PVCTRL-03 | QML/source audit plus runtime camera orbit/pan/zoom screenshot checks. |
| PV-RIGHT-LEGEND | Compact role/color table with visibility affordances, totals, and legend rows matching screenshot density. | `StatsPanel`, `VisibilityFilter`, and `Legend` are stacked inside a scroll area; labels contain mojibake; role filter is structurally present and uses real `roleVisibilities`. | `src/qml_gui/components/StatsPanel.qml`; `src/qml_gui/components/VisibilityFilter.qml`; `src/qml_gui/components/Legend.qml`; `PreviewViewModel.*` | `GCodeViewer.cpp:296-323`; `GCodeViewer.cpp:4351-4688`; `libvgcode/src/Settings.hpp` | Replace visible panel composition while preserving ViewModel APIs and role-mask semantics. | Panel composition is not screenshot-level; visible labels and grouping need restoration. | Critical | Phase 80, Phase 82 | PVLAYOUT-02, PVLAYOUT-03, PVRENDER-01 | QML audit for row order/labels plus role-toggle unit tests. |
| PV-RIGHT-GCODE | Monospace G-code text panel under the right legend/statistics panel, synchronized to current move. | `PreviewPage.qml` has a `ListView` bound to `previewVm.gcodeLines`; ViewModel tracks `currentGcodeLine`. Labels include mojibake. | `src/qml_gui/pages/PreviewPage.qml`; `PreviewViewModel.*`; `tests/E2EWorkflowTests.cpp` | `GCodeViewer.hpp:134-157`; `GCodeViewer.cpp` GCodeWindow/marker flow | Modify visual density and controls; preserve bounded line-window behavior. | Correct data path exists, but visual row density, header text, and placement need screenshot alignment. | High | Phase 80, Phase 81 | PVLAYOUT-02, PVCTRL-02 | E2E current-line test plus runtime screenshot of highlighted source row. |
| PV-LAYER-RAIL | Far-right vertical layer slider with numeric top/bottom labels and stable thumb/rail geometry. | `PreviewPage.qml` uses a simple vertical `Slider` that sets min=max; richer `LayerSlider.qml` exists but is horizontal/dual-thumb and not used here. | `src/qml_gui/pages/PreviewPage.qml`; `src/qml_gui/components/LayerSlider.qml`; `PreviewViewModel.*` | `GUI_Preview.cpp:423-591`; `IMSlider.*`; `GCodeViewer.hpp:303-337` | Replace simple rail with screenshot-aligned vertical layer control backed by existing ViewModel APIs. | Current rail cannot express full upstream-like range/current-layer UX at target density. | Critical | Phase 81 | PVCTRL-01, PVLAYOUT-01 | ViewModel tests for layer range plus runtime slider interaction evidence. |
| PV-MOVE-BAR | Bottom playback/move rail with compact play/step controls, time labels, and move-position feedback. | `MoveSlider.qml` has play/pause, tool-change band, slider, time labels, and hover time; screenshot bottom rail is lower-profile and more integrated with the viewport. | `src/qml_gui/components/MoveSlider.qml`; `PreviewViewModel.*` | `GCodeViewer.cpp:4717-4718`; `IMSlider.*`; `GCodeViewer.hpp:303-306` | Modify the existing component; add missing step controls only when mapped to upstream move slider semantics. | Playback exists, but visual shape and expected step controls need restoration. | High | Phase 81 | PVCTRL-02, PVLAYOUT-01 | Unit test for move/current line sync plus runtime playback/step evidence. |
| PV-COLOR-MODES | Screenshot-visible view/color mode selector maps to upstream 17 `EViewType` modes or honest blocked states. | `PreviewViewModel::viewModes()` exposes 17 modes; some data-unavailable modes render uniform gradient with one-time log; QML selector uses mojibake title and generic combo. | `PreviewViewModel.*`; `src/qml_gui/pages/PreviewPage.qml`; `src/qml_gui/components/Legend.qml` | `GCodeViewer.cpp:66-103`; `GCodeViewer.cpp:1070-1114`; `libvgcode/include/Types.hpp`; `libvgcode/include/Viewer.hpp` | Preserve 17-mode ordering; improve UI labels/gating and legend behavior. | Semantics are mostly real, but UI must not imply unavailable modes have full upstream data. | High | Phase 82 | PVRENDER-02 | ViewMode unit tests plus QML audit for honest gated labels. |
| PV-STATS | Totals, model/material estimates, time, cost, role/extruder breakdown, and metadata match upstream-like grouping. | `StatsPanel.qml` exposes many stats, but it is a custom card-like layout with mojibake labels and generic toggles. | `StatsPanel.qml`; `PreviewViewModel.*`; `SliceService.*` | `GCodeViewer.cpp:4553-4688`; `GCodeProcessor.hpp`; `PrintEstimatedStatistics` usage | Modify/recompose panel; preserve existing stat properties when source-truth-correct. | Data exists but grouping/density/copy differ from target. | High | Phase 80, Phase 82 | PVLAYOUT-02, PVRENDER-03 | QML audit for required stat rows plus ViewModel fixture checks. |
| PV-INTERACTION | Layer/move/camera/role interactions do not clear Preview payload or desynchronize state. | Debug records show previous unusable controls and orbit-disappears bugs are fixed; tests cover payload survival and role mask shape. | `PreviewPage.qml`; `PreviewViewModel.*`; `RhiViewport.*`; `tests/*` | `GCodeViewer.*`; `IMSlider.*`; `GLCanvas3D.*` | Preserve existing invariants and extend visual/manual coverage. | Automated payload coverage exists; runtime visual drag evidence remains required. | Critical | Phase 81, Phase 83 | PVCTRL-01, PVCTRL-02, PVCTRL-03, PVRENDER-03, PVVERIFY-02 | Targeted tests plus Phase 83 runtime drag checklist. |
| PV-CLEANUP | Replaced Preview paths leave no stale files, imports, resource entries, tests, or disconnected controls. | `SoftwareViewport` still exists in tree as fallback, but tests assert Preview does not reference it; QML files contain mojibake visible strings from historical encoding issues. | `src/qml_gui/pages/PreviewPage.qml`; `src/qml_gui/components/*Preview*`; `src/qml_gui/qml.qrc`; `tests/QmlUiAuditTests.cpp` | Source-truth cleanup rule; `GCodeViewer.*`; `GUI_Preview.*` | Remove stale Preview UI paths only when replaced; keep explicit renderer fallback if still guarded and tested. | Cleanup must distinguish real fallback from stale UI; mojibake must be removed from touched visible UI. | Medium | Phase 83 | PVCLEAN-01, PVVERIFY-01 | Source/QML audit for stale paths, placeholders, and required bindings. |

## Residual Preview Reconciliation

| Historical Item | v4.0 Owner | Status In This Audit |
|---|---|---|
| Phase 54 restored Preview only at major-region level. | Phase 80 | Reopened as screenshot-level layout/panel restoration. |
| Phase 55 deferred visual parity for role visibility UI. | Phase 80, Phase 82 | Reopened as right legend/statistics and role visibility restoration. |
| Drag stability visual UAT was manual-only/deferred. | Phase 81, Phase 83 | Reopened as runtime interaction evidence requirement. |
| Preview orbit disappearance bug. | Phase 81 | Existing fix retained; verify through source audit and runtime evidence. |
| Preview controls previously threw QML TypeError because setters were not invokable. | Phase 81 | Existing fix retained; audit all restored controls for callable backing. |
| D3D12 crash root cause. | Future | Out of scope; v4.0 stays on verified D3D11/auto path. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| PVAUDIT-01 | This matrix maps all Preview regions to source, Qt targets, decisions, owner phases, and verification. |
| PVLAYOUT-01 | PV-TOP, PV-LEFT, PV-VIEWPORT, PV-LAYER-RAIL, PV-MOVE-BAR. |
| PVLAYOUT-02 | PV-RIGHT-LEGEND, PV-RIGHT-GCODE, PV-STATS. |
| PVLAYOUT-03 | PV-TOP, PV-LEFT, PV-RIGHT-LEGEND, PV-RIGHT-GCODE, PV-CLEANUP. |
| PVCTRL-01 | PV-LAYER-RAIL, PV-INTERACTION. |
| PVCTRL-02 | PV-MOVE-BAR, PV-RIGHT-GCODE, PV-INTERACTION. |
| PVCTRL-03 | PV-VIEWPORT, PV-INTERACTION. |
| PVRENDER-01 | PV-RIGHT-LEGEND, PV-COLOR-MODES. |
| PVRENDER-02 | PV-COLOR-MODES, PV-RIGHT-LEGEND. |
| PVRENDER-03 | PV-STATS, PV-INTERACTION. |
| PVCLEAN-01 | PV-CLEANUP. |
| PVVERIFY-01 | PV-CLEANUP plus every row's source/QML audit method. |
| PVVERIFY-02 | Phase 83 final runtime visual evidence and canonical verifier. |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 80 | Restore screenshot-visible Preview layout, left/sidebar reuse, right panel, G-code panel, labels, and placeholder/mojibake cleanup. |
| 81 | Restore layer rail, move/playback controls, keyboard interaction, camera stability, and runtime interaction evidence. |
| 82 | Align role colors, role visibility, color/view modes, legend semantics, and payload preservation. |
| 83 | Remove stale paths, add source/QML audits, run canonical verifier, launch app, and capture final Preview visual evidence. |
