# Implementation Audit

Date: 2026-06-23
Scope: read-only audit of planning residue and current Qt6/QML implementation state.

## Executive Summary

The repository is not in a clean implementation state. It has a usable Qt6/QML shell and many real bridges, but it still carries a large amount of partial migration surface: `*Mock` service names remain central, some of those services contain real integrations plus fallback behavior, and QML still has disabled or placeholder workflows. The planning tree also mixes v1 CrealityPrint history, v2 OrcaSlicer migration records, and current v2.6 completion status without a single index until this file set was added.

## Workspace State

`git status --short` reported local modifications and untracked files at audit time:

- Modified: `CMakeLists.txt`, `src/core/services/CalibrationServiceMock.*`, `src/core/services/SliceService.*`, `src/qml_gui/BackendContext.*`, `src/qml_gui/main_qml.cpp`
- Untracked: `.agents/`, `.codex/`, `AGENTS.md`, `IMPLEMENTATION_SUMMARY.md`, `src/core/services/AppSettingsService.*`, `src/core/services/SliceService.cpp.backup`

Do not clean or revert these without confirming ownership. The backup file is a clear residual candidate, but it may belong to the active local changes.

## Planning Findings

### P1: Missing rule files referenced by instructions

`AGENTS.md` references `.Codex/rules/source-truth-migration.md` and `.Codex/rules/build-rules.md`, but `.Codex/rules/` does not exist. The visible `.Codex` content is limited to `agents/source-truth-migration.toml` and `hooks.json`.

Impact: new agents may follow missing documentation paths and fall back to stale or inferred rules.

Recommendation: recreate the rule files or update `AGENTS.md` to point at the actual canonical documents.

### P1: Mixed upstream identity

Active project text says OrcaSlicer/OWzx, but older docs and comments still contain CrealityPrint-era references. Some are historical, some are implementation comments.

Examples:

- `README.md` still links `docs/CrealityPrint_Qt_GUI重写架构.md`.
- `src/cli/CliRunner.h` says it aligns with upstream `CrealityPrint.cpp`.
- `src/core/viewmodels/PreviewViewModel.cpp` refers to `CrealityPrint GCodeViewer`.
- `src/qml_gui/pages/PreparePage.qml` references upstream `CrealityPrint Plater`.

Impact: this makes source-truth audits ambiguous unless each task states whether it is using historical CrealityPrint evidence or active OrcaSlicer behavior.

Recommendation: preserve historical docs, but tag current source-truth references as OrcaSlicer and move historical CrealityPrint notes into archive sections.

### P2: Requirement ledger title and status drift

`.planning/REQUIREMENTS.md` starts as "Milestone v2.0" but now contains v2.1 through v2.6 ledgers. `.planning/STATE.md` says v2.6 complete with one deferred task, while the implementation still contains placeholders and fallback paths.

Impact: "complete" can be misread as product-complete rather than phase-complete.

Recommendation: keep `STATE.md` for milestone state, use `REQUIREMENTS.md` only as a ledger, and require verification references for `[x]`.

## Implementation Findings

### P0: Core service layer still depends on Mock-named services

The composition root constructs:

- `CalibrationServiceMock`
- `PresetServiceMock`
- `DeviceServiceMock`
- `ProjectServiceMock`
- `NetworkServiceMock`
- `CameraServiceMock`
- `CloudServiceMock`

Some of these contain real integrations, but the naming and fallback paths make it hard to tell what is production behavior.

Impact: future work can accidentally build product behavior on simulation paths.

Recommendation: classify each service as one of:

- `Real`: production integration complete
- `Hybrid`: real path plus fallback/mock path
- `Mock`: simulation only

Then reflect this in `REQUIREMENTS.md` and eventually rename or split services when the blast radius is acceptable.

### P0: Residual backup source file

`src/core/services/SliceService.cpp.backup` exists next to the active `SliceService.cpp`.

Impact: it can confuse searches, audits, and future agents. It is not compiled by the current CMake source list, but it appears in broad text scans.

Recommendation: delete it after confirming it is not needed, or move it to an explicit archive outside `src/`.

### P1: QML still carries disabled or placeholder workflows

Representative findings:

