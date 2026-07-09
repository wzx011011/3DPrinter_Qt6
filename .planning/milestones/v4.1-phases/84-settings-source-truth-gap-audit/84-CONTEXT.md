# Phase 84: Settings Source-Truth Gap Audit - Context

**Gathered:** 2026-07-07
**Status:** Ready for planning
**Mode:** Autonomous discuss defaults (`--auto`)

<domain>
## Phase Boundary

Phase 84 is a read-only audit phase for the v4.1 Parameter Settings Dialogs
Source-Truth Restoration milestone. It freezes the settings region map before
implementation by mapping printer, material, and process settings work to:

- the target settings screenshots,
- OrcaSlicer source-truth files,
- current Qt/QML/C++ targets,
- Phase 56 residual visual-UAT evidence,
- downstream phase ownership,
- requirements, replacement decisions, and verification expectations.

This phase does not modify production UI source code. It should produce the
canonical v4.1 settings gap matrix that Phase 85, Phase 86, Phase 87, and Phase
88 execute against.

In scope:

- Printer settings visual/source-truth audit using
  `shotScreen/打印机参数设置页.png`.
- Material settings visual/source-truth audit using
  `shotScreen/材料参数设置页.png`.
- Process settings source-truth audit by OrcaSlicer parity because no process
  screenshot exists.
- Reconciliation of Phase 56 deferred visual/runtime evidence into v4.1 owner
  phases.
- Classification of current `SettingsDialog.qml`, `OptionRow.qml`, and
  `GroupNavSidebar.qml` gaps as modify, replace, remove, or defer.

Out of scope:

- Production QML/C++ edits.
- LAN device discovery, device send/upload, cloud print, Monitor lifecycle,
  ModelMall/Home WebView/cloud workflows, live camera/network streams, or
  printer-connected hardware workflows.
- AssembleView.
- D3D12/Vulkan backend promotion.
- Full upstream `PresetBundle` import/export compatibility beyond preserving
  the already-wired save/reset/preset-selection semantics required by v4.1.

</domain>

<decisions>
## Implementation Decisions

### Source And Visual Truth

- **D-01:** Use `shotScreen/打印机参数设置页.png` and
  `shotScreen/材料参数设置页.png` as the printer/material visual truth. Both
  images are 736x593 and show a dense dark independent settings window with a
  preset/action row, horizontal tabs, compact section headers, typed rows, and
  no visible left group-navigation sidebar.
- **D-02:** Treat process settings as a same-shell, same-semantics source-truth
  surface driven by OrcaSlicer settings sources. Do not invent a process-only
  visual design because no process screenshot was supplied.
- **D-03:** Use OrcaSlicer source as behavior truth, especially `Tab.*`,
  `PresetComboBoxes.*`, `ConfigManipulation.*`, preset save/unsaved dialogs,
  and libslic3r preset/config definitions.
- **D-04:** Current runtime/source evidence is useful for identifying gaps, but
  Phase 84 must not claim pixel parity. Runtime visual proof belongs to Phase
  88 after restoration work lands.

### Audit Artifact Shape

- **D-05:** Produce a canonical `84-GAP-MATRIX.md`, following the Phase 79
  Preview audit pattern. Each row should include target observation, current
  evidence, Qt targets, upstream source, decision, gap, severity, owner phase,
  requirement mapping, and verification method.
- **D-06:** Use settings-specific region IDs rather than generic UI labels.
  Recommended regions: `SET-SHELL`, `SET-PRESET-ACTIONS`, `SET-TABS`,
  `SET-SECTIONS`, `SET-TYPED-ROWS`, `SET-STATE-INDICATORS`,
  `SET-ENTRYPOINTS`, `SET-SEARCH-MODE`, `SET-PRESET-SEMANTICS`,
  `SET-PERSISTENCE`, and `SET-CLEANUP`. The planner may split printer/material
  rows if the screenshot differences require it.
- **D-07:** A visible control is incomplete if it is off-target visually,
  exposes mojibake/raw/internal labels, is disconnected/no-op, lacks upstream
  source mapping, or has no verification route.

### Phase 56 Residual Reconciliation

- **D-08:** Preserve Phase 56 backend semantics as the baseline: independent
  non-modal windows, per-tier preset selection, dirty/save/save-as/reset,
  typed option models, search/filtering, slice invalidation, and project
  persistence.
- **D-09:** Reopen Phase 56's deferred visual-UAT items as v4.1 work:
  settings visual parity maps to Phase 85/86/88, typed-control rendering maps
  to Phase 86/88, and non-modal cross-window live-edit evidence maps to
  Phase 87/88.
- **D-10:** Do not treat Phase 56's old "left group-nav" expectation as final
  visual truth. The current target screenshots show horizontal tabs and
  section flow without a visible left group navigation panel. Phase 84 should
  classify that historical conflict explicitly.

### Current Qt Classification Defaults

