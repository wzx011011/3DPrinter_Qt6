# Roadmap: OWzx Slicer

## Milestones

- Complete: v2.9 Implementation Realignment and Stabilization - Phases 10-15
- Complete: v3.0 PartPlate Core - Phases 16-22
- Complete: v3.1 QRhi Rendering - Phases 23-28
- Complete: v3.2 Multi-Plate Data Polish - Phases 29-32
- Complete: v3.3 Slice Preview Main Flow MVP - Phases 33-36
- Complete: v3.4 Import to G-code Complete Workflow - Phases 37-43
- Complete: v3.5 Preset Authoring Complete Workflow - Phases 44-49
- Complete: v3.6 Screenshot-Driven OrcaSlicer UI Restoration - Phases 50-58
- Complete: v3.7 Screenshot-Level UI Parity Closure - Phases 59-64
- Complete: v3.8 RHI Gizmo Parity - Phases 65-73
- Complete: v3.9 Prepare Page UI Restoration - Phases 74-78
- Complete: v4.0 Preview Page UI Restoration - Phases 79-83
- Complete: v4.1 Parameter Settings Dialogs Source-Truth Restoration - Phases 84-88
- Complete: v4.2 AssembleView Source-Truth Restoration - Phases 89-93
- Complete: v4.3 Real Thumbnail Capture And 3MF Round-Trip - Phases 94-98
- Complete: v4.4 Wipe-Tower Geometry Readback And Real Rendering - Phases 99-102
- Complete: v4.5 Backlog Closure - Phases 103-116
- Complete: v4.6 Core Feature Completion Sweep - Phases 117-128
- Complete: v4.7 Polish, i18n & Advanced Feature Recovery - Phases 129-135
- Complete: v4.8 Dependency Unlock, Assembly Transform & i18n Completion - Phases 136-140
- Complete: v5.0 Advanced Feature Recovery & Tech-Debt Closure - Phases 141-153
- Complete: v5.1 v5.0 Deferred Items Closure - Phases 154-159
- Complete: v5.2 UI Excellence - Phases 160-170
- Complete: v5.3 Feature Completion & v5.2 Closure - Phases 171-179
- Code-closed: v5.4 Upstream Sync Closure - Phases 180-187, with i18n 184/185 deferred
- Complete: v5.5 Build/Run Parity and Dependency Provenance - Phases 188-192
- Complete: v5.6 Deferred Backlog Closure - Phases 193-205
- Complete: v5.7 D3D12 Backend Investigation - Phases 206-211

## Latest Completed Milestone: v5.7 D3D12 Backend Investigation

**Goal:** Investigate promoting D3D12 to the default QRhi backend; mitigate
the three candidate seams; verify on real hardware.

**Completion:** D3D12 promotion attempted then **reverted** — real-machine
verification found D3D12 crashes at QQuickWindow swapchain init on AMD Radeon
APU (mainstream integrated graphics). D3D11-first default restored. Phase 207-210
retained (diagnostics + seam A/B/C mitigations). Canonical build (D3D11 default)
exited `0`, `APP_RUNNING_PID=80404`, all ctest + E2E passed. The "promote
D3D12?" question is closed: **no**, until the AMD APU swapchain issue is
resolved. See `.planning/milestones/v5.7-MILESTONE-AUDIT.md`.

## Previous Completed Milestone: v5.6 Deferred Backlog Closure

**Goal:** Dispose of all 11 v5.5 deferred items in one milestone. Implement the
8 offline-deliverable items; document the remaining 3 per user decisions
(H2C/A2L kept deferred, AMS architecture-only, D3D12 root-cause-only).

**Scope rule:** No reopening of LAN/device/cloud/printer-hardware scope; no
CPython embedding; no D3D12 default promotion; no upstream OrcaSlicer patches;
sole canonical build command and `build/` directory.

