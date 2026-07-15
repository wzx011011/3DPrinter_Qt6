# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and local G-code export paths, screenshot-restored Prepare/Preview/settings workflows, and a default QRhi/D3D11 rendering path that owns gizmo, cut plane, wipe tower, precise picking, and G-code preview rendering. v3.8 retired the legacy OpenGL viewport path; v4.1 completed the parameter settings dialog restoration. Remaining work is now future milestone scope rather than the default renderer or settings foundation.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current State: post-v4.6 (planning next milestone)

**Last shipped milestone:** v4.6 Core Feature Completion Sweep (Mega-Milestone) (2026-07-15).

**v4.6 shipped state (new in this milestone):**
- Preview TickCode/IMSlider closed loop end-to-end (TICK-01..05): the orphaned LayerSlider tick rendering + right-click add/edit/delete menus consolidated into the vertical PreviewLayerRail (source-truth-aligned with upstream IMSlider); tick CRUD wired into libslic3r `plates_custom_gcodes` (direct field write — BBS removed `set_custom_gcode_per_print_z`) + re-slice on tick edit; all 5 upstream tick types round-trip; drag-to-relocate; explicit TickType→CustomGCode::Type switch map (orders differ, no static_cast).
- Gizmo triangle-paint engine (PAINT-01..05): TriangleSelector ported by REUSE (already compiled in libslic3r, pure C++, zero reimplementation); new PaintEngine (per-volume owner) bridges 3 structural gaps; QRhi colored-facet overlay (reuses m_fillPipeline + rhi_viewport.qsb, zero new shaders) + Software QPainter mirror; brush sphere cursor; Support/Seam/MMU paint feeds the slice via ModelVolume FacetsAnnotation members (not new ModelVolume, not PrintConfig); 3MF persistence automatic.
- Calibration mode completion (CALIB-01..03): 3 tower modes added (Vol_speed=7, VFA=8, Retraction=9) — 6/9 software modes now dispatch via transparent SliceService passthrough; range input UI; real PA K-value readback (4 firmware variants); honest non-PA manual-interpretation notes.
- Tech-debt convergence (CLEAN-01, I18N-01, PROC-01): 4 dead-code pages + AuxiliaryService deleted (removed LAN/device/cloud scope); i18n pipeline documented + zh_CN core strings translated; VALIDATION.md backfilled.
- Cross-workstream regression (REGRESS-01): 12 source-audit slots + v46CrossWorkstreamRegressionLocked gate; canonical build (j6) exit 0; 5/5 ctest PASS.

**Carry-forward from v4.5:** CLI fixtures, D3D12 debug-layer, auto filament-map, Option B wipe-tower mesh, full GLGizmoMeasure engine — all shipped in v4.5, retained.

**Carry-forward outside v4.6:** Dedicated calibration .drc tower models (geometry tech-debt — current-plate model used). Non-en translations are baseline (unfinished, not 100%). ColorChange default color (picker future). Build j6 adaptation (32GB machine MSVC heap constraint). MEASURE-06 Assembly-mode transformation actions. D3D12 default-backend promotion. Full PLATE-09 save/reload state assertions. Runtime visual evidence still relies on argv fixtures (Windows capture API workaround). LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from forward scope.

## Previous State: v4.3 Real Thumbnail Capture And 3MF Round-Trip (shipped 2026-07-10)

**Shipped state:**
- Real QRhi texture readback thumbnail capture replaces the solid-color stub.
- Both 3MF thumbnail write-side populate sites closed: `PlateData::plate_thumbnail` + `StoreParams::thumbnail_data`.
- Save→reload 3MF thumbnail round-trip closed end-to-end with exact RGBA8888 pixel match.
- Read-side `extractPlateThumbnailFrom3mf` helper + ordering fix (thumbnails restored AFTER arrangeObjects).
- Mock thumbnail generators removed cleanly; `plateThumbnailBase64` accessor for the UI fallback.
- Closed v3.2 THUMB-02/THUMB-03; unblocked the shared `store_bbs_3mf` writer (also unblocks FIXTURE-02).

