---
phase: 226
slug: source-mapped-process-hierarchy
status: draft
shadcn_initialized: false
preset: none
created: 2026-08-03
---

# Phase 226 - UI Design Contract

> Visual and interaction contract for the FFF Process Settings dialog hierarchy.
> This contract covers HIER-01 and HIER-02 only.

---

## Scope And Source Truth

The Process tier remains the existing non-modal `SettingsDialog` window. It is
not a new settings surface and it does not gain a navigation sidebar. The
source of truth is `TabPrint::build()` in
`third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2005-2376`.

The Process page strip MUST use these technical page keys and this exact visual
order:

1. Quality
2. Strength
3. Speed
4. Support
5. Multimaterial
6. Others

`Base`, `Cooling`, `Retraction`, and singular `Other` are not Process pages.
They MUST NOT appear as Process tabs. Page keys and group keys are untranslated
technical identities; their visible labels use `qsTr()` and may be localized.

The ordered group manifest below is the page-level visual contract. Within each
group, option membership and option order come from the corresponding upstream
`append_single_option_line` sequence. A supported Process key that is absent
from that source-derived manifest is a mapping defect, not a candidate for an
alphabetical, category, or `Others` fallback.

| Process page | Ordered groups |
|--------------|----------------|
| Quality | Layer height; Line width; Seam; Precision; Ironing; Wall generator; Walls and surfaces; Bridging; Overhangs |
| Strength | Walls; Top/bottom shells; Infill; Advanced |
| Speed | Initial layer speed; Other layers speed; Overhang speed; Travel speed; Acceleration; Jerk(XY); Advanced |
| Support | Support; Raft; Support filament; Advanced; Tree supports |
| Multimaterial | Prime tower; Filament for Features; Ooze prevention; Flush options; Advanced |
| Others | Skirt; Brim; Special mode; G-code output; Post-processing Scripts; Notes |

Phase 226 supplies only the source-correct page and ordered group projection.
It must preserve the existing ConfigViewModel and ConfigOptionModel value path.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none; Qt Quick Controls 2 Basic plus existing Cx controls |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic, `CxComboBox`, `CxIconButton`, `CxTextField`, `CxSwitch`, and `OptionRow` |
| Icon library | existing qrc SVG icon set only; no new icon package |
| Font | existing Qt application UI font; do not introduce a Process-specific font |

There is no React, Vite, Next.js, Tailwind, shadcn, or third-party registry in
this application. The shadcn initialization and registry safety gates are not
applicable.

---

## Spacing Scale

Every Phase 226 addition and modified layout uses only 4px-grid values through
existing `Theme` tokens or fixed values. Do not add hard-coded spacing or sizing
literals outside that grid to the Process branch. Unchanged host-shell spacing
may retain its current tokens, including `Theme.spacingSM` (6px); that exception
does not apply to any Phase-owned layout.

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | Tab-strip and list micro-gaps; separator clearance |
| md | 8px | Group-title inline spacing and content inset |
| xl | 16px | Page-level inset where a new container needs one |
| 2xl | 24px | Only for an existing major section break; not between Process groups |

Exception: unchanged host-shell spacing may retain existing Theme tokens,
including `Theme.spacingSM` (6px) and `Theme.spacingLG` (12px). No Phase 226
addition or modified layout may use those values; the Phase 226 scale is limited
to 4px, 8px, 16px, and 24px.

---

## Typography

Use only the existing Theme sizes below for new Process hierarchy visuals. The
two allowed weights are regular (400) and semibold (600).

| Role | Size | Weight | Line Height |
|------|------|--------|-------------|
| Caption and inactive tab | 11px (`Theme.fontSizeSM`) | 400 | 1.2 |
| Group title and active tab | 12px (`Theme.fontSizeMD`) | 600 | 1.2 |
| Empty-state body | 12px (`Theme.fontSizeMD`) | 400 | 1.33 |

Existing `OptionRow` typography and typed control typography are unchanged in
this phase. No display typography is introduced.

---

## Color