- **D-11:** Keep `ConfigViewModel`, `ConfigOptionModel`, and
  `PresetServiceMock` semantics unless the audit finds a source-truth bug.
  Their APIs are the proven bridge for existing tests and Phase 56 behavior.
- **D-12:** Treat `SettingsDialog.qml` as a likely modify-or-recompose target:
  preserve non-modal shell and dialog wiring, but audit visible strings, title
  text, top action affordances, tab density, search/advanced presentation, row
  flow, and close/action buttons against the screenshots.
- **D-13:** Treat `OptionRow.qml` as a likely modify target for Phase 86:
  preserve typed dispatch and `optionModel.setValue()` wiring, but audit unit
  fields, enum combos, checkboxes, color/text fields, paired min/max rows,
  dirty/read-only/value-source/validation states, and section header visuals.
- **D-14:** Treat `GroupNavSidebar.qml` as remove/hide/defer for the visible
  settings window unless Phase 84 finds specific upstream evidence requiring it
  in the screenshot-restored layout.
- **D-15:** Mojibake cleanup is not optional. Settings-visible strings and
  labels must be audited with the same strictness used in Prepare and Preview
  restoration.

### Verification Routing

- **D-16:** Phase 84 verification should be source/document focused:
  gap-matrix completeness, requirement coverage, `git diff --check`, and
  encoding guard. Full canonical build and app launch are reserved for Phase
  88 unless the Phase 84 plan introduces executable checks.
- **D-17:** Phase 88 must record printer, material, and process settings visual
  evidence. Printer/material evidence must be compared against the two target
  screenshots; process evidence must prove same-shell/source-truth parity.
- **D-18:** Future QML audits should cover clean text, absence of dead controls,
  settings entry-point operability, option-control binding, and stale path
  cleanup.

### the agent's Discretion

- The planner may choose the exact `84-GAP-MATRIX.md` table layout and whether
  to add supplementary notes, as long as every Phase 84 success criterion and
  `SETAUDIT-*` requirement is covered.
- The planner may classify individual current QML regions as modify vs replace
  based on source reads, but must preserve proven C++ semantics unless a
  specific source-truth mismatch is found.
- The planner may add lightweight static audit commands for Phase 84. It should
  not run the full canonical verifier just to create a docs-only audit unless
  the plan gives a concrete reason.

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Planning Scope

- `.planning/PROJECT.md` - current v4.1 milestone goal, source-truth rule,
  screenshot truth, scope guard, removed network/device work, and settings
  source-truth candidates.
- `.planning/REQUIREMENTS.md` - v4.1 requirements, especially
  `SETAUDIT-01`, `SETAUDIT-02`, `SETLAYOUT-*`, `SETCTRL-*`, `SETSEM-*`, and
  `SETVERIFY-*`.
- `.planning/ROADMAP.md` - Phase 84 success criteria and downstream Phase
  85-88 ownership.
- `.planning/STATE.md` - current milestone state, carry-forward status, and
  canonical verifier expectations.

### Visual Truth

- `shotScreen/打印机参数设置页.png` - printer settings target screenshot.
- `shotScreen/材料参数设置页.png` - material settings target screenshot.

### Prior Settings Evidence

- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-CONTEXT.md` - historical Phase 56 decisions and the old left group-nav assumption that Phase 84 must reconcile against current screenshots.
- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-VERIFICATION.md` - Phase 56 verified semantics plus deferred visual-UAT items.
- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-VALIDATION.md` - manual-only visual/runtime rows that v4.1 reopens.
- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-UI-SPEC.md` - prior settings UI contract; use as evidence, not as visual truth when it conflicts with the current screenshots.

### Audit Format Precedent

- `.planning/milestones/v4.0-phases/79-preview-source-truth-gap-audit/79-CONTEXT.md` - recent source-truth audit context pattern.
- `.planning/milestones/v4.0-phases/79-preview-source-truth-gap-audit/79-GAP-MATRIX.md` - canonical region-matrix structure to reuse for settings.
- `.planning/milestones/v4.0-phases/83-preview-verification-and-cleanup/83-VERIFICATION.md` - final verification pattern for canonical build, launch, UI automation, and visual evidence.

### Current Qt Targets

- `src/qml_gui/dialogs/SettingsDialog.qml` - current non-modal settings shell,
  preset/action row, tab strip, filtering, and dialog wiring.
- `src/qml_gui/components/OptionRow.qml` - current typed-option row renderer.
- `src/qml_gui/components/GroupNavSidebar.qml` - current left option-group
  navigation component; likely off-design for visible v4.1 layout.
- `src/qml_gui/dialogs/SavePresetDialog.qml` - current Save As flow.
- `src/qml_gui/dialogs/UnsavedChangesDialog.qml` - current unsaved-close guard.
- `src/qml_gui/main.qml` - settings dialog instances and
  `settingsRequested` dispatch.
- `src/qml_gui/panels/LeftSidebar.qml` - Prepare/sidebar settings entry points
  and preset edit buttons.
