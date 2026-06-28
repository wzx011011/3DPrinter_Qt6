# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and G-code export paths, Prepare and Preview renderers, partial preset IO, and hybrid device/camera/network integrations. Milestone v3.1 shipped the Qt-native QRhi rendering foundation with D3D11 as the stable default. Milestone v3.2 shipped multi-plate data polish with documented thumbnail and writer-integration tech debt. Milestone v3.3 proved the slice-to-Preview MVP, but user UAT showed the full local import-to-G-code workflow still needs a complete source-truth pass.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current Milestone: v3.4 Import to G-code Complete Workflow

**Goal:** Complete the full local user workflow from importing a model/project through local G-code export, with source-truth-aligned Prepare readiness, slicing/reslicing, D3D11 QRhi Preview, and export finalization.

**Target features:**
- Complete import and project-restore behavior for locally supported model/project formats exposed by the UI.
- Prepare page readiness, per-plate result state, and slice/Preview/export invalidation after all slice-affecting edits.
- Complete local slicing/reslicing state machine, including current plate, all printable plates, cancellation, failure, and previous G-code reuse.
- Complete Preview data semantics for the local workflow, including view modes, statistics, legend, marker, tick/custom-code data, and stale-state prevention.
- Stable D3D11 QRhi Preview rendering under layer/move/camera interactions without normal-path `SoftwareViewport` fallback.
- Complete local G-code export and finalization for current plate and all printable plates, with safe naming, progress, errors, and output validation.
- End-to-end automated and manual verification for import -> Prepare -> slice -> Preview -> export.

## Requirements

### Validated

These are current baseline capabilities inferred from implementation, git history, and milestone evidence. They remain subject to upstream parity audits when touched.

- Qt6/QML application shell with `BackendContext` as the composition root.
- QML Prepare/Preview/Monitor/Settings-style surfaces with C++ viewmodels and services behind them.
- Real 3MF/STL/OBJ model loading through libslic3r-backed project paths.
- Real slicing and G-code export path through `SliceService` and libslic3r.
- Existing G-code preview data model and rendering path.
- Undo/redo infrastructure for common object operations.
- Project save/load paths with thumbnails and partial multi-plate support.
- v3.0 PartPlate/PartPlateList domain model, plate lifecycle operations, 3MF multi-plate persistence, and per-plate slice scheduling.
- v3.1 QRhi renderer infrastructure, benchmark path, Prepare/Preview integration, and default D3D11 startup path.
- v3.2 plate-grid arrangement, manual filament map, real STL fixture, and partial thumbnail/writer integration hooks.

### Active

- [ ] Import and project restore are complete for the local import-to-G-code workflow.
- [ ] Prepare accurately gates slicing, Preview, and export based on current per-plate validity.
- [ ] Slicing/reslicing and previous-G-code reuse follow upstream local workflow semantics.
- [ ] Preview data and controls are complete enough for source-truth local G-code inspection.
- [ ] D3D11 QRhi Preview remains visible and responsive under real layer/move/camera interaction.
- [ ] Local G-code export finalizes current/all printable plates safely and validates the written files.
- [ ] End-to-end tests and manual UAT cover the complete local workflow.

### Future

- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip (`THUMB-03`).
- Full PLATE-09 save/reload state assertions after shared 3MF writer integration is fixed (`FIXTURE-02` carry-forward).
- AssembleView source-truth completion and multi-plate polish.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Full device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- Full upstream Preview parity outside the local G-code inspection workflow.
- D3D12 crash root cause and Vulkan evaluation after the SDK/runtime path is ready.
- Upstream-compatible preset bundle and CreatePresetsDialog workflows.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond the current basic Qt translation infrastructure.
- WebRTC/MetaRTC camera flows when dependencies and protocols are available.

### Out of Scope

- Changing libslic3r algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- Completing blocked dependency areas such as OpenVDB and WebRTC in v3.4 unless the dependency block is independently resolved.
- Making D3D12 or Vulkan the default backend in v3.4.
- Device send/upload/cloud print and Monitor print-job workflows.
- Full application-wide preset authoring outside the preset/config behavior needed for local import, slicing, Preview, and export correctness.

## Context

- Active upstream source truth: `third_party/OrcaSlicer`.
- Active product brand: OWzx.
- Historical CrealityPrint-era notes remain evidence only; new work must cite OrcaSlicer upstream paths unless the task is explicitly cleaning historical compatibility.
- Several `*Mock` services now contain real production-like paths plus fallback/mock behavior. The name alone does not describe implementation status.
- v2.9 realigned planning with implementation history and verification evidence.
- v3.0 shipped PartPlate Core: phases 16-22, 14/14 requirements satisfied, code/UI review P0/P1 findings fixed, canonical verification passed.
- v3.1 shipped the QRhi rendering path. The current default on Windows is D3D11 QRhi; D3D12 remains explicit opt-in until the crash root cause is understood.
- v3.2 shipped Multi-Plate Data Polish with 8/10 requirements complete and 2 deferred integration gaps (`THUMB-02`, `FIXTURE-02`).
- v3.3 shipped the slice-to-Preview MVP at code/test level, but user UAT exposed Preview disappearing under layer/camera interactions and confirmed the need for a complete local workflow milestone.
- Current Qt SDK reality: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`, so Vulkan is not a v3.4 default backend candidate.
- Known carry-forward tech debt: `.Codex` path casing diverges from git-tracked lowercase `.codex` on Windows; normalize before case-sensitive CI.

## Constraints

- **Tech Stack:** C++17, Qt 6.10, QML, CMake, Ninja, MSVC, Windows 10/11.
- **Build Command:** the only full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Build Directory:** the only build directory is `build/`.
- **Architecture:** durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring.
- **Source Truth:** user-visible behavior must be mapped to OrcaSlicer upstream before being considered complete.
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
| Prioritize complete local import-to-G-code workflow before device/cloud workflows | A trustworthy local G-code output is the prerequisite for meaningful device send and print workflow testing. | Active - v3.4 |

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

*Last updated: 2026-06-28 for v3.4 milestone definition.*
