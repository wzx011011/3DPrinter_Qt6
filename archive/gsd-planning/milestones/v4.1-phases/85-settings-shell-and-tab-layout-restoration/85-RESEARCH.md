# Phase 85 Research: Settings Shell And Tab Layout Restoration

**Date:** 2026-07-07
**Mode:** inline GSD fallback, autonomous

## Inputs Read

- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/phases/85-settings-shell-and-tab-layout-restoration/85-CONTEXT.md`
- `.planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md`
- `shotScreen/打印机参数设置页.png`
- `shotScreen/材料参数设置页.png`
- `src/qml_gui/dialogs/SettingsDialog.qml`
- `src/qml_gui/main.qml`
- `src/qml_gui/panels/LeftSidebar.qml`
- `src/qml_gui/controls/CxIconButton.qml`
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

## Source-Truth Findings

Phase 84 already froze the canonical region map. Phase 85 owns these regions:

- `SET-SHELL`: independent 736x593 non-modal settings windows, native title,
  right-side scrollbar, and no visible left group navigation.
- `SET-PRESET-ACTIONS`: preset combo on the left and compact action/icon
  affordances on the right.
- `SET-TABS`: screenshot-matched printer/material tab text/order; process uses
  the same shell and upstream print-process tab parity.
- `SET-ENTRYPOINTS`: existing Prepare/sidebar dispatch through
  `BackendContext::forwardSettingsRequest(category)` and `main.qml`
  `settingsRequested(category)` remains the entry behavior.
- `SET-SEARCH-MODE`: keep filtering semantics, but make search and advanced
  mode compact in the shell.
- `SET-CLEANUP`: remove or hide off-design shell surfaces and visible mojibake
  in this restored settings window.

## Current Qt Findings

- `SettingsDialog.qml` already provides the correct structural base:
  `ApplicationWindow`, `Qt.NonModal`, 736x593 size, three preset tiers,
  dirty-close guard, `SavePresetDialog`, `UnsavedChangesDialog`, and filtering
  through `ConfigViewModel`/`ConfigOptionModel`.
- The current visible shell diverges from the screenshots:
  - title and tab labels contain mojibake;
  - search is always visible as a wide `CxTextField`;
  - `Save` and `Save As...` are text buttons in the top row;
  - there is an extra in-row close button duplicating native close;
  - group filtering state remains in the dialog even though the screenshot has
    no visible left group navigation.
- `main.qml` already has the correct three independent `SettingsDialog`
  instances and dispatches `printer`, `filament`, `print`, and `process`.
- `LeftSidebar.qml` already opens printer/material/process settings from
  compact settings icons via `backend.forwardSettingsRequest(...)`.
- `CxIconButton.qml` supports existing icon assets and tooltips. Available
  assets include `settings.svg`, `device-floppy.svg`, `dots.svg`,
  `list-details.svg`, `printer.svg`, and `x.svg`. No new icon system is needed.
- `tests/QmlUiAuditTests.cpp` contains old Phase 56 tests that currently lock
  `GroupNavSidebar`/selected-group behavior. Phase 85 must update that audit
  because the left group sidebar is explicitly off-design for the restored
  screenshots.
- `ViewModelSmokeTests.cpp` already covers the backend signal path and active
  tier update for settings open behavior. Phase 85 can rely on those smoke tests
  unless implementation changes backend dispatch.

## Implementation Implications

- Patch `SettingsDialog.qml` in place rather than replacing it wholesale; it
  already carries important Phase 56 behavior.
- Keep all behavior routed through existing QML properties and C++ viewmodel
  APIs; do not move save/search/filter business logic into QML beyond existing
  presentation wiring.
- Replace visible shell affordances, not Phase 86 row internals.
- Update source audits before/with the QML change so the old off-design group
  navigation invariant cannot keep the wrong UI alive.

## Deferred Research

Runtime pixel screenshots, full canonical build, and launched-app evidence are
owned by Phase 88 unless Phase 85 uncovers a compile/runtime blocker.
