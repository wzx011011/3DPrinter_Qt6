# PLAN: v14-01 Component System Hardening

## Goal

Establish a complete, theme-token-based custom component library. After this phase:
- Zero hardcoded hex colors in `controls/`
- Zero raw Qt Quick Control usage in pages/panels/dialogs (all wrapped by Cx*)
- All controls respond to Theme changes

## Dependency Order

```
Wave A → Wave B → Wave C → Wave D → Verify
```

---

## Wave A: Theme Token Additions

**Task A1: Add missing tokens to Theme.qml**

File: `src/qml_gui/Theme.qml`

Add these properties (prefer aliasing to existing tokens):

| Token | Value | Aliases |
|-------|-------|---------|
| `switchTrackOff` | `"#2a3040"` | new |
| `switchTrackOn` | same as `accent` | alias |
| `switchKnob` | same as `textPrimary` | alias |
| `progressTrack` | same as `borderSubtle` | alias |
| `progressFill` | same as `accent` | alias |
| `overlayDim` | `"#000000"` | new |
| `menuBackground` | `"#1a202b"` | new |
| `selectionColor` | same as `accent` | alias |
| `selectionText` | same as `bgBase` | alias |

Only add genuinely new values. Reuse existing tokens where color matches.

**Verify:** `grep -c 'switchTrackOff\|progressTrack\|menuBackground\|overlayDim' src/qml_gui/Theme.qml` → ≥4

---

## Wave B: Upgrade Existing Controls (6 files, no new files)

Each upgrade: replace hardcoded hex → Theme tokens, add missing states, add Behavior animations.

### Task B1: CxTextField token-ify

File: `src/qml_gui/controls/CxTextField.qml`

Changes:
- Replace all `#xxxxxx` with Theme tokens per UI-SPEC Section 4.6 mapping
- Add `Behavior on color { ColorAnimation { duration: 120 } }` for background/border
- Add disabled state (opacity 0.45, muted colors)
- Ensure `placeholderText` uses `Theme.textDisabled`
- Use `Theme.controlHeightSM` for implicitHeight

**Verify:** `grep -c '#' src/qml_gui/controls/CxTextField.qml` → 0

### Task B2: CxButton token-ify

File: `src/qml_gui/controls/CxButton.qml`

Changes:
- Replace 18 hardcoded hex values per UI-SPEC Section 4.1 mapping
- Add `Behavior on color` for background and text transitions
- Use `Theme.controlHeightSM`/`Theme.controlHeightMD` for implicitHeight
- Use `Theme.radiusSM` for border radius

**Verify:** `grep -c '#' src/qml_gui/controls/CxButton.qml` → 0

### Task B3: CxCheckBox token-ify

File: `src/qml_gui/controls/CxCheckBox.qml`

Changes:
- Replace 8 hardcoded hex values per UI-SPEC Section 4.2
- Add hover state, Behavior animations
- Add disabled state opacity

**Verify:** `grep -c '#' src/qml_gui/controls/CxCheckBox.qml` → 0

### Task B4: CxComboBox token-ify

File: `src/qml_gui/controls/CxComboBox.qml`

Changes:
- Replace 11 hardcoded hex values per UI-SPEC Section 4.3
- Add disabled state, Behavior animations
- Ensure popup colors use Theme tokens

**Verify:** `grep -c '#' src/qml_gui/controls/CxComboBox.qml` → 0

### Task B5: CxSlider token-ify + enhance

File: `src/qml_gui/controls/CxSlider.qml`

Changes:
- Replace 5 hardcoded hex values per UI-SPEC Section 4.4
- Add hover state for handle
- Add disabled state (muted track, dim handle)
- Add optional `valueLabelVisible` property with tooltip above handle

**Verify:** `grep -c '#' src/qml_gui/controls/CxSlider.qml` → 0

### Task B6: CxSpinBox token-ify

File: `src/qml_gui/controls/CxSpinBox.qml`

Changes:
- Replace 10 hardcoded hex values per UI-SPEC Section 4.5
- Add Behavior animations, disabled state

**Verify:** `grep -c '#' src/qml_gui/controls/CxSpinBox.qml` → 0

### Task B-fix: CxIconButton hardcoded hex fix

File: `src/qml_gui/controls/CxIconButton.qml`

- Line 56: `#2f3d54` → `Theme.chromeBorder` or closest token

**Verify:** `grep -c '#' src/qml_gui/controls/CxIconButton.qml` → 0

### Wave B Gate

```bash
grep -rn '#[0-9a-fA-F]\{6\}' src/qml_gui/controls/ --include="*.qml"
# Expected: 0 matches
```

Build: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

---

## Wave C: Create New Controls (7 new files)

### Task C1: CxSwitch

Create: `src/qml_gui/controls/CxSwitch.qml`
Extends: `Switch`
Spec: UI-SPEC Section 5.1

Key properties:
- Track: 36x20, pill radius 10
- Knob: 16x16, circle
- Colors: Theme tokens for on/off/hover/disabled states
- Animation: `NumberAnimation { duration: 150 }` on knob position

