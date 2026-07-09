# Milestones History

## v4.2 AssembleView Source-Truth Restoration (Shipped: 2026-07-09)

**Phases completed:** 5 phases, 5 plans
**Audit:** passed; 12/12 requirements satisfied, 5/5 cross-phase integration chains wired, 3/3 E2E flows reachable, Prepare/Preview regression-free.
**Known deferred items at close:** full GLGizmoMeasure feature-picking engine (needs per-volume ITS + scene raycaster), AssembleViewDataPool ModelObjectsClipper resource (needs ITS), runtime visual evidence capture-blocked (Windows capture API; SETVERIFY-02 precedent), no Nyquist VALIDATION.md files (process debt).

**Key accomplishments:**

- Created the v4.2 AssembleView source-truth gap matrix mapping 11 ASM-* regions across 3 screenshots to OrcaSlicer source anchors, Qt targets, replacement decisions, and Phase 90-93 ownership (Phase 89).
- Replaced the Plater.qml AssembleView placeholder with a real canvas host: CanvasAssembleView=2 enum in RhiViewport, AssemblePage.qml 4-region shell, BBLTopbar navigation toggle, and Plater CanvasAssembleView routing branches mirroring upstream Plater.cpp conditionals — Prepare/Preview untouched (Phase 90).
- Added explosionRatio Q_PROPERTY + slider + per-volume radial separation rendering + yellow dashed connector guide lines on the default RHI/D3D11 path, with ProjectServiceMock per-volume blob restructure regression-gated by PrepareSceneDataTests (Phase 91).
- Ported the Assembly measurement gizmo (Ctrl+Y, GLGizmoAssembly/ONLY_ASSEMBLY): GizmoAssemblyMeasure=19 enum, activability gating (AssembleView + explosion≈1.0 + ≥2 volumes), AssemblyMeasureGeometry C++ helper, overlay (dashed line + arrowheads + teal value box), and right-side 测量 panel (Phase 92).
- Added AssembleViewDataPool C++ helper (ModelObjectsInfo cache) isolated from Prepare/Preview by the activeCanvasType==2 gate, consolidated milestone audit slots, placeholder-absent regression lock, canonical build clean, and runtime evidence (Phase 93).

**Git range:** `v4.1..v4.2` (55 commits, 52 files, +9374/-209)

**Scope simplifications (documented):** measurement engine uses AABB-center distance + longest-axis angle (full per-triangle feature picking deferred); ModelObjectsClipper pool resource deferred (enum slot reserved).

**Archives:**

- `.planning/milestones/v4.2-ROADMAP.md`
- `.planning/milestones/v4.2-REQUIREMENTS.md`
- `.planning/milestones/v4.2-MILESTONE-AUDIT.md`

---

## v4.1 Parameter Settings Dialogs Source-Truth Restoration (Shipped: 2026-07-09)

**Phases completed:** 5 phases, 5 plans
**Audit:** passed; 14/14 requirements satisfied, 5/5 Nyquist compliant, canonical verifier passed, runtime settings visual evidence captured.
**Known deferred items at close:** none blocking — direct automated SettingsDialog window capture was blocked by the Windows capture API; SETVERIFY-02 accepts manual click-through plus runtime evidence, and startup deep links (`--open-page`, `--open-dialog`, `--load-model`) were added to support future deterministic visual evidence.

**Key accomplishments:**

