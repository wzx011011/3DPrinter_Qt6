# Phase 3 -- UI Review

**Audited:** 2026-06-01
**Baseline:** Abstract 6-pillar standards (no UI-SPEC.md exists)
**Screenshots:** Not captured (no dev server -- code-only audit)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | qsTr() used consistently; two untranslated string fragments in PreviewPage |
| 2. Visuals | 3/4 | Clear hierarchy via Theme tokens; icon-only elements lack aria-labels |
| 3. Color | 3/4 | Theme singleton well-structured with 39 tokens; 61+ hardcoded hex remain in PrintSettings, 40 in PreparePage |
| 4. Typography | 2/4 | Only 162/365 (44%) of font.pixelSize values use Theme tokens across audited files |
| 5. Spacing | 3/4 | Theme spacing tokens exist but adoption is partial; many raw integer values remain |
| 6. Experience Design | 3/4 | Loading, error, and empty states present in some files; not uniformly covered |

**Overall: 17/24**

---

## Top 3 Priority Fixes

1. **Typography token adoption at 44%** -- Only 162 of 365 font.pixelSize assignments across the 9 audited files use Theme.fontSize tokens. SliceProgress is 5/39 (13%), LayerSlider is 1/15 (7%), StatsPanel is 6/34 (18%). Replace all hardcoded pixelSize values that match a Theme token (10, 11, 12, 14, 16, 20) with the corresponding token.

