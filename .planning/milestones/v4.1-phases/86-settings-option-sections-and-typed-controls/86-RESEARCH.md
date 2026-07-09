# Phase 86 Research: Settings Option Sections And Typed Controls

## Current Qt Findings

- `OptionRow.qml` is the central renderer for settings rows in both
  `SettingsDialog.qml` and `LeftSidebar.qml`.
- It already supports bool, int, double, percent, enum, and string dispatch,
  and all writes route through `optionModel.setValue`.
- It currently renders section headers as text plus divider only, without the
  screenshot-style icon rail and compact title/divider rhythm.
- It uses a large switch for bool rows; screenshots show checkbox-like option
  rows.
- Numeric units currently use `optUnit()` only in some branches. The C++ model
  exposes `optSidetext()` from upstream and should be preferred.
- `ConfigOptionModel` already exposes `optMin`, `optMax`, `optStep`,
  `optReadonly`, `optIsDirty`, `optNullable`, `optIsVector`, and
  `optSidetext`.
- `SettingsDialog.qml` already passes `showGroupHeader`, `oGroup`, compact
  widths, and `valueSource` into `OptionRow`.

## Source Truth Anchors

- OrcaSlicer `Tab.*` builds settings pages from option groups and
  `append_single_option_line` / `append_option_line` calls.
- OrcaSlicer `OptionsGroup.*` is the source-truth concept for grouped settings
  sections with titles and option lines.
- `PrintConfig.cpp` defines option `sidetext`, min/max metadata, nullable flags,
  vector option types, and range-style options such as temperature ranges.

## Implementation Constraints

- Keep durable settings behavior in C++ viewmodels/models.
- Avoid changing `ConfigOptionModel` unless a required metadata point is missing.
- Maintain compatibility with `LeftSidebar.qml`, which uses narrower compact
  `OptionRow` dimensions.
- Use source audits for the visual contract and leave final screenshot evidence
  to Phase 88.

## Risks

- True multi-key paired editing may require a deeper data-model change. Phase 86
  should restore visual range affordances using existing metadata and avoid
  inventing hidden persistence behavior.
- Existing files contain mojibake comments and labels. New edits should be
  ASCII where practical and must pass the encoding guard.
- QML source audits can become token-name checks if too narrow. Tests should
  check meaningful implementation markers tied to UI behavior.