- Created the v4.1 settings source-truth gap matrix tying screenshot-visible printer/material/process settings regions to OrcaSlicer source anchors, Qt targets, replacement decisions, owner phases, and verification methods (Phase 84).
- Restored the settings dialog shell: independent 736x593 non-modal windows, compact preset/action row, clean titles/tabs, removed the off-design left group sidebar, and no mojibake or raw labels (Phase 85).
- Restored typed option sections: compact section headers/dividers/icons, checkbox/numeric-unit/enum/text-color/range controls all routed through existing `optionModel.setValue`, dirty/read-only/value-source/validation states without row overlap (Phase 86).
- Wired the dirty pending preset guard (`pendingUnsavedChangesRequested` → `UnsavedChangesDialog`) distinguishing close-window flows from preset-switch flows, and preserved read-only Save As handling (Phase 87).
- Locked final verification: normalized settings QML resources, final QML audits for region mapping/text/layout/option bindings/upstream anchors, canonical verifier pass, app launch, and runtime settings visual evidence (Phase 88).
- Added extensible startup deep-link arguments (`--open-page`, repeated `--open-dialog`, `--skip-first-run`, `--load-model`) so future visual inspection can open pages/dialogs and load models without simulated clicks.
- Pre-close fix restored Prepare topbar/settings-entry/viewport layout (topbar title controls constrained to 36px, settings entries routed to `forwardSettingsRequest`, lower-left view controls moved above the plate bar) with regression-guarding QmlUiAuditTests.

**Git range:** `349ea7a..a218972` (20 commits, 71 files, +5294/-522)

**Archives:**

- `.planning/milestones/v4.1-ROADMAP.md`
- `.planning/milestones/v4.1-REQUIREMENTS.md`
- `.planning/milestones/v4.1-MILESTONE-AUDIT.md`

---

## v4.0 Preview Page UI Restoration (Shipped: 2026-07-07)

**Phases completed:** 5 phases, 5 plans, 0 tasks
**Audit:** complete; 13/13 requirements satisfied, canonical verifier passed, runtime Preview visual evidence captured.
**Known deferred items at close:** loaded-G-code runtime screenshot still needs deterministic GUI fixture loading; D3D12 remains future opt-in investigation.

**Key accomplishments:**

- Created the v4.0 Preview source-truth gap matrix tying screenshot-visible Preview regions to OrcaSlicer source anchors, Qt targets, owner phases, and verification methods.
- Restored Preview layout density for top controls, viewport, side panels, layer slider, bottom move/playback controls, statistics, metadata, and legend surfaces.
- Restored layer range/current layer controls, move stepping, playback, and Preview camera fit/orbit stability without losing payload state.
- Surfaced G-code role colors, role visibility, and Preview color-mode availability with honest gating for blocked modes.
- Replaced the workflow tab delegate with a real accessible `Button`, fixing runtime Preview navigation by UI automation.
- Passed the canonical verifier, launched `OWzxSlicer.exe`, invoked the Preview tab at runtime, and captured final Preview visual evidence.

---

## v3.9 Prepare Page UI Restoration (Shipped: 2026-07-06)

**Phases completed:** 5 phases, 5 plans, 0 tasks
**Audit:** tech_debt; 12/12 requirements satisfied, with process/visual evidence debt only.
**Known deferred items at close:** no blocking open artifacts; Nyquist validation files are missing for Phases 74-78, and final visual evidence uses the default Prepare startup screen.

**Key accomplishments:**

- Created the v3.9 Prepare source-truth gap matrix tying screenshot-visible regions to OrcaSlicer source anchors, Qt targets, owner phases, and verification methods.
- Restored the Prepare left sidebar density, preset/filament rows, display labels, scope controls, search, settings entry points, and dirty-state surfaces.
- Restored compact object/volume list rows, plate strip cards, and honest slice/cancel/export readiness surfaces.
- Repositioned Prepare viewport controls, vertical gizmo tools, and move/rotate/scale floating panels into screenshot-aligned RHI-backed UI.
- Rewired the disconnected slice/progress/export path and removed startup QML warnings.
- Passed the canonical verifier, launched `OWzxSlicer.exe`, and captured final Prepare runtime visual evidence.

---

## v3.8 RHI Gizmo Parity (Shipped: 2026-07-04)

**Phases completed:** 9 phases, 8 plans, 0 tasks
**Audit:** tech_debt; 21/21 requirements satisfied, with one deferred visual-evidence item.
**Known deferred items at close:** 8 (see `.planning/STATE.md` Deferred Items)

