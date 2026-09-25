# Cx* Controls — UI Review (v5.1 retroactive, 16 controls)

**Audited:** 2026-07-17
**Scope:** `src/qml_gui/controls/` — every Cx* primitive (16 files)
**Baseline:** `Theme.qml` token system + Qt Quick Controls 2 conventions + cross-control consistency
**Mode:** Code-only audit (no dev server, no screenshots)
**Adversarial stance:** Foundation layer — every inconsistency here propagates to the entire app. No score inflation.

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 2/4 | Embedded glyph text (`"✓"` `"▾"` `"✕"` `"▲"` `"▼"`) hardcoded in controls; **zero `qsTr()` wrappers** in entire library; missing `placeholderText` defaults |
| 2. Visuals | 3/4 | Visual hierarchy mostly clean, but **only CxIconButton exposes ToolTip** — 15 controls have no tooltip support; visual style enums diverge (`cxStyle` vs `primary` vs none) |
| 3. Color | 2/4 | Strong token usage but **3 controls manipulate tokens at runtime** (`Qt.darker(statusError, 1.5)` in CxButton, `Qt.darker(accentSubtle, 1.1)` in CxIconButton); **2 distinct disabled-state patterns** (opacity 0.45 vs hardcoded color branches) |
| 4. Typography | 2/4 | Token adoption is good, but **`fontSizeXS - 2` arithmetic in CxSpinBox** (8px) bypasses the scale and falls below the documented XS=10 floor; CxButton uses `fontSizeMD`=12 for body text — undersized |
| 5. Spacing | 2/4 | Three different padding vocabularies: token (`spacingLG`) in CxButton/CxTextField, **magic `14` literals** in CxPillAction, **absent** in CxComboBox content/delegate; no consistent height scale (28/34/40 honored in some, `44` magic in CxDialog header, `20` magic in CxSwitch) |
| 6. Experience Design | 2/4 | Press-scale animation exists in only 3 of 16 controls (CxIconButton/CxSlider/CxSwitch); **CxButton has no press-scale** despite being the most-clicked control; missing focus border on CxButton; signal verbs are consistent (`clicked`/`activated`/`triggered`/`toggled` match Qt QC2 defaults) but CxPillAction reuses `clicked` for what should be `activated`/`triggered` |

**Overall: 13/24**

This is a **needs-work** score for a design-system foundation. Token *naming* is honored broadly; token *discipline* (no runtime manipulation, consistent scale use) is not.

---

## Top 3 Priority Fixes

1. **Replace runtime `Qt.darker()` color manipulation with explicit Theme tokens** — `CxButton.qml:31-32` (`Qt.darker(Theme.statusError, 1.5/1.1)`) and `CxIconButton.qml:48` (`Qt.darker(Theme.accentSubtle, 1.1)`) produce unthemeable, untestable colors that bypass the design system. Fix: add `statusErrorDark` / `statusErrorPressed` and `accentSubtlePressed` tokens to `Theme.qml`, reference them directly. This is the #1 systemic defect — runtime manipulation defeats the entire point of a token system.

2. **Standardize the disabled-state pattern** — Currently two patterns coexist: (a) `opacity: root.enabled ? 1.0 : 0.45` on the background (CxButton/CxSpinBox/CxSwitch/CxTextField/CxTextArea/CxComboBox/CxCheckBox/CxSlider/CxProgressBar) and (b) hardcoded color branches (`if (!root.enabled) return Theme.bgPanel`, etc.). Some controls do **both** — opacity multiplies over an already-dimmed color, producing inconsistent dimness across the library. Fix: pick **one** pattern (opacity on the whole control via `enabled: false` + a shared `opacity` binding), apply it uniformly, delete the color branches.

3. **Backfill the missing state-coverage gaps across the library** — CxButton has **no press-scale** animation while CxIconButton/CxSlider/CxSwitch all have it (`scale: pressed ? 0.92-0.95 : 1.0`); CxButton/CxPillAction have **no focus border** while CxComboBox/CxSpinBox/CxTextField/CxTextArea all show `Theme.borderFocus` on `activeFocus`; **only CxIconButton** has ToolTip support. Fix: define a shared `Behavior on scale { NumberAnimation { duration: 100 } }` recipe and apply to all interactive controls; standardize a `toolTipText` property across all interactive controls (not just CxIconButton); add focus-border to CxButton/CxPillAction.

---

## Control API/Behavior Matrix