- `src/core/viewmodels/ConfigViewModel.h`
- `src/core/viewmodels/ConfigViewModel.cpp`
- `src/qml_gui/Models/ConfigOptionModel.h`
- `src/qml_gui/Models/ConfigOptionModel.cpp`
- `src/core/services/PresetServiceMock.h`
- `src/core/services/PresetServiceMock.cpp`
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`
- `tests/E2EWorkflowTests.cpp`

### OrcaSlicer Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/ConfigManipulation.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/ConfigManipulation.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/CreatePresetsDialog.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/CreatePresetsDialog.cpp`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.cpp`
- `third_party/OrcaSlicer/src/libslic3r/Preset.hpp`
- `third_party/OrcaSlicer/src/libslic3r/Preset.cpp`
- `third_party/OrcaSlicer/src/libslic3r/PresetBundle.hpp`
- `third_party/OrcaSlicer/src/libslic3r/PresetBundle.cpp`

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- `SettingsDialog.qml` already uses an independent `ApplicationWindow`,
  `Qt.NonModal`, three preset tiers (`printer`, `filament`, `print`), save-as
  and unsaved-change dialog instances, tab filtering, and option-model binding.
- `OptionRow.qml` already dispatches typed controls for bool, int, double,
  percent, enum, and string values and routes edits through
  `optionModel.setValue()`.
- `ConfigViewModel.*` exposes preset lists, selected presets, dirty state,
  request-save/discard/cancel flows, filtering, value source, reset operations,
  merged config values, and project-config application.
- `ConfigOptionModel.*` exposes typed option metadata, group/page names, dirty
  counts, reset/setValue, category/page/group filtering, nullable and vector
  flags, units, and tooltips.
- `PresetServiceMock.*` loads vendor presets where available, keeps
  category-aware preset lists, selected-preset persistence, compatibility
  checks, read-only/user preset gates, save/create/delete/rename flows, and
  fallback defaults.
- `SavePresetDialog.qml` and `UnsavedChangesDialog.qml` are reusable current
  dialog flows for Phase 87 if the visible shell is recomposed.
- `QmlUiAuditTests.cpp`, `ViewModelSmokeTests.cpp`, and
  `E2EWorkflowTests.cpp` already contain Phase 56 settings regression coverage
  that Phase 85-88 should preserve and extend.

### Established Patterns

- OrcaSlicer source is behavior truth; screenshots are visual/layout truth for
  screenshot-driven milestones.
- QML is presentation and wiring only. Durable behavior, validation,
  persistence, and source-truth mapping belong in C++ services/viewmodels.
- Custom QML controls use the `Cx*` family where possible; raw controls need a
  specific reason and test coverage.
- Replaced UI paths should be removed in the same milestone rather than left as
  stale routes, qrc entries, imports, or disconnected tests.
- Documentation-only audit phases can be verified with source reads,
  `git diff --check`, encoding guard, and committed planning artifacts.

### Integration Points

- Prepare/sidebar settings entry points route through `BackendContext` and
  `settingsRequested(category)` into the three `SettingsDialog` instances in
  `main.qml`.
- Settings edits flow through `ConfigOptionModel::setValue` and
  `ConfigViewModel::stateChanged`, which is already connected to slice
  invalidation through `BackendContext`.
- Project save/load preservation flows through `ConfigViewModel::mergedConfigValues`,
  `applyProjectConfig`, and `ProjectServiceMock` scoped config paths.
- Phase 84 should not create new integration code; it should identify which
  existing integration points each later phase must preserve or harden.

</code_context>

<specifics>
## Specific Ideas

- The target screenshots show compact top tabs, section headers with small
  icons/dividers, tight numeric fields with unit suffixes, and row groups that
  flow vertically in the main content area.
- The screenshots do not show a visible left group-navigation sidebar.
  Historical docs that expected left group navigation must be treated as older
  evidence to reconcile, not as the current visual truth.
- Top action controls in the screenshots read as compact icon/action affordances
  rather than the current wide search field and text-heavy Save/Save As row.
  The audit should map each current control to screenshot/upstream evidence
  before Phase 85 changes it.
- The settings UI has known mojibake debt in current visible strings. v4.1
  should use real target labels or source-mapped translated labels, never
  corrupted strings.
- Process settings must share the restored shell and source-truth semantics but
  should not be pixel-compared to a non-existent process screenshot.

</specifics>

<deferred>
## Deferred Ideas

- AssembleView remains a future source-truth milestone.
- D3D12 root-cause and Vulkan evaluation remain future backend work.
- Full upstream preset bundle import/export compatibility remains outside
  v4.1 unless explicitly reopened.
- Network/device/cloud/Monitor/ModelMall/live-camera workflows remain removed
  from forward scope unless the user explicitly reopens them.

</deferred>

---

*Phase: 84-Settings Source-Truth Gap Audit*
*Context gathered: 2026-07-07*