**Key accomplishments:**

- Extracted gizmo math and geometry into pure, unit-tested C++ helpers shared by the rendering paths.
- Wired the default RHI viewport state pipeline for gizmo mode, cut plane state, selected-object center, and update-triggered synchronization.
- Ported move, rotate, scale, cut plane, and wipe tower rendering to the default D3D11 QRhi path.
- Added RHI gizmo pick/drag interaction for move, rotate, and scale with ViewModel transform application and undo coalescing.
- Replaced default-path object selection with precise ray-AABB plus ray-triangle picking.
- Deleted the legacy OpenGL viewport and G-code renderer path, removed `OWZX_OPENGL`, and kept `OWzxGL.GLViewport` as a stable RHI/Software QML compatibility alias.
- Passed the canonical verifier after Phase 73: app build, QML UI audit, PreviewParser, app launch smoke, and E2E pipeline.

---

## v3.6 Screenshot-Driven OrcaSlicer UI Restoration (Shipped: 2026-07-03)

**Phases completed:** 9 phases, 25 plans, 81 tasks

**Key accomplishments:**

- Single canonical inventory doc mapping all 34 screenshot-visible regions across 4 screenshots to a frozen 9-column schema, with per-cluster upstream coverage anchors, modify-vs-replace decisions, and a greppable cleanup checklist.
- Frozen Phase 50 inventory contract embedding the 34-region canonical tables, a §2 Verification & Sign-Off recording all 9 deterministic checks as PASS, and a full INV-01..05 traceability matrix with real evidence counts.
- 8 shell action gate Q_PROPERTY + 4 blocked-reason label Q_PROPERTY on BackendContext, forwarding to EditorViewModel/PreviewViewModel, plus a bulk stateChanged() signal wired from the owned viewmodels so shell gate state stays live across a Prepare↔Preview round-trip.
- Bound the screenshot-visible shell action controls in `BBLTopbar.qml` to the 8 C++ gate properties added in Plan 51-01, fixing the concrete UX bug where Undo/Redo were clickable when the undo/redo stack was empty (page-gated only). All 8 shell gate properties (canImport, canSlice, isSlicing, canExport, canSave, canUndo, canRedo, isBusy) are now consumed in QML — no dead C++ gate properties remain.
- Closed out Phase 51 with the verification + cleanup layer: an automated C++ shell-state test proving the 8 BackendContext gates are registered and the Prepare <-> Preview round-trip preserves state, two QML source-grep audits locking the BBLTopbar gate bindings and notification-surface placement, deletion of the stale unused components/Toolbar.qml, and the inventory PREP-TOP qt_target reconciliation to BBLTopbar.qml.
- Staleness Q_PROPERTYs on EditorViewModel plus a CRITICAL preset-change slice-invalidation connect in BackendContext, and an honest deferred settings-request entry point -- all C++, no QML touched
- Wired the Prepare left sidebar's six stubbed/dead controls to existing C++ viewmodel state: dynamic extruder-count filament slots, hidden dead color picker, dirty dots + read-only gating, enabled Setting entry point, live search filter, and the complete Global/Object/Plate scope triad -- QML-only, no C++ touched
- Two C++ regression guards (PREPSB-05 connect-fires invalidation + PREPSB-02 honest settings forward) plus a QML source-text audit (PREPSB-01..04) that lock every Plan 52-01/52-02 binding against regression -- test files only, all green via the sanitized-PATH build pattern
- OrcaSlicer-style G-code fixture + registered PreviewParserTests target (four RED/GREEN scaffold slots) so Plan 02's 20-role parser, 17-mode viewModes, and Summary legend have a deterministic place to land assertions
- 20-role canonical libvgcode extrusion-role parser, 17 upstream EViewType view modes, render-side per-role visibility, and the GCV1 wire-format `int role` extension — closing GCODE-01/02 at the data+renderer contract level
- Collapsible "Visible Line Types" card (18 per-role CxCheckBoxes bound to previewVm.toggleRoleVisibility) inserted into the Preview right panel, with a render-side GLViewport.roleVisibility binding that filters drawing on every toggle without repacking gcodePreviewData
- 14 new QtTest methods locking GCODE-01 (no-placeholder real-data path), GCODE-03 (legend + G-code-text coherence), GCODE-04 (SoftwareViewport/role-skip/sizeof guards), and GCODE-05 (reslice/export/page-switch/slice-failed payload-survival) behind automated, headless regression coverage — the no-placeholder and role-toggle-no-repack assertions are hard QVERIFY2.
- Phase-55-tagged D3D11 startup-policy audit guard (RhiViewport default + SoftwareViewport fallback-only) plus the signed-off VALIDATION.md with a fully-populated Per-Task Verification Map — all five GCODE requirements now have green automated commands and the phase is ready for /gsd:verify-work
- ConfigOptionModel extended with nullable/isVector/sidetext/percent type support, CxSpinBox gained unit-suffix property, and Wave 0 RED test scaffolds laid down for all SETTINGS-01..07 behaviors.
- Plan:
- Plan:
- 1. [Rule 3 - Blocking] Removed a second test the plan audit missed
- 1. [Rule 1 - Bug] Stripped pre-existing UTF-8 BOM from qml.qrc
- Encoded the 9 Phase 50 section 2 deterministic inventory checks as a 12-slot Qt Test target that runs in every canonical verify pass, locking region counts / schema / status+verification enums / region-ID format / INV-02-03-04 coverage anchors / cleanup format / no-blank-upstream against both the canonical doc and the frozen snapshot.
- Audited the existing automated coverage (VERIFY-02 workflow transitions / VERIFY-03 Preview stability are already covered by E2E + QmlUiAuditTests — no new test code needed), produced the user-runnable 4-screenshot UAT checklist (VERIFY-04), and ran the canonical verify end-to-end classifying every failure (VERIFY-05: 7/8 ctest targets PASS, the single CliTests failure is pre-existing carry-forward).