**Verify:** File exists, no hardcoded hex

### Task C2: CxDialog

Create: `src/qml_gui/controls/CxDialog.qml`
Extends: `Dialog`
Spec: UI-SPEC Section 5.2

Key properties:
- `dialogTitle` (not `title` — suppresses default header)
- `titleIcon` string
- `showCloseButton` bool
- Custom header: 44px, bgSurface background, close button
- Custom background: bgElevated, radiusLG, borderInput
- Overlay: semi-transparent black
- `contentSpacing: Theme.spacingLG`

**Verify:** File exists, no hardcoded hex

### Task C3: CxPopup

Create: `src/qml_gui/controls/CxPopup.qml`
Extends: `Popup`
Spec: UI-SPEC Section 5.3

Key properties:
- `popupRadius: Theme.radiusLG`
- Background: bgElevated, borderDefault, radiusLG
- Optional drop shadow

**Verify:** File exists, no hardcoded hex

### Task C4: CxMenu + CxMenuItem

Create: `src/qml_gui/controls/CxMenu.qml`
Create: `src/qml_gui/controls/CxMenuItem.qml`
Spec: UI-SPEC Section 5.4

CxMenu:
- Background: bgElevated, borderDefault, radiusSM
- Padding: spacingSM vertical

CxMenuItem:
- Normal: transparent bg, textPrimary
- Hover: bgHover, textPrimary
- Disabled: transparent, textDisabled
- Height 28, font fontSizeMD

**Verify:** Both files exist, no hardcoded hex

### Task C5: CxScrollView

Create: `src/qml_gui/controls/CxScrollView.qml`
Extends: `ScrollView`
Spec: UI-SPEC Section 5.5

Key styling:
- ScrollBar track: transparent
- ScrollBar thumb: bgPressed at 50% opacity, 8px wide
- ScrollBar hover: bgPressed at 80%

**Verify:** File exists, no hardcoded hex

### Task C6: CxProgressBar

Create: `src/qml_gui/controls/CxProgressBar.qml`
Extends: `ProgressBar`
Spec: UI-SPEC Section 5.6

Key properties:
- `barHeight: 6`
- `fillColor: Theme.accent`
- `trackColor: Theme.borderSubtle`
- Normal/Complete/Error/Disabled states
- `NumberAnimation { duration: 150 }` on fill width

**Verify:** File exists, no hardcoded hex

### Task C7: CxTextArea

Create: `src/qml_gui/controls/CxTextArea.qml`
Extends: `TextArea`
Spec: UI-SPEC Section 5.7

Identical styling to CxTextField (same tokens). Adds scroll via CxScrollView pattern.

**Verify:** File exists, no hardcoded hex

### Wave C Gate

```bash
ls src/qml_gui/controls/CxSwitch.qml src/qml_gui/controls/CxDialog.qml \
    src/qml_gui/controls/CxPopup.qml src/qml_gui/controls/CxMenu.qml \
    src/qml_gui/controls/CxMenuItem.qml src/qml_gui/controls/CxScrollView.qml \
    src/qml_gui/controls/CxProgressBar.qml src/qml_gui/controls/CxTextArea.qml
# All must exist
grep -rn '#[0-9a-fA-F]\{6\}' src/qml_gui/controls/ --include="*.qml"
# Expected: 0
```

Build: verify compilation passes

---

## Wave D: Migrate Raw Usages in Consuming Files

Order by impact (highest instance count first).

### Task D1: PreferencesPage — Switch + Button migration

File: `src/qml_gui/pages/PreferencesPage.qml`

- Replace 12 `Switch {` → `CxSwitch {`
- Replace 2 `Button {` → `CxButton {`
- Remove inline color overrides on each
- Add `import "../controls"` if missing

**Verify:** `grep -c 'Switch\s*{' src/qml_gui/pages/PreferencesPage.qml` → 0

### Task D2: PrintSettings — Switch + ComboBox + Popup migration

File: `src/qml_gui/panels/PrintSettings.qml`

- Replace `Switch {` → `CxSwitch {`
- Replace raw `ComboBox {` → `CxComboBox {` (remove 40+ lines of inline styling per instance)
- Replace `Popup {` → `CxPopup {`
- Replace `Button {` → `CxButton {`
- Remove inline background/border/delegate overrides

**Verify:** `grep -cE '^\s*(Switch|ComboBox|Popup|Button)\s*{' src/qml_gui/panels/PrintSettings.qml` → 0

### Task D3: SliceProgress — ProgressBar migration

File: `src/qml_gui/panels/SliceProgress.qml`

- Replace `ProgressBar {` → `CxProgressBar {`
- Remove inline background/contentItem overrides

**Verify:** `grep -c 'ProgressBar\s*{' src/qml_gui/panels/SliceProgress.qml` → 0

### Task D4: Dialogs — Dialog migration (14 files)

Files: All `src/qml_gui/dialogs/*.qml`

