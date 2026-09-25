---
phase: v14-01
slug: component-system-hardening
status: draft
shadcn_initialized: false
preset: none
created: 2026-06-03
---

# Phase v14-01 -- Component System Hardening

> UI design contract for establishing a complete, theme-token-based custom
> component library (Cx* controls). This contract covers 6 existing controls
> to upgrade and 7 new controls to create, followed by migration of 156+
> raw Qt Quick Control instances across pages, panels, and dialogs.

---

## 1. Design System

| Property | Value |
|----------|-------|
| Tool | none (Qt6/QML -- not React/Next.js) |
| Preset | not applicable |
| Component library | Custom Cx* controls in `src/qml_gui/controls/` |
| Icon library | Inline Unicode glyphs + SVG (project convention) |
| Theme | `Theme.qml` singleton (`pragma Singleton`, registered via `qmldir`) |

---

## 2. Design Principles

All Cx* controls MUST satisfy these rules without exception.

### 2.1 Theme-Token Exclusivity

- **ZERO hardcoded hex colors** in any file under `controls/`.
- All colors reference `Theme.*` tokens from `src/qml_gui/Theme.qml`.
- All radii reference `Theme.radius*` tokens.
- All font sizes reference `Theme.fontSize*` tokens.
- All spacing/sizing reference `Theme.spacing*` / `Theme.control*` tokens.
- Rationale: When Theme.qml values change (dark/light mode, scaling), every
  control updates automatically without per-file edits.

### 2.2 Visual State Coverage

Every interactive control MUST define visual treatment for all 5 states:

| State | Trigger | Required Treatment |
|-------|---------|-------------------|
| Normal | Default | Background, border, text color for resting state |
| Hover | Mouse within bounds | Subtle background shift, border highlight |
| Pressed | Mouse down | Darker/louder background, slight scale (0.96-0.98) |
| Disabled | `enabled: false` | Reduced opacity (0.45), muted colors |
| Focused | Keyboard/tab focus | Accent border ring, visible focus indicator |

### 2.3 API Consistency

- Property naming: `camelCase`, matching Qt Quick Controls conventions.
- Signal naming: `onActivated`, `onToggled`, `onMoved`, `onClicked`.
- Size properties: `implicitHeight` uses `Theme.controlHeightSM` (28px) by default.
- Font: `Theme.fontSizeMD` (12px) for content, `Theme.fontSizeSM` (11px) for compact.
- All controls import `".."` for Theme access (same directory convention as CxIconButton).

### 2.4 Animation Consistency

- Color transitions: `ColorAnimation { duration: 120; easing.type: Easing.OutCubic }`
- Scale/opacity transitions: `NumberAnimation { duration: 100-150 }`
- All transitions use `Easing.OutCubic` unless physically模拟 (spring, bounce).
- Match CxIconButton as the reference implementation for animation patterns.

### 2.5 Reference Implementation

`CxIconButton.qml` and `CxPillAction.qml` are the gold-standard controls.
They use Theme tokens exclusively, have correct state coverage, and include
smooth animations. New and upgraded controls MUST match their quality level.

---

## 3. Theme Token Audit

### 3.1 Current Token Inventory (Theme.qml)

**Background (10 tokens):**
`bgBase`, `bgSurface`, `bgPanel`, `bgCard`, `bgElevated`, `bgInset`,
`bgFloating`, `bgHover`, `bgPressed`, `bgTooltip`

**Accent (4 tokens):**
`accent`, `accentLight`, `accentDark`, `accentSubtle`

**Text (6 tokens):**
`textPrimary`, `textSecondary`, `textTertiary`, `textDisabled`, `textMuted`, `textOnAccent`

**Border (5 tokens):**
`borderDefault`, `borderSubtle`, `borderStrong`, `borderFocus`, `borderInput`

**Chrome (10 tokens):**
`chromeSurface`, `chromeSurfaceAlt`, `chromeHover`, `chromePressed`,
`chromeBorder`, `chromeText`, `chromeTextMuted`, `chromeDangerHover`, `chromeDangerPressed`

**Status (5 tokens):**
`statusSuccess`, `statusWarning`, `statusError`, `statusInfo`,
`bgErrorSubtle`, `bgWarningSubtle`

**Typography (7 tokens):**
`fontSizeXS` (10), `fontSizeSM` (11), `fontSizeMD` (12), `fontSizeLG` (14),
`fontSizeXL` (16), `fontSizeXXL` (20)

