# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- ✅ **v4.5** Backlog Closure — Phases 103-116 (shipped 2026-07-13)
- 🚧 **v4.6** Core Feature Completion Sweep — Phases 117-128 (in progress)

## Current Milestone: v4.6 Core Feature Completion Sweep (Mega-Milestone)

**Goal:** In one cycle, lift the four highest-value main-flow gaps from "skeleton-level" to "end-to-end usable": close the Preview TickCode/IMSlider read-back-and-write loop, add the Gizmo triangle-paint engine (Support/Seam/MMU), complete the software-sliceable Calibration mode set, and converge process/quality tech debt (i18n, missing VALIDATION.md, legacy dead-code pages).

**Scope rule:** All four workstreams are local/offline (no printer hardware, no network). WS1 (TickCode) and WS2 (paint engine) are cross-cutting engine work that reuse the v4.5 per-volume ITS + SceneRaycaster infrastructure. WS3 (Calibration) limits scope to software-sliceable libslic3r modes — modes requiring live printer hardware (ManualLeveling/BedLeveling/Vibration) stay out of scope under the existing printer-hardware removal rule. WS4 is quality/polish convergence. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

**Target features (4 workstreams, 17 requirements across 12 phases, 117-128):**
- **WS1 — Preview TickCode/IMSlider closed loop (TICK-01..05):** surface the existing-but-orphaned `LayerSlider.qml` (tick render + right-click add/edit/delete menu) into `PreviewPage`; wire `PreviewViewModel` tick CRUD into libslic3r `Model::set_custom_gcode_per_print_z` (currently zero refs in the tree) and re-slice on tick edit, closing the upstream Preview core differentiation (color change / pause / custom G-code / filament change / template at a layer). Read-side parse of sliced G-code already exists.
- **WS2 — Gizmo triangle-paint engine (PAINT-01..05):** port the upstream `TriangleSelector` triangle-pick + adaptive-subdivide + paint-state pipeline (reusing v4.5 per-volume ITS + SceneRaycaster); add colored-facet overlay rendering on the QRhi/D3D11 path (Software QPainter mirrors); wire brush interaction (radius sphere/circle + size + paint/erase/smart-fill); then wire Support/Seam paint (enforcer/blocker + seam painting) and MMU segmentation end-to-end so painted facets feed the slice as modifiers. Hollow/FaceDetector/SlaSupports stay future — they share this engine and unlock after WS2.
- **WS3 — Calibration mode completion (CALIB-01..03):** add the 4 software-sliceable CalibModes that libslic3r supports but Qt6 does not yet dispatch (MaxVolumetricSpeed, VolumetricRate, RetractionTune, FlowRateProxy) into the existing real `SliceService::setCalibParams` → `print.set_calib_params` path (raising coverage from 3/9 to 7/9 software modes); add range (start/end/step) input UI to `CalibrationDialog`; replace the mock K-value writeback with real result readback where libslic3r provides it. Hardware-dependent modes (ManualLeveling/BedLeveling/Vibration) stay out of scope.
- **WS4 — Tech-debt convergence (I18N-01, PROC-01, CLEAN-01):** i18n translation coverage (zh_CN/ja/ko/de/fr are ~0% translated); fill missing Nyquist VALIDATION.md for previously-shipped phases; remove/repair legacy dead-code pages (DeviceListPage force-empty bug, AuxiliaryPage semantic mismatch, ConfigPage redundancy).

**Current state (from code-truth gap audit during `/gsd:new-milestone`):**
- WS1: `LayerSlider.qml` is functionally complete (tick render + right-click add/edit/delete menu, `LayerSlider.qml:81-124,531-624`) but **orphaned — never instantiated**. `PreviewPage` renders `PreviewLayerRail.qml` (a simple range slider, **no ticks**). `custom_gcode_per_print_z` has **zero references** in the tree; `PreviewViewModel` tick CRUD (`addPauseAtLayer`/`removeTickAtLayer`/`editCustomGcodeAtLayer` at `PreviewViewModel.cpp:1778-1873`) mutates memory only and is **silently lost on re-slice**. Read-side parse (`PreviewViewModel.cpp:993-1021`) exists.
- WS2: Support/Seam/MMU have `GizmoMode` enum values + buttons + Q_PROPERTY panels (`RhiViewport.h:93-119`), but **no pick / no render / no execute**. `setTriangleSupportState` (`EditorViewModel.cpp:1012`) takes a triangle index that **no QML/C++ code ever computes**. Zero `renderSupport`/`renderSeam`/`renderMmu` functions. `SupportPaintTypes.h` data layer + `BucketFill`/`SmartFill`/`GapFill` enums exist; TriangleSelector pipeline absent.
- WS3: Real libslic3r slice path exists for 3/9 modes (PA/FlowRate/TempTower via `SliceService.cpp:567-579`). MaxVolumetricSpeed has UI but `mode=0` (no slice, `CalibrationServiceMock.cpp:173`). 4 more software-sliceable modes absent. Range params hardcoded (`CalibrationServiceMock.cpp:55-57`); K-value writeback is mock (`:514` `0.04f + item*0.01`).
- WS4: i18n non-en translation ~0% (`未移植功能盘点.md:75-78`); multiple shipped phases missing Nyquist VALIDATION.md (process debt); legacy dead-code pages persist (DeviceListPage force-empty bug, AuxiliaryPage semantic mismatch, ConfigPage redundancy — `差距盘点_全局.md:34-37`).

