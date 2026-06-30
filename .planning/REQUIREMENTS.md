# Requirements: OWzx Slicer v3.5 Preset Authoring Complete Workflow

**Defined:** 2026-06-30
**Status:** Active - requirements defined, roadmap ready
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Scope Contract

v3.5 is not an MVP milestone. It is the complete preset authoring workflow for the local application:

```text
Load preset bundle -> select compatible printer/filament/process presets -> edit config -> save/create/import/export presets -> slice/export with the edited config
```

Every user-visible behavior in this path must either be implemented, verified, or explicitly classified as blocked by an unavailable dependency or upstream mismatch. Device sending, cloud printing, Monitor print jobs, AssembleView, and rendering backend promotion remain separate workflows.

v3.4 Phase 43 manual UAT is still pending because it cannot be run right now. v3.5 may proceed, but that pending UAT must remain visible and must not be marked complete without evidence.

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, credential, protocol, product decision, or upstream feature that is not locally available.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## v3.5 Requirements

### Preset Data and Persistence

- [ ] **PSET-01:** User can load printer, filament, and process presets from upstream-compatible system/vendor/user preset locations without relying on hardcoded mock-only defaults.
- [ ] **PSET-02:** User can see and select preset category metadata including built-in vs user preset, read-only state, inheritance parent, vendor/model/nozzle/material identity, and current dirty/modified state.
- [ ] **PSET-03:** User preset selections for printer, filament, and process persist through app settings and are restored on restart or normal project load where upstream semantics allow it.
- [ ] **PSET-04:** User can import and export preset bundles through a real service path with validation of category, format, duplicate names, and incompatible or corrupt entries.
- [ ] **PSET-05:** Missing, corrupt, unsupported, or version-incompatible preset files produce user-visible errors or warnings and cannot silently fall back to stale selections.
- [ ] **PSET-06:** Preset data is exposed through deterministic C++ service APIs used by `ConfigViewModel`, slicing, project restore, CLI, and existing calibration integration instead of divergent local copies.

### Compatibility and Validation

- [ ] **COMP-01:** User sees printer, filament, and process preset compatibility state aligned with upstream `PresetBundle::update_compatible` behavior for the local FFF workflow.
- [ ] **COMP-02:** Changing the selected printer updates compatible filament and process choices, preserves a valid selection where possible, and prompts or auto-matches only where upstream does.
- [ ] **COMP-03:** Incompatible or invalid preset combinations are clearly marked and cannot produce a silent stale slice, invalid Preview, or exportable G-code.
- [ ] **COMP-04:** Built-in/system presets are protected from destructive edits, rename, and delete; attempts route to Save As or produce a clear blocked action reason.
- [ ] **COMP-05:** Config value validation uses upstream option definitions for type, enum, min/max, nullable state, unit, and dependency rules where available.
- [ ] **COMP-06:** Preset validation warnings, compatibility warnings, and blocking errors are surfaced through the same notification/error system used by Prepare, Slice, and Export.

### Editing, Dirty State, and Reset

- [ ] **EDIT-01:** User can edit printer, filament, and process options through model-driven Qt controls that expose typed values, labels, units, choices, categories, and advanced/basic filtering.
- [ ] **EDIT-02:** Edited options update dirty state, modified option counts, modified suffix/display state, and value source metadata for the active preset tier.
- [ ] **EDIT-03:** User can reset one option, all modified options, or a scoped override back to the correct inherited/default level with the resulting value shown immediately.
- [ ] **EDIT-04:** User can inspect enough value-source and difference information to understand whether a value came from printer, filament, process, project, plate, object, volume, or default config.
- [ ] **EDIT-05:** Switching preset, page, scope, or project while unsaved edits exist prompts Save, Discard, or Cancel and never silently loses changes.
- [ ] **EDIT-06:** QML pages and dialogs do not own preset business rules; validation, dirty state, save routing, and reset semantics live in C++ services/viewmodels.

### Preset Lifecycle Actions

- [ ] **LIFE-01:** User can Save edits to a writable user preset and the saved values persist to disk and reload correctly.
- [ ] **LIFE-02:** User can Save As from a built-in or user preset with upstream-compatible name validation, duplicate handling, modified suffix handling, and category routing.
- [ ] **LIFE-03:** User can rename a user preset and dependent selections, current UI labels, project references, and compatibility state update coherently.
- [ ] **LIFE-04:** User can delete a user preset only when deletion is valid; current selections and dependent presets repair to a valid fallback or show a clear blocker.
- [ ] **LIFE-05:** User can compare or review modified preset values before saving when upstream would show an unsaved-changes/diff workflow.

### Create Presets and Bundle Workflows

- [ ] **CREATE-01:** User can create printer presets through a CreatePresetsDialog-equivalent workflow using vendor/model/variant/nozzle inputs supported by the local upstream bundle data.
- [ ] **CREATE-02:** User can create filament presets through a CreatePresetsDialog-equivalent workflow using vendor/material/profile inputs and compatible printer context.
- [ ] **CREATE-03:** User can create process presets with the correct printer technology and compatibility metadata for the selected printer/profile context.
- [ ] **CREATE-04:** Newly created presets appear immediately in the correct selector, can be selected, saved, reloaded, and used for slicing without restarting the app.
- [ ] **CREATE-05:** Create/import/export flows clearly classify unsupported vendor data, blocked cloud-only behavior, duplicate profiles, invalid names, and partial imports.