**Spacing (6 tokens):**
`spacingXS` (4), `spacingSM` (6), `spacingMD` (8), `spacingLG` (12),
`spacingXL` (16), `spacingXXL` (24)

**Radii (5 tokens):**
`radiusSM` (3), `radiusMD` (5), `radiusLG` (8), `radiusXL` (12), `radiusXXL` (16)

**Control sizing (7 tokens):**
`controlHeightSM` (28), `controlHeightMD` (34), `controlHeightLG` (40),
`iconButtonSizeSM` (32), `iconButtonSizeMD` (34), `iconButtonSizeLG` (38),
`pillHeight` (34), `panelPadding` (12)

### 3.2 Tokens Needed But Missing

The following tokens MUST be added to Theme.qml before control creation:

| Token | Value | Purpose |
|-------|-------|---------|
| `switchTrackOff` | `"#2a3040"` | Switch track when unchecked |
| `switchTrackOn` | `"#18c75e"` (same as accent) | Switch track when checked |
| `switchKnob` | `"#e8edf6"` (same as textPrimary) | Switch handle/knob |
| `progressTrack` | `"#2a3040"` (same as borderSubtle) | ProgressBar background track |
| `progressFill` | `"#18c75e"` (same as accent) | ProgressBar fill |
| `overlayDim` | `"#000000"` at 50% opacity | Dialog/popup background dim |
| `menuBackground` | `"#1a202b"` | Menu background |
| `menuItemHover` | `"#2e3444"` (same as bgHover) | Menu item hover |
| `scrollbarTrack` | `"transparent"` | ScrollBar track |
| `scrollbarThumb` | `"#3a4258"` (same as bgPressed) | ScrollBar thumb |
| `selectionColor` | `"#18c75e"` | Text selection highlight |
| `selectionText` | `"#0d1017"` | Text over selection |

Note: Several of these can reuse existing tokens (e.g., `progressFill = accent`).
The executor should prefer token reuse via aliasing in the control implementation
over adding new Theme tokens, unless a genuinely unique color value is needed.

---

## 4. Existing Control Upgrades

### 4.1 CxButton (6 hardcoded hex values)

**File:** `src/qml_gui/controls/CxButton.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | State/Element | Theme Token |
|------|-----------|---------------|-------------|
| 26 | `#264c38` | Primary disabled bg | `accentSubtle` at reduced opacity |
| 27 | `#16a354` | Primary pressed bg | `accentDark` |
| 28 | `#1ed36b` | Primary hover bg | `accentLight` |
| 29 | `#18c75e` | Primary normal bg | `accent` |
| 31 | `#4a2828` | Danger disabled bg | Derive via `Qt.darker(statusError, ...)` |
| 32 | `#c93030` | Danger pressed bg | `Qt.darker(statusError, 1.1)` |
| 33 | `#e03535` | Danger normal bg | `statusError` |
| 36 | `#3a4050` | Ghost pressed bg | `bgPressed` |
| 37 | `#2e3442` | Ghost hover bg | `bgHover` |
| 41 | `#2a2f38` | Secondary disabled bg | `bgPanel` at reduced opacity |
| 42 | `#4a5060` | Secondary pressed bg | `bgPressed` |
| 43 | `#424957` | Secondary hover bg | `bgHover` |
| 44 | `#363c4a` | Secondary normal bg | `bgElevated` |
| 49 | `#566070` | Ghost hover border | `borderDefault` |
| 50 | `#4e5568` | Secondary border | `borderStrong` |
| 58 | `#5a6270` | Disabled text | `textDisabled` |
| 59 | `#ffffff` | Primary/Danger text | `textOnAccent` |
| 62 | `#d8e0ec` | Secondary/Ghost text | `textPrimary` |

**Additional enhancements:**
- Add `Behavior on color { ColorAnimation { duration: 120 } }` to background
  and text for smooth state transitions (match CxIconButton pattern).
- Use `Theme.controlHeightSM` / `Theme.controlHeightMD` for implicitHeight
  instead of hardcoded 24/30.
- Use `Theme.radiusSM` (3) for border radius instead of hardcoded 4.
  Decision: keep radius 4 and add `Theme.radiusControl: 4` token, or use
  `radiusMD` (5). **Recommendation: use `radiusSM`** (3) for compact, and
  add a `radiusControl` token at 4 for standard controls.

### 4.2 CxCheckBox (4 hardcoded hex values)

