# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and G-code export paths, preview rendering, partial preset IO, and hybrid device/camera/network integrations. Milestone v3.0 shipped the PartPlate core migration. The active v3.1 milestone establishes a QRhi high-performance rendering foundation for Prepare and Preview before continuing AssembleView and preset completion work.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current Milestone: v3.1 QRhi High-Performance Prepare/Preview Rendering

**Goal:** Establish the Qt-native high-performance rendering foundation for Prepare and Preview using QRhi with D3D12-first/D3D11 fallback, while keeping upstream-visible behavior aligned and preserving the current stable fallback path.

**Target features:**
- Gated QRhi renderer infrastructure, shader build pipeline, backend selection, and benchmark evidence.
- Prepare QRhi rendering for bed/plate, loaded model meshes, plate switching, camera interaction, selection, and hover feedback.
- Preview QRhi rendering for G-code segment buffers, layer-range scrubbing, color modes, visibility toggles, playback, and synchronized legend/statistics.
- Performance instrumentation and verification gates proving GPU-resident data paths and fallback safety.
- Vulkan documented as a future SDK prerequisite, not a v3.1 blocker, because the installed Qt 6.10 SDK has QtGui Vulkan disabled.

## Requirements

### Validated

These are current baseline capabilities inferred from implementation, git history, and the latest milestone evidence. They remain subject to upstream parity audits when touched.

- Qt6/QML application shell with `BackendContext` as the composition root.
- QML Prepare/Preview/Monitor/Settings-style surfaces with C++ viewmodels and services behind them.
- Real 3MF/STL/OBJ model loading through libslic3r-backed project paths.
- Real slicing and G-code export path through `SliceService` and libslic3r.
- Existing G-code preview rendering path.
- Undo/redo infrastructure for common object operations.
- Project save/load paths with thumbnails and partial multi-plate support.
- v3.0 PartPlate/PartPlateList domain model, plate lifecycle operations, 3MF multi-plate persistence, and per-plate slice scheduling.
- SSDP discovery, MQTT control, FTP print upload, and camera streaming code paths exist with deterministic protocol/fallback evidence, but remain Hybrid until live printer/broker/upload/RTSP verification is performed.
- Calibration PA, Flow Rate, and Temp Tower slice dispatch paths are visible and covered by deterministic regression tests; broader calibration hardware modes remain pending or blocked.

### Active

- [ ] Gated QRhi renderer infrastructure with D3D12-first/D3D11 fallback and stable default viewport unchanged.
- [ ] Prepare QRhi rendering for bed/plate, model meshes, camera interaction, selection, and plate switching.
- [ ] Preview QRhi rendering for G-code segment buffers, layer-range draw control, color modes, toggles, playback, and legend/statistics sync.
- [ ] Performance and verification gates for GPU-resident data, benchmark evidence, fallback safety, code review, and UI review.

### Future

- AssembleView source-truth completion and multi-plate polish after the QRhi rendering foundation is stable.
- Upstream-compatible preset bundle and CreatePresetsDialog workflows.
- Vulkan backend evaluation after replacing or rebuilding Qt 6.10 with public QtGui Vulkan support enabled.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond the current basic Qt translation infrastructure.
- Full calibration mode coverage beyond PA, Flow Rate, and Temp Tower.
- WebRTC/MetaRTC camera flows when dependencies and protocols are available.

### Out of Scope

- Changing libslic3r algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- Completing blocked dependency areas such as OpenVDB and WebRTC in v3.1 unless the dependency block is independently resolved.
- Promoting Vulkan as default backend with the current Qt SDK.

## Context

- Active upstream source truth: `third_party/OrcaSlicer`.
- Active product brand: OWzx.
- Historical CrealityPrint-era notes remain evidence only; new work must cite OrcaSlicer upstream paths unless the task is explicitly cleaning historical compatibility.
- Several `*Mock` services now contain real production-like paths plus fallback/mock behavior. The name alone does not describe implementation status.
- v2.9 realigned planning with implementation history and verification evidence.
- v3.0 shipped PartPlate Core: phases 16-22, 14/14 requirements satisfied, code/UI review P0/P1 findings fixed, canonical verification passed.
- Rendering spike evidence: `.planning/spikes/001-rendering-performance-architecture/README.md` selected QRhi as the high-performance architecture, and `.planning/spikes/002-render-bench-qrhi-backend/README.md` validated the current Windows backend reality.
- Current Qt SDK reality: `E:/Qt6.10/lib/cmake/Qt6Gui/Qt6GuiTargets.cmake` lists `vulkan` under `QT_DISABLED_PUBLIC_FEATURES`, so Vulkan cannot be a valid v3.1 backend until the Qt SDK changes.
- Known carry-forward tech debt: `.Codex` path casing diverges from git-tracked lowercase `.codex` on Windows; normalize before case-sensitive CI.

## Constraints

- **Tech Stack:** C++17, Qt 6.10, QML, CMake, Ninja, MSVC, Windows 10/11.
- **Build Command:** the only full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Build Directory:** the only build directory is `build/`.
- **Architecture:** durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring.
- **Source Truth:** user-visible behavior must be mapped to OrcaSlicer upstream before being considered complete.
- **Dependencies:** CGAL is available; OpenVDB, FFmpeg-related runtime availability, WebRTC/MetaRTC, and closed device protocols must be handled according to the local dependency state and current build rules.
- **Rendering Backend:** v3.1 performance path uses QRhi with D3D12 first and D3D11 fallback on Windows; Vulkan is future work until a Vulkan-enabled Qt SDK is available and benchmarked.
- **Worktree Safety:** unrelated local code changes must not be reverted or cleaned during planning updates.

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| Use OrcaSlicer as active source truth | The product is an OrcaSlicer Qt6/QML migration, not a free-form slicer redesign. | Good |
| Keep libslic3r as the slicing engine | Rewriting slicing algorithms is outside GUI migration scope and high risk. | Good |
| Classify services as Real/Hybrid/Mock/Blocked/Placeholder | `*Mock` class names no longer reflect implementation reality. | Good - v2.9 vocabulary adopted |
| Default to SoftwareViewport, gate OpenGL behind `OWZX_OPENGL` | Software viewport startup is the safer default; OpenGL path preserved for opt-in. | Good - enforced by QmlUiAuditTests |
| Use QRhi as the high-performance rendering architecture | `QQuickFramebufferObject` is OpenGL-only; QRhi supports modern GPU backends and keeps rendering inside Qt. | Pending - v3.1 implementation |
| Use D3D12-first/D3D11 fallback for v3.1 on Windows | Current Qt SDK disables Vulkan, while QRhi D3D12/D3D11 benchmark initializes and renders successfully. | Pending - v3.1 implementation |
| Keep QRhi behind an explicit gate until validated | Stable default startup must not regress while the new renderer is built and verified. | Pending - v3.1 implementation |
| Defer AssembleView/preset completion behind the rendering foundation | Prepare/Preview rendering performance is foundational for AssembleView and large Preview workloads. | Pending - v3.2+ scope |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition**:
1. Move verified requirements to Validated with phase evidence.
2. Move invalidated or blocked requirements to Future or Out of Scope with a reason.
3. Add new requirements only when implementation evidence or user scope expands.
4. Update service classifications when Real/Hybrid/Mock/Blocked status changes.

**After each milestone**:
1. Review whether the project description still matches the code.
2. Confirm the core source-truth rule still applies.
3. Reconcile `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `INDEX.md`, and `MILESTONES.md`.

---

*Last updated: 2026-06-27 after v3.1 milestone definition.*