2. **PrintSettings.qml retains 61 hardcoded hex colors** -- Plan 02 targeted reducing from 98 to <=12, but the file still has 61. Value-chain level indicator colors, delete-button backgrounds (#2e1a1a), hover states (#1c2a3e), and the modified-option highlight (#e8864a) were intentionally kept, but many panel backgrounds and borders remain hardcoded (lines 267, 362, 558, 576, 585, 594-595, 641, 650, 863, 881, 890, 899-900, 953, 970, 996, 1013, 1062, 1071, 1080-1081, 1256, 1291, 1301, 1303, 1308, 1468-1469, 1483, 1556, 1569, 1573, 1589, 1600, 1603, 1618, 1688, 1701, 1704). Audit and tokenize these remaining non-semantic colors.

3. **PreviewPage line 166-167 has untranslated Chinese fragments** -- `" 层 · "` and `" 步"` are concatenated with numeric values outside qsTr(), making them invisible to the translation system. Wrap with qsTr() and use arg() substitution for i18n correctness.

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

**Good:** All CTA labels, button text, section headers, and empty-state messages across the 9 audited files consistently use qsTr(). No generic labels like "Click Here" or "Submit" found. Dialog buttons properly use qsTr("OK") and qsTr("Cancel"). Error/warning messages use qsTr() throughout.

**Finding C1 (WARNING):** PreviewPage.qml lines 166-167 concatenate Chinese text outside qsTr():
```qml
text: (root.previewVm ? root.previewVm.layerCount : 0) + " 层 · "
      + (root.previewVm ? root.previewVm.moveCount : 0) + " 步"
```
These fragments (" 层" and " 步") will not be extracted for translation. Should be:
```qml
text: qsTr("%1 layers · %2 moves").arg(...).arg(...)
```

**Finding C2 (WARNING):** SliceProgress.qml line 314 similarly concatenates `qsTr(" 层")` as a suffix. While the qsTr() call is present, the concatenation approach (number + qsTr(" 层")) may produce grammatically incorrect results in languages with different word order. Prefer qsTr("%1 layers").arg(value).

### Pillar 2: Visuals (3/4)

**Good:** Visual hierarchy is clear: section headers use Theme.fontSizeLG + font.bold: true, labels use textSecondary/textTertiary, values use textPrimary. The Theme singleton provides a well-differentiated background stack (bgBase -> bgSurface -> bgPanel -> bgElevated -> bgHover). Accent color (green #18c75e) is reserved for actionable/success elements.

**Finding V1 (WARNING):** Camera preset buttons in PreviewPage.qml lines 113-134 are 26x26px -- well below the 44x44px recommended touch target minimum. The MouseArea fills the full Rectangle, so the hit area is only 26x26px. Add transparent padding or increase the Rectangle size.

**Finding V2 (WARNING):** SliceProgress status icon area (line 48) uses Unicode emoji characters ("gear" U+2699, "check" U+2714, "cloud" U+2601). Emoji rendering varies across platforms and may not match the dark theme aesthetic. Consider using SVG icons or icon font for consistency.

### Pillar 3: Color (3/4)

**Good:** Theme.qml is well-structured with 39 color tokens, 6 new tokens added in this phase (bgTooltip, textMuted, borderInput, bgErrorSubtle, bgWarningSubtle, radiusXXL). Token adoption is strong for background and border colors in SliceProgress (69 Theme refs, 5 hardcoded hex), PreviewPage (40 Theme refs, 0 hardcoded hex), LayerSlider (37 Theme refs, 2 hardcoded hex -- both "#fff" for active thumb text, acceptable), StatsPanel (48 Theme refs, 1 hardcoded hex -- semantic green/yellow/red), and Sidebar (44 Theme refs, 0 hardcoded hex after axis colors in model data).

**Hardcoded hex audit by file:**

| File | Hardcoded | Theme Refs | Assessment |
|------|-----------|------------|------------|
| SliceProgress.qml | 5 | 69 | Acceptable: extruder brand colors, hover states |
| PreviewPage.qml | 0 | 40 | Clean |
| LayerSlider.qml | 2 | 37 | Clean: "#fff" on active thumb text |
| StatsPanel.qml | 1 | 48 | Clean: semantic green/yellow/red |
| MoveSlider.qml | 2 | 8 | Acceptable: semantic teal colors |
| Sidebar.qml | 0* | 44 | Clean (*axis colors in model data, intentional) |
| PrintSettings.qml | 61 | 161 | Needs work: many non-semantic colors remain |
| PreparePage.qml | 40 | 370 | Acceptable: axis, brand, semantic, gradient colors |

**Finding C3 (BLOCKER):** PrintSettings.qml has 61 remaining hardcoded hex colors, far exceeding the plan target of <=12. Many are non-semantic:
- Delete button hover backgrounds: `#2e1a1a` (lines 594, 899, 1080, 1468, 1600, 1701)
- Save/rename button hover backgrounds: `#1c2a3e` (lines 576, 585, 650, 881, 890, 1062, 1071, 1483, 1569, 1589)
- Delete icon color: `#e06666` (lines 595, 900, 1081, 1469, 1603, 1704)
- Active option highlight: `#1c6e42` (lines 996, 1256, 1569)
- Search clear hover: `#1e2535` (line 1556)
- Revert button: `#2e3a1a`, `#5a3a2a`, `#e8864a` (lines 1301, 1303, 1308)
- Value chain popup: 12+ colors (lines 169-267, some semantic but many are background shades)

These should be tokenized as they are recurring structural colors, not one-off brand/semantic values.

### Pillar 4: Typography (2/4)

**Finding T1 (BLOCKER):** Font token adoption across the 9 audited files is only 162/365 (44%):

| File | Themed | Total | Percentage |
|------|--------|-------|------------|
| SliceProgress.qml | 5 | 39 | 13% |
| PreviewPage.qml | 2 | 5 | 40% |
| LayerSlider.qml | 1 | 15 | 7% |
| StatsPanel.qml | 6 | 34 | 18% |
| MoveSlider.qml | 0 | 5 | 0% |
| Sidebar.qml | 2 | 23 | 9% |
| PrintSettings.qml | 72 | 78 | 92% |
| PreparePage.qml | 74 | 166 | 45% |

The Plan 01 spec explicitly required normalizing font sizes (e.g., `font.pixelSize: 11 -> Theme.fontSizeSM`, `12 -> Theme.fontSizeMD`, `14 -> Theme.fontSizeLG`). This was largely not executed outside of PrintSettings and PreparePage.

SliceProgress.qml has 34 hardcoded pixelSize values matching Theme token sizes (11, 12, 14) that were not tokenized. LayerSlider has 14. StatsPanel has 28. MoveSlider has 5.

**Finding T2 (WARNING):** Font sizes of 8, 9, and 13 appear throughout (e.g., SliceProgress lines 171, 177 use font.pixelSize: 9; PrintSettings line 389 uses font.pixelSize: 8; LayerSlider line 444 uses font.pixelSize: 13). These have no Theme token. The research document acknowledges this but does not resolve whether new tokens should be added. Values 8 and 9 are legitimate micro-label sizes. font.pixelSize: 13 appears in LayerSlider jump dialog input -- this should use Theme.fontSizeMD (12) or a new token.

### Pillar 5: Spacing (3/4)

**Good:** Theme provides 6 spacing tokens (XS=4 through XXL=24). Spacing adoption is reasonable in PreviewPage (uses Theme.spacingMD, Theme.spacingLG, Theme.spacingSM), LayerSlider (Theme.spacingSM), and StatsPanel (Theme.spacingSM). The Theme also provides controlHeight tokens (28/34/40) used for buttons and inputs.

**Finding S1 (WARNING):** SliceProgress.qml uses raw integer spacing values: `spacing: 12` (line 32), `spacing: 6` (line 44), `spacing: 4` (line 67), `spacing: 4` (line 151). All of these match Theme spacing tokens (spacingLG, spacingSM, spacingXS). Similarly, PreviewPage uses `spacing: 4` (line 104) and `anchors.margins: 14` (line 83) -- 14 has no Theme token but is used as a consistent page-level margin.

**Finding S2 (WARNING):** Control heights are inconsistent with Theme tokens. SliceProgress buttons are height: 28 (matches Theme.controlHeightSM) but height: 30 for the slice/cancel button (line 499). PreviewPage camera preset buttons are 26x26 (no token match). These small inconsistencies prevent full Theme token adoption.

### Pillar 6: Experience Design (3/4)

**Good:** Loading state coverage exists: slicing state uses the `slicingNow` boolean to show animated progress (SliceProgress). Error state coverage: `bgErrorSubtle` token for error backgrounds, `canRequestSlice` guard with visual disabled state, error-tinted border and text when slicing is blocked. Empty state: `hasSliceResult` controls visibility of the results panel. Cancel confirmation: the cancel button shows different text during slicing vs idle.

**Finding E1 (WARNING):** LayerSlider jump dialog (lines 408-484) has no input validation feedback. If the user types an out-of-range number and presses Enter, nothing happens silently. Should show an inline error or shake animation.

**Finding E2 (WARNING):** No loading skeleton or spinner for the PreviewPage viewport. When switching from Prepare to Preview, the GL viewport loads synchronously but there is no visual indicator during G-code data loading. The `slicing` property on PreviewViewModel exists but no skeleton UI is shown while data populates.

**Finding E3 (WARNING):** MoveSlider.qml (line 65) has a fallback hex `#009688` when `previewVm` is null -- this is a defensive default but the visual effect of a teal-colored band appearing during null state is unexpected. Should fall back to Theme.borderSubtle or a neutral color.

---

## Registry Safety

**Registry audit:** Skipped -- no components.json exists (not a shadcn/npm project). This is a Qt6/QML C++ project with no third-party UI registry dependencies.

---

## Files Audited

- `src/qml_gui/Theme.qml` (98 lines) -- Theme singleton with 39 color, 6 font, 6 spacing, 5 radii, and control sizing tokens
- `src/qml_gui/panels/SliceProgress.qml` (620 lines) -- Slice progress panel
- `src/qml_gui/pages/PreviewPage.qml` (303 lines) -- Preview mode page
- `src/qml_gui/components/LayerSlider.qml` (489 lines) -- Dual-thumb layer range slider
- `src/qml_gui/components/StatsPanel.qml` (317 lines) -- Statistics panel with bar chart
- `src/qml_gui/components/MoveSlider.qml` (169 lines) -- Move timeline slider
- `src/qml_gui/panels/Sidebar.qml` (510 lines) -- Right sidebar with tabs
- `src/qml_gui/panels/PrintSettings.qml` (~1700 lines, first 100 lines + full hardcoded hex audit) -- Print settings panel
- `src/qml_gui/pages/PreparePage.qml` (~3600 lines, full hardcoded hex audit) -- Prepare workspace page