**File:** `src/qml_gui/controls/CxCheckBox.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | Element | Theme Token |
|------|-----------|---------|-------------|
| 16 | `#18c75e` | Checked bg | `accent` |
| 16 | `#1e2330` | Unchecked bg | `bgPanel` |
| 17 | `#18c75e` | Checked border | `accent` |
| 17 | `#4e5568` | Hovered border | `borderStrong` |
| 17 | `#3a4050` | Normal border | `borderDefault` |
| 22 | `"white"` | Checkmark text | `textOnAccent` |
| 33 | `#c8d0dc` | Content text | `textPrimary` |
| 33 | `#566070` | Disabled text | `textDisabled` |

**Additional enhancements:**
- Add hover state to indicator (currently only `root.hovered` on border).
- Add `Behavior on color` for smooth transition between checked/unchecked.
- Use `Theme.fontSizeMD` (12) for content text instead of hardcoded 12.
- Add disabled state opacity (0.45).

### 4.3 CxComboBox (9 hardcoded hex values)

**File:** `src/qml_gui/controls/CxComboBox.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | Element | Theme Token |
|------|-----------|---------|-------------|
| 13 | `#3a4050` | Pressed bg | `bgPressed` |
| 13 | `#353c4a` | Hovered bg | `bgHover` |
| 13 | `#2d3340` | Normal bg | `bgElevated` |
| 14 | `#18c75e` | Focus border | `accent` (or `borderFocus`) |
| 14 | `#454d5e` | Normal border | `borderStrong` |
| 22 | `#d8e0ec` | Content text | `textPrimary` |
| 33 | `#8a96a8` | Indicator text | `textMuted` |
| 43 | `#252a34` | Popup bg | `bgElevated` |
| 44 | `#454d5e` | Popup border | `borderDefault` |
| 63 | `#1c6e42` | Highlighted delegate | `accentSubtle` |
| 67 | `#d8e0ec` | Delegate text | `textPrimary` |

**Additional enhancements:**
- Add `Behavior on color` for smooth transitions.
- Use `Theme.controlHeightSM` for implicitHeight (already 28, matches).
- Use `Theme.radiusSM` for border radius (currently 4, close enough).
- Add disabled state styling.

### 4.4 CxSlider (4 hardcoded hex values)

**File:** `src/qml_gui/controls/CxSlider.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | Element | Theme Token |
|------|-----------|---------|-------------|
| 15 | `#2e3440` | Track bg | `borderSubtle` |
| 21 | `#18c75e` | Fill bar | `accent` |
| 30 | `#1ed36b` | Pressed handle | `accentLight` |
| 30 | `#18c75e` | Normal handle | `accent` |
| 32 | `#0a1a0f` | Handle border | `accentDark` |

**Additional enhancements:**
- Add hover state for handle (currently only pressed/normal).
- Add disabled state (muted track, dim handle).
- Add optional value label (tooltip-style above handle).
- Use `Theme.controlHeightSM` sizing.

### 4.5 CxSpinBox (6 hardcoded hex values)

**File:** `src/qml_gui/controls/CxSpinBox.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | Element | Theme Token |
|------|-----------|---------|-------------|
| 12 | `#252a34` | Background | `bgElevated` |
| 13 | `#18c75e` | Focus border | `accent` |
| 13 | `#3a4050` | Normal border | `borderDefault` |
| 22 | `#d8e0ec` | Content text | `textPrimary` |
| 28 | `#18c75e` | Selection color | `accent` |
| 29 | `#0d1017` | Selected text | `bgBase` |
| 36 | `#3a4050` / `#2e3440` | Up indicator | `bgPressed` / `bgHover` |
| 37 | `#8a96a8` | Arrow text | `textMuted` |
| 45 | `#3a4050` / `#2e3440` | Down indicator | `bgPressed` / `bgHover` |
| 46 | `#8a96a8` | Arrow text | `textMuted` |

**Additional enhancements:**
- Add `Behavior on color` for smooth transitions.
- Add disabled state.
- Use `Theme.fontSizeMD` for content text.

### 4.6 CxTextField (5 hardcoded hex values)

**File:** `src/qml_gui/controls/CxTextField.qml`

**Hardcoded colors to replace:**

| Line | Hardcoded | Element | Theme Token |
|------|-----------|---------|-------------|
| 11 | `#d8e0ec` | Text color | `textPrimary` |
| 12 | `#18c75e` | Selection color | `accent` |
| 13 | `#0d1017` | Selected text | `bgBase` |
| 14 | `#566070` | Placeholder text | `textDisabled` |
| 18 | `#1e2330` | Focused bg | `bgPanel` |
| 18 | `#252a34` | Normal bg | `bgElevated` |
| 19 | `#18c75e` | Focus border | `borderFocus` |
| 19 | `#4e5568` | Hover border | `borderStrong` |
| 19 | `#3a4050` | Normal border | `borderDefault` |