Use the existing dark Theme palette. No new color token or literal is allowed.

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | `Theme.bgBase` (`#0d0f12`) | Process list background and unframed content surface |
| Secondary (30%) | `Theme.bgPanel` (`#161a23`) and `Theme.chromeSurface` (`#10161e`) | Page strip and existing preset/action chrome |
| Accent (10%) | `Theme.accent` (`#18c75e`) | Active Process tab underline, active tab text, and keyboard focus border only |
| Destructive | `Theme.statusError` (`#e04040`) | Existing host error affordances only; no destructive Process action is added |

Accent is reserved for the active page and focus indication. A static Process
group header does not use accent as a decorative plus, chevron, badge, or
sidebar marker in this phase.

---

## Layout And Sizing

- Keep the existing non-modal `SettingsDialog` minimum width and height at
  736px by 593px. It may grow using its existing layout rules; this phase does
  not introduce an alternate compact dialog, a mobile layout, or a new window.
- Preserve the 44px preset/action bar and 38px page strip. The Process strip
  contains exactly six equal-width tabs at the minimum width. Labels remain
  single-line and use `Text.ElideRight`; they never wrap or grow the strip.
- The Process option area remains one full-width scrolling list on
  `Theme.bgBase`. It has no left group-navigation column, card wrapper, or
  nested panel.
- Preserve the existing compact `OptionRow` geometry for this phase:
  `compact: true`, 210px label column, 96px numeric field, and 190px enum
  field. Typed controls, metadata lanes, reset controls, and validation
  presentation are owned by later phases.
- A source-derived static group heading follows the compact 28px header rhythm
  already used by `OptionRow`. It uses the current left/right insets and a
  `Theme.borderSubtle` divider. It is a title and separator only in Phase 226.
- Do not use the legacy decorative `+` rail on Process group headings. Do not
  add a chevron until Phase 227 supplies real independent disclosure state.

---

## Interaction And Visual States

### Page navigation

| State | Required presentation and behavior |
|-------|------------------------------------|
| Initial open | Quality is selected when the Process dialog initializes; its source-derived group list is shown. |
| Inactive tab | `Theme.textSecondary` on a transparent surface. The full equal-width tab cell is the pointer target. |
| Hovered tab | `Theme.bgHover`; do not use accent for hover alone. |
| Active tab | `Theme.accent` text with a 2px `Theme.accent` bottom underline. Exactly one Process page is active. |
| Keyboard focus | Visible `Theme.borderFocus` outline without changing the active-page accent state until activation. |
| Activation | Pointer click, Enter, Space, Left/Right, Home, and End select the corresponding Process page using standard tab semantics. Selection rebuilds the Process projection through C++ data, not QML grouping logic. |
| Long localized label | Remains one line, ellides at the right edge, and retains its full accessible name. |

The tab control must expose button/tab semantics to assistive technology. The
selected state and position in the six-tab set must be available even when the
visible label is elided.

### Group presentation

| State | Required presentation and behavior |
|-------|------------------------------------|
| Mapped group | Render source order within its active source-derived page before its mapped option rows. Use its localized title and existing subdued text/divider treatment. |
| First or adjacent group | Keep its own heading and divider; never merge it with an identically named group from another page. |
| Empty active page | Reuse the existing `No options` empty-state treatment. This phase does not fabricate a category or move options to populate the page. |
| Unmapped supported option | Do not render it in a fallback group or page. Treat it as a source-mapping verification failure. |

Group headings have no pointer action, focus stop, expanded/collapsed state,
count, dirty count, reset action, or accessibility toggle state in Phase 226.
Those controls arrive only with the presentation model in Phase 227.

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Primary action | Select Process page |
| Page labels | Quality, Strength, Speed, Support, Multimaterial, Others; all visible labels use `qsTr()` |
| Group labels | Use the source-derived group title through `qsTr()`; do not expose technical page/group keys as UI copy |
| Empty state heading | No options |
| Empty state body | No additional Phase 226 body copy; retain the compact existing dialog treatment |
| Error state | No new user-facing error state. An unmapped Process option is a mapping/test failure and must never be explained by a fallback category. Existing Settings dialog notification behavior remains unchanged. |
| Destructive confirmation | None in scope |