Legend: ✓ = present, ✗ = missing, ◐ = partial.

| Control | Default height | Hardcoded colors | Hover | Press | Disabled | Focus | Signal verb | Notes |
|---|---|---|---|---|---|---|---|---|
| **CxButton** | `controlHeightSM`(28)/`controlHeightMD`(34) via `compact` | Qt.darker x2 (statusError), `transparent` | ✓ color fade | ✓ color, ✗ no scale | ◐ opacity 0.45 + color branch | ✗ no focus border | `clicked` | No press-scale; animation duration 120ms only |
| **CxCheckBox** | inherited (no override) | `transparent` (indicator) | ✓ border color | ✗ no press feedback | ◐ opacity 0.45 + color branch | ✗ no focus | `toggled`/`clicked` | Indicator fixed 16x16; checkmark glyph hardcoded |
| **CxComboBox** | `controlHeightSM` (28) | `transparent` (delegate) | ✓ bg color | ✓ bg color | ◐ opacity 0.45 + color branch | ✓ border focus | `activated` | No press-scale; delegate height = `controlHeightSM - 2` (magic) |
| **CxDialog** | n/a | `transparent` (close btn) | ✓ close btn bg | ✗ no press scale | n/a | ✗ no focus ring | `accepted`/`rejected` | Header height magic `44`; close button magic `28x28`; hardcoded opacity `0.3` |
| **CxIconButton** | `iconButtonSizeMD` (34) | `transparent`, `Qt.darker` x1 | ✓ color, ✓ border | ✓ scale 0.92 + 0.96 | ◐ opacity 0.45 + color branch | ✗ no focus | `clicked` | Richest control; sets the bar the others miss |
| **CxMenu** | inherited | none | ✗ no override | ✗ | ✗ | ✗ | n/a (parent) | Skeleton wrapper (15 lines); no padding/margin override |
| **CxMenuItem** | inherited | `transparent` | ✓ bg color | ✓ bg color | ✓ color branch only (no opacity) | ✗ no focus | `triggered` | Arrow Canvas hardcodes `8x8`; no disabled opacity |
| **CxPillAction** | `pillHeight` (34) | `transparent` (implicit) | ✓ color | ✓ color, ✗ no scale | ✗ **no disabled handling at all** | ✗ no focus | `clicked` | Padding literals `14`; `spacing: 6` literal; no enabled-state styling |
| **CxPopup** | n/a | none | n/a | n/a | n/a | n/a | `opened`/`closed` | Correctly tokenized; thin wrapper |
| **CxProgressBar** | n/a (`barHeight: 6`) | none | ✗ n/a | ✗ n/a | ◐ opacity 0.45 | ✗ | n/a | `barHeight: 6` literal; no streaming/disabled color |
| **CxScrollView** | n/a | none | ✗ n/a | ✗ n/a | ✗ | ✗ | n/a | Scrollbar radius magic `4`; color uses `bgPressed` (semantic mismatch) |
| **CxSlider** | `20` literal | none | ✗ no hover on handle | ✓ scale 0.95 | ◐ opacity 0.45 | ✗ no focus border | `valueChanged`/`moved` | implicitHeight magic `20`; track magic `4`; handle magic `14` |
| **CxSpinBox** | `controlHeightSM` (28) | `transparent` (up/down) | ✓ up/down | ✓ up/down | ◐ opacity 0.45 + color branch | ✓ border focus | `valueModified` | `fontSizeXS - 2` (=8px) breaks typography scale; magic `22` width |
| **CxSwitch** | inherited | none | ✓ color | ✓ scale 0.9 | ◐ opacity 0.45 + color branch | ✗ no focus | `toggled` | Track magic `36x20`, knob magic `16`; press-scale present |
| **CxTextArea** | inherited | none | ✓ border | ✗ no press | ◐ opacity 0.45 + color branch | ✓ border focus | `textChanged` | No implicitHeight → collapses without external sizing |
| **CxTextField** | `controlHeightSM` (28) | none | ✓ border | ✗ no press | ◐ opacity 0.45 + color branch | ✓ border focus | `editingFinished`/`textChanged` | Best-tokenized input control |

---

## Theme Token Compliance Per Control

Methodology: count color/size/spacing references against literal vs `Theme.*` references.