- `src/qml_gui/main.qml`: export project/model handlers are TODOs; one placeholder tab remains visible as an "占位 Tab".
- `src/qml_gui/BBLTopbar.qml`: account/model-store/publish and FilamentGroupPopup are placeholders; calibration submenu entries are disabled placeholders.
- `src/qml_gui/components/GLToolbars.qml`: layer editing is disabled.
- `src/qml_gui/panels/LeftSidebar.qml`: Simple/Advanced, Compare, Setting, ObjectLayers, and Params page-view behavior still have TODO or placeholder notes.
- `src/qml_gui/dialogs/NetworkTestDialog.qml`: real network testing is disabled.
- `src/qml_gui/dialogs/ExportPresetBundleDialog.qml`: comment says real export needs service extension.
- `src/core/viewmodels/ModelMallViewModel.h`: `webViewAvailable()` returns false.

Impact: UI may look broad but several actions are still non-functional or intentionally deferred.

Recommendation: keep these as `[-]` requirements unless the exact workflow is wired and verified.

### P1: Business logic in QML needs triage

There are many QML-side loops, filtering functions, and action handlers. Some are acceptable view logic, but several areas should be reviewed against the rule "QML only presents and wires interactions":

- `src/qml_gui/components/ParamsPage.qml`: filtering and option list rebuild logic.
- `src/qml_gui/dialogs/EditGCodeDialog.qml`: placeholder list flattening, filtering, category state, and insertion behavior.
- `src/qml_gui/panels/AuxiliaryListPanel.qml`: folder/file iteration and operations orchestration.
- `src/qml_gui/dialogs/CaliHistoryDialog.qml`: history reload logic.

Impact: this may create duplicated business rules outside viewmodels.

Recommendation: keep trivial UI filtering in QML, but move durable domain rules, validation, persistence decisions, and upstream behavior mapping into C++ viewmodels/services.

### P1: Calibration remains partial

Planning explicitly defers full calibration to v2.7. Code has `CalibrationServiceMock.*` modified locally and still named Mock. The v2.6 state says Calibration complete only as skeleton/deferred.

Impact: calibration should not be presented as source-truth complete.

Recommendation: next migration phase should target `CalibrationServiceMock`, `CalibrationViewModel`, `CalibrationDialog`, and upstream calibration classes together.

### P2: PartPlate and AssembleView are still partial

`BackendContext.h` marks AssembleView as v2.0 placeholder. `ProjectServiceMock` still describes PartPlate scoped config and thumbnail generation as Mock or TODO in several places.

Impact: multi-plate workflows may be visually present but not upstream-equivalent.

Recommendation: keep multi-plate/AssembleView as a dedicated milestone, not incidental cleanup.

### P2: Brand cleanup is incomplete by strict naming

The project already accepts some historical names, but there are still implementation references to `CrealityPrint`, `Creality`, and `Crality3D` outside `third_party/`.

Impact: user-facing strings appear mostly cleaned, but internal references still cause audit noise.

Recommendation: decide whether internal namespace/comment cleanup is required now. If yes, do it as a mechanical task with a narrow verification script.

## Verification Performed

Read-only commands used:

- Listed `.planning` tree and planning markdown files.
- Checked `.Codex` contents and confirmed missing `.Codex/rules`.
- Searched for plan files under `.planning`, `.Codex`, and `docs`.
- Ran `git status --short`.
- Searched for backup/temp artifacts.
- Searched implementation for `TODO`, placeholders, stubs, Mock/fallback language, and disabled UI.
- Searched QML for non-trivial inline handlers/functions.
- Searched non-third-party files for legacy Creality/Crality references.

Build was not run. Per project rules, the only allowed full verification command is:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Recommended Next Cleanup

1. Confirm whether `src/core/services/SliceService.cpp.backup` can be removed.
2. Restore or redirect `.Codex/rules/source-truth-migration.md` and `.Codex/rules/build-rules.md`.
3. Add a service-status matrix to `REQUIREMENTS.md`: Real / Hybrid / Mock.
4. Reclassify placeholder UI requirements from `[x]` to `[-]` unless verified against upstream.
5. Pick the next source-truth task: Calibration, PartPlate/AssembleView, or QML business-logic extraction.
