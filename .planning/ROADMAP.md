# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Automated verification passed, manual UAT deferred: **v3.4 Import to G-code Complete Workflow** - Phases 37-43
- Active: **v3.5 Preset Authoring Complete Workflow** - Phases 44-49

## Active Milestone: v3.5 Preset Authoring Complete Workflow

**Goal:** Complete the source-truth-aligned preset authoring workflow so users can load, select, edit, validate, save, create, import/export, and apply printer, filament, and process presets through the Qt UI, with the resulting configuration feeding Prepare, Slice, Preview, Export, and CLI paths.

**Success criteria:**
- A user can load upstream-compatible printer, filament, and process presets and understand built-in/user, inheritance, compatibility, and dirty state.
- A user can edit typed config options, see modified values and value sources, reset changes, and avoid silent loss through unsaved-change prompts.
- A user can Save, Save As, rename, delete, create, import, and export presets through real C++ service paths, not placeholder QML-only flows.
- Preset compatibility and validation prevent invalid combinations from producing stale Preview/export state or silent bad G-code.
- Edited presets drive Prepare invalidation, SliceService merged config, generated G-code behavior where observable, local export availability, and CLI slicing.
- Automated and manual UAT cover the complete preset-authoring-to-slice workflow before the milestone is marked complete.

## Phases

- [ ] **Phase 44:** Preset Bundle Service Foundation
- [ ] **Phase 45:** Compatibility and Selection State
- [ ] **Phase 46:** Config Editing, Dirty State, and Reset Semantics
- [ ] **Phase 47:** Preset Lifecycle Actions
- [ ] **Phase 48:** Create Presets and Bundle Workflows
- [ ] **Phase 49:** Slice Integration, Verification, and Handoff

### Phase 44: Preset Bundle Service Foundation

**Goal:** Replace mock-only preset assumptions with a deterministic C++ preset service aligned with upstream `PresetBundle`, `PresetCollection`, and `Preset` semantics.

**Requirements:** `PSET-01`, `PSET-02`, `PSET-03`, `PSET-04`, `PSET-05`, `PSET-06`.

**Deliverables:**
- Source-truth comparison against upstream `libslic3r/Preset.*`, `libslic3r/PresetBundle.*`, and Qt local `PresetServiceMock`.
- Service model for printer, filament, and process presets, including category, inheritance, built-in/user, read-only, vendor/model/nozzle/material metadata, and selected state.
- Real load path for upstream-compatible system/vendor/user presets with deterministic fallback classification.
- Real import/export service contract or explicit source-truth-compatible local format when upstream archive behavior cannot be used directly.
- Failure and diagnostics path for missing/corrupt/version-incompatible preset data.

**Success criteria:**
1. Preset lists are populated from real upstream-compatible data when available and never silently degrade to stale mock defaults.
2. Preset metadata and selection state are visible through C++ APIs used by `ConfigViewModel`.
3. Preset selections persist across restart or simulated app reload in deterministic tests.
4. Bundle import/export validates data and reports failures without corrupting existing presets.

### Phase 45: Compatibility and Selection State

**Goal:** Make printer, filament, and process selection behavior match upstream compatibility expectations for the local FFF workflow.

**Requirements:** `COMP-01`, `COMP-02`, `COMP-03`, `COMP-04`, `COMP-05`, `COMP-06`.

**Deliverables:**
- Source-truth comparison against upstream `PresetBundle::update_compatible`, `PresetComboBoxes`, `ConfigWizard`, and related selection flows.
- Compatibility model for the current printer, filament, and process selection.
- Selection repair/auto-match behavior for printer changes, with clear user-facing reasons for blocked or incompatible choices.
- Validation coverage for option type, enum, min/max, nullable state, unit, and dependency constraints.
- Read-only and built-in preset protection surfaced through action enablement and errors.

**Success criteria:**
1. Changing printer updates compatible filament/process options and preserves or repairs selections according to documented upstream mapping.
2. Incompatible combinations cannot produce a silent stale slice, invalid Preview, or exportable result.
3. Built-in/system presets are protected from destructive actions.
4. Validation warnings and blocking errors are visible through the normal notification/error path.

### Phase 46: Config Editing, Dirty State, and Reset Semantics

**Goal:** Make configuration editing model-driven, type-aware, and safe under preset/scope changes.

**Requirements:** `EDIT-01`, `EDIT-02`, `EDIT-03`, `EDIT-04`, `EDIT-05`, `EDIT-06`.

**Deliverables:**
- Source-truth comparison against upstream config tabs, `ConfigManipulation`, option metadata, and unsaved-change triggers.
- C++ model support for typed option values, labels, units, choices, categories, search, and basic/advanced filtering.
- Dirty-state and modified-option tracking per active preset tier.
- Value-source and difference APIs for inherited/default/project/plate/object/volume/layer-range values.
- Reset-one, reset-all, and reset-scope override behavior.
- Unsaved-change decision flow for switching preset, scope, page, or project.