## Previous State: v4.2 AssembleView Source-Truth Restoration (shipped 2026-07-09)

**Shipped state:**
- Prepare page visual/source-truth gap inventory exists as the canonical v3.9 region map.
- Prepare left preset/settings sidebar is restored for upstream-like density, display names, scope state, and no visible unavailable placeholders.
- Prepare object list, plate strip, view controls, vertical toolbar, slice status, and workflow controls are restored to the screenshot layout.
- RHI-backed viewport controls and gizmo floating panels are integrated into the restored Prepare page without overlap or dead controls.
- Preview page visual/source-truth gap inventory exists as the canonical v4.0 region map.
- Preview layout, layer/move controls, playback, role visibility, color-mode availability, statistics/legend surfaces, and Preview navigation are restored and audited.
- Parameter settings dialogs (printer, material, process) are restored to the screenshot/source-truth shell, compact preset/action row, clean titles/tabs, typed option sections, dirty/read-only/value-source/validation states, and preset save/save-as/reset/discard/unsaved-close semantics.
- AssembleView (assembly view) is restored as the third canvas host (`CanvasAssembleView = 2`) on the default RHI/D3D11 path, with explosion-ratio separation rendering, yellow dashed connector guide lines, the Assembly measurement gizmo (`Ctrl+Y`, `ONLY_ASSEMBLY` mode), an isolated AssembleView data pool, and Plater `CanvasAssembleView` routing branches.
- Startup deep links (`--open-page`, repeated `--open-dialog`, `--skip-first-run`, `--load-model`) support deterministic visual inspection without simulated clicks.
- Final verification passed through source/QML audits, canonical build, running application, and recorded Prepare/Preview/settings/AssembleView visual evidence.

**Carry-forward from v3.8:** The default QRhi/D3D11 path owns move, rotate, scale, cut plane, wipe tower, precise object picking, and Preview G-code rendering. Legacy `GLViewport*` / `GCodeRenderer*` files and the `OWZX_OPENGL` startup path are retired. The QML `OWzxGL.GLViewport` name remains as a compatibility alias backed by RHI or Software rendering.

**Carry-forward outside v4.2:** D3D12 remains explicit opt-in future investigation. Full GLGizmoMeasure feature-picking engine (needs per-volume ITS + scene raycaster) and the AssembleViewDataPool ModelObjectsClipper resource are deferred to a future milestone. LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows are removed from forward scope unless the user explicitly reopens them.

## Previous State: v4.5 Backlog Closure (Mega-Milestone) (shipped 2026-07-13)

**Shipped state:**
- CLI fixtures + argv GUI fixture loading (`--load-model`/`--open-page`/`--open-dialog`) closed FIXTURE-01..04; the recurring Windows-capture-API runtime-evidence blocker is worked around via a one-shot `QQuickWindow::frameSwapped` gate so external screenshot capture is deterministic.
- D3D12 debug-layer wired behind `OWZX_D3D12_DEBUG` env flag (D3D12-01); time-boxed crash root-cause investigation documented the 0xc0000005 access violation as a non-reproducing hypothesis + tooling gap (D3D12-02/03). D3D12 stays opt-in.
- Auto filament-map recommendation: Qt6 enum widened from 2-value to upstream 4-value (fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault) with a 3MF read-side migration; post-slice `Print::get_filament_maps()` readback captured by value in the SliceService worker, delivered via `filamentMapReady`, surfaced in the `FilamentGroupPopup` UI; save→reload round-trip verified (FMAP-01..04).
- Option B real wipe-tower mesh: convex hull of merged `real_wipe_tower_mesh` + `real_brim_mesh` captured by value in the SliceService worker and rendered as a triangle set when `wipe_tower_mesh_data` is populated; Option A dimensioned box preserved as the else-branch fallback (re-opens Phase 99 Frozen Decision 2; WTMESH-01..04).
- Full GLGizmoMeasure engine: per-volume indexed_triangle_set accessor with shallow-share ownership contract; pure-CPU `MeshRaycaster` + thin Qt6 `SceneRaycaster` port reusing libslic3r AABBMesh BVH (two-stage pick + per-volume cache); per-volume `Measure::Measuring` instantiated and wired to the raycaster hit, producing real angle/direct/perpendicular/XYZ measurements; GLGizmoMeasure snap UX wired end-to-end (MEASURE-01..05).

