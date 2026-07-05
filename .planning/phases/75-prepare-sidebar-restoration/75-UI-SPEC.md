---
phase: 75-prepare-sidebar-restoration
status: approved
approved_by: autonomous
requirements: [SIDE-01, SIDE-02, SIDE-03]
---

# Phase 75 UI Specification

## Design Intent

The Prepare sidebar should read as a compact OrcaSlicer-style work panel:
dense, mostly flat, visually secondary to the viewport, and free of raw
internal labels. The user should see printer, filament, and process preset
state as functional controls rather than decorative cards.

## Layout Contract

- Sidebar default width: clamp around the target screenshot ratio instead of a
  fixed 390 px at every window size.
- Minimum readable width: 312 px.
- Maximum width: 390 px.
- Expanded sidebar keeps 14 px top/bottom shell margins already used by
  `DockableSidebar`.
- Inner sidebar margins should be 4-6 px, not wide card spacing.
- Section vertical spacing should be 4-6 px.

## Section Contract

- Use a compact mode for Prepare sidebar sections.
- Compact sections use:
  - 28 px header height
  - 4 px radius or less
  - subtle border
  - `Theme.bgPanel`/transparent content treatment
  - 4 px content top gap
- Do not change `CollapsibleSection` globally for other pages unless guarded by
  a new compact property.

## Printer And Process Controls

- Printer and process preset combos remain functional and keep existing backend
  calls.
- Dirty dots remain visible but small.
- Action buttons remain icon-sized and honest: no visible disabled placeholders.
- Scope controls remain Global/Object/Plate and use compact segmented controls.

## Filament Controls

- Filament should render as compact slot rows, not large color cards.
- Each slot displays:
  - a small filament color swatch
  - 1-based slot number
  - current preset combo
  - compatibility warning affordance when incompatible
- Slot height target: 32-36 px.

## Process Option Rows

- Compact process rows target 28-30 px for bool/enum/numeric rows.
- Numeric fields use 22 px height and no oversized empty box.
- Value-source text must be localized:
  - `default` -> `默认`
  - `print` -> `打印`
  - `filament` -> `耗材`
  - `printer` -> `打印机`
- Option labels should use upstream labels when present; for common Prepare
  rows that remain English, map by option key to the existing Chinese UI copy.
- Search/category tab labels remain localized and compact.

## Interaction Contract

- Collapse/expand behavior remains unchanged.
- Resize drag remains unchanged, but clamps to the new min/max values.
- Preset changes, filament changes, scope changes, and settings entry points
  keep their existing viewmodel calls.
- No new no-op visible controls are introduced.

## Visual Acceptance

The Phase 75 visual pass is acceptable when the current Prepare page no longer
shows a broad rounded-card sidebar, large colored filament blocks, or raw
`default`/`print`/`filament` source labels in the inline option list.
