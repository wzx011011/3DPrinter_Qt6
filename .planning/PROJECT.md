# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and local G-code export paths, screenshot-restored Prepare/Preview/settings workflows, and a default QRhi/D3D11 rendering path that owns gizmo, cut plane, wipe tower, precise picking, and G-code preview rendering. v3.8 retired the legacy OpenGL viewport path; v4.1 completed the parameter settings dialog restoration. Remaining work is now future milestone scope rather than the default renderer or settings foundation.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current State: post-v4.3 (planning next milestone)

**Last shipped milestone:** v4.3 Real Thumbnail Capture And 3MF Round-Trip (2026-07-10).

**v4.3 shipped state (new in this milestone):**
- Real QRhi texture readback thumbnail capture replaces the solid-color stub: offscreen single-sample `QRhiTexture` render-target at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()` + render-thread capture queue mirroring the `synchronize()` pattern + queued `QImage` callback to the GUI thread.
- Both 3MF thumbnail write-side populate sites closed: `PlateData::plate_thumbnail` (per-plate XML `<metadata thumbnail_file="Metadata/plate_N.png">` reference) and `StoreParams::thumbnail_data` (project + per-plate PNG bytes), wired via a file-local `qimageToThumbnailData` helper producing `Format_RGBA8888` bytes symmetric with the read side.
- Save→reload 3MF thumbnail round-trip closed end-to-end with exact RGBA8888 pixel match: a known synthesized thumbnail survives `saveProject` → fresh `loadFile` through the real `bbs_3mf` writer/reader, verified by automated tests asserting lossless byte equality (`thumbnailSaveReloadRoundTrip` + `thumbnailMultiPlateSaveReloadRoundTrip`).
- Read-side `extractPlateThumbnailFrom3mf` helper reads `Metadata/plate_N.png` straight out of the archive via miniz; thumbnails restored AFTER `arrangeObjects` so the rebuild does not wipe them.
- Mock thumbnail generators removed cleanly (no dead paths): real QRhi capture is the sole source; a `plateThumbnailBase64` accessor exposes the persisted `PartPlate::thumbnail()` for the UI plate-card fallback.
- Closed v3.2 THUMB-02/THUMB-03 deferred items; unblocked the shared `store_bbs_3mf` writer (also unblocks FIXTURE-02).

**Carry-forward from v4.2:** Prepare/Preview/settings dialog restoration + AssembleView source-truth restoration (explosion ratio, measurement gizmo, isolated data pool) all shipped. Default QRhi/D3D11 path owns all gizmo/pick/cut/wipe/Preview rendering.

**Carry-forward outside v4.3:** Full GLGizmoMeasure feature-picking engine (needs per-volume ITS + scene raycaster), AssembleViewDataPool ModelObjectsClipper resource, auto filament-map recommendation + wipe-tower, D3D12 root-cause investigation, and CLI fixtures are deferred to future milestones. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from forward scope.

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

## Current Milestone: (none — planning next)

**Status:** v4.3 shipped 2026-07-10. No active milestone. Run `/gsd:new-milestone` to define the next scope.

**Scope rule (carry-forward):** Local/offline only. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed from scope.

## Next Milestone

Not yet defined. Candidate backlog after v4.3:
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Missing CLI fixtures and deterministic argv-based GUI fixture loading for screenshots (FIXTURE-02 — now unblocked by v4.3's shared-writer fix).
- D3D12 crash root cause and Vulkan evaluation after the SDK/runtime path is ready.
- Full GLGizmoMeasure feature-picking engine + AssembleViewDataPool clipper (needs per-volume ITS).

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

### Active

(No active milestone — run `/gsd:new-milestone` to define the next scope.)

### Future

- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Full PLATE-09 save/reload state assertions — now unblocked by v4.3's shared-writer fix (`FIXTURE-02`).
- D3D12 crash root cause and Vulkan evaluation after the SDK/runtime path is ready.
- Full i18n translation coverage beyond strings touched by active workflows.

### Out of Scope

- Changing libslic3r slicing algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows. These are removed from forward scope unless the user explicitly reopens them.
- Completing AssembleView or auto filament-map recommendation before a dedicated source-truth milestone. (AssembleView is now the active v4.2 milestone; auto filament-map remains future.)
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
| Scope v4.3 to real thumbnail capture + 3MF round-trip | v3.2 THUMB-02/THUMB-03 are the longest-running deferred items; both share one root cause (the upstream writer needs real GL pixels). Closing them also unblocks FIXTURE-02. | Active - v4.3 |

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

*Last updated: 2026-07-10 after v4.3 milestone shipped.*
