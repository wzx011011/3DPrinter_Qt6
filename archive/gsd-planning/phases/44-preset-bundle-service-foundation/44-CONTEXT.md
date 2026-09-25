# Phase 44: Preset Bundle Service Foundation - Context

**Gathered:** 2026-06-30
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 44 replaces mock-only preset assumptions with a deterministic C++ preset service aligned with upstream `PresetBundle`, `PresetCollection`, and `Preset` semantics. It owns service-level data, metadata, category identity, selection persistence, and import/export validation. It does not implement full UI dialogs, unsaved-change UI, CreatePresetsDialog flows, or final slice integration; those are Phase 45-49.

</domain>

<decisions>
## Implementation Decisions

### Service Boundary
- Keep the public class name `PresetServiceMock` for compatibility, but make Phase 44 behavior real/hybrid and explicitly service-owned.
- Use the existing enum mapping as the canonical local category mapping: `PrintCat = 0`, `FilamentCat = 1`, `PrinterCat = 2`; fix callers that used raw reversed values.
- Preserve existing `presetValues()` and category list APIs so current slicing, config, CLI, and calibration call sites remain source-compatible.
- Add structured metadata APIs rather than forcing QML or viewmodels to infer built-in/user/category/inheritance state from names.

### Persistence and Import/Export
- Persist selected printer/filament/print preset names through `QSettings` in the service because upstream `AppConfig` owns preset selections.
- Keep import/export local and deterministic for Phase 44 using validated JSON with category and metadata fields; upstream archive/zip UI parity belongs to Phase 48.
- Export user/custom presets by default and include system presets only when the service format needs them for deterministic test fixtures.
- Reject malformed imports without mutating existing presets.

### Upstream Alignment
- Treat OrcaSlicer `Preset` fields `is_default`, `is_system`, `is_user()`, `inherits`, `vendor`, and `setting_id` as the metadata model to mirror locally.
- Treat vendor-loaded presets as built-in/system/read-only; user-created/imported presets are writable unless explicitly marked read-only.
- Preserve inherited parent names loaded from vendor JSON and expose them through metadata.
- Leave full expression-condition compatibility to Phase 45; Phase 44 only preserves the data needed for that work.

### Verification
- Use TDD against `ViewModelSmokeTests` for category mapping, metadata, selection persistence, and import/export validation.
- Run targeted tests first, then the canonical repository verification command before claiming Phase 44 completion.

### the agent's Discretion
- Internal helper names, JSON field names, and error-string wording may be chosen by the agent if they remain deterministic and testable.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/services/PresetServiceMock.{h,cpp}` already loads builtin fallback presets, attempts vendor profile loading, resolves inheritance, and exposes category-aware lists.
- `src/core/viewmodels/ConfigViewModel.{h,cpp}` already merges printer/filament/print preset values into global config, but currently uses raw category integers incorrectly in several places.
- `src/qml_gui/Models/PresetListModel.cpp` already uses the enum constants correctly when populating the selector list.
- `tests/ViewModelSmokeTests.cpp` already contains preset/config smoke tests and is the right first regression target.

### Established Patterns
- Services expose `Q_INVOKABLE` where QML needs access, and tests instantiate services/viewmodels directly.
- Service errors are currently reported through boolean returns and `qWarning`; Phase 44 can stay with that pattern and reserve richer user notifications for later UI phases.
- Planning artifacts use phase-numbered files under `.planning/phases/<phase-slug>/` and commit after context/plan/summary transitions.

### Integration Points
- `ConfigViewModel::loadDefault`, `printerPresetNames`, `printPresetNames`, `deletePreset`, and reset helpers need enum-based category fixes.
- `CliRunner` already uses enum constants for default printer/filament/print config and should remain compatible.
- `CalibrationViewModel` reads filament defaults by raw `1`, which matches `FilamentCat` and should remain stable.

</code_context>

<specifics>
## Specific Ideas

- Start with a regression test proving `ConfigViewModel` selects a printer preset containing `nozzle_diameter`, a filament preset containing `nozzle_temp`, and a print preset containing `layer_height`.
- Add tests that built-in presets report read-only metadata and that a custom preset reports writable/user metadata.
- Add tests that importing malformed JSON fails without adding a preset.
- Add tests that selected presets persist through a new `PresetServiceMock` instance after using a test-specific `QSettings` scope.

</specifics>

<deferred>
## Deferred Ideas

- Full upstream compatibility-expression evaluation is Phase 45.
- Full config editing dirty/reset UI is Phase 46.
- SavePresetDialog/UnsavedChangesDialog parity is Phase 47.
- CreatePresetsDialog and upstream bundle UI parity is Phase 48.
- Slice/project/CLI end-to-end proof is Phase 49.

</deferred>
