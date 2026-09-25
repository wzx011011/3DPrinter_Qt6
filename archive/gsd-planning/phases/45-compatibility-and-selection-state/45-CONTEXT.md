# Phase 45: Compatibility and Selection State - Context

**Gathered:** 2026-06-30
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 45 completes the compatibility and selection-state layer for printer, filament, and process presets. It builds on Phase 44's metadata and persisted selection foundation, adds upstream-aligned compatibility evaluation where local preset data supports it, repairs or blocks invalid selections, exposes compatibility/validation state to C++ viewmodels and QML bindings, and ensures incompatible choices cannot silently keep Prepare, Preview, or Export in a valid state.

This phase does not implement the full config editing dirty/reset model, Save/Save As lifecycle dialogs, CreatePresetsDialog, bundle UI, or final slice/project/CLI proof. Those remain Phase 46-49.

</domain>

<decisions>
## Implementation Decisions

### Compatibility Model
- Treat upstream OrcaSlicer `PresetBundle::update_compatible`, `Preset::is_compatible`, `PresetComboBoxes`, and `ConfigWizard` behavior as the source-truth target.
- Implement deterministic local compatibility in C++ using the preset data already loaded by `PresetServiceMock`: `compatible_printers`, nozzle diameter ranges, material/profile metadata, process/profile tags, and read-only/user metadata where available.
- If an upstream compatibility expression cannot be fully evaluated from local data yet, classify that path explicitly as `Hybrid` and fail toward visible warning/blocking state instead of silently accepting stale selections.
- Keep QML as presentation. Compatibility state, reasons, validation severity, and selection repair decisions live in C++ services/viewmodels.

### Selection Repair
- On printer changes, preserve the current filament/process selection only when it remains compatible.
- If the active filament or process becomes incompatible, repair to the first compatible preset in stable service order and expose the repair reason through viewmodel state.
- If no compatible preset exists, keep the old selection visibly invalid and block slice/preview/export rather than silently falling back to a stale or arbitrary preset.
- Persist repaired selections through the Phase 44 service selection API so restart state matches the visible UI state.

### Validation and Blocking
- Built-in/system/read-only presets remain protected from destructive actions and overwrite attempts; Phase 45 surfaces action blockers, while Phase 47 owns full Save As dialog parity.
- Validate selected printer, filament, and process existence, category correctness, compatibility, read-only action state, and basic upstream option definitions available from `libslic3r` schema.
- Incompatible or invalid combinations must produce a C++ validation result with severity, user-visible message, and affected preset names.
- Invalid compatibility state must invalidate or block stale Preview/export readiness where the current code can reach those viewmodel paths without waiting for Phase 49's full slice integration.

### User Feedback
- Use existing `BackendContext::postError` / notification path when user action causes an invalid or repaired preset state.
- Also expose deterministic properties/methods on `ConfigViewModel` so QML controls can disable actions and show compatibility reasons without embedding business rules.
- Prefer concise reason strings that are stable in tests; richer localized UI copy can be refined in later UI phases.

### Verification
- Use TDD against `ViewModelSmokeTests` for compatibility filtering, repair, blocking, action enablement, and validation messages.
- Add tests that changing printer repairs compatible filament/process selections or blocks when no compatible preset exists.
- Add tests that incompatible selections cannot be treated as valid current configuration.
- Run targeted ViewModel tests first, then the canonical repository verification command; if the canonical script still times out in existing E2E, record that separately without claiming full canonical pass.

### the agent's Discretion
- Internal data structures, helper method names, and exact reason-code strings may be chosen by the agent if they are deterministic, source-truth-mapped, and test-covered.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/services/PresetServiceMock.{h,cpp}` now owns category metadata, read-only/user classification, selection persistence, import/export validation, simple filament/printer compatibility helpers, and `findCompatibleFilament`.
- `src/core/viewmodels/ConfigViewModel.{h,cpp}` owns current printer/filament/process selection, merged config state, tier-aware save, selector lists, and the existing `isCurrentFilamentCompatible` / `isFilamentCompatible` entry points.
- `src/core/viewmodels/EditorViewModel.{h,cpp}` and `SliceService` contain Prepare/Slice readiness paths that Phase 45 can block only through existing reachable APIs; Phase 49 owns full merged-config integration.
- `src/qml_gui/panels/PrintSettings.qml`, `LeftSidebar.qml`, `FilamentPanel.qml`, and `PreparePage.qml` bind to preset lists and setters; QML should stay thin.
- `tests/ViewModelSmokeTests.cpp` already covers Phase 44 preset metadata, selection persistence, category mapping, and viewmodel smoke behavior.

### Established Patterns
- Service/viewmodel actions return booleans for blocked operations and expose `Q_PROPERTY`/`Q_INVOKABLE` state for QML.
- Tests instantiate `PresetServiceMock`, `ProjectServiceMock`, and `ConfigViewModel` directly.
- QSettings state in tests must use scoped organization/application identities and clean relevant keys.
- Phase closeout records summary, verification, and review artifacts in `.planning/phases/<phase>/`.

### Integration Points
- `PresetServiceMock::isFilamentCompatibleWithPrinter`, `findCompatibleFilament`, `setSelectedPresetForCategory`, and new Phase 45 APIs are the primary service layer.
- `ConfigViewModel::setCurrentPrinterPreset`, `setCurrentFilamentPreset`, `setCurrentPrintPreset`, `autoMatchFilament`, and action enablement methods are the primary viewmodel layer.
- `BackendContext` can be used for user-visible errors if a blocked action needs notification routing.

</code_context>

<specifics>
## Specific Ideas

- Add a compatibility result struct or deterministic service methods that expose `compatible`, `severity`, and `reason` for the current preset combination.
- Extend compatibility beyond the current nozzle-range-only filament check to include explicit `compatible_printers` and process/printer compatibility when preset data contains those fields.
- Add `compatibleFilamentPresetNames`, `compatiblePrintPresetNames`, `currentPresetCompatibilityMessage`, and `canUseCurrentPresetCombination` style APIs on `ConfigViewModel`.
- Ensure legacy wrong paths are removed: raw category integers, silent fallback to default, and accepting nonexistent preset names should be rejected or visibly classified.

</specifics>

<deferred>
## Deferred Ideas

- Full upstream expression parser parity is deferred unless Phase 45 can reuse local data safely; unsupported expression coverage must be classified as Hybrid/Blocked.
- Full dirty-state and reset semantics are Phase 46.
- Save/Save As, rename/delete dialogs, and unsaved-change prompts are Phase 47.
- CreatePresetsDialog and bundle UI are Phase 48.
- End-to-end slice/project/CLI invalidation proof is Phase 49.

</deferred>