**Completion:** The canonical script exited `0` on 2026-07-24, reported
`APP_RUNNING_PID=79708`, all ctest targets passed (incl.
`v56CrossWorkstreamRegressionLocked`), and the E2E pipeline reported "All
pipeline tests passed". See `.planning/milestones/v5.6-MILESTONE-AUDIT.md`.

## Previous Completed Milestone: v5.5 Build/Run Parity and Dependency Provenance

**Goal:** Make the local build/run path explainable, reproducible, and
comparable with GitHub CI without changing product behavior.

**Scope rule:** All work uses the canonical build command and `build/` only.
Qt is consumed as a prebuilt dependency. v5.5 may update planning docs and the
canonical verification script; it must not change slicer behavior or patch
upstream OrcaSlicer source.

**Completion:** The canonical script exited `0` on 2026-07-23, logged selected
build inputs and `APP_RUNNING_PID=104080`, and passed E2E. See
`.planning/milestones/v5.5-MILESTONE-AUDIT.md`.

## Phases

### Phase 193: Planning State Reconciliation

- [x] Phase 193: Planning State Reconciliation (DOC, DOC-01)

Update `.planning/STATE.md`, `.planning/ROADMAP.md`, `.planning/PROJECT.md`,
and `.planning/INDEX.md` so v5.6 is active and v5.5 is archived as complete.

### Phase 194: Cmp-03 OptionRow and Slider Unification

- [x] Phase 194: Cmp-03 OptionRow and Slider Unification (UI, UI-01)

Promote inline `NumericEdit`/`Badge` from `OptionRow.qml` to
`controls/CxNumericEdit.qml`/`CxBadge.qml`; unify `MoveStepButton` +
`RailButton` into `controls/CxStepButton.qml`; add `controls/CxBusyIndicator.qml`;
migrate all reference sites.

### Phase 195: KBShortcutsDialog Extraction and Grouping

- [x] Phase 195: KBShortcutsDialog Extraction and Grouping (UI, UI-02)

Extract inline Dialog from `main.qml` to `dialogs/KBShortcutsDialog.qml`;
reorganize as 5 groups (Global/Prepare/Toolbar/ObjectsList/Preview) aligned
with upstream; reconcile displayed vs bound shortcuts.

### Phase 196: XD-02 Emboss Spinner and SliceProgress States

- [x] Phase 196: XD-02 Emboss Spinner and SliceProgress States (FEAT, FEAT-01)

Add `EditorViewModel::embossRunning` Q_PROPERTY; Emboss panel shows spinner via
`CxBusyIndicator`; `SliceProgress.qml` binds `sliceState` enum with
Cancelled/Error branches.

### Phase 197: Calibration Dedicated Tower Geometry

- [x] Phase 197: Calibration Dedicated Tower Geometry (FEAT, FEAT-02)

Correct the `.drc` term to `.stl`/`.3mf`/`.step`; package upstream
`resources/calib/*` tower models; `CalibrationServiceMock` loads the
mode-matching tower; apply per-mode config overrides.

### Phase 198: ObjectList Tree Deepening (Auxiliary file-tree panel)

- [x] Phase 198: ObjectList Tree Deepening (FEAT, FEAT-03)

Deepen `ObjectList.qml` from 2-level toward upstream `GUI_ObjectList.cpp`;
add layer level and object/part settings sub-panel entries. Does NOT revive
the deleted `AuxiliaryPage` (removed device scope).

### Phase 199: ConfigWizard Vendor/Model Enumeration Layer

- [x] Phase 199: ConfigWizard Vendor/Model Enumeration Layer (DLG, DLG-01)

Extend `PresetServiceMock` with `vendors()`, `printerModelsForVendor(v)`,
`materialsForVendor(v)` reusing parsed `machineEntries`/`filamentEntries`;
exclude `__upstream_defaults__`; add regression test.

### Phase 200: ConfigWizard Single-Vendor Wizard Rewrite