## Current Milestone: v4.6 Core Feature Completion Sweep (Mega-Milestone)

**Goal:** In one cycle, lift the four highest-value main-flow gaps from "skeleton-level" to "end-to-end usable": close the Preview TickCode/IMSlider read-back-and-write loop, add the Gizmo triangle-paint engine (Support/Seam/MMU), complete the software-sliceable Calibration mode set, and converge process/quality tech debt (i18n, missing VALIDATION.md, legacy dead-code pages).

**Scope rule:** All four workstreams are local/offline (no printer hardware, no network). Preview TickCode (WS1) and the Gizmo paint engine (WS2) are both cross-cutting engine work that reuse shared per-volume ITS / raycaster infrastructure built in v4.5. Calibration (WS3) limits scope to software-sliceable modes — modes requiring live printer hardware (ManualLeveling/BedLeveling/Vibration) stay out of scope under the existing "printer-hardware workflows removed" rule. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

**Target features (4 workstreams, ~16-20 phases starting from 117):**
- **Preview TickCode/IMSlider closed loop (WS1):** surface the existing-but-orphaned `LayerSlider.qml` (tick rendering + right-click add/edit/delete menu) into `PreviewPage`; wire `PreviewViewModel` tick CRUD into libslic3r `Model::set_custom_gcode_per_print_z` and re-slice on tick edit, closing the upstream Preview core differentiation (color change / pause / custom G-code at a layer). Currently `custom_gcode_per_print_z` has zero references in the tree; the read-side parse already exists.
- **Gizmo triangle-paint engine (WS2):** port the upstream `TriangleSelector` triangle-pick + subdivide + paint-state pipeline; add colored-facet overlay rendering on the QRhi/D3D11 path; wire Support-paint / Seam-paint / MMU-segmentation (the three paint gizmos whose enums/buttons/panels already exist but have no pick/render/execute). Hollow/FaceDetector/SlaSupports stay future — they share this engine and unlock after WS2.
- **Calibration mode completion (WS3):** add the software-sliceable CalibModes that libslic3r supports but Qt6 does not yet dispatch (MaxVolumetricSpeed/VolumetricRate/RetractionTune/FlowRateProxy) into the existing `SliceService::setCalibParams` → `print.set_calib_params` path; add range (start/end/step) input UI to `CalibrationDialog`; replace the mock K-value writeback with real result readback where libslic3r provides it. Hardware-dependent modes (ManualLeveling/BedLeveling/Vibration) stay out of scope.
- **Tech-debt convergence (WS4):** i18n translation coverage (zh_CN/ja/ko/de/fr are ~0% translated); fill missing Nyquist VALIDATION.md for previously-shipped phases; remove/repair legacy dead-code pages (DeviceListPage force-empty bug, AuxiliaryPage semantic mismatch, etc.).

## Next Milestone

v4.6 is the active milestone (core feature completion). After v4.6, the candidate backlog will be re-evaluated based on what ships and what new gaps emerge.

## Requirements

### Validated

These are current baseline capabilities inferred from implementation, git history, and milestone evidence. They remain subject to upstream parity audits when touched.

- Qt6/QML application shell with `BackendContext` as the composition root.
- QML Prepare/Preview/Monitor/Settings-style surfaces with C++ viewmodels and services behind them.
- Real 3MF/STL/OBJ model loading through libslic3r-backed project paths.
- Real slicing and local G-code export path through `SliceService` and libslic3r.
- Existing G-code preview data model and D3D11 QRhi rendering path.
- Undo/redo infrastructure for common object operations.
- Project save/load paths with thumbnails and partial multi-plate support.
- v3.0 PartPlate/PartPlateList domain model, plate lifecycle operations, 3MF multi-plate persistence, and per-plate slice scheduling.
- v3.1 QRhi renderer infrastructure, benchmark path, Prepare/Preview integration, and default D3D11 startup path.
- v3.2 plate-grid arrangement, manual filament map, real STL fixture, and partial thumbnail/writer integration hooks.
- v3.4 local import-to-G-code workflow is closed by automated E2E coverage and current runtime launch evidence; do not describe it as a separate manual user click-through.
- v3.5 Phase 44-46 preset/config foundations exist as historical evidence; v3.5 Phase 47-49 are superseded by v3.6.