**Success criteria:**
1. Editing options updates C++ state, modified counts, displayed values, and dirty indicators immediately.
2. Reset operations restore the correct inherited/default/scoped value and refresh UI models.
3. Switching context with unsaved edits prompts Save, Discard, or Cancel and cannot silently lose changes.
4. QML config pages contain presentation wiring only; business rules remain in C++.

### Phase 47: Preset Lifecycle Actions

**Goal:** Complete the user preset lifecycle: Save, Save As, rename, delete, and diff/unsaved review.

**Requirements:** `LIFE-01`, `LIFE-02`, `LIFE-03`, `LIFE-04`, `LIFE-05`.

**Deliverables:**
- Source-truth comparison against upstream `SavePresetDialog`, `UnsavedChangesDialog`, and preset delete/rename behavior.
- Save path for writable user presets with disk persistence and reload verification.
- Save As path for built-in or user presets with name validation, duplicate handling, category routing, and modified suffix handling.
- Rename and delete behavior with dependent selection repair and clear blocker reasons.
- Difference/review data for modified preset values before saving.

**Success criteria:**
1. Saving a writable user preset persists edited values and reloads them correctly.
2. Saving from a built-in/read-only preset routes to Save As or reports an actionable blocker.
3. Rename/delete update selectors, active selections, compatibility state, and dependent references coherently.
4. Unsaved/diff information is sufficient for the user to decide whether to save or discard changes.

### Phase 48: Create Presets and Bundle Workflows

**Goal:** Implement CreatePresetsDialog-equivalent creation plus real preset bundle import/export UI.

**Requirements:** `CREATE-01`, `CREATE-02`, `CREATE-03`, `CREATE-04`, `CREATE-05`.

**Deliverables:**
- Source-truth comparison against upstream `CreatePresetsDialog`, `ConfigWizard`, `PresetBundleDialog`, and `ExportPresetBundleDialog`.
- Qt dialog/viewmodel flow for creating printer presets from vendor/model/variant/nozzle inputs available in local preset data.
- Qt dialog/viewmodel flow for creating filament and process presets with compatibility metadata.
- Bundle import/export UI backed by Phase 44 service APIs, including duplicate and partial-import handling.
- User-visible classification of unsupported vendor data, blocked cloud-only behavior, invalid names, and partial imports.

**Success criteria:**
1. Newly created printer, filament, and process presets appear immediately in selectors and survive reload.
2. Created presets can be selected and used for a real local slice without app restart.
3. Import/export flows no longer operate as placeholder-only QML paths.
4. Unsupported or partial preset data is classified and reported without corrupting existing presets.

### Phase 49: Slice Integration, Verification, and Handoff

**Goal:** Prove edited presets drive the real local slicing/export workflow and regression-guard the milestone.

**Requirements:** `FLOW-01`, `FLOW-02`, `FLOW-03`, `FLOW-04`, `FLOW-05`, `FLOW-06`, `VERIFY-01`, `VERIFY-02`, `VERIFY-03`, `VERIFY-04`, `VERIFY-05`.

**Deliverables:**
- Source-truth comparison against upstream `PresetBundle::full_fff_config`, `BackgroundSlicingProcess`, project config restore/save, and CLI preset usage.
- Merged-config integration for UI slicing, export, project import/save, per-plate/object/volume/layer overrides, and CLI slicing.
- Slice invalidation hooks for every relevant preset selection or edit.
- Automated tests for preset lifecycle, compatibility, dirty/reset state, merged slicing config, import/export, QML binding, and local slice/export output.
- Manual UAT checklist for create/edit/save/reload/select/slice/export.
- Handoff note that preserves v3.4 manual UAT status if still unavailable.

**Success criteria:**
1. Edited preset values reach `SliceService` through the same merged-config path used by UI and CLI workflows.
2. Preset edits and selection changes invalidate stale Prepare, Preview, and Export state immediately.
3. 3MF project load/save preserves the config data needed for equivalent local slicing behavior.
4. Canonical verification passes with preset authoring coverage.
5. Manual UAT covers the complete preset-authoring-to-G-code flow before v3.5 is marked complete.

## Deferred Backlog

- v3.4 Phase 43 manual UAT for import -> Prepare -> slice -> Preview -> export remains pending until the user can verify it.
- Device send/upload/cloud print and Monitor task workflow.
- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion.
- ModelMall/Home WebView and cloud workflows.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Next Step

Start Phase 44:

```text
$gsd-plan-phase 44
$gsd-execute-phase 44
```

---

*Last updated: 2026-06-30 for v3.5 milestone planning.*