**Additional enhancements:**
- Add `Behavior on color` for smooth focus/hover transitions.
- Add disabled state (reduced opacity, muted colors).
- Add optional `placeholderText` as explicit property with Theme styling.
- Use `Theme.controlHeightSM` for implicitHeight.

---

## 5. New Control Specifications

### 5.1 CxSwitch (26 instances across 5 files)

**File to create:** `src/qml_gui/controls/CxSwitch.qml`
**Extends:** `QtQuick.Controls.Switch`
**Upstream reference:** Upstream uses `wxBitmapComboBox` / toggle switches in
PreferencesDialog. Visual: CrealityPrint v7 dark theme toggle switch, green
accent when on, dark track when off.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| (inherited) | | | All Switch properties (`checked`, `enabled`, etc.) |
| `onToggled` | signal | | Emitted when checked state changes |

**Visual states:**

| State | Track Color | Knob Color | Knob Position |
|-------|------------|------------|---------------|
| Normal Off | `Theme.bgElevated` | `Theme.textPrimary` | Left |
| Normal On | `Theme.accent` | `Theme.textOnAccent` | Right |
| Hover Off | `Theme.bgHover` | `Theme.textPrimary` | Left |
| Hover On | `Theme.accentLight` | `Theme.textOnAccent` | Right |
| Disabled | `Theme.bgPanel` at 0.45 opacity | `Theme.textDisabled` | Current |
| Pressed | Track same, knob scaled 0.9 | | Current |

**Sizing:**
- Track: 36x20, radius 10 (rounded pill)
- Knob: 16x16, radius 8 (circle)
- Transition: knob position via `NumberAnimation { duration: 150 }`

**Color mapping:** All Theme tokens, zero hardcoded hex.

---

### 5.2 CxDialog (29 instances across 14+ dialogs)

**File to create:** `src/qml_gui/controls/CxDialog.qml`
**Extends:** `QtQuick.Controls.Dialog`
**Upstream reference:** Upstream uses `wxDialog` with custom dark styling.
All 14 dialogs in `src/qml_gui/dialogs/` share identical boilerplate:
`background: Rectangle { color: "#1a1f28"; radius: 8; border.color: "#2e3848" }`.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `dialogTitle` | string | `""` | Title text shown in header |
| `titleIcon` | string | `""` | Unicode icon prefix (e.g., "🖨") |
| `showCloseButton` | bool | `true` | Whether to show X close button in header |
| `contentSpacing` | int | `Theme.spacingLG` | Spacing within content Column |
| (inherited) | | | `modal`, `width`, `height`, `closePolicy`, etc. |

**Visual structure:**
```
+--[header: 44px]------------------------------------------+
| [icon] Title Text                        [X close btn]   |
+----------------------------------------------------------+
| [content area]                                           |
|                                                          |
|                                                          |
+----------------------------------------------------------+
```

**Element styling:**

| Element | Token/Value |
|---------|-------------|
| Background | `Theme.bgElevated` |
| Border | `Theme.borderInput` |
| Radius | `Theme.radiusLG` (8) |
| Header bg | `Theme.bgSurface` |
| Header text | `Theme.textPrimary`, `Theme.fontSizeLG`, bold |
| Close button hover | `Theme.chromeDangerHover` at low opacity |
| Close button text | `Theme.textMuted` |
| Overlay/dim | Semi-transparent black |

**Boilerplate eliminated:** Currently each dialog repeats ~25 lines of
background/header/close-button code. CxDialog internalizes all of this.

**Important:** The `title` property from base Dialog is suppressed in favor
of `dialogTitle` because the base `title` renders a default label that
conflicts with our custom header.

---

### 5.3 CxPopup (14 instances across 6+ files)

**File to create:** `src/qml_gui/controls/CxPopup.qml`
**Extends:** `QtQuick.Controls.Popup`
**Upstream reference:** Upstream uses `wxPopupTransientWindow`.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `popupRadius` | int | `Theme.radiusLG` | Corner radius |
| (inherited) | | | `modal`, `closePolicy`, `padding`, etc. |

**Visual states:**