- v3.8 RHI gizmo math, geometry, state wiring, move/rotate/scale interaction, cut plane, wipe tower, precise picking, and legacy OpenGL retirement shipped with 21/21 requirements satisfied.
- Default renderer foundation now rests on QRhi/D3D11 plus Software fallback; the old OpenGL viewport is no longer a selectable application path.
- v3.9 Prepare page UI restoration shipped with 12/12 requirements satisfied, canonical verification passed, current runtime launch evidence, and final Prepare screenshot evidence.
- v4.0 Preview page UI restoration shipped with 13/13 requirements satisfied, canonical verification passed, current runtime launch evidence, and final Preview screenshot evidence.
- v4.1 Parameter settings dialogs source-truth restoration shipped with 14/14 requirements satisfied, milestone audit passed, canonical verifier passing, runtime settings visual evidence recorded, and startup deep links for deterministic visual inspection.
- v4.2 AssembleView source-truth restoration shipped with 12/12 requirements satisfied, milestone audit passed (5/5 integration chains, 3/3 E2E flows), canonical build clean, and Prepare/Preview regression-free across all phases.
- v4.3 Real Thumbnail Capture And 3MF Round-Trip shipped with 12/12 requirements satisfied, milestone audit tech_debt status (no critical blockers), real QRhi readback capture replacing the solid-color stub, 3MF write side closed, save→reload round-trip verified with exact RGBA8888 byte match (single-plate + multi-plate), mock generators removed cleanly, and the shared `store_bbs_3mf` writer unblocked.
- v4.4 Wipe-Tower Geometry Readback And Real Rendering shipped with 8/8 requirements satisfied, milestone audit tech_debt status (no critical blockers), post-slice wipe-tower geometry readback wired end-to-end (Print::wipe_tower_data() captured by value in SliceService worker, has_wipe_tower() gate enforced, EditorViewModel Q_PROPERTYs + PreparePage.qml bindings), Option A dimensioned-box rendering LOCKED as v4.4 baseline, SoftwareViewport QPainter box closes the fallback-path rendering gap, Phase 100 REVIEW W1 corner→center fix, and 8 WT-* region anchors locked by Phase 102 regression test.
- v4.5 Backlog Closure shipped with 20/20 active requirements satisfied across 5 workstreams (CLI fixtures FIXTURE-01..04; D3D12 debug-layer + time-boxed root-cause D3D12-01..03; auto filament-map recommendation FMAP-01..04 with 4-value enum + 3MF migration + readback + popup + round-trip; Option B real wipe-tower mesh WTMESH-01..04 with convex-hull rendering + Option A fallback; full GLGizmoMeasure engine MEASURE-01..05 with per-volume ITS + MeshRaycaster/SceneRaycaster + Measure::Measuring + snap UX), milestone verified by canonical build + regression ctest + launch liveness.

### Active

- [ ] Preview TickCode/IMSlider closed loop: surface orphaned LayerSlider.qml into PreviewPage + wire tick CRUD into libslic3r `custom_gcode_per_print_z` + re-slice on tick edit (WS1).
- [ ] Gizmo triangle-paint engine: port upstream TriangleSelector (pick + subdivide + paint-state) + colored-facet overlay on QRhi/D3D11 + wire Support/Seam/MMU paint gizmos (WS2).
- [ ] Calibration mode completion: add software-sliceable CalibModes (MaxVolumetricSpeed/VolumetricRate/RetractionTune/FlowRateProxy) + range input UI + real K-value readback (WS3).
- [ ] Tech-debt convergence: i18n translation coverage (non-en) + missing Nyquist VALIDATION.md + legacy dead-code page removal (WS4).