## Phases

- [x] Phase 117: IMSlider Integration And Tick Rendering (WS1) (completed 2026-07-15)
- [x] Phase 118: custom_gcode_per_print_z Writeback And Re-Slice Loop (WS1) (completed 2026-07-15)
- [ ] Phase 119: Tick Type Coverage And Drag Relocation (WS1)
- [ ] Phase 120: TriangleSelector Engine Port (WS2)
- [ ] Phase 121: Painted-Facet Overlay Render And Brush Interaction (WS2)
- [ ] Phase 122: Support And Seam Paint End-To-End (WS2)
- [ ] Phase 123: MMU Segmentation Paint End-To-End (WS2)
- [ ] Phase 124: Software-Sliceable Calibration Mode Completion (WS3)
- [ ] Phase 125: Calibration Range Input UI And Real K-Value Readback (WS3)
- [ ] Phase 126: Legacy Dead-Code Page Cleanup (WS4)
- [ ] Phase 127: i18n Translation Coverage And VALIDATION.md Backfill (WS4)
- [ ] Phase 128: v4.6 Verification And Cross-Workstream Regression

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 117 | IMSlider Integration And Tick Rendering | Surface the orphaned LayerSlider into PreviewPage so users see tick marks on the slider | TICK-01 |
| 118 | custom_gcode_per_print_z Writeback And Re-Slice Loop | Close the zero-reference gap: tick edits write back to libslic3r and trigger re-slice | TICK-02, TICK-03 |
| 119 | Tick Type Coverage And Drag Relocation | All 5 upstream tick types round-trip; ticks are drag-relocatable | TICK-04, TICK-05 |
| 120 | TriangleSelector Engine Port | Port the pick + subdivide + paint-state foundation reused by all paint gizmos | PAINT-01 |
| 121 | Painted-Facet Overlay Render And Brush Interaction | QRhi/D3D11 colored-facet overlay + Software mirror + brush radius/size/fill interaction | PAINT-02, PAINT-03 |
| 122 | Support And Seam Paint End-To-End | Support enforcer/blocker + seam paint feed the slice as modifiers | PAINT-04 |
| 123 | MMU Segmentation Paint End-To-End | Painted regions assigned to extruders feed the multi-material slice | PAINT-05 |
| 124 | Software-Sliceable Calibration Mode Completion | Wire the 4 missing software-sliceable modes into the real Calib_Params path | CALIB-01 |
| 125 | Calibration Range Input UI And Real K-Value Readback | User-set ranges + real K-value readback replace hardcoded/mock values | CALIB-02, CALIB-03 |
| 126 | Legacy Dead-Code Page Cleanup | Repair/remove DeviceListPage, AuxiliaryPage, ConfigPage with full cleanup | CLEAN-01 |
| 127 | i18n Translation Coverage And VALIDATION.md Backfill | Non-en translations + fill missing Nyquist validation files | I18N-01, PROC-01 |
| 128 | v4.6 Verification And Cross-Workstream Regression | Canonical build + regression ctest + Prepare/Preview regression-free | REGRESS-01 |

### Build Order (parallelism guidance for the executor)

Phase numbers are sequential, but several phases are parallel-safe and may be executed concurrently:

- **Wave A (parallel, no deps):** Phase 117 (WS1 slider integration — UI-only, no slice path change). Phase 120 (WS2 TriangleSelector engine — reuses v4.5 ITS/raycaster, no WS1 dep). Phase 124 (WS3 mode completion — touches `SliceService` but independent of WS1/WS2). Phase 126 (WS4 dead-code cleanup — independent; run early so later phases don't depend on removed pages).
- **Wave B (after 117):** Phase 118 (WS1 writeback — needs the slider surface from 117). Phase 121 (WS2 overlay/brush — needs TriangleSelector from 120).
- **Wave C (after 118):** Phase 119 (WS1 type coverage + drag — needs the writeback loop from 118). Phase 122 (WS2 Support/Seam — needs overlay/brush from 121). Phase 125 (WS3 range/K-value — needs the modes from 124).
- **Wave D (after 122):** Phase 123 (WS2 MMU — needs the paint pipeline from 122; MMU is the most complex paint gizmo, ships last in WS2).
- **Wave E (parallel, independent):** Phase 127 (WS4 i18n + VALIDATION.md — independent, can run any time after 126).
- **Wave F (last):** Phase 128 (verification) — needs all feature phases (117-127).

### Phase 117: IMSlider Integration And Tick Rendering

**Status:** Not started
**Workstream:** WS1 (Preview TickCode)

Success criteria:
1. `PreviewPage` instantiates the functionally-complete `LayerSlider.qml` (or its integrated successor) instead of the tick-less `PreviewLayerRail.qml`, so the right-side layer rail and/or bottom slider renders tick marks driven by `previewVm.tickMarks`.
2. Tick marks are color-coded by type (PausePrint=orange, ToolChange=blue, ColorChange=green, CustomGcode=teal) and appear at the correct layer positions for a sliced G-code that already contains `;...COLOR_CHANGE` / `PAUSE_PRINT` / `CUSTOM_GCODE` / `MANUAL_TOOL_CHANGE` comments (the read-side parse at `PreviewViewModel.cpp:993-1021` already feeds `tickMarks_`).
3. Right-click on the slider track's empty area shows the Add menu (Add Pause / Add Custom G-code...), and right-click on an existing tick shows the Edit/Delete menu — both already implemented in `LayerSlider.qml:531-624` and now reachable in the running Preview.
4. The orphaned-component state is removed: either `LayerSlider.qml` is wired in directly, or its functionality is consolidated into the in-use slider with no dead duplicate left behind (No-Deprecated-UI rule).

### Phase 118: custom_gcode_per_print_z Writeback And Re-Slice Loop

**Status:** Not started
**Workstream:** WS1 (Preview TickCode)

Success criteria:
1. When the user adds/removes/edits a tick, `PreviewViewModel`/`ProjectServiceMock` calls libslic3r `Model::set_custom_gcode_per_print_z` with the corresponding `CustomGCode::Item` vector — closing the gap that `custom_gcode_per_print_z` currently has zero references in the tree (verified by `grep -rn custom_gcode_per_print_z src/`).
2. A tick edit triggers a re-slice (reusing the existing `SliceService` slice path), and the resulting G-code actually contains the user's markers at the chosen layer (Pause → `M0`/`PAUSE_PRINT` comment; ColorChange → `T`/`COLOR_CHANGE` with new color; CustomGcode → the user-entered text).
3. The writeback is data-driven and gated: when the slice has no objects or the tick layer is out of range, no invalid `CustomGCode::Item` is emitted (mirrors the v4.4/v4.5 valid-gate pattern).
4. A round-trip assertion exists: add a pause tick at layer N → re-slice → re-parse the new G-code → the tick reappears at layer N (proving write→slice→parse is closed, not just write→memory).

### Phase 119: Tick Type Coverage And Drag Relocation

**Status:** Not started
**Workstream:** WS1 (Preview TickCode)

Success criteria:
1. All 5 upstream tick types round-trip through add → writeback → re-slice → re-parse: PausePrint, CustomGcode, ToolChange (filament change), ColorChange, Template (the `TickCodeTypes.h:7-24` enum is already complete; this phase proves each type is wired, not just the first two).
2. The user can drag an existing tick along the slider to a new layer and the relocated tick persists through re-slice (adds the missing `moveTick`/relocate path on `PreviewViewModel`; upstream IMSlider supports tick drag).
3. ColorChange tick supports per-extruder color selection (the dialog picks a target extruder + new color); ToolChange supports target-extruder selection — matching upstream `CustomGcodeDialog` semantics.
4. Template type is documented as the upstream "save current state as template" anchor; if its full UI is out of this milestone's reach, it is at least round-tripped at the data level (writeback + re-parse) and its UI gap is logged, not silently dropped.

### Phase 120: TriangleSelector Engine Port

**Status:** Not started
**Workstream:** WS2 (Gizmo Paint Engine)

Success criteria:
1. A `TriangleSelector`-style C++ component is ported (or adapted from upstream `TriangleSelector.cpp`) that resolves a ray-hit (from v4.5 `SceneRaycaster`) to a specific triangle index on the selected object's mesh, with adaptive subdivision so painting is not limited to the original facet resolution.
2. The selector holds per-triangle paint-state (None/Enforcer/Blocker for support; per-extruder index for MMU; painted/unpainted for seam) using the existing `SupportPaintTypes.h` data structures, extended as needed.
3. A pure-C++ unit test exercises pick → triangle index → set-state → get-state without requiring the renderer (mirrors the v4.5 Measure engine's unit-testable boundary).
4. No business logic leaks into QML: the selector exposes `Q_INVOKABLE paintAt(hit, brushMode, value)` / `clearPainted()` to QML; the ray-hit-to-triangle mapping and subdivision stay in C++.

### Phase 121: Painted-Facet Overlay Render And Brush Interaction

**Status:** Not started
**Workstream:** WS2 (Gizmo Paint Engine)

Success criteria:
1. The QRhi/D3D11 renderer draws a colored-facet overlay for painted triangles (support enforcer=red, blocker=blue, seam=highlight, MMU=per-extruder color) — a new `renderPaintOverlay`-style function on `RhiViewportRenderer` (the renderer currently has zero `renderSupport`/`renderSeam`/`renderMmu`).
2. The Software fallback path mirrors the overlay with QPainter (symmetric with the v4.4 wipe-tower QPainter mirror).
3. Brush interaction works: mouse-move shows a brush-radius sphere/circle cursor on the mesh; brush size is adjustable (slider/wheel); paint/erase toggles; smart-fill (bucket) fills a connected painted region in one click.
4. Painting performance is acceptable on a typical model (subdivision is bounded; no per-frame full-mesh re-tessellation — reuse the v4.5 per-frame pick perf lesson).

### Phase 122: Support And Seam Paint End-To-End

**Status:** Not started
**Workstream:** WS2 (Gizmo Paint Engine)

Success criteria:
1. GLGizmoSupportPainter works end-to-end: the user paints support enforcer (red) and blocker (blue) facets via the brush; the painted facets become a `ModelVolume` of type `SupportEnforcer`/`SupportBlocker` (upstream `ModelVolumeType`) that feeds the slice as a support modifier.
2. The generated supports respect the painted enforcer/blocker regions (a blocker region suppresses auto-support there; an enforcer region forces support).
3. GLGizmoSeamPainter works end-to-end: the user paints seam facets; the painted region influences the seam placement in the sliced G-code.
4. The existing TODO stubs are resolved: `clearSeamPaintOnSelection()` (`EditorViewModel.cpp:1058`) and the support clear path are implemented (no longer `TODO: Implement actual clear logic`).

### Phase 123: MMU Segmentation Paint End-To-End

**Status:** Not started
**Workstream:** WS2 (Gizmo Paint Engine)

Success criteria:
1. GLGizmoMmuSegmentation works end-to-end: the user paints mesh regions with per-extruder colors (one color per loaded filament/extruder), and the segmentation feeds the multi-material slice so each painted region is printed by its assigned extruder.
2. The existing TODO stub is resolved: `clearMmuSegmentation()` (`EditorViewModel.cpp:1229`) is implemented (no longer `TODO: Clear per-triangle MMU facet data...` returning false).
3. Multi-material-only behavior is honest: when the project has a single extruder, the MMU gizmo is either disabled or surfaces an honest "requires multi-material" reason (no silent no-op).
4. The segmentation survives save→reload (painted regions persist in 3MF) where upstream persists them.

### Phase 124: Software-Sliceable Calibration Mode Completion

**Status:** Not started
**Workstream:** WS3 (Calibration)

Success criteria:
1. The 4 software-sliceable libslic3r Calibration modes that Qt6 does not yet dispatch (MaxVolumetricSpeed, VolumetricRate, RetractionTune, FlowRateProxy) are wired into `CalibrationServiceMock` → `SliceService::setCalibParams` → `print.set_calib_params` and produce real calibration G-code (raising coverage from 3/9 to 7/9 software modes; the 2 hardware modes stay out of scope).
2. MaxVolumetricSpeed's current `mode=0` (UI present but no slice, `CalibrationServiceMock.cpp:173`) is replaced with the real Calib mode mapping so it actually slices.
3. Each new mode's G-code is upstream-aligned (matches the corresponding `Calib_*` branch in libslic3r `GCode::do_export`), verified by a deterministic slice test that produces non-empty calibration output.
4. The `Pending: outside Phase 12` mock markers are removed for the 4 in-scope modes (replaced with real dispatch); the 2 hardware modes keep an honest "requires live printer hardware" reason.

### Phase 125: Calibration Range Input UI And Real K-Value Readback

**Status:** Not started
**Workstream:** WS3 (Calibration)

Success criteria:
1. `CalibrationDialog` (currently a progress-only dialog) gains range input fields (start / end / step) so the user controls the calibration sweep, replacing the hardcoded ranges in `CalibrationServiceMock.cpp:55-57`.
2. The user-set ranges flow into `setCalibParams(calibMode, start, end, step, printNumbers)` and reach libslic3r's `Calib_Params`.
3. Calibration result (K-value / flow rate / etc.) is read back from the real slice output and written into the active preset where libslic3r provides it, replacing the mock K-value writeback (`0.04f + item*0.01` at `CalibrationServiceMock.cpp:514`).
4. Where libslic3r does not provide a machine-readable result, the limitation is documented honestly (no fabricated readback); the user is told how to read the calibration print manually (upstream behavior).

### Phase 126: Legacy Dead-Code Page Cleanup

**Status:** Not started
**Workstream:** WS4 (Tech-Debt Convergence)

Success criteria:
1. The DeviceListPage force-empty bug is fixed (the page currently force-writes empty device data — `差距盘点_全局.md:36`); either the bug is repaired or the page is removed if device-list is confirmed out of forward scope.
2. The AuxiliaryPage semantic mismatch is reconciled: the UI shows analysis-tool content but the service is file-copy (`差距盘点_全局.md:37`); either the service is aligned to the UI's analysis semantics or the page is removed honestly.
3. ConfigPage redundancy is resolved (it was superseded by SettingsPage per `差距盘点_全局.md:37` — it does not exist on disk now, confirmed); any residual registrations/imports/routes are removed per the No-Deprecated-UI rule.
4. No dead page is left half-removed: old files, routes, registrations, resource entries, imports, and tests are cleaned up in the same phase.

### Phase 127: i18n Translation Coverage And VALIDATION.md Backfill

**Status:** Not started
**Workstream:** WS4 (Tech-Debt Convergence)

Success criteria:
1. Non-en translation coverage (zh_CN / ja / ko / de / fr, currently ~0%) is filled for the strings touched by the v4.6 workstreams (TickCode dialog strings, paint gizmo panel strings, calibration range labels) and the main-flow UI surfaces; the translation pipeline (`lupdate`/`lrelease`) produces non-empty `.qm` files for each language.
2. At least one non-en language (zh_CN recommended) is brought to high coverage as the proof-of-pipeline; the others are filled to a documented baseline with a clear remaining-work estimate (honest scoping rather than claiming full coverage of all 5).
3. Nyquist VALIDATION.md files are produced for the v4.6 phases (117-127) and the gap is closed for previously-shipped phases missing them (process debt carry-forward from v4.4/v4.5).
4. The translation workflow is documented so future phases keep `.ts` files updated (prevents the coverage from silently rotting back to ~0%).

### Phase 128: v4.6 Verification And Cross-Workstream Regression

**Status:** Not started
**Workstream:** Cross-Cutting (Verification)

Success criteria:
1. The canonical build (`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`) exits 0 with all WS1-WS4 changes integrated.
2. Regression ctest passes (existing tests + new v4.6 regression tests for TickCode round-trip, TriangleSelector pick, calibration mode dispatch, dead-page removal) — no Prepare/Preview/settings/AssembleView behavior regressed.
3. The cross-workstream interactions are exercised end-to-end: (a) WS1 TickCode re-slice does not break WS2 paint modifiers or WS3 calibration slices; (b) WS2 paint modifiers survive a WS1 re-slice; (c) WS3 calibration modes still slice after WS1/WS2 land.
4. A source-audit regression slot locks the v4.6 region anchors (TickCode writeback wired, TriangleSelector present, 7/9 software calibration modes dispatch, dead pages gone) so the milestone contracts are protected against future drift (mirrors the v4.5 Phase 116 regression-lock pattern).

---

*v4.6 roadmap created: 2026-07-14 — 12 phases (117-128), 17 requirements, 100% mapped.*
