# Milestones History

This file summarizes milestone-level history. Detailed phase evidence stays in `.planning/phases/`, audits, git commits, and verification logs.

## v1.x Series - CrealityPrint Era

Historical migration foundation based on CrealityPrint-era work. These artifacts remain as evidence and were superseded by the active OrcaSlicer/OWzx direction.

### v1.0 - Initial Migration Foundation

**Shipped:**
- CMake + Qt6 + QML application foundation.
- BackendContext composition root and initial viewmodel/service injection.
- Theme tokens and base QML controls.
- GL viewport foundation.

### v1.1 - End-to-End Slicing Workflow

**Shipped:**
- Basic model load to slice to G-code export workflow.
- G-code preview rendering.
- Prepare/Preview workflow shell.
- Notification infrastructure.

### v1.2 - Project Workflow and Preset Foundation

**Shipped:**
- Project save/load foundation.
- Initial upstream preset loading and config inheritance.
- Dialog skeletons for common workflows.

### v1.3 - CLI Port (Partial)

**Shipped:**
- CLI target and argument parsing for headless workflows.
- Partial preset and slicing orchestration.

**Incomplete:**
- Some headless libslic3r contexts remained unstable.

## v2.x Series - OrcaSlicer / OWzx Era

### v2.0 - OrcaSlicer UI Full Restoration

**Shipped:**
- 9-page notebook-style shell and BBLTopbar foundation.
- Prepare/Preview shared Plater-style structure.
- Sidebar and Prepare page section skeletons.
- GL toolbar overlays.
- Initial brand cleanup from legacy UI.

### v2.1 - Slice and Preview Deep Dive

**Shipped:**
- Slice/preview workflow improvements.
- Preset and settings dialog improvements.
- Crash and E2E workflow fixes.

### v2.2 - Page Completion and Cleanup

**Shipped:**
- Broader page/dialog coverage.
- Continued UI parity and cleanup work.

### v2.3 - UI Completion Polish

**Shipped:**
- UI finishing work and i18n infrastructure improvements.
- Additional dialog/page polish.

### v2.4 - Project and Preset Real IO

**Shipped:**
- Project IO improvements.
- Preset IO foundation improvements.

### v2.5 - Real Device Integration

**Shipped:**
- Initial real/hybrid device integration work.
- MQTT and print-send foundations.

### v2.6 - v2.5 Remaining Completion

**Shipped:**
- SSDP discovery work.
- Camera stream work with FFmpeg/RTSP path.
- E2E pipeline and arrange bug fixes.

**Deferred:**
- Full calibration completion.
- Some camera/timelapse and broader integration regressions.

### v2.7 - Slice and Calibration Real Path Follow-up

**Evidence:** git history on `main`.

**Shipped:**
- `fix(slice,v2.7 P0)`: bed_shape and loadFinished wait fixes; slice E2E pipeline works.
- `feat(calib,v2.7 P1)`: Calibration PA/Flow/Temp G-code generation through the slice pipeline.
- `feat(mqtt,v2.7 P2-A)`: real MQTT connect, live telemetry, and access-code UI.
- `feat(mqtt,v2.7 P2-B)`: print control commands through MQTT publish.

**Status:** Landed in code, but planning artifacts were not fully synchronized until v2.9.

### v2.8 - FTP Send Print and Review Fixes

**Evidence:** git history on `main`.

**Shipped:**
- `feat(ftp,v2.8 P2-C)`: send print job via FTPS upload and MQTT print command.
- `fix(review,v2.8)`: critical and warning fixes from code review.

**Status:** Landed in code, but planning artifacts were not fully synchronized until v2.9.

### v2.9 - Implementation Realignment and Stabilization

**Started:** 2026-06-24
**Shipped:** 2026-06-25
**Status:** ✅ Complete
**Phases:** 10-15 (6 phases, 6 plans)
**Git range:** `a34d666..4a1b009` (20 commits, 82 files, +4984/-517)

**Goal:** Reconcile planning with real implementation, stabilize the current dirty baseline, and close the most visible hybrid/placeholder workflows before starting the next large source-truth migration module.