| Element | Token/Value |
|---------|-------------|
| Background | `Theme.bgElevated` |
| Border | `Theme.borderDefault`, width 1 |
| Radius | `Theme.radiusLG` (8) |
| Shadow | Optional drop shadow (5px blur, `Theme.bgBase` at 30%) |
| Overlay (modal) | `Theme.bgBase` at 50% opacity |

---

### 5.4 CxMenu + CxMenuItem (~111 instances across 5 files)

**Files to create:**
- `src/qml_gui/controls/CxMenu.qml`
- `src/qml_gui/controls/CxMenuItem.qml`

**Extends:** `QtQuick.Controls.Menu` / `QtQuick.Controls.MenuItem`
**Upstream reference:** Upstream uses `wxMenu` with dark theme styling.
Current raw `Menu` usage in `main.qml` (~5 menus), `PreparePage.qml` (3 menus),
`ObjectList.qml` (5 menus), `LayerSlider.qml` (1 menu).

**CxMenu Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| (inherited) | | | All Menu properties |

**CxMenu styling:**

| Element | Token/Value |
|---------|-------------|
| Background | `Theme.bgElevated` |
| Border | `Theme.borderDefault`, width 1 |
| Radius | `Theme.radiusSM` (3) |
| Padding | `Theme.spacingSM` (6) vertical |

**CxMenuItem Properties:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| (inherited) | | | `text`, `enabled`, `checkable`, `checked` |
| `highlighted` | bool | auto | Bound to parent Menu `highlightedIndex` |

**CxMenuItem styling:**

| State | Background | Text Color |
|-------|-----------|------------|
| Normal | transparent | `Theme.textPrimary` |
| Hover/Highlighted | `Theme.bgHover` | `Theme.textPrimary` |
| Disabled | transparent | `Theme.textDisabled` |
| Pressed | `Theme.bgPressed` | `Theme.textPrimary` |

**Sizing:** Height 28, text at `Theme.fontSizeMD`, left padding `Theme.spacingLG`.

**Note on MenuSeparator:** Do NOT create a custom separator. Use
`MenuSeparator {}` with a padding override to match theme. Add a
`CxMenuSeparator.qml` only if the default visual does not match.

---

### 5.5 CxScrollView (14 instances across 10+ files)

**File to create:** `src/qml_gui/controls/CxScrollView.qml`
**Extends:** `QtQuick.Controls.ScrollView`
**Upstream reference:** Upstream uses `wxScrolledWindow`.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| (inherited) | | | `clip`, `contentWidth`, `contentHeight` |

**Styling (via ScrollBar customization):**

| Element | Token/Value |
|---------|-------------|
| ScrollBar track | transparent |
| ScrollBar thumb | `Theme.bgPressed` at 50% opacity |
| ScrollBar thumb hover | `Theme.bgPressed` at 80% opacity |
| ScrollBar width | 8px |
| ScrollBar radius | 4 (half width) |

**Implementation approach:** Override `ScrollBar.vertical` and
`ScrollBar.horizontal` with custom `ScrollBar` components that use Theme tokens.

---

### 5.6 CxProgressBar (3 instances across 3 files)

**File to create:** `src/qml_gui/controls/CxProgressBar.qml`
**Extends:** `QtQuick.Controls.ProgressBar`
**Upstream reference:** Upstream uses a progress bar in slice status display.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `barHeight` | int | `6` | Track height |
| `fillColor` | color | `Theme.accent` | Fill bar color |
| `trackColor` | color | `Theme.borderSubtle` | Track background |
| `animateWidth` | bool | `true` | Whether to animate fill width |
| (inherited) | | | `from`, `to`, `value` |

**Visual states:**

| State | Fill Color |
|-------|-----------|
| Normal (active) | `Theme.accent` |
| Complete (100%) | `Theme.accentDark` |
| Error | `Theme.statusError` |
| Disabled/Idle | `Theme.textDisabled` |

**Implementation:** Replace default `background` and `contentItem` with
Theme-token Rectangles. Use `NumberAnimation { duration: 150 }` on fill width
when `animateWidth` is true.

---

### 5.7 CxTextArea (1 instance)

**File to create:** `src/qml_gui/controls/CxTextArea.qml`
**Extends:** `QtQuick.Controls.TextArea`
**Upstream reference:** G-code editor in `EditGCodeDialog.qml`.

**Properties API:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| (inherited) | | | `text`, `placeholderText`, `wrapMode`, etc. |

**Styling:** Identical to CxTextField (same background, border, text color tokens).
Additional: scroll bars via CxScrollView pattern, line count display.