---

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
**Shipped:** 2026-06-28
**Status:** Complete with tech debt
**Phases:** 23-28 (6 phases, 14 plans)
**Git range:** `v3.0..v3.1` (52 commits, 94 files, +10879/-125)

**Goal:** Establish the Qt-native high-performance rendering foundation for Prepare and Preview using QRhi, while keeping upstream-visible behavior aligned and preserving the current stable fallback path.

**Phases shipped:**

- Phase 23: QRhi Renderer Foundation And Backend Gate - complete
- Phase 24: Prepare Scene Data And Plate Rendering - complete
- Phase 25: Prepare Model Mesh Rendering And Camera Interaction - complete
- Phase 26: Preview G-Code GPU Pipeline - complete
- Phase 27: Preview Interaction And Performance Gate - complete
- Phase 28: Fallback, Verification, Reviews, And Handoff - complete

**Requirements:** 26/26 satisfied at code/benchmark level (`RHI-01..06`, `PREP-01..07`, `PREV-01..07`, `PERF-01..06`).

**Key accomplishments:**

1. Added QRhi renderer gate and Qt-native render path behind `OWZX_RHI_RENDERER`.
2. Implemented Prepare bed/plate/model mesh rendering with dirty-gated buffers, camera interaction, and selection picking.
3. Implemented Preview G-code segment rendering path with draw-range layer filtering and travel toggle.
4. Added optional `owzx-render-bench`; D3D11 benchmarked at 1M and 5M segment workloads.
5. Kept software viewport as stable fallback and OpenGL path behind `OWZX_OPENGL`.
6. Fixed review findings: BUG-V31-1 (beginPass ordering) and DESIGN-V31-4 (travel toggle).

**Final verification:** Passed on 2026-06-28.

**Evidence:**