### Future

- Full PLATE-09 save/reload state assertions — unblocked by v4.3's shared-writer fix (`FIXTURE-02`); may be partially addressed by v4.5 workstream 3.
- Hollow / FaceDetector / SlaSupports gizmos — share the WS2 TriangleSelector engine; unlock after WS2 ships.
- MEASURE-06 Assembly-mode transformation actions (deferred from v4.5; needs stable feature-picking foundation, now shipped).
- D3D12 default-backend promotion (deferred from v4.5; needs confirmed root cause + clean repro on the original machine).

### Out of Scope

- Changing libslic3r slicing algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows. These are removed from forward scope unless the user explicitly reopens them.
- Hardware-dependent Calibration modes (ManualLeveling / BedLeveling / Vibration) — require live printer hardware and stay out of scope under the existing printer-hardware removal rule unless the user explicitly reopens them.
- Making D3D12 or Vulkan the default backend before the backend crash/runtime constraints are resolved.
- Claiming separate manual user click-through for v3.4 Phase 43. It is closed by E2E/runtime evidence, not by a distinct manual session.
- Resuming v3.5 Phase 47-49 unless the user explicitly reopens that milestone.

## Context

- Active upstream source truth: `third_party/OrcaSlicer`.
- Screenshot visual truth directory: `shotScreen/`.
- Active product brand: OWzx.
- Historical CrealityPrint-era notes remain evidence only; new work must cite OrcaSlicer upstream paths unless the task is explicitly cleaning historical compatibility.
- Current screenshot inputs:
  - `shotScreen/准备页.png`
  - `shotScreen/预览页.png`
  - `shotScreen/打印机参数设置页.png`
  - `shotScreen/材料参数设置页.png`