### Slice, Project, and CLI Integration

- [ ] **FLOW-01:** Slicing receives a merged FFF config aligned with upstream precedence for printer, filament, process, project, plate, object, volume, and layer-range overrides.
- [ ] **FLOW-02:** Any slice-affecting preset edit or preset selection change invalidates affected Prepare, Preview, and Export state immediately with a visible reason.
- [ ] **FLOW-03:** 3MF project import restores embedded config and preset matching without clobbering user preset files or hiding partial-match warnings.
- [ ] **FLOW-04:** 3MF project save/export preserves project-scoped preset/config data needed to reopen the project with equivalent local slicing behavior.
- [ ] **FLOW-05:** CLI slicing and UI slicing use the same preset service and merged-config semantics for supported local workflows.
- [ ] **FLOW-06:** Existing implemented calibration paths that read or write preset values continue to work with the real preset service or are explicitly classified as blocked/future.

### Verification and Handoff

- [ ] **VERIFY-01:** Automated tests cover preset load, inheritance, selection persistence, save, Save As, rename, delete, import, export, and reload.
- [ ] **VERIFY-02:** Automated tests cover compatibility filtering, validation failures, dirty state, reset, unsaved-change decisions, and blocked built-in preset edits.
- [ ] **VERIFY-03:** Automated tests prove edited presets affect merged slicing config, Prepare invalidation, generated G-code metadata where observable, and local export availability.
- [ ] **VERIFY-04:** QML/UI audits prove preset dialogs and config pages are bound to C++ APIs and no visible preset workflow remains placeholder-only.
- [ ] **VERIFY-05:** Manual UAT covers create/edit/save/reload/select/slice/export for printer, filament, and process presets, and records the still-pending v3.4 manual UAT separately if it remains unavailable.

## Future Requirements

- Device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- AssembleView source-truth completion.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip.
- Full PLATE-09 save/reload assertions after shared 3MF writer integration is fixed.
- D3D12 or Vulkan default promotion after separate backend feasibility and stability work.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond strings touched by this milestone.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Out of Scope

| Feature | Reason |
|---|---|
| Device send, upload, cloud print, and Monitor print-job workflow | Separate workflow after trustworthy local config and G-code output are in place. |
| Changing libslic3r slicing algorithms | GUI migration preserves libslic3r behavior. |
| Making D3D12 or Vulkan the default backend | D3D11 QRhi remains the known stable Qt-native Windows backend. |
| AssembleView, auto filament-map, and wipe-tower rendering | Separate multi-plate/product workflow, not preset authoring. |
| Claiming v3.4 manual UAT completion | User cannot verify it right now; it remains pending until evidence exists. |
| Cloud-only vendor account workflows | Preset authoring must work locally; cloud account workflows belong to future cloud/device milestones. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| PSET-01 | Phase 44 | Pending |
| PSET-02 | Phase 44 | Pending |
| PSET-03 | Phase 44 | Pending |
| PSET-04 | Phase 44 | Pending |
| PSET-05 | Phase 44 | Pending |
| PSET-06 | Phase 44 | Pending |
| COMP-01 | Phase 45 | Satisfied |
| COMP-02 | Phase 45 | Satisfied |
| COMP-03 | Phase 45 | Partial - Phase 45 exposes blocking state; Phase 49 must wire Slice/Preview/Export hard gating. |
| COMP-04 | Phase 45 | Satisfied |
| COMP-05 | Phase 45 | Partial - compatibility constraints covered; full config option validation remains Phase 46. |
| COMP-06 | Phase 45 | Partial - panel warning state covered; broader notification/error integration remains Phase 49. |
| EDIT-01 | Phase 46 | Pending |
| EDIT-02 | Phase 46 | Pending |
| EDIT-03 | Phase 46 | Pending |
| EDIT-04 | Phase 46 | Pending |
| EDIT-05 | Phase 46 | Pending |
| EDIT-06 | Phase 46 | Pending |
| LIFE-01 | Phase 47 | Pending |
| LIFE-02 | Phase 47 | Pending |
| LIFE-03 | Phase 47 | Pending |
| LIFE-04 | Phase 47 | Pending |
| LIFE-05 | Phase 47 | Pending |
| CREATE-01 | Phase 48 | Pending |
| CREATE-02 | Phase 48 | Pending |
| CREATE-03 | Phase 48 | Pending |
| CREATE-04 | Phase 48 | Pending |
| CREATE-05 | Phase 48 | Pending |
| FLOW-01 | Phase 49 | Pending |
| FLOW-02 | Phase 49 | Pending |
| FLOW-03 | Phase 49 | Pending |
| FLOW-04 | Phase 49 | Pending |
| FLOW-05 | Phase 49 | Pending |
| FLOW-06 | Phase 49 | Pending |
| VERIFY-01 | Phase 49 | Pending |
| VERIFY-02 | Phase 49 | Pending |
| VERIFY-03 | Phase 49 | Pending |
| VERIFY-04 | Phase 49 | Pending |
| VERIFY-05 | Phase 49 | Pending |

**Coverage:** 39 total; 39 mapped; 0 unmapped; 3 satisfied; 3 partial.

---

*Requirements defined: 2026-06-30*
*Last updated: 2026-06-30 after Phase 45 completion.*
