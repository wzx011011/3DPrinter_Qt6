# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and local G-code export paths, Prepare and Preview renderers, partial preset IO, and hybrid device/camera/network integrations. v3.4 brought the local import-to-G-code workflow to automated verification, but manual UAT is deferred because it cannot be run right now. v3.5 focuses on the next blocker for real daily use: complete printer, filament, and process preset authoring.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current Milestone: v3.5 Preset Authoring Complete Workflow

**Goal:** Complete the source-truth-aligned preset authoring workflow so users can load, select, edit, validate, save, create, import/export, and apply printer, filament, and process presets through the Qt UI, with the resulting configuration feeding Prepare, Slice, Preview, Export, and CLI paths.

**Target features:**
- Real preset bundle and user preset storage for printer, filament, and process presets, including inheritance, built-in/user metadata, read-only state, and persisted selections.
- Source-truth-aligned preset compatibility and validation for printer, filament, and process combinations.
- Complete configuration editing through C++ models and QML surfaces, including dirty state, modified option lists, value-source visibility, reset, and unsaved-change handling.
- Save, Save As, rename, delete, and restore workflows for user presets, with safe name validation and protected system presets.
- CreatePresetsDialog-equivalent workflow for creating printer, filament, and process presets from upstream-compatible inputs.
- Real preset bundle import/export with validation and user-visible failure reporting.
- End-to-end integration proving edited presets affect Prepare readiness, slice invalidation, merged slicing config, generated G-code, and exported output.

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

### Active

- [ ] Preset service uses upstream preset bundle semantics for printer, filament, and process presets.
- [ ] Preset selections, compatibility, and validation are visible and persistent across normal app workflows.
- [ ] Config editing is model-driven, type-aware, dirty-state aware, and free of QML-owned business logic.
- [ ] Save, Save As, rename, delete, reset, diff, and unsaved-change workflows are implemented for user presets.
- [ ] CreatePresetsDialog-equivalent creation and preset bundle import/export workflows are real and user-visible.
- [ ] Edited presets invalidate stale slice results and feed the same merged config into UI slicing, export, project restore, and CLI paths.
- [ ] Automated and manual UAT cover preset authoring through a real slice/export result.

### Future

- Device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- AssembleView source-truth completion.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip (`THUMB-03`).
- Full PLATE-09 save/reload state assertions after shared 3MF writer integration is fixed (`FIXTURE-02` carry-forward).
- Full upstream Preview parity outside the local G-code inspection workflow.
- D3D12 crash root cause and Vulkan evaluation after the SDK/runtime path is ready.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond strings touched by active workflows.
- WebRTC/MetaRTC camera flows when dependencies and protocols are available.

### Out of Scope

- Changing libslic3r slicing algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- Completing device send/upload/cloud print and Monitor print-job workflows in v3.5.
- Completing AssembleView, auto filament-map recommendation, or wipe-tower rendering in v3.5.
- Making D3D12 or Vulkan the default backend in v3.5.
- Treating v3.4 manual UAT as complete without running it.

## Context

- Active upstream source truth: `third_party/OrcaSlicer`.
- Active product brand: OWzx.
- Historical CrealityPrint-era notes remain evidence only; new work must cite OrcaSlicer upstream paths unless the task is explicitly cleaning historical compatibility.
- Several `*Mock` services now contain real production-like paths plus fallback/mock behavior. The name alone does not describe implementation status.
- Current preset-related Qt files include `src/core/services/PresetServiceMock.*`, `src/core/viewmodels/ConfigViewModel.*`, `src/qml_gui/pages/ConfigPage.qml`, `src/qml_gui/panels/PrintSettings.qml`, `src/qml_gui/dialogs/SavePresetDialog.qml`, and `src/qml_gui/dialogs/ExportPresetBundleDialog.qml`.
- Upstream preset source truth includes `third_party/OrcaSlicer/src/libslic3r/Preset.*`, `third_party/OrcaSlicer/src/libslic3r/PresetBundle.*`, `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.*`, `CreatePresetsDialog.*`, `PresetComboBoxes.*`, `ConfigWizard.*`, `ConfigManipulation.*`, and `UnsavedChangesDialog.*`.
- v3.4 Phase 43 manual UAT remains pending because it cannot be run right now. v3.5 planning may proceed, but release/handoff language must keep that fact visible.
- Current Qt SDK reality: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`, so Vulkan is not a default backend candidate.
- Known carry-forward tech debt: `.Codex` path casing diverges from git-tracked lowercase `.codex` on Windows; normalize before case-sensitive CI if touched.

## Constraints

- **Tech Stack:** C++17, Qt 6.10, QML, CMake, Ninja, MSVC, Windows 10/11.
- **Build Command:** the only full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Build Directory:** the only build directory is `build/`.
- **Architecture:** durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring.
- **Source Truth:** user-visible behavior must be mapped to OrcaSlicer upstream before being considered complete.
- **Completeness Rule:** each milestone must implement the complete declared target behavior. If old Qt behavior is simplified, mock, legacy, or semantically wrong for that target, replace it instead of preserving compatibility with the wrong behavior. Temporary fallbacks require explicit status classification, removal conditions, and follow-up ownership.
- **Preset Rule:** preset behavior must be implemented against upstream `PresetBundle`/`PresetCollection` semantics where feasible; simplified JSON/mock behavior must be removed or explicitly classified as fallback.
- **Dependencies:** CGAL is available; OpenVDB, FFmpeg-related runtime availability, WebRTC/MetaRTC, and closed device protocols must be handled according to the local dependency state and current build rules.
- **Rendering Backend:** QRhi/D3D11 is the default high-performance Windows path. D3D12 is explicit opt-in pending root-cause work. Vulkan is future work until a Vulkan-enabled Qt SDK/runtime is available and benchmarked.
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
| Prioritize complete preset authoring for v3.5 | Local G-code can be produced, but real daily use needs trustworthy printer, filament, and process configuration authoring before device workflow work. | Active - v3.5 |
| Prefer replacement over compatibility with wrong legacy Qt behavior | The migration target is complete upstream-aligned behavior, not maintaining simplified interim implementations. | Active rule for all future milestones |
| Keep v3.4 manual UAT visible | Automated verification passed, but user cannot manually verify right now; the project must not claim full manual completion. | Active carry-forward |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition:**
1. Move verified requirements to Validated with phase evidence.
2. Move invalidated or blocked requirements to Future or Out of Scope with a reason.
3. Add new requirements only when implementation evidence or user scope expands.
4. Update service classifications when Real/Hybrid/Mock/Blocked status changes.

**After each milestone:**
1. Review whether the project description still matches the code.
2. Confirm the core source-truth rule still applies.
3. Reconcile `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `INDEX.md`, and `MILESTONES.md`.

---

*Last updated: 2026-06-30 for v3.5 milestone definition.*