| Element | Token/Value |
|---------|-------------|
| Background | `Theme.bgElevated` |
| Border (normal) | `Theme.borderDefault` |
| Border (focus) | `Theme.borderFocus` |
| Text | `Theme.textPrimary` |
| Placeholder | `Theme.textDisabled` |
| Selection | `Theme.accent` |
| Radius | `Theme.radiusSM` |

---

## 6. Migration Plan

### 6.1 Dependency Order

Controls must be created/upgraded in this order to satisfy dependencies:

```
Phase A: Theme token additions (add missing tokens to Theme.qml)
    |
Phase B: Upgrade existing controls (no new files)
    B1. CxTextField  (foundation for CxTextArea, Dialog TextField instances)
    B2. CxButton     (foundation for Dialog button instances)
    B3. CxCheckBox
    B4. CxComboBox   (foundation for dialog/page combos)
    B5. CxSlider
    B6. CxSpinBox
    |
Phase C: Create new controls
    C1. CxSwitch     (no dependencies)
    C2. CxDialog     (depends on CxButton for footer buttons)
    C3. CxPopup      (no dependencies)
    C4. CxProgressBar (no dependencies)
    C5. CxScrollView  (no dependencies)
    C6. CxMenu + CxMenuItem (no dependencies)
    C7. CxTextArea   (depends on CxTextField pattern)
    |
Phase D: Migrate raw usage in consuming files
    D1. PreferencesPage.qml (Switch -> CxSwitch, Button -> CxButton)
    D2. PrintSettings.qml   (Switch -> CxSwitch, ComboBox -> CxComboBox, Popup -> CxPopup)
    D3. SliceProgress.qml   (ProgressBar -> CxProgressBar)
    D4. dialogs/*.qml       (Dialog -> CxDialog, Button -> CxButton, TextField -> CxTextField)
    D5. main.qml            (Menu -> CxMenu, MenuItem -> CxMenuItem)
    D6. PreparePage.qml     (Menu -> CxMenu, ComboBox -> CxComboBox)
    D7. ObjectList.qml      (Menu -> CxMenu)
    D8. SettingsPage.qml    (Switch -> CxSwitch, ComboBox -> CxComboBox, Slider -> CxSlider)
    D9. HomePage.qml        (TextField -> CxTextField)
    D10. Other files        (residual Switch, Popup, ScrollView, etc.)
```

### 6.2 Migration Strategy Per Control Type

#### Switch -> CxSwitch (25 replacements in 5 files)

**Files:** PreferencesPage.qml (15), PrintSettings.qml (1), PreparePage.qml (4),
StatsPanel.qml (4), SettingsPage.qml (1)

**Search pattern:** `Switch {` -> `CxSwitch {`
**API differences:** None -- CxSwitch inherits all Switch properties.
Properties `checked`, `onToggled`, `enabled` remain identical.
**Risk:** LOW. Drop-in replacement with no API changes.

#### Dialog -> CxDialog (14 replacements in 14 dialog files)

**Files:** All `src/qml_gui/dialogs/*.qml`, plus inline dialogs in PrintSettings.qml (3)

**Migration steps per dialog:**
1. Change `Dialog {` to `CxDialog {`
2. Move title text from inline header to `dialogTitle:` property
3. Remove custom `background:` Rectangle (CxDialog provides it)
4. Remove custom `header:` Rectangle (CxDialog provides it)
5. Remove close button MouseArea (CxDialog provides it)
6. Keep `contentItem:` as-is (may need spacing adjustment)
7. Keep `onAccepted:` / `onRejected:` as-is

**Lines removed per dialog:** ~25 lines of boilerplate
**Total lines removed:** ~350 lines across 14 dialogs

**Risk:** MEDIUM. Each dialog has unique content layout. Must verify
no dialog relies on the exact structure of the old header Rectangle
for anchoring or positioning.

#### Popup -> CxPopup (14 replacements)

**Search pattern:** `Popup {` -> `CxPopup {`
**Risk:** LOW. CxPopup is a thin wrapper that only replaces `background:`.
Popup content and behavior remain identical.

#### Menu -> CxMenu / MenuItem -> CxMenuItem (~111 replacements)

**Files:** main.qml (5 menus, ~30 items), PreparePage.qml (3 menus, ~15 items),
ObjectList.qml (5 menus, ~40 items), LayerSlider.qml (1 menu, ~6 items)

