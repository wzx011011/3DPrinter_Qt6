# Phase 46: Config Editing, Dirty State, and Reset Semantics - Context

**Gathered:** 2026-06-30
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 46 replaces the current partial config-editing implementation with an upstream-aligned editing model for printer, filament, and process presets. The phase covers typed option editing, per-tier dirty tracking, value-source reporting, reset semantics, scope override behavior, and unsaved-change decisions for preset/scope/page/project transitions.

This phase does not finish Save/Save As/rename/delete preset lifecycle parity, create-preset dialogs, preset-bundle UI, or the final slice/export proof. Those remain Phase 47-49. Phase 46 must, however, establish the correct C++ editing semantics those later phases depend on.

</domain>

<decisions>
## Implementation Decisions

### Dirty and Modified Semantics
- Treat upstream `PresetCollection::current_is_dirty`, `saved_is_dirty`, `current_dirty_options`, `Preset::label`, and `Tab::update_dirty()` as the source-truth target.
- Replace the current default-value-based dirty model with a tier-aware edited-vs-reference model:
  - current dirty = edited active tier config vs currently selected preset config for that tier;
  - saved dirty = edited config vs last saved snapshot where needed for unsaved/save routing;
  - modified suffix, modified counts, and modified option lists must be derived from these tier-aware comparisons, not from schema defaults.
- Keep QML presentation-only. QML may display dirty badges, counts, and diff lists, but the comparison logic lives in C++ viewmodels/services.

### Reset and Value Source Semantics
- `reset one` and `reset all` for preset editing must restore the selected preset value of the active tier, not blindly reset to schema defaults.
- Explicit reset-to-level/value-chain behavior remains available for inherited printer/filament/print/default layers, but it must no longer mutate saved snapshots in a way that clears dirty state incorrectly.
- Scope override reset must restore the effective inherited value beneath the scope:
  - plate override -> merged global preset value;
  - object/volume override -> merged global preset value plus any outer-scope override if upstream precedence requires it;
  - layer-range override -> object/volume effective value beneath the layer range.
- Value-source APIs must distinguish preset inheritance levels from project/plate/object/volume/layer-range override levels so later dialogs and review UIs can explain where a value came from.

### Unsaved-Change Decision Flow
- Treat upstream `Tab::may_discard_current_dirty_preset()` and `UnsavedChangesDialog` as the source-truth target for decision flow.
- Preset/scope/page/project transitions that would discard dirty edits must be guarded in C++, not only by ad hoc QML dialogs.
- The C++ layer should expose a deterministic decision flow equivalent to Save / Discard / Cancel, with enough pending-action state for QML dialogs to render the choice and then resume or abort the requested transition.
- Existing direct setter paths that silently replace dirty edits on preset change are not acceptable and must be replaced or wrapped by guarded transition APIs.

### Option Model and Validation
- Reuse the existing schema-driven `ConfigOptionModel` and upstream option metadata pipeline instead of inventing new QML-local option definitions.
- The current `ConfigOptionModel` dirty tracking is only valid as a generic "value differs from model default" helper; Phase 46 must either replace it for preset editing or feed it a correct tier reference so UI dirty markers align with upstream semantics.
- Keep validation/type enforcement in C++. QML controls should continue to bind to typed ranges, labels, units, choices, and readonly state without owning validation rules.