- [x] Phase 200: ConfigWizard Single-Vendor Wizard Rewrite (DLG, DLG-02)

Rewrite `ConfigWizardDialog.qml` to read lists dynamically; close the
`wizardFinished` loop via `setConfigWizardCompleted(true)`; position as
single-vendor (Creality) wizard.

### Phase 201: AMS Architecture Cleanup (mock to ViewModel)

- [x] Phase 201: AMS Architecture Cleanup (mock to ViewModel) (DLG, DLG-03)

New `AmsMaterialsViewModel` carries the hardcoded `AMSSettingsDialog` data;
dialog rebinds; enable the "add mapping" button. Data source stays mock; no
network.

### Phase 202: Plugin Manager UI Real Backend (no Python)

- [x] Phase 202: Plugin Manager UI Real Backend (no Python) (DLG, DLG-04)

New `PluginService` (C++): local registry + download/install/uninstall;
`PluginManagerDialog.qml` binds the real service, aligned with upstream
`WebDownPluginDlg`. No CPython embedding.

### Phase 203: D3D12 Root-Cause Confirmation (no default promotion)

- [x] Phase 203: D3D12 Root-Cause Confirmation (no default promotion) (RHI, RHI-01)

Time-boxed root-cause of the `0xC0000005` crash (original machine + debugger,
historical minidumps, A/B/C mitigation probes). Deliverable is a report;
default stays D3D11; the regression slot keeps its direction.

### Phase 204: de/fr/ja/ko Translation Long Tail to >=85%

- [x] Phase 204: de/fr/ja/ko Translation Long Tail to >=85% (I18N, I18N-01)

Reuse `scripts/translate_core_i18n.py` + glossary; drop unfinished to <=252 per
language; `lupdate` refresh; `lrelease`; UTF-8/XML validity check.

### Phase 205: Cross-Workstream Regression Gate and Milestone Audit

- [x] Phase 205: Cross-Workstream Regression Gate and Milestone Audit (GATE, GATE-01)

`v56CrossWorkstreamRegressionLocked` slot + per-phase slots; canonical build
exit 0; ctest PASS; E2E PASS; launch liveness; produce
`v5.6-MILESTONE-AUDIT.md`.

## v5.5 Phase Archive (188-192, completed 2026-07-23)

| Phase | Name | Status | Requirement |
|---|---|---|---|
| 188 | Planning State Reconciliation | Complete | DOC-01 |
| 189 | Dependency Provenance Contract | Complete | PROV-01 |
| 190 | Canonical Script Result Classification | Complete | VERIFY-01 |
| 191 | Local App Launch Liveness Gate | Complete | RUN-01 |
| 192 | CI/Local Parity Audit | Complete | GATE-01 |

Phases 188-192 completed in order. Phase 192 uses the fresh canonical-script
evidence recorded in `v5.5-MILESTONE-AUDIT.md`.

## Archives

- `.planning/milestones/v5.6-ROADMAP.md`
- `.planning/milestones/v5.6-REQUIREMENTS.md`
- `.planning/milestones/v5.6-phases/`
- `.planning/milestones/v5.5-ROADMAP.md`
- `.planning/milestones/v5.5-REQUIREMENTS.md`
- `.planning/milestones/v5.5-phases/`

## Future Candidates

After v5.6, the remaining deferred items (per explicit user decisions on
2026-07-24):

- H2C/A2L multi-nozzle UI (bb3 fork submodule + product decision pending).
- Per-extruder config editor UI (Cmp-03 sub-item; needs multi-extruder fixture).
- ConfigWizard multi-vendor selection + PresetUpdater + AppConfig.
- D3D12 default-backend promotion itself (pending root cause).
- AMS real device/cloud data sources (printer-hardware scope).
- Python script/macro framework / CPython embedding.

SLA print path and LAN/cloud/device/camera/printer-hardware workflows remain
declined unless the user explicitly reopens them.