**Migration steps:**
1. Add `import "../controls"` where missing
2. Change `Menu {` to `CxMenu {`
3. Change `MenuItem {` to `CxMenuItem {`
4. Remove any inline `background:` / `delegate:` overrides
5. Keep `MenuSeparator {}` as-is (or replace with CxMenuSeparator if created)

**Risk:** MEDIUM. Some menus have nested sub-menus. CxMenu must support
the `Menu { title: "..."; ... }` nesting pattern.

#### ScrollView -> CxScrollView (14 replacements)

**Search pattern:** `ScrollView {` -> `CxScrollView {`
**Risk:** LOW. Content remains identical, only ScrollBar styling changes.

#### ProgressBar -> CxProgressBar (3 replacements)

**Files:** SliceProgress.qml, and 2 others.

**Migration steps:**
1. Replace `ProgressBar {` with `CxProgressBar {`
2. Remove inline `background:` and `contentItem:` overrides
3. Keep `from:`, `to:`, `value:` properties

**Risk:** LOW. ProgressBar has a simple API.

#### TextField -> CxTextField (26 raw replacements)

**Files with raw TextField that bypasses CxTextField:** PrintSettings.qml (1),
dialog TextField instances (several), HomePage.qml (search field).

**Migration steps:**
1. Change raw `TextField {` to `CxTextField {`
2. Remove inline `background:`, `color:`, `placeholderTextColor:` overrides
3. Keep `placeholderText:`, `text:`, `onTextChanged:`, etc.

**Risk:** LOW. CxTextField inherits all TextField properties.

---

## 7. Quality Gates

### 7.1 Grep-Based Verification Commands

All commands should return 0 results after migration:

```bash
# Gate 1: Zero hardcoded hex colors in controls/
grep -rn '#[0-9a-fA-F]\{6\}' src/qml_gui/controls/ --include="*.qml"
# Expected: 0 matches (Theme.qml and CxIconButton/CxPillAction already pass)

# Gate 2: Zero raw Switch in pages/panels/dialogs (should all use CxSwitch)
grep -rn '^\s*Switch\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml"
# Expected: 0 matches

# Gate 3: Zero raw Button in pages/panels/dialogs (should use CxButton)
grep -rn '^\s*Button\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml"
# Expected: 0 matches

# Gate 4: Zero raw TextField with background override in consuming files
grep -rn 'TextField\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml"
# Note: TextField inside CxTextField implementation itself is allowed

# Gate 5: Zero raw ProgressBar in consuming files
grep -rn '^\s*ProgressBar\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml"
# Expected: 0 matches

# Gate 6: All controls import Theme
grep -rL 'import "\.\."' src/qml_gui/controls/Cx*.qml
# Expected: no output (all Cx* files import parent for Theme)

# Gate 7: All Cx controls have Behavior on color
grep -rL 'Behavior on color' src/qml_gui/controls/Cx*.qml
# Expected: only CxTextArea if it delegates to CxTextField pattern
```

### 7.2 Functional Verification

| Check | Method |
|-------|--------|
| Theme changes propagate | Change one Theme.qml color, verify all controls update |
| All states visible | Tab through controls, verify focus rings |
| Disabled states | Set `enabled: false`, verify opacity and muted colors |
| Hover feedback | Mouse over each control type, verify visual response |
| Press feedback | Click and hold, verify press state |
| Dialog open/close | Open each CxDialog, verify background, header, close button |
| Menu interaction | Open each CxMenu, verify items, separators, sub-menus |
| Switch toggle | Toggle CxSwitch, verify animation and state |
| Scroll bars | Scroll CxScrollView, verify themed thumb |

### 7.3 Build Verification

