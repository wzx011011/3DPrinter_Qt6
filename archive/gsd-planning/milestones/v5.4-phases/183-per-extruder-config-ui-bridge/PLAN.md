# Phase 183: Per-Extruder Config UI Bridge

**Status:** Planned
**Workstream:** FEAT
**Requirement:** FEAT-04
**Dependencies:** none (Wave A parallel)

## Goal

Bridge bb3's `ConfigOption*Nullable` types to the Qt6 UI. bb3 enabled per-extruder speed/acceleration/jerk settings via nullable vector types; Qt6 currently renders single values. Make the Qt6 UI aware of nullable fields and provide a per-extruder editor surface.

## Background

The 2026-07-19 bb3 sync (`edbca0aa55`) accepted upstream's `ConfigOptionFloatsNullable` / `ConfigOptionBoolsNullable` / `ConfigOptionFloatsOrPercentsNullable` types (these were previously rolled back in the OWzx fork). End-to-end slice tests confirmed bb3's engine handles these types correctly (Test 3 config overlay applied `layer_height` cleanly).

However, the Qt6 UI layer (`ConfigOptionModel`, `ConfigViewModel`) still treats these as single-value fields. For single-extruder printers this is fine (the nullable vector's element 0 = the value). For multi-extruder printers, users need to edit per-extruder values independently.

## Scope

### Data layer (183a candidate)

- **ConfigOptionModel**: detect nullable fields via `opt->nullable` flag (already read by Qt6 — see `ConfigOptionModel.cpp:366` and `ConfigViewModel.cpp:1917`). When nullable, expose:
  - A `perExtruderValues` role (list of values, one per extruder).
  - A `nullable` role (drives UI mode switch in OptionRow.qml).
- **ConfigViewModel**: add `setPerExtruderValue(key, extruderIdx, value)` Q_INVOKABLE; add `extruderCount` property (already exists upstream-mapped).

### UI layer (183b candidate)

- **OptionRow.qml**: when `nullable == true`, render a per-extruder editor (one row/column per extruder) instead of a single value field. Mirror upstream `ConfigManipulation.cpp` multi-extruder UI logic.
- **ParameterPage.qml** / **PrintSettings.qml**: ensure the per-extruder editor is reachable in the parameter panel.

### Mapping to upstream

Upstream `src/slic3r/GUI/ConfigManipulation.cpp` is the behavioral truth for multi-extruder UI:
- Which fields are per-extruder (speed/accel/jerk families).
- How the UI switches between single-value and per-extruder mode.
- How a "global" value propagates to all extruders vs. per-extruder overrides.

## Out of Scope

- H2C/A2L multi-nozzle device UI (separate milestone).
- Per-extruder printer calibration flows.
- Changes to libslic3r.

## Verification

- QmlUiAuditTests: add FEAT-04 anchor in Phase 187.
- Manual test: load a multi-extruder printer preset, verify speed/accel/jerk fields show per-extruder editor, edit per-extruder values, slice — G-code reflects per-extruder settings.
- Single-extruder regression: verify single-extruder presets still render the simple single-value editor (no UI regression).
- Slice test: owzx-cli with a multi-extruder preset + config overlay — per-extruder values applied correctly.
- Canonical build exits 0, 0 errors.

## Risk Notes

- **Estimated 800-1200 lines.** **Decision (2026-07-20): keep as single phase, no 183a/b split.** Implement data layer (ConfigOptionModel nullable-aware) + UI layer (OptionRow.qml per-extruder editor) together. Only revisit split if data layer alone exceeds 500 lines.
- Qt6 ConfigOptionModel's existing nullable flag (`opt->nullable`) is already wired — verify it's actually populated correctly after bb3 sync (the bb3 PrintConfig.cpp sets `def->nullable = true` on the right fields).
- Multi-extruder testing requires a multi-extruder preset fixture. Check if one exists in `tests/fixtures/` or if we need to create one.
