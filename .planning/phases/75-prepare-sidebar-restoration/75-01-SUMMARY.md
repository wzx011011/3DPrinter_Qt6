---
phase: 75-prepare-sidebar-restoration
plan: 75-01-sidebar-ui-restoration
status: complete
completed: 2026-07-05T18:45:00+08:00
requirements: [SIDE-01, SIDE-02, SIDE-03]
key_files:
  created:
    - .planning/phases/75-prepare-sidebar-restoration/75-01-SUMMARY.md
    - .planning/phases/75-prepare-sidebar-restoration/75-VERIFICATION.md
    - .planning/phases/75-prepare-sidebar-restoration/75-REVIEW.md
    - .planning/phases/75-prepare-sidebar-restoration/75-UI-REVIEW.md
  modified:
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp
    - src/qml_gui/pages/Plater.qml
    - src/qml_gui/pages/PreparePage.qml
    - src/qml_gui/panels/LeftSidebar.qml
    - src/qml_gui/components/CollapsibleSection.qml
    - src/qml_gui/components/FilamentSlot.qml
    - src/qml_gui/components/OptionRow.qml
    - tests/ViewModelSmokeTests.cpp
---

# Phase 75 Summary: Prepare Sidebar Restoration

## Completed

- Reduced Prepare sidebar width contract to `312..390` with default `328`.
- Added one-time migration from the pre-v3.9 default width `390` to the compact
  default while preserving user-set `390` as the new maximum after the settings
  version marker is written.
- Added compact mode to `CollapsibleSection` and applied it only from
  `LeftSidebar`.
- Tightened sidebar margins, section spacing, process topbar, search box, and
  inline parameter panel dimensions.
- Reworked filament slots from large color cards into compact rows with swatch,
  slot index, preset combo, and incompatibility warning affordance.
- Localized inline option value-source display keys and added common Prepare
  option-key label fallbacks.
- Updated sidebar width smoke tests for the new clamp/default contract and the
  legacy width migration path.

## Verification

- `git diff --check` passed.
- Encoding guard passed before staging.
- `QmlUiAuditTests` passed after restoring the required honest Phase 56 TODO
  marker for the still-disabled filament color picker.
- Canonical verifier passed:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- The built app launched successfully as `OWzx Slicer`; Windows reported the
  process responding.

## Residuals

- Windows Graphics Capture failed for this run with
  `SetIsBorderRequired failed: unsupported interface (0x80004002)`, so Phase 75
  visual evidence is source/test/runtime based. Full screenshot evidence remains
  in Phase 78.
- Object list, plate strip, slice action placement, viewport controls, and gizmo
  panels remain explicitly deferred to Phases 76 and 77.