```bash
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

The build MUST pass with 0 QML warnings related to control imports or
property bindings after migration.

---

## 8. Spacing Scale

Declared values (multiples of 4 or 6 -- matching Theme.qml existing tokens):

| Token | Value | Usage |
|-------|-------|-------|
| spacingXS | 4 | Icon gaps, tight inline spacing |
| spacingSM | 6 | Compact element spacing, menu item padding |
| spacingMD | 8 | Standard control padding, dialog margins |
| spacingLG | 12 | Section padding, group spacing |
| spacingXL | 16 | Layout gaps, dialog content margins |
| spacingXXL | 24 | Page-level margins, major section breaks |

Exceptions: `controlHeightSM` (28), `controlHeightMD` (34), `controlHeightLG` (40)
are not multiples of 4 but follow Qt Quick Controls conventions for touch-friendly
sizing. These are pre-existing tokens and must not change.

---

## 9. Typography

| Role | Size (Token) | Weight | Line Height | Usage |
|------|-------------|--------|-------------|-------|
| Body | fontSizeMD (12) | 400 (normal) | 1.5 | Control content, descriptions |
| Label | fontSizeSM (11) | 400 | 1.4 | Compact labels, secondary text |
| Heading | fontSizeLG (14) | 700 (bold) | 1.2 | Section headers, dialog titles |
| Display | fontSizeXL (16) | 700 (bold) | 1.2 | Page titles, prominent headings |

Font sizes used in controls: `fontSizeXS` (10) for badges/tiny text,
`fontSizeSM` (11) for compact, `fontSizeMD` (12) for standard, `fontSizeLG` (14)
for dialog headers.

---

## 10. Color

| Role | Token | Hex | Usage |
|------|-------|-----|-------|
| Dominant (60%) | bgBase, bgSurface | #0d0f12, #131720 | Page backgrounds, sidebar |
| Secondary (30%) | bgPanel, bgElevated | #1a1e28, #252b38 | Control backgrounds, cards |
| Accent (10%) | accent | #18c75e | CTA buttons, focus rings, checked states, progress fill, links |
| Destructive | statusError | #e04040 | Delete buttons, error states, danger actions |

**Accent reserved for:**
- CTA button background (CxButton Primary)
- Focus border on all input controls (CxTextField, CxComboBox, CxSpinBox)
- Checked state indicators (CxCheckBox, CxSwitch track)
- Progress bar fill (CxProgressBar)
- Active/selected tab indicator
- Links and interactive text highlights
- Slider fill track and handle

Accent MUST NOT be used for: body text, background surfaces, borders at rest,
or decorative elements that do not indicate interactive state.

---

## 11. Copywriting Contract

| Element | Copy |
|---------|------|
| Primary CTA (slice) | "Start Slice" / qsTr("Start Slice") |
| Primary CTA (export) | "Export G-code" / qsTr("Export G-code") |
| Primary CTA (save) | "Save" / qsTr("Save") |
| Empty state heading | Not applicable (controls have no empty state) |
| Error state | Control-level: red border + `Theme.statusError` text |
| Destructive confirmation | "Delete Preset" / qsTr("Delete Preset") -- requires user confirmation via CxDialog |
| Dialog close | "X" button in header (no text label) |
| Dialog cancel | "Cancel" / qsTr("Cancel") |
| Dialog confirm | "OK" / qsTr("OK") or action-specific verb |

Note: This phase creates infrastructure controls. The copywriting contract
is minimal since controls do not contain their own copy -- they expose
properties for consuming pages to set labels and text.

---

## 12. Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | not applicable | not applicable |
| Third-party | none | not applicable |

This project uses Qt6/QML, not React. No shadcn or third-party registries.
All controls are implemented as QML files in `src/qml_gui/controls/`.

---

## 13. Implementation Notes

### 13.1 Import Convention

All Cx controls must include:
```qml
import QtQuick
import QtQuick.Controls
import ".."
```

The `import ".."` provides access to the Theme singleton registered via
`src/qml_gui/qmldir`.

### 13.2 Enum Pattern

Follow CxIconButton's enum pattern for style variants:
```qml
enum Style { Primary, Secondary, Danger, Ghost }
```

### 13.3 File Naming

- Controls: `Cx*.qml` in `src/qml_gui/controls/`
- One control per file
- File name matches primary type name exactly

### 13.4 Backward Compatibility

Existing usages of CxButton, CxComboBox, etc. must continue working after
upgrades. The property API must remain identical -- only the internal
implementation changes (replacing hardcoded colors with Theme tokens).

### 13.5 Test Baseline

Before starting migration, capture current behavior:
- Screenshot each page (Prepare, Preferences, Settings, Home)
- Screenshot each dialog (About, Print, Calibration, BedShape, etc.)
- Note current visual issues (hardcoded colors that differ from Theme intent)

After migration, verify visual parity (same layout, same behavior, but
using Theme tokens instead of hardcoded hex).

---

## Checker Sign-Off

- [ ] Dimension 1 Copywriting: PASS
- [ ] Dimension 2 Visuals: PASS
- [ ] Dimension 3 Color: PASS
- [ ] Dimension 4 Typography: PASS
- [ ] Dimension 5 Spacing: PASS
- [ ] Dimension 6 Registry Safety: PASS

**Approval:** pending
