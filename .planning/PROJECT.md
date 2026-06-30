# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and local G-code export paths, Prepare and Preview renderers, partial preset IO, and hybrid device/camera/network integrations. v3.6 pivots from partial page completion to screenshot-driven full restoration of the Prepare, Preview, and parameter settings workflows, because the existing UI has drifted too far from the target OrcaSlicer experience in several visible areas.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current Milestone: v3.6 Screenshot-Driven OrcaSlicer UI Restoration

**Goal:** Restore the Prepare page, Preview page, and parameter settings workflows as complete OrcaSlicer-equivalent user flows, using screenshots as visual/layout truth and OrcaSlicer source as behavior truth.

**Target features:**
- Screenshot-to-source inventory for Prepare, Preview, printer settings, and material settings, with every visible module mapped to Qt targets and upstream behavior.
- OrcaSlicer-like application shell, top navigation, menu actions, page switching, and workflow action states.
- Prepare page restoration: left preset/settings sidebar, object/plate workflows, model import/edit operations, viewport controls, camera/view controls, and gizmo behavior.
- Preview page restoration: G-code viewport, layer slider, move slider, plate thumbnail, left state panel, right legend/statistics panel, G-code text/current-line panel, color modes, and filters.
- Parameter settings restoration: independent printer/material/process settings dialogs with tabs, option groups, typed controls, search, basic/advanced filtering, dirty state, save/reset, compatibility, and validation.
- Deprecated UI removal: replace off-design pages/components when needed and remove abandoned files, routes, registrations, resource entries, imports, and tests.
- End-to-end verification for import -> configure -> prepare -> slice -> preview -> export, including visual screenshot comparison and source-truth behavior checks.

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
- v3.4 local import-to-G-code workflow automated verification has passed; manual UAT is deferred and remains a carry-forward release gate.
- v3.5 Phase 44-46 preset/config foundations exist as historical evidence; v3.5 Phase 47-49 are superseded by v3.6.

### Active

- [ ] Every screenshot-visible Prepare, Preview, printer settings, and material settings module is mapped to an upstream OrcaSlicer behavior source and a Qt target.
- [ ] The application shell, page navigation, menu actions, and workflow action states visually and behaviorally match the screenshot/source-truth contract.
- [ ] Prepare left sidebar, preset controls, object/plate operations, viewport controls, camera controls, and gizmos are restored as complete user workflows.
- [ ] Preview page layout, layer/move controls, color/filter controls, right-side panels, G-code text sync, and renderer interaction remain stable during camera and slider changes.
- [ ] Printer, material, and process settings are restored as independent dialogs/pages with real config option models, save/reset workflows, compatibility, validation, and dirty-state handling.
- [ ] Off-design or obsolete UI is replaced rather than patched when replacement is the cleaner path, and deprecated files/routes/resources/tests are removed.
- [ ] Import -> configure -> prepare -> slice -> preview -> export is verified with automated checks and manual visual/UAT checklists.

### Future

- Device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- AssembleView source-truth completion.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip (`THUMB-03`).
- Full PLATE-09 save/reload state assertions after shared 3MF writer integration is fixed (`FIXTURE-02` carry-forward).
- D3D12 crash root cause and Vulkan evaluation after the SDK/runtime path is ready.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond strings touched by active workflows.
- WebRTC/MetaRTC camera flows when dependencies and protocols are available.

### Out of Scope

- Changing libslic3r slicing algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- Completing device send/upload/cloud print and Monitor print-job workflows in v3.6.
- Completing AssembleView, auto filament-map recommendation, or wipe-tower rendering in v3.6.
- Making D3D12 or Vulkan the default backend in v3.6.
- Treating v3.4 manual UAT as complete without running it.
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
- v3.4 Phase 43 manual UAT remains pending because it could not be run when v3.4 closed. v3.6 planning may proceed, but release/handoff language must keep that fact visible.
- Current Qt SDK reality: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`, so Vulkan is not a default backend candidate.
- Known carry-forward tech debt: `.Codex` path casing diverges from git-tracked lowercase `.codex` on Windows; normalize before case-sensitive CI if touched.

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
- **Dependencies:** CGAL is available; OpenVDB, FFmpeg-related runtime availability, WebRTC/MetaRTC, and closed device protocols must be handled according to the local dependency state and current build rules.
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

*Last updated: 2026-07-01 for v3.6 milestone definition.*