### Scope Safety and Migration Rule
- Preserve the user's standing rule for this milestone and future milestones: when the old implementation does not satisfy the required behavior, replace it instead of layering more compatibility hacks on top.
- Phase 46 therefore favors deleting or bypassing wrong local editing semantics over preserving them for continuity.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/viewmodels/ConfigViewModel.{h,cpp}` already owns preset selection, merged config values, scope activation, option search/filter helpers, value-chain reporting, and reset entry points. It is the right place to centralize dirty/unsaved/reset rules.
- `src/qml_gui/Models/ConfigOptionModel.{h,cpp}` already exposes typed option metadata, values, categories, pages, search helpers, and schema loading for print/machine/filament tabs.
- `src/core/services/PresetServiceMock.{h,cpp}` already provides preset metadata, category ownership, persistence, compatibility filtering, and save/create/rename/delete primitives needed by later preset lifecycle work.
- `src/qml_gui/dialogs/UnsavedChangesDialog.qml` and `SavePresetDialog.qml` already exist as placeholder Qt dialogs and can be kept as presentation shells if their business logic is moved into C++.
- `src/qml_gui/panels/PrintSettings.qml` and `src/qml_gui/pages/SettingsPage.qml` already expose dirty summaries, reset actions, save buttons, value-chain popups, and scope panels. These are the primary QML integration points.

### Established Patterns
- Viewmodel/service logic is expected to stay in C++ with `Q_PROPERTY` + `Q_INVOKABLE` APIs; QML is presentation wiring only.
- Tests in `tests/ViewModelSmokeTests.cpp` already cover preset foundation, compatibility selection, tier-aware save filtering, and direct `ConfigViewModel` behavior. Phase 46 should extend this file rather than creating a new test harness.
- Scope edits currently flow through `ConfigViewModel::handleOptionValueChanged()` into `ProjectServiceMock` scoped override storage, then back through `applyScopeValues()`.
- The canonical build/test contract remains the single repository verification script `scripts/auto_verify_with_vcvars.ps1`, with targeted `ViewModelSmokeTests` runs used for fast iteration.

### Integration Points
- `ConfigViewModel::isPresetDirty()`, `globalModifiedCount()`, `globalModifiedKey()`, `resetGlobalOption()`, `resetAllGlobalOptions()`, `resetOptionToLevel()`, `valueChainForKey()`, and `applyScopeValues()` are the current editing/dirty/reset surface and need semantic correction.
- `ConfigOptionModel::setValue()`, `optIsDirty()`, `dirtyCount()`, `resetOption()`, `resetToDefaults()`, and `applyValues()` currently compare against model defaults, which is the main local semantic gap.
- `PrintSettings.qml` contains a direct `printOptions.resetToDefaults()` path and direct preset setters (`setCurrentPrinterPreset`, `setCurrentPrintPreset`, `setCurrentFilamentPreset`) that bypass any unsaved-change guard.
- `SettingsPage.qml` instantiates unsaved/save dialogs, but those dialogs are not wired into the actual preset switching flow used by the settings panels/sidebar.
- Upstream references are concentrated in:
  - `third_party/OrcaSlicer/src/libslic3r/Preset.{hpp,cpp}`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Tab.{hpp,cpp}`
  - `third_party/OrcaSlicer/src/slic3r/GUI/ConfigManipulation.hpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.{hpp,cpp}`

</code_context>

<specifics>
## Specific Ideas

- The current implementation has several concrete semantic mismatches that Phase 46 should correct directly:
  - `ConfigViewModel::isPresetDirty()` compares merged global values against `savedPresetValues_`, while `mergePresetHierarchy()` resets `savedPresetValues_` whenever presets change; this is not equivalent to upstream selected-vs-edited dirty tracking.
  - `ConfigViewModel::globalModifiedCount()` and related diff helpers compare against schema defaults instead of the selected preset or parent preset.
  - `ConfigOptionModel::dirtyCount()` and `optIsDirty()` are also default-based, so current QML dirty badges are wrong for preset authoring.
  - `resetGlobalOption()` and `resetAllGlobalOptions()` reset to schema defaults, not to the last saved preset values.
  - `resetOptionToLevel()` currently writes into `savedPresetValues_`, which can erase dirty state rather than only changing the edited value/reference.
  - `PrintSettings.qml` still has a direct `printOptions.resetToDefaults()` button that bypasses `ConfigViewModel`.
  - Existing preset setters are called directly from QML without any unsaved-change guard, so Save/Discard/Cancel is not enforced on real transitions.
- A practical implementation direction is to introduce an explicit edited-tier snapshot/reference model in `ConfigViewModel` and teach `ConfigOptionModel` about reference values or externally supplied dirty keys rather than relying on defaults.
- Another practical direction is to add guarded transition APIs such as "request switch preset/scope/page" plus "resolve pending unsaved decision" so QML dialogs remain thin and the state machine is testable in C++.

</specifics>

<deferred>
## Deferred Ideas

- Full Save/Save As dialog parity, name validation, rename/delete flows, and unsaved diff review UX remain Phase 47.
- CreatePresetsDialog-equivalent workflows and preset bundle UI remain Phase 48.
- Final Prepare/Preview/Export invalidation proof and slice/project/CLI merged-config verification remain Phase 49.
- Rich notification-center integration for every validation/error path can stay lightweight here as long as the C++ APIs expose deterministic blocking/warning state for later phases to consume.

</deferred>