For each dialog:
1. `Dialog {` → `CxDialog {`
2. Move title text to `dialogTitle:` property
3. Remove `background:` Rectangle (CxDialog provides it)
4. Remove `header:` Rectangle (CxDialog provides it)
5. Remove close button MouseArea (CxDialog provides it)
6. Keep content and signal handlers as-is

Target files:
- AboutDialog.qml
- AMSSettingsDialog.qml
- BedShapeDialog.qml
- CalibrationDialog.qml
- CaliHistoryDialog.qml
- ConfigWizardDialog.qml
- EditGCodeDialog.qml
- EnableLiteModeDialog.qml
- FirmwareDialog.qml
- PrintDialog.qml
- PrintHostDialog.qml
- SpeedLimitDialog.qml
- WipeTowerDialog.qml
- PluginManagerDialog.qml

**Verify:** `grep -rl 'Dialog\s*{' src/qml_gui/dialogs/ --include="*.qml" | wc -l` → 0

### Task D5: main.qml — Menu migration

File: `src/qml_gui/main.qml`

- Replace `Menu {` → `CxMenu {`
- Replace `MenuItem {` → `CxMenuItem {`
- Remove any inline background/delegate overrides on menus
- ~5 menus, ~30 MenuItems

**Verify:** `grep -cE '^\s*(Menu|MenuItem)\s*{' src/qml_gui/main.qml` → 0

### Task D6: PreparePage — Menu + ComboBox migration

File: `src/qml_gui/pages/PreparePage.qml`

- Replace `Menu {` → `CxMenu {`, `MenuItem {` → `CxMenuItem {`
- Replace raw `ComboBox {` → `CxComboBox {` (5 instances, ~45 lines each)
- Replace raw `TextField {` → `CxTextField {`
- Replace raw `Slider {` → `CxSlider {`
- Replace `Switch {` → `CxSwitch {`

**Verify:** `grep -cE '^\s*(Menu|MenuItem|ComboBox|TextField|Slider|Switch)\s*{' src/qml_gui/pages/PreparePage.qml` → 0

### Task D7: ObjectList — Menu migration

File: `src/qml_gui/panels/ObjectList.qml`

- Replace `Menu {` → `CxMenu {`, `MenuItem {` → `CxMenuItem {`
- ~5 menus, ~40 MenuItems

**Verify:** `grep -cE '^\s*(Menu|MenuItem)\s*{' src/qml_gui/panels/ObjectList.qml` → 0

### Task D8: SettingsPage — Switch + ComboBox + Slider migration

File: `src/qml_gui/pages/SettingsPage.qml`

- Replace `Switch {` → `CxSwitch {`
- Replace `ComboBox {` → `CxComboBox {`
- Replace `Slider {` → `CxSlider {`

**Verify:** `grep -cE '^\s*(Switch|ComboBox|Slider)\s*{' src/qml_gui/pages/SettingsPage.qml` → 0

### Task D9: HomePage — TextField migration

File: `src/qml_gui/pages/HomePage.qml`

- Replace `TextField {` → `CxTextField {`
- Remove inline background overrides

**Verify:** `grep -c 'TextField\s*{' src/qml_gui/pages/HomePage.qml` → 0

### Task D10: Remaining files — residual migration

Files: MonitorPage, MultiMachinePage, ModelMallPage, CalibrationPage, StatsPanel, LayerSlider, SearchDialog, other panels

- Replace any remaining raw `Switch`, `ComboBox`, `TextField`, `Slider`, `SpinBox`, `ProgressBar`, `Popup`, `ScrollView` with Cx* equivalents
- Replace any remaining raw `Button`, `CheckBox` with CxButton, CxCheckBox

### Wave D Gate

```bash
# All gates must pass:
grep -rn '^\s*Switch\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml" | wc -l
# → 0

grep -rn '^\s*Button\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml" | wc -l
# → 0

grep -rn '^\s*ProgressBar\s*{' src/qml_gui/pages/ src/qml_gui/panels/ src/qml_gui/dialogs/ --include="*.qml" | wc -l
# → 0
```

Build: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

---

## Final Verification

1. **Hardcoded hex gate:** `grep -rn '#[0-9a-fA-F]\{6\}' src/qml_gui/controls/ --include="*.qml"` → 0
2. **Raw control gate:** No raw `Switch`, `Button`, `ProgressBar`, `ComboBox`, `TextField`, `Slider`, `SpinBox`, `CheckBox` in pages/panels/dialogs
3. **Build:** `auto_verify_with_vcvars.ps1` passes with 0 QML warnings
4. **Visual:** Each page renders correctly (no broken colors/layouts from token migration)

---

## Task Summary

| Wave | Tasks | New Files | Files Modified | Est. LOC Changed |
|------|-------|-----------|---------------|------------------|
| A | 1 | 0 | 1 (Theme.qml) | ~15 |
| B | 7 | 0 | 7 (Cx* controls) | ~200 |
| C | 7 | 8 | 0 | ~600 |
| D | 10 | 0 | ~25 | ~800 (net: -350 from boilerplate removal) |
| **Total** | **25** | **8** | **~33** | **~1265** |