| Control | Color refs | Hardcoded color | Size refs | Hardcoded size | Verdict |
|---|---|---|---|---|---|
| CxButton | 14 | 3 (Qt.darker x2 + transparent) | 4 | 0 | **WARN** — runtime color manipulation |
| CxCheckBox | 6 | 1 (transparent border) | 2 | 0 | PASS |
| CxComboBox | 9 | 1 (transparent delegate) | 4 | 1 (`controlHeightSM - 2`) | **WARN** — magic subtraction |
| CxDialog | 9 | 1 (transparent close bg) | 6 | 3 (`44`, `28`, `0.3` opacity) | **FAIL** — multiple magic sizes |
| CxIconButton | 16 | 4 (transparent x3 + Qt.darker) | 6 | 2 (`iconSize: 16` default, height/4 radius) | **WARN** — runtime manipulation |
| CxMenu | 2 | 0 | 0 | 0 | PASS (trivial) |
| CxMenuItem | 4 | 2 (transparent) | 3 | 2 (`8x8` arrow) | **WARN** — magic arrow |
| CxPillAction | 9 | 0 | 5 | 4 (`14`, `14`, `6`, `14x14`) | **FAIL** — no spacing tokens at all |
| CxPopup | 2 | 0 | 1 | 0 | PASS |
| CxProgressBar | 2 | 0 | 4 | 1 (`barHeight: 6`) | **WARN** — magic bar height |
| CxScrollView | 2 | 0 | 4 | 4 (`8`, `100`, `4`, `100`, `8`, `4`) | **FAIL** — entirely magic |
| CxSlider | 5 | 0 | 6 | 3 (`20`, `4`, `14`, `7`) | **FAIL** — magic sizes |
| CxSpinBox | 11 | 1 (transparent) | 7 | 4 (`22`, `22`, `fontSizeXS-2`, `6`) | **FAIL** — magic + scale break |
| CxSwitch | 7 | 0 | 4 | 3 (`36`, `20`, `16`, `10`, `8`) | **FAIL** — magic sizes |
| CxTextArea | 6 | 0 | 1 | 0 | PASS |
| CxTextField | 6 | 0 | 2 | 0 | PASS |

**Token-compliance summary:** 6 controls fully token-clean, 4 with warnings (runtime manipulation / minor magic), **6 with failing compliance** (multiple magic sizes that should be tokens). CxPillAction is the worst offender in the input-family — it uses **zero** spacing tokens.

**Token coverage:** approximately **60%** of all property references use `Theme.*` — but the **sizing token coverage** is much weaker than color coverage. The library added color tokens carefully but **never added sizing tokens** for common control internals (slider tracks, switch tracks, scrollbar thumbs, dialog headers, spin buttons).

---

## Detailed Findings

### Pillar 1: Copywriting (2/4)

**Findings:**

- **BLOCKER — Zero `qsTr()` wrappers across the entire control library.** A grep for `qsTr|qsTranslate` in `src/qml_gui/controls/` returns **zero matches**. AGENTS.md QML conventions mandate `qsTr()` for all user-visible strings. The hardcoded glyph literals below are not user-visible strings (they render glyphs), but this means **no control-level copy is translatable**.
- **WARNING — Hardcoded glyph text in 5 controls:**
  - `CxCheckBox.qml:25` — `text: "✓"` (checkmark)
  - `CxComboBox.qml:39` — `text: "▾"` (dropdown caret)
  - `CxDialog.qml:63` — `text: "✕"` (close X)
  - `CxSpinBox.qml:63,72` — `text: "▲"` / `text: "▼"` (spin arrows)
  - These glyphs are not emoji-rendered; on Windows with the default font they are typographically inconsistent. Should be SVG icons sourced through `Theme` or an icon registry.
- **WARNING — No default `placeholderText`** in CxTextField or CxTextArea — consumers must always supply one or the field renders empty with no affordance. Should default to `qsTr("Enter text…")` or accept the empty state explicitly.
- **WARNING — No empty-state copy** for CxComboBox when `model` is empty (renders as blank rectangle).

### Pillar 2: Visuals (3/4)

**Findings:**

- **WARNING — ToolTip support exists in only 1 of 16 controls.** `CxIconButton.qml:92-94` exposes `toolTipText` and binds it to `ToolTip`. None of CxButton / CxComboBox / CxSpinBox / CxTextField / CxSlider / CxSwitch / CxCheckBox expose tooltip properties. For a dense app like a slicer, this is a real usability gap — iconless controls in toolbars get no hover help.
- **WARNING — Three divergent visual-style APIs:**
  - `CxButton`: `property int cxStyle` + `enum Style { Primary, Secondary, Danger, Ghost }`
  - `CxIconButton`: `property int cxStyle` + `enum Style { Surface, Ghost, Chrome, ChromeDanger }`
  - `CxPillAction`: `property bool primary` (boolean, not enum)
  - Other styled controls (none): no style API at all
  - Result: consumers must learn a different API per control. The enum naming spaces also clash — `CxButton.Style.Ghost` and `CxIconButton.Style.Ghost` are different values.
