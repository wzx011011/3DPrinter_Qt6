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