**Phases shipped:**
- Phase 10: Planning Truth Reset — complete
- Phase 11: Source Hygiene Stabilization — complete
- Phase 12: Calibration Closure for Implemented Modes — complete
- Phase 13: Hybrid Integration Verification — complete
- Phase 14: Visible Placeholder Triage — complete
- Phase 15: Verification and Handoff — complete

**Requirements:** 28/28 satisfied (PLAN-01..05, HYGIENE-01..04, CAL-01..05, INT-01..06, UI-01..05, VERIFY-01..03).

**Key accomplishments:**
1. Reconciled all planning entry files with git history through v2.8; v2.7/v2.8 now recorded as landed history, not future scope.
2. Adopted an evidence-based Real/Hybrid/Mock/Blocked/Placeholder service-classification vocabulary across requirements, audits, and planning.
3. Removed targeted source hygiene damage (literal `\r\n` artifacts, mojibake) and `SliceService.cpp.backup`; promoted `AppSettingsService.*` and `SoftwareViewport.*` to tracked implementation.
4. Wired implemented calibration modes (PA/Flow Rate/Temp Tower) to stable topbar ids with deterministic ViewModel regression coverage; marked unimplemented modes explicit Pending/Blocked.
5. Added deterministic protocol-level test coverage for SSDP/MQTT/FTP/camera paths and enforced the SoftwareViewport default + OpenGL-behind-`OWZX_OPENGL` startup contract.
6. Triaired visible placeholder UI so disabled/no-op controls are hidden or honestly classified rather than counted as feature completion.

**Final verification:** Passed on 2026-06-25.

**Evidence:**
- `build/ViewModelSmokeTests.phase15.txt`: **32 passed, 0 failed, 0 skipped, 0 blacklisted**.
- `build/QmlUiAuditTests.phase15.txt`: **7 passed, 0 failed**.
- `scripts/auto_verify_with_vcvars.ps1`: exited 0; QML UI audit passed; E2E pipeline passed.
- `.planning/milestones/v2.9-MILESTONE-AUDIT.md`: 28/28 requirements, 14/14 integration checks, 4/4 E2E flows, 0 orphans.

**Audit status:** `tech_debt` — all requirements satisfied, no critical blockers.

**Known deferred items at close:** 15 (see `.planning/STATE.md` Deferred Items). Includes live MQTT/FTP/RTSP hardware verification, full calibration mode coverage, ModelMall/WebView, AssembleView, and preset bundle completion — all scoped to v3.0/v3.1 or later.

**Handoff:** Recommended next milestone is v3.0 PartPlate and AssembleView.

**Archives:**
- `.planning/milestones/v2.9-ROADMAP.md`
- `.planning/milestones/v2.9-REQUIREMENTS.md`
- `.planning/milestones/v2.9-MILESTONE-AUDIT.md`

---

*Last updated: 2026-06-25 via `/gsd-complete-milestone v2.9`.*

---

### v3.0 - PartPlate Core

**Started:** 2026-06-26
**Shipped:** 2026-06-26
**Status:** ✅ Complete
**Phases:** 16-22 (5 mainline + 2 review-driven)
**Git range:** `v2.9..v3.0` (21 commits, 49 files, +5520/-502)

**Goal:** Replace the mock plate shell (`int plateCount_` + 9 parallel `QList` vectors) with a real PartPlate/PartPlateList domain model that fully round-trips multi-plate state through 3MF and supports upstream-equivalent multi-plate slice scheduling. Fixes all 5 problems from the v2.9 gap analysis.

**Phases shipped:**
- Phase 16: PartPlate Data Model Foundation — complete (2 plans: model+tests, big-bang migration)
- Phase 17: Plate Lifecycle Completion — complete (clone/reorder/printable + QML)
- Phase 18: 3MF Multi-Plate Persistence — complete (store_to_3mf_structure write path)
- Phase 19: Per-Plate Slice Scheduling — complete (config.apply full merge)
- Phase 20: Verification and Handoff — complete
- Phase 21: Review-Driven Bug Fixes — complete (code review: BUG-1/BUG-2 + merge-direction test)
- Phase 22: UI Review-Driven Fixes — complete (failure feedback + audit guards)