- **PASS — Visual hierarchy is clear** in CxDialog (header/body/footer), CxComboBox (selected vs hover vs default), CxIconButton (selected glow overlay at lines 69-76).

### Pillar 3: Color (2/4)

**Findings:**

- **BLOCKER — Runtime `Qt.darker()` color manipulation bypasses the token system.**
  - `CxButton.qml:31` — `Qt.darker(Theme.statusError, 1.5)` (disabled Danger style)
  - `CxButton.qml:32` — `Qt.darker(Theme.statusError, 1.1)` (pressed Danger style)
  - `CxIconButton.qml:48` — `Qt.darker(Theme.accentSubtle, 1.1)` (pressed selected style)
  - These produce colors that are **not in `Theme.qml`** and **cannot be overridden by theme authors**. This is the most serious systemic defect in the library — it defeats the entire purpose of a token system. The Theme already demonstrates the correct pattern (`accent`/`accentLight`/`accentDark`/`accentSubtle` as separate tokens).
- **WARNING — Two coexisting disabled-state patterns.** Most controls set `opacity: root.enabled ? 1.0 : 0.45` on the background rectangle AND branch on color (`if (!root.enabled) return Theme.bgPanel`). The double-dimming produces inconsistent visual dimness — opacity 0.45 over a dimmed background is much darker than 0.45 over a normal background.
- **WARNING — `"transparent"` used as a magic value** (6 occurrences across CxButton, CxMenuItem, CxSpinBox, CxComboBox). Should be a `Theme.transparent` or just literal but documented as a sanctioned value.
- **PASS — Color tokens are otherwise well-used.** The accent/border/text/bg/switchTrack/progressFill tokens cover the library's needs without much drift.
- **PASS — Selection colors** (`Theme.selectionColor`, `Theme.selectionText`) correctly applied in CxSpinBox/CxTextField/CxTextArea.

### Pillar 4: Typography (2/4)

**Findings:**

- **BLOCKER — `CxSpinBox.qml:63,72` uses `font.pixelSize: Theme.fontSizeXS - 2`** (= 10 − 2 = 8). This bypasses the typography scale and produces **8px text** — below the documented XS floor (10px) and below WCAG legibility guidance. The spin arrow glyphs render at 8px which is unreadable at standard DPI. Fix: either add `fontSizeXXS: 8` to Theme (and document it) or use `fontSizeXS` directly.
- **WARNING — CxButton body text uses `fontSizeMD` = 12px.** Most desktop button libraries use 13–14px for primary actions. The `compact` variant drops to `fontSizeSM` = 11px which is very small. No size token covers the typical 13–14px button-text range; the gap between `fontSizeMD`=12 and `fontSizeLG`=14 is large.
- **WARNING — Font weight is inconsistent.** CxDialog header uses `font.bold: true` (`CxDialog.qml:46`); CxPillAction uses `font.bold: root.primary` (`CxPillAction.qml:50`); all other controls use regular weight. No `fontWeight` token exists in Theme — bold is applied ad-hoc.
- **PASS — Font sizes are tokenized** everywhere except the CxSpinBox arithmetic above.

### Pillar 5: Spacing (2/4)

**Findings:**

- **BLOCKER — CxPillAction uses zero spacing tokens.** `leftPadding: 14`, `rightPadding: 14`, Row `spacing: 6`, Image `width/height: 14`. These are the **exact same values** as `Theme.spacingLG` (12, close to 14) and `Theme.spacingSM` (6) — but they are written as literals. This is the cleanest example of token drift in the library.
- **WARNING — Magic heights outside the controlHeight scale:**
  - `CxDialog.qml:27` — header `height: 44` (not `controlHeightLG`=40, not a `dialogHeaderHeight` token)
  - `CxDialog.qml:56` — close button `width/height: 28` (matches `controlHeightSM` but written as literal)
  - `CxSlider.qml:8` — `implicitHeight: 20` (no `sliderHeight` token)
  - `CxSlider.qml:14` — track `height: 4` (no `sliderTrackHeight` token)
  - `CxSlider.qml:31` — handle `width/height: 14` (no `sliderHandleSize` token)
  - `CxSwitch.qml:11-12` — track `width: 36, height: 20` (no `switchTrackWidth`/`Height` tokens — even though Theme has `switchTrackOff`/`switchTrackOn`/`switchKnob` *color* tokens, the *sizes* are missing)
  - `CxSpinBox.qml:61,70` — spin button `implicitWidth: 22`
  - `CxComboBox.qml:67` — delegate `height: Theme.controlHeightSM - 2` (magic subtraction)
  - `CxProgressBar.qml:8` — `barHeight: 6`
  - `CxScrollView.qml:11,23` — scrollbar `implicitWidth/Height: 8/100`
