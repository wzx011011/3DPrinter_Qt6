# Phase 81 UI-SPEC: Preview Layer Move And Playback Controls

## Scope

This UI contract covers only the Preview far-right layer rail, bottom
move/playback rail, and compact camera controls.

## Layout

- The far-right rail stays at the existing narrow Preview edge width.
- The rail shows top and bottom layer values, a compact vertical range slider,
  and small current-layer step affordances.
- The bottom move rail stays inside the existing 50 px move bar and must not
  change the viewport or side-panel layout dimensions.
- Camera controls stay in the Preview header next to view-mode controls.

## Controls

- Layer rail controls call `PreviewViewModel` methods:
  `setLayerRange`, `jumpToLayer`, and `moveLayerRange`.
- Move rail controls call `stepCurrentMove`, `setCurrentMove`, and
  `togglePlayPause`.
- Camera fit calls an explicit RHI viewport fit method backed by Preview data.
- Controls are disabled when the relevant `PreviewViewModel` counts are zero.

## Visual Style

- Dark, compact, work-focused styling matching the existing Preview panels.
- No marketing copy, help text blocks, decorative gradients, or nested cards.
- Button text must fit in the 50 px move bar and 38 px layer rail.
- Use readable localized labels and tooltips; do not expose raw internal names.

## Interaction States

- Disabled controls are visibly dimmed and cannot mutate state.
- Hover states use the existing Theme colors.
- Keyboard shortcuts and button clicks must share ViewModel behavior.
- Dragging layer/move controls must not clear Preview payload state.

## Verification

- Source-audit tests must lock required bindings and QML method calls.
- ViewModel tests must prove layer/move clamp and current-line synchronization.
- Phase 83 will provide runtime visual evidence against the target screenshot.