- Prepare source-truth candidates include `third_party/OrcaSlicer/src/slic3r/GUI/Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`, `GUI_ObjectSettings.*`, and `Gizmos/*`.
- Preview source-truth candidates include `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`, `GCodeViewer.*`, `GLCanvas3D.*`, and `third_party/OrcaSlicer/src/libslic3r/GCode/*`.
- Settings source-truth candidates include `third_party/OrcaSlicer/src/slic3r/GUI/Tab.*`, `PresetComboBoxes.*`, `ConfigManipulation.*`, `UnsavedChangesDialog.*`, `CreatePresetsDialog.*`, and `third_party/OrcaSlicer/src/libslic3r/PrintConfig.*`, `Preset.*`, `PresetBundle.*`.
- Current Qt candidates include `src/qml_gui/main.qml`, `src/qml_gui/pages/PreparePage.qml`, `PreviewPage.qml`, `ConfigPage.qml`, `SettingsPage.qml`, sidebar/panel components, `src/core/viewmodels/EditorViewModel.*`, `PreviewViewModel.*`, `ConfigViewModel.*`, `src/core/services/ProjectServiceMock.*`, `PresetServiceMock.*`, and QRhi renderer classes.
- Several `*Mock` services now contain real production-like paths plus fallback/mock behavior. The name alone does not describe implementation status.
- v3.4 Phase 43 is closed by canonical E2E coverage plus current runtime launch evidence. Do not describe it as a separate manual user click-through.
- Current Qt SDK reality: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`, so Vulkan is not a default backend candidate.
- Known carry-forward tech debt: `.Codex` path casing diverges from git-tracked lowercase `.codex` on Windows; normalize before case-sensitive CI if touched.
- v3.8 closure state: RHI is the default functional renderer for gizmo/pick/cut/wipe scope; Phase 68 still lacks optional manual visual-capture evidence, tracked as tech debt rather than a blocker.
- v3.9 restored the Prepare page and archived its phase evidence under `.planning/milestones/v3.9-phases/`.
- v4.1 restored the parameter settings dialogs (printer/material/process) to screenshot/source-truth parity and shipped on 2026-07-09. Prepare and Preview were dependency/regression surfaces, not the v4.1 implementation target.

## Constraints

- **Tech Stack:** C++17, Qt 6.10, QML, CMake, Ninja, MSVC, Windows 10/11.
- **Build Command:** the only full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Build Directory:** the only build directory is `build/`.
- **Architecture:** durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring.
- **Source Truth:** user-visible behavior must be mapped to OrcaSlicer upstream before being considered complete.
- **Screenshot Truth:** screenshot-driven milestones use `shotScreen/` as the visual/layout truth. A visible control is incomplete until the upstream behavior source, Qt target, and verification path are recorded.
- **Completeness Rule:** each milestone must implement the complete declared target behavior. If old Qt behavior is simplified, mock, legacy, or semantically wrong for that target, replace it instead of preserving compatibility with the wrong behavior.
- **No Deprecated UI Rule:** when a page/component is replaced, remove the old files, routes, registrations, resource entries, imports, tests, and disconnected code paths in the same milestone.
- **Preset Rule:** preset behavior must be implemented against upstream `PresetBundle`/`PresetCollection` semantics where feasible; simplified JSON/mock behavior must be removed or explicitly classified as fallback.
- **Dependencies:** CGAL is available. OpenVDB remains unavailable. FFmpeg/WebRTC/closed device protocol work is not forward product scope unless explicitly reopened.
- **Rendering Backend:** QRhi/D3D11 is the default high-performance Windows path. D3D12 is explicit opt-in pending root-cause work. Vulkan is future work until a Vulkan-enabled Qt SDK/runtime is available and benchmarked.
- **Comments and Encoding:** new or modified source comments must be English and ASCII-only; preserve UTF-8 without BOM.
- **Worktree Safety:** unrelated local code changes must not be reverted or cleaned during planning updates.

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| Use OrcaSlicer as active source truth | The product is an OrcaSlicer Qt6/QML migration, not a free-form slicer redesign. | Good |
| Keep libslic3r as the slicing engine | Rewriting slicing algorithms is outside GUI migration scope and high risk. | Good |
| Classify services as Real/Hybrid/Mock/Blocked/Placeholder | `*Mock` class names no longer reflect implementation reality. | Good - v2.9 vocabulary adopted |
| Use QRhi as the high-performance rendering architecture | `QQuickFramebufferObject` is OpenGL-only; QRhi supports modern GPU backends and keeps rendering inside Qt. | Good - v3.1 shipped |
| Default to D3D11 QRhi on Windows | D3D11 initializes reliably in the local Qt 6.10 runtime and avoids the known D3D12 startup crash. | Good - default changed after v3.2 audit |
| Keep D3D12 explicit opt-in | D3D12 has demonstrated crashes in the app path; it remains available only for focused debugging. | Open tech debt |
| Defer Vulkan default evaluation | Current Qt SDK disables public Vulkan support, so Vulkan cannot be the known-good default backend yet. | Future |
| Supersede v3.5 Phase 47-49 | The user wants screenshot/source-truth full UI restoration now; preset lifecycle work must be folded into settings restoration where relevant. | Active - v3.6 |
| Use screenshots as visual truth for v3.6 | The user supplied target screenshots and rejected the current UI design quality. | Active - v3.6 |
| Prefer replacement over compatibility with wrong legacy Qt behavior | The migration target is complete upstream-aligned behavior, not maintaining simplified interim implementations. | Active rule for all future milestones |
| Remove deprecated UI when replacing pages | The user explicitly wants no abandoned/dead UI code left in the project. | Active rule for all future milestones |
| Keep comments English and ASCII-only | Avoid recurrent Windows encoding/mojibake failures and keep source comments tool-friendly. | Active rule for all future milestones |
| Keep v3.4 manual UAT visible | Automated verification passed, but user could not manually verify then; the project must not claim full manual completion. | Active carry-forward |
| Retire legacy OpenGL viewport after RHI parity | Keeping two interactive renderers after RHI parity would preserve wrong fallback behavior and increase regression risk. | Good - v3.8 shipped |
| Preserve `OWzxGL.GLViewport` as a QML compatibility alias | QML imports stay stable while the implementation resolves to RHI or Software rendering. | Good - v3.8 shipped |
| Put gizmo math, geometry, and object picking in pure C++ helpers | Deterministic unit tests are cheaper and more reliable than renderer-only validation for interaction math. | Good - v3.8 shipped |
| Scope v3.9 to Prepare page UI restoration | The user explicitly selected "准备页 UI 还原"; Preview, settings, device, and AssembleView should not dilute this milestone. | Good - v3.9 shipped |
| Scope v4.0 to Preview page UI restoration | After Prepare shipped, the next highest-value screenshot/source-truth gap is Preview; device, settings, and AssembleView stay future unless directly required. | Good - v4.0 shipped |
| Remove LAN/device/network/cloud work from forward scope | User direction on 2026-07-07: LAN devices and networking are no longer done. | Active scope rule |
| Scope v4.1 to parameter settings dialogs | Settings has real Phase 56 backend semantics but target screenshots still expose visual/text/layout debt. | Good - v4.1 shipped |
| Add startup deep-link arguments for settings/dialogs/models | Direct SettingsDialog window capture was blocked by the Windows capture API; argv-based deep links let future visual evidence open pages/dialogs and load models without simulated clicks. | Good - v4.1 shipped |
| Scope v4.2 to AssembleView source-truth restoration | After Prepare/Preview/settings shipped, AssembleView is the last screenshot-level UI surface; Arrange (auto-arrangement) is already complete and distinct from the assembly canvas. | Good - v4.2 shipped |
| Scope v4.3 to real thumbnail capture + 3MF round-trip | v3.2 THUMB-02/THUMB-03 are the longest-running deferred items; both share one root cause (the upstream writer needs real GL pixels). Closing them also unblocks FIXTURE-02. | Good - v4.3 shipped |
| Scope v4.4 to wipe-tower geometry readback only | Exploration showed auto filament-map + wipe-tower is too large for one milestone and the two are loosely coupled. Wipe-tower readback is smaller, higher-ROI (renderer is 90% done), and avoids touching the slice architecture / per-plate-Print gap. Auto filament-map deferred to a future milestone. | Good - v4.4 shipped |
| Scope v4.5 to backlog closure (5 workstreams) | The deferred backlog (filament-map, Option B mesh, CLI fixtures, D3D12 root cause, GLGizmoMeasure engine) is long-standing; clearing it in one cycle unblocks downstream work and removes the recurring evidence/tooling gaps. | Good - v4.5 shipped |
| Scope v4.6 to core feature completion (4 workstreams) | A code-truth gap audit found the next four highest-value gaps all sit at "skeleton-level" with existing scaffolding: Preview TickCode loop (orphaned LayerSlider + zero `custom_gcode_per_print_z` refs), Gizmo triangle-paint engine (TriangleSelector pick/render missing despite Support/Seam/MMU enums/buttons/panels existing), Calibration (3/9 software-sliceable modes), and i18n/dead-code tech debt. Lifting all four in one cycle raises main-flow completeness from skeleton to end-to-end usable. | Active - v4.6 |
| Keep Hollow/FaceDetector/SlaSupports out of v4.6 | These gizmos share the WS2 TriangleSelector engine; adding them dilutes scope before the paint engine foundation is stable. They unlock naturally as a follow-up once WS2 ships. | Active - v4.6 |
| Keep hardware-dependent Calibration modes out of v4.6 | ManualLeveling/BedLeveling/Vibration require live printer hardware and fall under the existing printer-hardware removal rule; only software-sliceable libslic3r modes are in scope. | Active - v4.6 |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition:**
1. Move verified requirements to Validated with phase evidence.
2. Move invalidated or blocked requirements to Future or Out of Scope with a reason.
3. Add new requirements only when implementation evidence or user scope expands.
4. Update service classifications when Real/Hybrid/Mock/Blocked status changes.
5. Re-check whether replaced pages left old files, routes, registrations, resource entries, imports, or tests behind.

**After each milestone:**
1. Review whether the project description still matches the code.
2. Confirm the core source-truth rule still applies.
3. Reconcile `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `INDEX.md`, and `MILESTONES.md`.

---

*Last updated: 2026-07-15 after v4.6 milestone completion + archive.*