- **WARNING — Inconsistent padding tokens.** CxButton uses `Theme.spacingLG` for left/right padding (correct). CxTextField uses `Theme.spacingMD` (correct). CxComboBox content uses `Theme.spacingMD`/`Theme.spacingXS` (correct). But **CxPillAction uses `14` literals** and **CxSpinBox suffix uses `6` literal** (`anchors.rightMargin: root.up.indicator.width + 6`). The library has the right vocabulary but does not speak it consistently.
- **PASS — `controlHeightSM/MD/LG` (28/34/40)** are honored by CxButton, CxComboBox, CxSpinBox, CxTextField — the four most-used controls. This is the strongest part of the spacing pillar.

### Pillar 6: Experience Design (2/4)

**Findings:**

- **BLOCKER — Press-scale animation present in only 3 of 16 controls.** CxIconButton (`scale: 0.92`), CxSlider (`scale: 0.95`), CxSwitch (`scale: 0.9`) implement press feedback via scale. **CxButton — the most-clicked control in any app — does not.** It only animates `color`. This is a visible inconsistency: clicking an icon button gives tactile feedback, clicking a primary text button does not. Press-scale values also differ (0.92 / 0.95 / 0.9) with no shared constant.
- **WARNING — Focus border missing on CxButton and CxPillAction.** CxComboBox/CxSpinBox/CxTextField/CxTextArea all show `Theme.borderFocus` on `activeFocus`. CxButton (which is keyboard-focusable) shows no focus indicator at all — a keyboard accessibility regression.
- **WARNING — ToolTip support missing on 15 controls** (see Pillar 2).
- **WARNING — CxPillAction has no disabled-state styling.** A grep for `enabled` in `CxPillAction.qml` returns only the inherited `Button.enabled`. Setting `enabled: false` produces no visual feedback — the pill looks identical whether enabled or disabled. This is the only interactive control in the library with this defect.
- **WARNING — CxMenuItem has no disabled opacity.** Uses `color: root.enabled ? Theme.textPrimary : Theme.textDisabled` (line 19) but no opacity dimming — inconsistent with the rest of the library which uses opacity 0.45.
- **PASS — Signal verbs are consistent with Qt QC2 conventions:**
  - `clicked` — CxButton, CxIconButton, CxPillAction (button-family)
  - `toggled` — CxCheckBox, CxSwitch (toggle-family)
  - `activated` — CxComboBox (selection-family)
  - `triggered` — CxMenuItem (menu-family)
  - `valueModified` — CxSpinBox (input-family)
  - `editingFinished` / `textChanged` — CxTextField (text-family)
  - `accepted` / `rejected` — CxDialog (dialog-family)
  - These all match Qt Quick Controls 2 defaults, which is correct. No drift detected.
- **WARNING — CxPillAction's `clicked` signal is semantically wrong.** A "pill action" is closer to a menu item or tab than a button — it should expose `activated` or `triggered` to distinguish from a generic button click. Minor but contributes to API inconsistency.
- **PASS — Animation durations cluster around 100/120/150ms** with `Easing.OutCubic` — a consistent feel. CxIconButton is slightly noisier (adds 200ms glow opacity). No control uses durations outside the 100-200ms range.
- **PASS — ColorAnimation on background color** is applied consistently across CxButton/CxCheckBox/CxComboBox/CxIconButton/CxMenuItem/CxSlider/CxSwitch/CxTextArea/CxTextField.

---

## Per-Control Verdicts (one line each)