- `.planning/milestones/v3.1-MILESTONE-AUDIT.md`: 26/26 requirements, 6/6 phases, 6/6 integration, 5/5 flows.
- render_bench D3D11: 1M segments 0.36ms median, 5M segments 0.91ms median.
- QRhi invalid backend fallback verified to software viewport.

**Audit status:** `tech_debt` - D3D12 still crashes in the Prepare path; D3D11 is the safe default/workaround. Preview QRhi path is code-complete but needs manual visual verification in the app.

**Known deferred items at close:** D3D12 root-cause investigation, Preview visual verification, QRhi marker geometry, more precise upload timing, and moveEnd per-move precision.

**Archives:**

- `.planning/milestones/v3.1-ROADMAP.md`
- `.planning/milestones/v3.1-MILESTONE-AUDIT.md`

---

*Last updated: 2026-06-28 after v3.1 audit.*

---

### v3.2 - Multi-Plate Data Polish

**Started:** 2026-06-28
**Audited:** 2026-06-28
**Status:** Complete with tech debt
**Phases:** 29-32 (4 phases)
**Git range:** `d2543d5..d18bf8c`

**Goal:** Polish the PartPlate data model with multi-plate arrangement grid, thumbnail persistence, manual filament-map, and the real-model 3MF round-trip test fixture. AssembleView deferred.

**Phases shipped:**

- Phase 29: Multi-Plate Arrangement Grid - complete
- Phase 30: Thumbnail Persistence - partial (`THUMB-01` complete, `THUMB-02` deferred)
- Phase 31: Filament Map Manual - complete
- Phase 32: Test Fixture and Verification - partial (`FIXTURE-01` complete, `FIXTURE-02` framework complete)

**Requirements:** 8/10 complete. `THUMB-02` and `FIXTURE-02` are partial because the 3MF writer path is coupled to real GL capture / writer integration.

**Key accomplishments:**

1. Added PartPlateList grid geometry, plate origins, and plate-index decode.
2. Rewired arrangement rebuild logic to preserve/restore plate membership, including locked-plate exclusion.
3. Added deterministic PartPlateTests for arrangement, thumbnail variants, cache behavior, and manual filament maps.
4. Added thumbnail variants and PartPlate thumbnail cache.
5. Added manual per-plate filament map fields and ProjectServiceMock APIs.
6. Committed `tests/data/test_model.stl` and updated PLATE-09 to load a real fixture.

**Final verification:** v3.2 audit status `tech_debt`.

**Evidence:**

- `.planning/v3.2-MILESTONE-AUDIT.md`
- `.planning/milestones/v3.2-MILESTONE-AUDIT.md`
- `tests/PartPlateTests.cpp`: 48 passed in phase evidence.

**Audit status:** `tech_debt` - no blocking v3.2 defects, but two partial requirements share the 3MF writer integration blocker.

**Known deferred items at close:** `THUMB-03`, full `FIXTURE-02` save/reload assertions, AssembleView, auto filament-map recommendation, wipe-tower rendering, D3D12 root cause, and missing CLI fixtures.

**Archives:**

- `.planning/milestones/v3.2-ROADMAP.md`
- `.planning/milestones/v3.2-REQUIREMENTS.md`
- `.planning/milestones/v3.2-MILESTONE-AUDIT.md`

---

*Last updated: 2026-06-28 after v3.2 audit correction.*

---

### v3.3 - Slice Preview Main Flow MVP

**Started:** 2026-06-28
**Status:** Complete at MVP level, superseded by v3.4 for completeness
**Phases:** 33-36

**Goal:** Prove the basic local transition from slicing into a D3D11 QRhi G-code Preview with layer and move controls.

**Outcome:** The MVP path was implemented and verified at code/test level, but user UAT exposed Preview disappearing under layer/camera interaction. v3.4 superseded this milestone with a complete local import-to-G-code workflow pass.

**Known follow-up:** Do not use v3.3 as the final Preview completion evidence; use v3.4 Phase 40-43 artifacts instead.

---

### v3.4 - Import to G-code Complete Workflow

