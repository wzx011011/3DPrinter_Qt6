# Remaining Feature Migration Plan

Last updated: 2026-07-23

This is the ordered backlog after reconciling planning with current implementation. If older milestone wording conflicts with the dated audits or current code evidence, the code evidence and latest audit win.

## Scope and Assumptions

- Active product target: OWzx Slicer, Qt6/QML GUI, upstream behavior mapped from `third_party/OrcaSlicer`.
- Current active milestone: v5.5 Build/Run Parity and Dependency Provenance.
- Historical CrealityPrint-era notes are evidence only. New work must cite active OrcaSlicer upstream paths unless the task explicitly cleans historical compatibility.
- Build verification remains: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Build directory remains: `build/`.
- QML must stay presentation and wiring only; durable business rules, validation, persistence, and upstream behavior mapping belong in C++ services/viewmodels.
- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows are removed from forward scope unless the user explicitly reopens them. Historical code and phase evidence remain historical only.

## Current Baseline

The project has a usable Qt6/QML shell, real model/project IO, real slicing and G-code export paths, preview rendering, partial preset IO, and historical hybrid device/camera/network integration work that is no longer forward roadmap scope.

Code-verified baseline:

| Area | Status | Meaning |
|---|---|---|
| Model/project load and basic slicing | Hybrid/Real | Real libslic3r paths exist; E2E pipeline passed in canonical verify. |
| Calibration | Hybrid | PA/Flow/Temp slice dispatch exists, but topbar UI and deterministic tests are incomplete. |
| SSDP/device discovery | Removed scope | Historical real-path evidence exists, but no future LAN/device discovery work should be promoted. |
| MQTT/FTP/send print | Removed scope | Historical transport wrappers exist, but no future printer upload/control work should be promoted. |
| Camera | Removed scope | Historical FFmpeg/RTSP path exists, but no future live camera/network stream work should be promoted. |
| Preset bundle | Partial | Current bundle IO is simplified JSON, not upstream-compatible bundle behavior. |
| PartPlate/AssembleView | Hybrid/Placeholder | Plate APIs exist; upstream PartPlate config and AssembleView are not source-truth complete. |
| Web/model mall/cloud/multi-machine | Removed scope | Pages exist historically, but no future WebView/cloud/multi-machine work should be promoted. |

## Milestone Sequence

### v2.9: Implementation Realignment and Stabilization

Goal: make planning, implementation, visible UI status, and verification evidence agree before more migration work lands.

Tasks:

- [x] **PLAN** Reconcile `.planning` entry files and reflect v2.7/v2.8 git history as already landed.
- [x] **HYGIENE** Fix encoding damage, literal escape artifacts, residual backup files, and unclassified untracked implementation files.
- [x] **CAL** Close implemented PA/Flow/Temp calibration paths with UI wiring and deterministic tests.
- [x] **INT** Add deterministic verification for SSDP, MQTT, FTP, camera, software viewport, and app settings behavior.
- [x] **UI** Reclassify or wire visible disabled/no-op UI workflows.
- [x] **VERIFY** Run canonical verification and account for built-only tests.

Exit criteria:

- [x] `.planning/INDEX.md`, `PROJECT.md`, `STATE.md`, `ROADMAP.md`, and `REQUIREMENTS.md` agree on current status.
- [x] No unexplained backup source files remain under `src/`.
- [x] Every active service has an explicit migration status.
- [x] Visible placeholder workflows are not counted as complete.
- [x] Canonical verification passes after stabilization.

### v3.0 Candidate: PartPlate and AssembleView

Goal: make multi-plate behavior source-truth complete enough for plate ownership, per-plate config, slicing, thumbnails, and assembly view workflows.

Likely upstream areas:

- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate*`
- `third_party/OrcaSlicer/src/slic3r/GUI/Plater*`
- Assemble and multi-plate GUI paths.

Likely Qt6 target areas:

- `src/core/services/ProjectServiceMock.*`
- `src/core/viewmodels/EditorViewModel.*`
- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/pages/Plater.qml`
- `src/qml_gui/components/GLToolbars.qml`
- `src/qml_gui/panels/SliceProgress.qml`

Candidate tasks:

- [ ] Define real PartPlate data ownership independent of visual-only plate lists.
- [ ] Implement per-plate object assignment, reorder, duplicate, delete, and rename semantics.
- [ ] Implement per-plate config override read/write path.
- [ ] Wire bottom PartPlateList overlay and plate context actions.
- [ ] Replace AssembleView placeholder with a minimal real multi-plate assembly view.
- [ ] Add save/reload regression for multi-plate project state.

### v3.1 Candidate: Preset System Completion

Goal: replace the current simplified JSON preset approximation with workflows that match upstream preset semantics.

Status: partially promoted. v4.1 covers the visible parameter settings dialogs, typed option controls, dirty/save/reset semantics, and settings workflow stability. Full PresetBundle import/export compatibility remains future scope.

Likely upstream areas:

- `PresetBundle`
- `PresetComboBoxes`
- `SavePresetDialog`
- `UnsavedChangesDialog`
- `ExportPresetBundleDialog`
- `CreatePresetsDialog`
- Preset dirty-state and inheritance code.

Candidate tasks:

- [ ] Replace or extend simplified JSON import/export with upstream-compatible bundle metadata and behavior.
- [ ] Implement CreatePresetsDialog minimal source-truth version.
- [ ] Complete dirty-state propagation and unsaved-change prompts across page and preset switches.
- [ ] Implement Simple/Advanced filtering in C++ model/viewmodel where it affects behavior.
- [ ] Wire Compare/Diff preset flow from sidebar and settings.
- [ ] Add preset bundle round-trip tests.

### Removed Candidate: Web, Cloud, and Multi-Machine

Goal: none. This candidate is removed from the forward roadmap by user direction on 2026-07-07.

Do not promote ModelMall/Home WebView, cloud account/sync, LAN device discovery, multi-machine, device send/upload, Monitor task lifecycle, live camera/network streams, or printer-connected hardware workflows unless the user explicitly reopens them.

### Later Candidates

- Local calibration generation and preset workflows beyond PA, Flow Rate, and Temp Tower when they do not require live printer hardware or networking.
- Full i18n content migration beyond infrastructure.
- OpenVDB-dependent hollow/support paint workflows.
- SLA-specific GUI modules and gizmos.

## Backlog Rule

Current promoted candidate: none. v5.5 is an execution gate, not a migration
candidate.

Next recommended promotion candidate after v4.1: AssembleView, unless v4.1 uncovers a blocking settings/preset follow-up.

Do not promote a future candidate into an active milestone until:

1. v2.9 baseline classification is complete.
2. The candidate has a source-truth gap analysis or current implementation audit.
3. Requirements are testable and mapped to roadmap phases.
4. The expected verification path is known.