| # | Control | Verdict |
|---|---|---|
| 1 | **CxButton** | WARNING — Primary control ships without press-scale or focus border; runtime `Qt.darker` on Danger style bypasses Theme. |
| 2 | **CxCheckBox** | PASS — Cleanest control in the library; only nit is hardcoded `"✓"` glyph and no focus indicator. |
| 3 | **CxComboBox** | WARNING — Solid token usage; delegate height magic `-2` and no press-scale. |
| 4 | **CxDialog** | WARNING — Multiple magic sizes (44/28); close button opacity `0.3` is ad-hoc; no focus ring. |
| 5 | **CxIconButton** | PASS (exemplar) — Richest state coverage in the library; the bar every other control should meet; only nit is `Qt.darker` on accentSubtle. |
| 6 | **CxMenu** | PASS — 15-line wrapper, correctly tokenized, nothing to object to. |
| 7 | **CxMenuItem** | WARNING — No disabled opacity (library uses 0.45 elsewhere); arrow Canvas hardcodes 8x8. |
| 8 | **CxPillAction** | **FAIL** — Zero spacing tokens (all literals), no disabled styling, no focus border, wrong signal verb. |
| 9 | **CxPopup** | PASS — Thin, correct, tokenized. |
| 10 | **CxProgressBar** | WARNING — `barHeight: 6` should be a token; no indeterminate/streaming state. |
| 11 | **CxScrollView** | **FAIL** — Entirely magic sizes (8/100/4); uses `bgPressed` for scrollbar (semantic mismatch — should be a `scrollBarColor` token). |
| 12 | **CxSlider** | WARNING — Magic 20/4/14/7 sizes throughout; no hover state on handle. |
| 13 | **CxSpinBox** | **FAIL** — `fontSizeXS - 2` breaks the typography scale (8px text); magic 22 width; suffix margin literal 6. |
| 14 | **CxSwitch** | WARNING — Track 36x20 and knob 16 are magic despite Theme having switch *color* tokens (sizes missing). |
| 15 | **CxTextArea** | PASS — Cleanly tokenized; missing implicitHeight default is the only nit. |
| 16 | **CxTextField** | PASS — Best-tokenized input control; serves as the model for CxTextArea/CxSpinBox. |

---

## Cross-Cutting Defects (systemic, not per-control)

1. **No shared animation mixin.** Every control redefines `Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }` verbatim. QML cannot truly mixin, but a `Primitives/Animations.qml` singleton with named animation objects would centralize tuning.
2. **No shared disabled-state pattern.** Library has 2 patterns (opacity vs color branch); some controls do both.
3. **No shared focus-border recipe.** Input controls have it, button controls do not.
4. **No shared tooltip recipe.** Only CxIconButton.
5. **Missing size tokens in Theme.qml:** `dialogHeaderHeight`, `sliderHeight`, `sliderTrackHeight`, `sliderHandleSize`, `switchTrackWidth`, `switchTrackHeight`, `switchKnobSize`, `spinButtonWidth`, `scrollBarThickness`, `progressBarHeight`, `checkBoxSize`. Adding these would eliminate the majority of magic-number findings.
6. **Missing color tokens in Theme.qml:** `statusErrorDark`, `statusErrorPressed`, `accentSubtlePressed`, `scrollBarColor`, `transparent` (or sanctioned literal).
7. **No focus visual on any non-input interactive control** (CxButton/CxIconButton/CxPillAction/CxCheckBox). Keyboard users get no affordance on these.

---

## Files Audited

- `src/qml_gui/Theme.qml` (token baseline)
- `src/qml_gui/controls/CxButton.qml`
- `src/qml_gui/controls/CxCheckBox.qml`
- `src/qml_gui/controls/CxComboBox.qml`
- `src/qml_gui/controls/CxDialog.qml`
- `src/qml_gui/controls/CxIconButton.qml`
- `src/qml_gui/controls/CxMenu.qml`
- `src/qml_gui/controls/CxMenuItem.qml`
- `src/qml_gui/controls/CxPillAction.qml`
- `src/qml_gui/controls/CxPopup.qml`
- `src/qml_gui/controls/CxProgressBar.qml`
- `src/qml_gui/controls/CxScrollView.qml`
- `src/qml_gui/controls/CxSlider.qml`
- `src/qml_gui/controls/CxSpinBox.qml`
- `src/qml_gui/controls/CxSwitch.qml`
- `src/qml_gui/controls/CxTextArea.qml`
- `src/qml_gui/controls/CxTextField.qml`
- Cross-referenced consumers: `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/components/GLToolbars.qml`, `src/qml_gui/pages/PreparePage.qml`, `src/qml_gui/dialogs/*.qml`