**Requirements:** 14/14 satisfied (PLATE-01..14). PLATE-09 partial (round-trip test QSKIP'd, fixture gap). PLATE-11 documented (stack-local Print = per-plate isolation).

**Key accomplishments:**
1. New `src/core/model/` domain layer: PartPlate (instance-level membership, native DynamicPrintConfig, slice state machine) + PartPlateList (single source of truth, MAX_PLATE_COUNT=36).
2. Big-bang migration deleted all 9 parallel vectors from ProjectServiceMock; PartPlateList is the sole plate storage.
3. 3MF multi-plate round-trip: write path populates PlateDataPtrs into StoreParams; load path restores locked/bed-type/sequence/spiral via pendingPlate* staging.
4. Per-plate config fully honored during slicing via `config.apply(*plateCfg)` (replaces 3-hardcoded-key patch).
5. Clone/reorder/printable lifecycle ops work end-to-end with QML context-menu UI.
6. Two review cycles (code + UI): 4 P0/P1 findings all fixed + regression-guarded; QmlUiAuditTests extended to guard Phase 17 wiring.

**Final verification:** Passed on 2026-06-26.

**Evidence:**
- `build/ViewModelSmokeTests`: **44 passed, 0 failed, 1 skipped** (32 baseline + 12 v3.0).
- `build/QmlUiAuditTests`: **8 passed, 0 failed** (7 + 1 plate-wiring guard).
- `scripts/auto_verify_with_vcvars.ps1`: exited 0; QML UI audit + E2E pipeline passed.
- `.planning/milestones/v3.0-MILESTONE-AUDIT.md`: 14/14 requirements, 6/6 integration, 5/5 E2E; status `tech_debt`; review-clean (Phase 21+22).

**Audit status:** `tech_debt` — all requirements satisfied; 1 non-blocking gap (PLATE-09 fixture), documented v3.1-deferred items, review-clean.

**Known deferred items at close:** AssembleView (PLATE-15), m_print_list caching, per-plate wipe-tower geometry, PLATE-09 real-model fixture, multi-plate arrangement/thumbnail/filament-map UI, `.Codex` path casing (v2.9 carry-forward).

**Handoff:** Recommended next milestone was revised after rendering spike evidence: v3.1 is now QRhi High-Performance Prepare/Preview Rendering. AssembleView and preset completion move to v3.2+ behind the rendering foundation.

**Archives:**
- `.planning/milestones/v3.0-ROADMAP.md`
- `.planning/milestones/v3.0-REQUIREMENTS.md`
- `.planning/milestones/v3.0-MILESTONE-AUDIT.md`

---

*Last updated: 2026-06-27 via `$gsd-new-milestone` planning for v3.1.*

---

### v3.1 - QRhi High-Performance Prepare/Preview Rendering

**Started:** 2026-06-27
**Status:** Planning
**Phases:** 23-28 (planned)

**Goal:** Establish the Qt-native high-performance rendering foundation for Prepare and Preview using QRhi with D3D12-first/D3D11 fallback, while keeping upstream-visible behavior aligned and preserving the current stable fallback path.

**Planned phases:**
- Phase 23: QRhi Renderer Foundation And Backend Gate
- Phase 24: Prepare Scene Data And Plate Rendering
- Phase 25: Prepare Model Mesh Rendering And Camera Interaction
- Phase 26: Preview G-Code GPU Pipeline
- Phase 27: Preview Interaction And Performance Gate
- Phase 28: Fallback, Verification, Reviews, And Handoff

**Requirements:** 26 active requirements in `.planning/REQUIREMENTS.md`, all mapped in `.planning/ROADMAP.md`.

**Key decisions:**
1. Use QRhi as the rendering architecture instead of extending the legacy OpenGL/FBO path.
2. Use D3D12-first with D3D11 fallback on Windows for v3.1.
3. Keep QRhi explicitly gated until the milestone proves fallback safety and performance.
4. Treat Vulkan as a future SDK prerequisite because the installed Qt 6.10 SDK has QtGui Vulkan disabled.

**Next step:** `$gsd-discuss-phase 23` or `$gsd-plan-phase 23`.