**Started:** 2026-06-28
**Status:** Complete by automated E2E closure
**Phases:** 37-43

**Goal:** Complete the full local user workflow from importing a model/project through local G-code export, with source-truth-aligned Prepare readiness, slicing/reslicing, D3D11 QRhi Preview, and export finalization.

**Phases reached:**

- Phase 37: Complete Import and Project Restore - complete
- Phase 38: Prepare Readiness and Slice Invalidation - complete
- Phase 39: Complete Slicing and Reslicing State Machine - complete
- Phase 40: Complete Preview Data and Upstream View Semantics - complete
- Phase 41: D3D11 Preview Rendering and Interaction Stability - complete
- Phase 42: Local G-code Export and Finalization - complete
- Phase 43: End-to-End Verification and Handoff - closed by canonical E2E coverage and current runtime launch evidence

**Current status:** The old manual checklist was superseded on 2026-07-06 by canonical E2E coverage and current runtime launch evidence. This is not recorded as a separate manual user click-through.

**Planning transition:** v3.5 proceeded while v3.4 manual UAT was deferred; that deferred item is now closed by E2E/runtime evidence.

---

### v3.5 - Preset Authoring Complete Workflow

**Started:** 2026-06-30
**Superseded:** 2026-07-01
**Status:** Superseded after Phase 46
**Phases:** 44-49

**Goal:** Complete the source-truth-aligned preset authoring workflow so users can load, select, edit, validate, save, create, import/export, and apply printer, filament, and process presets through the Qt UI, with the resulting configuration feeding Prepare, Slice, Preview, Export, and CLI paths.

**Phases reached:**

- Phase 44: Preset Bundle Service Foundation - complete historical evidence
- Phase 45: Compatibility and Selection State - complete historical evidence
- Phase 46: Config Editing, Dirty State, and Reset Semantics - complete historical evidence
- Phase 47: Preset Lifecycle Actions - superseded by v3.6 settings restoration
- Phase 48: Create Presets and Bundle Workflows - superseded by v3.6 settings restoration
- Phase 49: Slice Integration, Verification, and Handoff - superseded by v3.6 end-to-end restoration

**Outcome:** User rejected continuing the partial preset-only milestone and requested full screenshot/source-truth restoration of Prepare, Preview, and parameter settings. Phase 47-49 are intentionally abandoned as standalone work. Reusable Phase 44-46 code may be folded into v3.6 only when it supports the restored UI and does not preserve off-design behavior.

**Next step:** Continue with v3.6.

---

### v3.6 - Screenshot-Driven OrcaSlicer UI Restoration

**Started:** 2026-07-01
**Status:** Active planning
**Phases:** 50-58

**Goal:** Restore the Prepare page, Preview page, and parameter settings workflows as complete OrcaSlicer-equivalent user flows, using screenshots as visual/layout truth and OrcaSlicer source as behavior truth.

**Planned phases:**

- Phase 50: Screenshot and Source-Truth Inventory
- Phase 51: Shell and Navigation Restoration
- Phase 52: Prepare Sidebar and Preset Controls
- Phase 53: Prepare Object, Plate, and Viewport Workflow
- Phase 54: Preview Layout, Sliders, and Right Panels
- Phase 55: G-code Preview Semantics and Rendering Stability
- Phase 56: Parameter Settings Dialogs Restoration
- Phase 57: Deprecated UI Removal and Architecture Cleanup
- Phase 58: End-to-End Visual and Functional Verification

**Key rules:**

- Screenshots under `shotScreen/` are visual/layout truth.
- OrcaSlicer source under `third_party/OrcaSlicer` is behavior truth.
- If an existing page is materially off-design, replace it and remove obsolete files, routes, registrations, resources, imports, tests, and disconnected code paths.
- New or modified source comments must be English and ASCII-only.

**Next step:** Plan and execute Phase 50.

---

*Last updated: 2026-07-01 after v3.6 planning.*