Search-specific copy, dirty and compatibility copy, reset labels, validation
messages, and preset lifecycle copy are intentionally deferred.

---

## Accessibility

- The Process page strip is keyboard operable as a six-item tab set. A focused
  tab has a visible focus indicator independent of hover and selected state.
- Each tab has an accessible localized name, selected state, and set position;
  ellipsis never replaces its accessible name.
- Text, focus borders, active underline, and tab selection must use Theme
  colors only and remain distinguishable without color alone through the active
  underline and accessible selected state.
- The six 38px-high desktop tab targets fill their equal-width cells. Do not
  shrink the active target to its text or underline.
- Static group titles are exposed as noninteractive text. Do not label them as
  buttons or accordions before independent disclosure exists.
- Retain existing OptionRow tooltip behavior for option help. Phase 226 adds no
  icon-only Process action, so it adds no new tooltip requirement.

---

## Implementation Boundaries

- Use the source-derived Process page/group manifest in C++ as the hierarchy
  authority. QML binds to the projection and forwards page selection only.
- Preserve the existing `ConfigOptionModel -> ConfigViewModel` data and edit
  routes. This phase does not revise value editing or introduce a second option
  store, preset service, persistence path, or QSettings state.
- Change only the `presetTier == "print"` hierarchy branch. Printer and
  Material tabs and their existing group behavior remain untouched.
- `GroupNavSidebar.qml` remains retired and unregistered in `qml.qrc`. Do not
  replace the page strip with a sidebar or register the retired component.

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| Qt/QML local controls | Existing in-repository Cx controls only | not applicable; no third-party registry |
| shadcn official | none | not applicable; this is not a React application |

---

## Verification Checkpoints

1. Add an automated source-manifest assertion for the six page keys and their
   exact order. It must reject `Base`, `Cooling`, `Retraction`, and `Other` as
   Process pages.
2. Assert the ordered page-qualified group manifest matches
   `TabPrint::build()`, including separate `Advanced` groups on Strength,
   Speed, Support, and Multimaterial.
3. Assert every supported Process option is mapped to an upstream page/group
   and that no alphabetical, category, or `Others` fallback is used.
4. Exercise Process page selection by pointer and keyboard. Verify one active
   tab, the correct ordered group sequence, and unchanged Printer/Material
   settings dialogs.
5. Inspect the Process dialog at 736px by 593px and at the existing higher UI
   scale fixture. Verify six tabs remain one line, group titles and rows do not
   overlap or clip, no group sidebar appears, and no decorative Process `+` or
   inactive chevron remains.
6. Confirm a page with no supported mapped rows retains the compact existing
   empty treatment and does not receive invented fallback content.

Documentation-only contract validation does not require a full application
build. Implementation verification must use the canonical project command:
`powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

---

## Explicit Non-Goals

- Group disclosure, collapse persistence, search behavior, mode behavior,
  visible/dirty counts, group reset, and group keyboard toggling.
- Typed editor redesign, validation feedback, source/inheritance metadata,
  vector-value presentation, or multi-nozzle/per-extruder editing.
- Preset dirty/compatibility workflow changes, Save/Save As/discard/close
  behavior, or slice-invalidation changes.
- Printer or Material hierarchy expansion, GroupNavSidebar revival, device,
  hardware, network, cloud, camera, monitor, or SLA settings.
- `libslic3r` algorithm changes, QML business-rule duplication, arbitrary
  cross-option auto-correction, or persistence in presets, projects, or
  QSettings.

---

## Checker Sign-Off

- [ ] Dimension 1 Copywriting: PASS
- [ ] Dimension 2 Visuals: PASS
- [ ] Dimension 3 Color: PASS
- [ ] Dimension 4 Typography: PASS
- [ ] Dimension 5 Spacing: PASS
- [ ] Dimension 6 Registry Safety: PASS

**Approval:** pending
