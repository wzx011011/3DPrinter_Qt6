# OWzx Slicer - OrcaSlicer Qt6/QML Migration

## What This Is

OWzx Slicer is a Windows desktop slicer migrating OrcaSlicer from its upstream C++/wxWidgets GUI to a C++17, Qt 6.10, and QML architecture. The GUI layer is being rewritten while preserving libslic3r and upstream user-visible behavior as the functional source of truth.

The project currently has a usable Qt6/QML shell, real model/project IO, real slicing and G-code export paths, preview rendering, partial preset IO, and hybrid device/camera/network integrations. Milestone v2.9 (shipped 2026-06-25) realigned planning, implementation status, visible UI classifications, and verification evidence. The next recommended work is source-truth gap analysis for PartPlate and AssembleView before starting v3.0.

## Core Value

OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Current State: v2.9 Shipped — Planning Next Milestone

**Last shipped:** v2.9 Implementation Realignment and Stabilization (2026-06-25) — 6 phases, 6 plans, 28/28 requirements satisfied. See `.planning/milestones/v2.9-ROADMAP.md`.

**Next milestone goal:** v3.0 PartPlate and AssembleView — start with a source-truth gap analysis (`$analyzing-source-truth-gap PartPlate and AssembleView`), then run `$gsd-new-milestone`.

**Baseline capabilities validated by v2.9:**
- Planning truth reset: `.planning` now agrees with git history, current code, and verification evidence.
- Source hygiene: encoding damage, literal escape artifacts, residual backup files, and untracked baseline files are classified and handled.
- Service classification: each active service surface is marked Real, Hybrid, Mock, Blocked, or Placeholder.
- Calibration closure for implemented modes: PA, Flow Rate, and Temp Tower are wired, verified, and separated from still-pending calibration modes.
- Hybrid integration verification: MQTT, FTP, SSDP, camera, software viewport, and app settings have deterministic checks or explicit block notes.
- Visible placeholder triage: top-level disabled/no-op UI paths are either wired to real viewmodel/service behavior or documented as deferred/blocked.

## Requirements

### Validated

These are current baseline capabilities inferred from implementation, git history, and the latest canonical verification command. They remain subject to upstream parity audits when touched.

- Qt6/QML application shell with `BackendContext` as the composition root.
- QML Prepare/Preview/Monitor/Settings-style surfaces with C++ viewmodels and services behind them.
- Real 3MF/STL/OBJ model loading through libslic3r-backed project paths.
- Real slicing and G-code export path through `SliceService` and libslic3r.
- G-code preview rendering path.
- Undo/redo infrastructure for common object operations.
- Project save/load paths with thumbnails and partial multi-plate support.
- SSDP discovery, MQTT control, FTP print upload, and camera streaming code paths exist and have Phase 13 deterministic protocol/fallback evidence, but remain Hybrid until live printer/broker/upload/RTSP verification is performed.
- Calibration PA, Flow Rate, and Temp Tower slice dispatch paths are visible and covered by deterministic Phase 12 regression tests, but broader calibration hardware modes remain pending or blocked.

### Active

No implementation phase is currently active. The next recommended planning action is a source-truth gap analysis for PartPlate and AssembleView.

### Future

- PartPlate and AssembleView source-truth completion.
- Upstream-compatible preset bundle and CreatePresetsDialog workflows.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond the current basic Qt translation infrastructure.
- Full calibration mode coverage beyond PA, Flow Rate, and Temp Tower.
- WebRTC/MetaRTC camera flows when dependencies and protocols are available.

### Out of Scope

- Changing libslic3r algorithms as part of GUI migration work.
- Adding product behavior that is not mapped to OrcaSlicer upstream or explicitly documented as an OWzx-only decision.
- Creating alternate build directories or using non-canonical build scripts.
- Completing blocked dependency areas such as OpenVDB and WebRTC in v2.9 unless the dependency block is independently resolved.

## Context

- Active upstream source truth: `third_party/OrcaSlicer`.
- Active product brand: OWzx.
- Historical CrealityPrint-era notes remain evidence only; new work must cite OrcaSlicer upstream paths unless the task is explicitly cleaning historical compatibility.
- Several `*Mock` services now contain real production-like paths plus fallback/mock behavior. The name alone does not describe implementation status.
- Planning before v2.9 overstated or understated several areas: v2.7/v2.8 code landed on `main`, while `.planning` still described v2.6 as the current milestone.
- Current verification evidence: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed on 2026-06-25 after Phase 15, including build, app smoke launch, QML UI audit, CLI/E2E targets, and E2E pipeline tests. `ViewModelSmokeTests.exe` was run explicitly and reported 32 passed, 0 failed; `QmlUiAuditTests.exe` was run explicitly and reported 7 passed, 0 failed.
- v2.9 audit: `.planning/milestones/v2.9-MILESTONE-AUDIT.md` — 28/28 requirements satisfied, 14/14 integration checks, 4/4 E2E flows, 0 orphans, `tech_debt` status (no blockers).
- Known non-blocking tech debt carried into v3.0: `.Codex` (capital C) path references diverge from git-tracked lowercase `.codex` (Windows-safe only); normalize before any case-sensitive CI.

## Constraints

- **Tech Stack:** C++17, Qt 6.10, QML, CMake, Ninja, MSVC, Windows 10/11.
- **Build Command:** the only full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- **Build Directory:** the only build directory is `build/`.
- **Architecture:** durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels; QML is presentation and wiring.
- **Source Truth:** user-visible behavior must be mapped to OrcaSlicer upstream before being considered complete.
- **Dependencies:** CGAL is available; OpenVDB, FFmpeg-related runtime availability, WebRTC/MetaRTC, and closed device protocols must be handled according to the local dependency state and current build rules.
- **Worktree Safety:** unrelated local code changes must not be reverted or cleaned during planning updates.

## Key Decisions

| Decision | Rationale | Outcome |
|---|---|---|
| Use OrcaSlicer as active source truth | The product is an OrcaSlicer Qt6/QML migration, not a free-form slicer redesign. | Good |
| Keep libslic3r as the slicing engine | Rewriting slicing algorithms is outside GUI migration scope and high risk. | Good |
| Classify services as Real/Hybrid/Mock/Blocked/Placeholder | `*Mock` class names no longer reflect implementation reality. | Good — v2.9 shipped; vocabulary adopted across planning |
| Treat visible disabled/no-op UI as migration debt | Broad UI coverage is not the same as source-truth workflow completion. | Good — v2.9 triage shipped (Phase 14) |
| Start the next milestone as v2.9 | Git history already contains v2.7 and v2.8 work; planning must not reuse stale version labels. | Good — v2.9 shipped 2026-06-25 |
| Skip new domain research for v2.9 | This milestone is a local code/planning realignment, not discovery of a new product domain. | Good |
| Default to SoftwareViewport, gate OpenGL behind `OWZX_OPENGL` | Software viewport startup is the safer default; OpenGL path preserved for opt-in. | Good — enforced by QmlUiAuditTests (INT-05) |
| Route calibration by stable ids, not list indexes | List-index routing is fragile across menu reordering. | Good — Phase 12 stable routing shipped |
| Defer PartPlate/AssembleView to v3.0, preset bundle to v3.1 | Large source-truth modules need dedicated milestones after the baseline is trustworthy. | Pending — v3.0/v3.1 scope |

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

*Last updated: 2026-06-25 after v2.9 milestone completion.*
