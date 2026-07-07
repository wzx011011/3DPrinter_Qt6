# Phase 86 UI-SPEC: Settings Option Sections And Typed Controls

**Status:** Approved for planning
**Date:** 2026-07-07
**Scope:** Option content region inside printer/material/process settings dialogs.

## Visual Contract

- The option pane remains a dense dark operational surface, not a card layout.
- Section headers use a compact cyan icon/glyph at the left, a bold title, and
  a divider line extending through the row.
- Regular rows use fixed-height compact layouts. Metadata indicators do not
  change row height after hover, dirty, read-only, nullable, or vector states
  appear.
- Label, metadata, editor, and unit columns must have stable widths in both the
  settings dialog and the narrower Prepare sidebar consumer.
- Numeric editor frames use dark inset surfaces, one-pixel borders, right
  aligned values, and adjacent unit text.
- Bool rows use checkbox-style affordances matching the screenshots rather than
  large toggles in the option list.
- Enum rows use compact combo boxes.
- String rows use compact text field/area controls; color-like string rows add a
  swatch/button affordance.
- Paired min/max range visuals use a single horizontal control cluster with
  labels such as "Min" and "Max", two numeric fields, and unit text.

## Interaction Contract

- Every visible editor remains actionable unless the option is read-only.
- All editor writes call `optionModel.setValue(root.optIdx, value)`.
- Read-only rows keep value visibility and reduce only interaction affordance.
- Dirty rows show a fixed-position marker and keep the existing value.
- Nullable/inherit and vector/per-extruder states show compact badges in the
  metadata lane.
- Numeric rows clamp through current min/max metadata before writing.
- Search/mode filtering remains owned by the existing `SettingsDialog.qml`
  and `ConfigViewModel` path.

## Design System Contract

- Use `Theme` tokens for colors, spacing, borders, and typography.
- Use `CxCheckBox`, `CxSpinBox`, `CxComboBox`, `CxTextField`, `CxTextArea`, and
  `CxIconButton` where applicable.
- Do not add raw Qt Quick Controls declarations for visible editors unless no
  project control exists and the exception is audited.
- Do not introduce broad global theme changes.
- Do not add nested cards inside the settings pane.

## Verification Contract

- A QML source audit must fail on the old Phase 85 `OptionRow.qml` before the
  Phase 86 implementation.
- The audit must lock section headers, typed control dispatch, sidetext/unit
  usage, range/min-max visuals, color-like row affordance, and fixed metadata
  indicators.
- Targeted `QmlUiAuditTests` and relevant settings smoke tests must pass.
- The canonical verifier must pass before Phase 86 is marked complete.

## Non-Goals

- Final screenshot evidence and pixel comparison; Phase 88 owns that.
- Preset save/reset/discard semantic redesign; Phase 87 owns that.
- New model-level multi-key range editing beyond the current option value path.
