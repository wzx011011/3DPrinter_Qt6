# Phase 3: UI Polish Pass - Research

**Researched:** 2026-06-01
**Domain:** QML visual consistency, Theme token compliance, upstream visual alignment
**Confidence:** HIGH

## Summary

This research audits the five core slicing-workflow pages (PreparePage, PrintSettings, SliceProgress, PreviewPage, and supporting components) for visual polish readiness. The codebase has a well-structured Theme singleton (`Theme.qml`) with 33 color tokens, 6 font size tokens, 6 spacing tokens, 4 radii tokens, and control sizing constants. However, the audit reveals approximately 200+ hardcoded hex color literals across 49 QML files, inconsistent use of Theme font/spacing tokens, and several layout patterns that deviate from the upstream wxWidgets reference.

The primary recommendation is to systematically replace hardcoded color/font/spacing values with Theme tokens, normalize control heights to Theme constants, and fix the most visually impactful inconsistencies first. This is a mechanical but large-scope pass -- each file should be audited individually but the pattern is uniform: find hex literal, map to closest Theme token, replace.

**Primary recommendation:** Execute this as a series of per-file mechanical replacements, not as an architectural change. The Theme token system is already good; the problem is incomplete adoption.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Theme token compliance | QML Presentation Layer | -- | Visual polish is purely a QML concern; no C++ changes needed |
| Layout spacing normalization | QML Presentation Layer | -- | Spacing and margin values live in QML bindings |
| Font size normalization | QML Presentation Layer | -- | font.pixelSize assignments are QML-side |
| Upstream visual alignment | QML + C++ ViewModel | -- | Some visual differences may require ViewModel property changes (e.g., missing data) |
| Accessibility basics | QML Presentation Layer | -- | Touch targets, contrast, keyboard nav are QML-side |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6.10 QML | 6.10 | UI framework | Project standard |
| Theme singleton | custom | Design token system | Already in `src/qml_gui/Theme.qml` |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt Quick Controls 2 | 6.10 | Base controls | Standard for all Cx* custom controls |
| Qt Quick Layouts | 6.10 | Layout engine | Standard for all page/panel layouts |

**Installation:** No new dependencies needed. This phase is purely QML token replacement and layout adjustment.

## Architecture Patterns

### Current Theme Token System (Theme.qml)

The Theme singleton provides a comprehensive design token system:

**Colors (33 tokens):**
- Background palette: `bgBase`, `bgSurface`, `bgPanel`, `bgCard`, `bgElevated`, `bgInset`, `bgFloating`, `bgHover`, `bgPressed`
- Accent: `accent`, `accentLight`, `accentDark`, `accentSubtle`
- Text: `textPrimary`, `textSecondary`, `textTertiary`, `textDisabled`, `textOnAccent`
- Border: `borderDefault`, `borderSubtle`, `borderStrong`, `borderFocus`
- Chrome: `chromeSurface`, `chromeSurfaceAlt`, `chromeHover`, `chromePressed`, `chromeBorder`, `chromeText`, `chromeTextMuted`, `chromeDangerHover`, `chromeDangerPressed`
- Status: `statusSuccess`, `statusWarning`, `statusError`, `statusInfo`

**Typography (6 tokens):**
- `fontSizeXS` (10), `fontSizeSM` (11), `fontSizeMD` (12), `fontSizeLG` (14), `fontSizeXL` (16), `fontSizeXXL` (20)

**Spacing (6 tokens):**
- `spacingXS` (4), `spacingSM` (6), `spacingMD` (8), `spacingLG` (12), `spacingXL` (16), `spacingXXL` (24)

**Radii (4 tokens):**
- `radiusSM` (3), `radiusMD` (5), `radiusLG` (8), `radiusXL` (12)

**Control Sizing:**
- Heights: `controlHeightSM` (28), `controlHeightMD` (34), `controlHeightLG` (40)
- Icon buttons: `iconButtonSizeSM` (32), `iconButtonSizeMD` (34), `iconButtonSizeLG` (38)
- Layout: `sidebarWidth` (240), `rightPanelWidth` (300), `panelPadding` (12)

### Recommended Project Structure
No structural changes needed. Files remain in their current locations.

### Pattern 1: Token Replacement (Primary Pattern)
**What:** Replace hardcoded hex literals with Theme tokens
**When to use:** Every color, font size, spacing, and radius value
**Example:**
```qml
// BEFORE (hardcoded)
color: "#2a3040"
font.pixelSize: 11
spacing: 6

// AFTER (tokenized)
color: Theme.bgElevated      // #2a3040 maps to ~bgElevated or borderSubtle
font.pixelSize: Theme.fontSizeSM
spacing: Theme.spacingSM
```

### Pattern 2: Control Height Normalization
**What:** Use Theme.controlHeightSM/MD/LG for consistent sizing
**When to use:** All Rectangle-based custom buttons, input fields, rows
**Example:**
```qml
// BEFORE
height: 28  // used inconsistently for buttons
height: 34  // sometimes used for same purpose

// AFTER
height: Theme.controlHeightSM  // 28 for compact items
height: Theme.controlHeightMD  // 34 for standard items
```

### Anti-Patterns to Avoid
- **Adding new Theme tokens without good reason:** The 33 color tokens cover most use cases. Only add if a genuinely new semantic color is needed.
- **Changing visual behavior:** This is polish, not redesign. Do not change colors that would make the UI look different from upstream intent.
- **Refactoring component structure:** Stay within the same file, same component hierarchy. Only change property values.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Design tokens | New token file or CSS-like system | Theme.qml singleton | Already exists, comprehensive, and registered as QML singleton |
| Color contrast checking | Custom contrast algorithm | Manual verification against WCAG 2.1 AA (4.5:1 for text) | Theme colors already designed for dark theme contrast |
| Animation timing | Custom easing curves | Qt built-in Easing.OutCubic/Easing.InOutCubic | Already used consistently in CollapsibleSection and other components |

**Key insight:** The infrastructure is already in place. The problem is 100% adoption gap, not missing infrastructure.

## Common Pitfalls

### Pitfall 1: Token Misassignment
**What goes wrong:** Mapping a hardcoded color to the wrong Theme token (e.g., mapping a border color to a background token)
**Why it happens:** Many hex values are similar (#1e2229 vs #1a1e28 vs #161c27) and the visual difference is subtle
**How to avoid:** Use the mapping table below for the most common hardcoded values:
- `#1e2229` / `#1a1e28` / `#161c27` -> `Theme.bgPanel` (#1a1e28)
- `#252b38` / `#252b38` -> `Theme.bgElevated` (#252b38)
- `#2a3040` / `#2e3540` -> `Theme.borderSubtle` (#2a3040)
- `#363d4e` / `#454d5e` -> `Theme.borderDefault` (#363d4e)
- `#566070` -> `Theme.textDisabled` (#566070)
- `#7a8fa3` / `#8a96a8` -> `Theme.textTertiary` (#7f90a6) or `Theme.textSecondary` (#a0abbe)
- `#9daaba` / `#a0abbe` -> `Theme.textSecondary` (#a0abbe)
- `#c8d4e0` / `#dde4ef` / `#e2e8f1` / `#e8edf6` -> `Theme.textPrimary` (#e8edf6)
- `#f59e0b` -> `Theme.statusWarning` (#f5a623)
- `#ef4444` -> `Theme.statusError` (#e04040)
- `#18c75e` / `#22c564` -> `Theme.accent` (#18c75e) / `Theme.statusSuccess` (#18c75e)
- `#0d1117` / `#0f1318` / `#10141c` -> `Theme.bgInset` (#10141c) or `Theme.bgBase` (#0d0f12)
- `#f05545` -> `Theme.statusError` (close: #e04040)
- `#5b8def` -> Keep as-is (blue accent for cut mode, no token yet)
- `#569cd6` -> Keep as-is (blue for Z-axis labels, semantic color)
- `#e8a838` -> Keep as-is (amber for connector type, semantic color)
**Warning signs:** A color change that makes the UI look different from before

### Pitfall 2: Over-normalizing Font Sizes
**What goes wrong:** Forcing all font.pixelSize to Theme tokens when some values are intentionally different
**Why it happens:** Blind find-replace without checking context
**How to avoid:** Some files use font sizes of 8, 9, 10 for micro-labels that don't map to any Theme token. These are intentional and should be left as-is or mapped to `Theme.fontSizeXS` (10) only where 10 is used. Values 8-9 have no Theme token and should remain hardcoded.
**Warning signs:** Text becoming too large or too small in tooltips or badges

### Pitfall 3: Breaking Layout By Changing Spacing
**What goes wrong:** Changing spacing from 4 to `Theme.spacingXS` (4) is fine, but changing from 3 or 5 to the nearest token can shift layout
**Why it happens:** Theme tokens are on a 4-6-8-12-16-24 scale, not every integer
**How to avoid:** Only replace spacing values that exactly match a token value. Values like 3, 5, 10, 14 should stay as-is unless they clearly should match a token.
**Warning signs:** Panels shifting, overlapping content, or uneven gaps

## Code Examples

### Hardcoded Color Audit Results (by file)

**PreparePage.qml** -- 172 hardcoded hex colors:
- Most are in gizmo overlay panels (measure, cut, support paint, etc.)
- `#161c27de` / `#161c27e0` -- floating panel background with alpha, close to `Theme.bgFloating`
- `#2d3443` -- panel border, close to `Theme.borderSubtle` (#2a3040)
- `#1a1e28` -- base panel, maps to `Theme.bgPanel` (#1a1e28)
- `#8b949e` -- muted text, between `Theme.textTertiary` and `Theme.textDisabled`
- Axis colors `#ef4444`, `#22c55e`, `#3b82f6` -- intentional semantic colors, keep as-is
- `#e06666` -- delete/red action, close to `Theme.statusError`

**PrintSettings.qml** -- 97 hardcoded hex colors:
- Value chain popup: many hardcoded colors for level indicators (#506070, #6ed4a0, #d4a06e, #6ea8d4)
- Option rows: `#1a2030`, `#161c28` for headers -- close to `Theme.bgPanel`
- Modified options: `#e8864a` for modified option keys -- semantic color for "dirty"
- Filament colors: `#b97914`, `#214bc2`, `#d63a21`, `#b9b9b9` -- intentional slot colors

**SliceProgress.qml** -- 25 hardcoded hex colors:
- Status icon area: `#1e2229`, `#2e3540` -- maps to Theme tokens
- Progress bar: `#2a3040`, `#22c564` -- maps to Theme tokens
- Filament color chip: `#18c75e` -- maps to Theme.accent
- Post-slice buttons: `#6d28d9` (preview), `#1d4ed8` (export) -- intentional brand colors

**PreviewPage.qml** -- 6 hardcoded hex colors:
- Camera preset buttons: `#2a3545`, `#1e2229` -- maps to Theme tokens
- Info badge: `#1e2229`, `#2e3540` -- maps to Theme tokens
- Time label: `#7a8a9a` -- maps to Theme.textTertiary
- Layer info: `#1e2229` -- maps to Theme.bgPanel

**StatsPanel.qml** -- 2 hardcoded hex colors:
- Bar chart tooltip: `#1e293b` -- close to `Theme.bgElevated`
- Bar chart colors: `#22c55e`, `#eab308`, `#ef4444` -- semantic (green/yellow/red), keep as-is

**LayerSlider.qml** -- 19 hardcoded hex colors:
- Track backgrounds: `#252b38` -> `Theme.bgElevated`
- Tooltip background: `#1a2332` -> close to `Theme.bgPanel`
- Thumb colors: `#c0d0e0`, `#8090a0`, `#506070` -- grayscale, keep or map to text tokens
- Input backgrounds: `#1e2229`, `#2e3540` -> Theme tokens

**MoveSlider.qml** -- 5 hardcoded hex colors:
- Track: `#2a3040` -> `Theme.borderSubtle`
- Tooltip: `#1a2332` -> close to `Theme.bgPanel`
- Current time label: `#80cbc4` -- teal color for elapsed time, intentional

**Sidebar.qml** -- 19 hardcoded hex colors:
- Panel background: `#1a202bd9` -> `Theme.bgFloating`
- Input fields: `#0d1117` -> `Theme.bgInset` or `Theme.bgBase`
- Input borders: `#2e3848` -> close to `Theme.borderSubtle`

**main.qml** -- 49 hardcoded hex colors:
- Title bar: mostly uses `chromeXxx` tokens already but some deviations
- Menu items: `#1a2233`, `#183425` -- close to Theme tokens
- Title bar icon area: `#152230`, `#0f161f`, `#2b394d` -- intentional gradient

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Ad-hoc hardcoded colors | Theme singleton + partial adoption | Gradual during migration | Theme system is ready, adoption is incomplete |
| Mixed font sizes | Theme.fontSizeXx tokens | Theme.qml created early | Many files still use raw numbers |
| Mixed spacing values | Theme.spacingXx tokens | Theme.qml created early | Most files still use raw numbers |

**Deprecated/outdated:**
- None -- this is the first polish pass

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Theme token colors are close enough matches for all hardcoded values | Token Replacement | Some hardcoded colors may need new tokens added to Theme.qml |
| A2 | No visual behavior change is desired -- only consistency improvement | Token Misassignment | User may want some visual changes that go beyond token replacement |
| A3 | font.pixelSize values of 8-9 are intentional micro-labels and should stay | Over-normalizing | If they should be normalized, Theme needs new tokens |
| A4 | Axis colors (red/green/blue for X/Y/Z) and filament slot colors are intentional semantic colors | Token Replacement | If these should be tokenized, Theme needs new tokens |
| A5 | The upstream wxWidgets reference screenshots (`prepare_ref.png`, `preview_ref.png`) show the target visual appearance | Upstream Alignment | If references are outdated, alignment efforts may target wrong state |

## Open Questions

1. **Should new Theme tokens be added for recurring patterns?**
   - What we know: Some colors appear 10+ times but have no token (e.g., `#1a2332` for tooltips, `#8b949e` for muted text)
   - What's unclear: Whether adding tokens increases maintenance burden vs. keeping well-named constants
   - Recommendation: Add 3-5 new tokens for the most common patterns: `bgTooltip` (#1a2332), `textMuted` (#8b949e), `borderInput` (#2e3848)

2. **Should the filament slot grid colors be tokenized?**
   - What we know: Hardcoded in PrintSettings.qml as brand-specific slot colors (#b97914, #214bc2, #d63a21, #b9b9b9)
   - What's unclear: Whether these should come from ViewModel data or remain static
   - Recommendation: Keep as-is for this pass; these are upstream-defined filament brand colors

3. **What is the scope of "upstream visual alignment"?**
   - What we know: Reference screenshots exist at `src/qml_gui/assets/prepare_ref.png` and `preview_ref.png`
   - What's unclear: How closely must the Qt6 version match the wxWidgets version pixel-for-pixel
   - Recommendation: Focus on layout structure and spacing, not pixel-perfect color matching. Qt6 dark theme is already intentionally slightly different from upstream.

## Environment Availability

Step 2.6: SKIPPED (no external dependencies -- this phase is purely QML token replacement and layout adjustment)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QML Test) |
| Config file | None -- visual QA is manual verification |
| Quick run command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |
| Full suite command | Same as quick (builds + runs smoke test) |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| POLISH-01 | No hardcoded hex colors in slicing workflow pages | manual-only | `grep -c "#[0-9a-fA-F]" src/qml_gui/pages/PreparePage.qml` | N/A (audit tool) |
| POLISH-02 | Theme tokens used for all colors/fonts/spacing | manual-only | Build + visual inspection | N/A |
| POLISH-03 | Build succeeds with zero QML warnings | automated | `scripts/auto_verify_with_vcvars.ps1` | Yes |
| POLISH-04 | No visual regression (pages render correctly) | manual-only | Launch app + click through pages | N/A |

### Sampling Rate
- **Per file commit:** Build check via `auto_verify_with_vcvars.ps1`
- **Per wave merge:** Full build + manual visual inspection
- **Phase gate:** All 5 pages inspected, zero build errors, zero QML warnings

### Wave 0 Gaps
- No automated visual regression test infrastructure exists
- Visual QA must be manual for this phase
- Consider adding QML screenshot comparison in future phases

## Security Domain

> Security enforcement is not applicable to this phase. No ASVS categories apply -- this is purely visual polish with no authentication, input validation, or data handling changes.

## Sources

### Primary (HIGH confidence)
- `src/qml_gui/Theme.qml` -- direct file read, verified token definitions
- `src/qml_gui/pages/PreparePage.qml` -- direct file read, 2500+ lines audited
- `src/qml_gui/panels/PrintSettings.qml` -- direct file read, 1723 lines audited
- `src/qml_gui/panels/SliceProgress.qml` -- direct file read, 621 lines audited
- `src/qml_gui/pages/PreviewPage.qml` -- direct file read, 303 lines audited
- `src/qml_gui/panels/Sidebar.qml` -- direct file read, 510 lines audited
- `src/qml_gui/components/*.qml` -- all component files read and audited

### Secondary (MEDIUM confidence)
- `src/qml_gui/main.qml` -- title bar and shell layout audited
- Upstream reference screenshots at `src/qml_gui/assets/prepare_ref.png`, `preview_ref.png` (exist, not analyzed in detail)

### Tertiary (LOW confidence)
- Upstream wxWidgets visual patterns -- [ASSUMED] based on codebase comments referencing upstream alignment

## Metadata

**Confidence breakdown:**
- Theme token system: HIGH -- fully verified by reading Theme.qml
- Hardcoded color count: HIGH -- verified by grep across all QML files
- Color-to-token mapping: MEDIUM -- some borderline cases where token match is approximate
- Upstream visual comparison: LOW -- reference screenshots exist but detailed comparison not performed in this session
- Accessibility assessment: LOW -- no automated contrast checking performed

**Research date:** 2026-06-01
**Valid until:** 2026-07-01 (stable -- Theme token system is unlikely to change)

---

## Detailed Audit Findings

### 1. PreparePage.qml -- Key Issues

**Toolbars and overlays:**
- `CxPanel` floating panels use `color: "#161c27de"` and `border.color: "#2d3443"` -- should use `Theme.bgFloating` and `Theme.borderSubtle`
- Top toolbar height is hardcoded at 50px -- no Theme token for this
- ProcessBar height is 28px -- matches `Theme.controlHeightSM`

**Gizmo panels (Cut, Measure, Support Paint, Seam Paint):**
- All use consistent pattern: `#161c27e0` background, `#2d3443` border
- Should be `Theme.bgFloating` and `Theme.borderSubtle`
- Axis labels use semantic colors (red/green/blue) -- keep as-is
- Cut mode colors (`#5b8def`, `#e8a838`) -- intentional semantic colors, keep

**Warning overlay:**
- `#4a1c1c` / `#3a3420` for error/warning background -- no direct token, could add `bgErrorSubtle` and `bgWarningSubtle`
- Border colors `#ef4444` / `#f59e0b` map to `Theme.statusError` / `Theme.statusWarning`

**File dialogs:**
- `nameFilters` properly uses `qsTr()` for all filter names -- good

### 2. PrintSettings.qml -- Key Issues

**CollapsibleSection headers:**
- Uses `#151a22` for title bar -- no direct token match; `Theme.bgPanel` (#1a1e28) is close
- Icon text color uses `Theme.accent` -- good

**Filament slot grid:**
- Fixed 3-column GridLayout with hardcoded slot colors -- intentionally hardcoded per upstream design
- Slot dimensions: 44px height, 70px width for text -- no Theme tokens for these specific values

**Value chain popup:**
- Level indicator colors are semantic (#6ed4a0 for print, #d4a06e for filament, #6ea8d4 for printer) -- keep as-is
- Background `#0f1520` is darker than any Theme token -- could add `bgPopup` or use `Theme.bgBase`

**Search dialog and option rows:**
- Page header `#1a2030` and group header `#161c28` -- close to `Theme.bgPanel`
- Option row height varies: 44 for sliders, 38 for bool/enum -- no Theme token
- Value source indicator dot: 5x5px -- too small for touch target (accessibility concern)

**Scope pills (Global/Object/Plate):**
- Width 38px is tight for text like "Global" -- may clip in some fonts
- Active state uses `Theme.bgElevated` -- good

### 3. SliceProgress.qml -- Key Issues

**Status icon area:**
- Emoji icons ("gear", "check", "cloud") used via Unicode text -- works but may not render consistently across platforms
- Height hardcoded at 80px

**Progress bar:**
- Custom implementation using ProgressBar with Rectangle contentItem
- Height 6px is below minimum touch target (44px recommended) but this is a display-only element, not interactive
- Color transition uses hardcoded values -- `#22c564` maps to `Theme.accent` / `Theme.statusSuccess`

**Post-slice action buttons:**
- Preview button: `#6d28d9` (purple) -- intentional brand color, keep
- Export button: `#1d4ed8` (blue) -- intentional brand color, keep
- "Slice All" button uses Theme tokens -- good

**Result statistics:**
- Uses monospace font family name "monospace" -- should use a consistent monospace font reference
- Font sizes mix 11, 12 -- should use Theme.fontSizeSM and Theme.fontSizeMD

**Filament color chip:**
- Hardcoded `#18c75e` -- maps to `Theme.accent`
- Per-extruder colors use index-based color array: `["#18c75e", "#3b82f6", "#f59e0b", "#ef4444"]` -- intentional, keep

### 4. PreviewPage.qml -- Key Issues

**Header bar:**
- Radius 14 -- no Theme token for this exact value (Theme.radiusXL = 12)
- Height 48px -- no Theme token
- Camera preset buttons: 26x26px, radius 6 -- below touch target minimum

**Layer slider panel:**
- Width 240px -- matches `Theme.sidebarWidth`
- Radius 16 -- no Theme token (Theme.radiusXL = 12)

**Stats panel:**
- Width 280px -- no Theme token for this
- Radius 16 -- same as above

**Move slider footer:**
- Height 56px -- no Theme token
- Radius 16 -- same issue

**Color mode selector:**
- Uses `CxComboBox` with `Theme.fontSizeLG` -- good

**Layer/move summary badge:**
- `#1e2229` background -> `Theme.bgPanel`
- `#7a8a9a` text -> `Theme.textTertiary`
- Chinese text "层" and "步" hardcoded -- should use `qsTr()` (already does via concatenation, but could be cleaner)

### 5. Theme Consistency -- Cross-Cutting Findings

**Most common hardcoded colors (frequency across all QML files):**

| Hex Value | Frequency | Closest Theme Token | Action |
|-----------|-----------|---------------------|--------|
| `#1e2229` | ~20 | `Theme.bgPanel` (#1a1e28) | Replace |
| `#161c27` / `#161c27de` | ~15 | `Theme.bgFloating` (#1a202bd9) | Replace |
| `#2d3443` | ~10 | `Theme.borderSubtle` (#2a3040) | Replace |
| `#2e3540` | ~10 | `Theme.borderSubtle` (#2a3040) | Replace |
| `#8b949e` | ~10 | Between `textTertiary` and `textDisabled` | Add `textMuted` token or use `textTertiary` |
| `#1a2332` | ~8 | Close to `Theme.bgPanel` | Add `bgTooltip` token |
| `#c8d4e0` / `#dde4ef` | ~8 | `Theme.textPrimary` (#e8edf6) | Replace with `textPrimary` or `textSecondary` depending on context |
| `#566070` | ~6 | `Theme.textDisabled` (#566070) | Replace |
| `#0d1117` | ~6 | `Theme.bgInset` (#10141c) or `Theme.bgBase` (#0d0f12) | Replace |

**Font size distribution:**

| Size | Theme Token | Frequency | Notes |
|------|-------------|-----------|-------|
| 10 | `fontSizeXS` | ~120 | Many intentional micro-labels |
| 11 | `fontSizeSM` | ~280 | Most common |
| 12 | `fontSizeMD` | ~250 | Second most common |
| 13 | none | ~15 | Used in a few dialogs -- no token |
| 14 | `fontSizeLG` | ~50 | Headers and titles |
| 16 | `fontSizeXL` | ~20 | Section headers |
| 8-9 | none | ~30 | Micro-labels in tooltips -- no token |

### 6. Accessibility Assessment

**Touch targets:**
- Many interactive elements are below the 44x44px recommended minimum:
  - Filament slot color chip: 10x10px (display only)
  - Value source indicator dot: 5x5px (but has expanded MouseArea with margins: -4)
  - Camera preset buttons: 26x26px
  - CxIconButton: 34x34px (close to minimum, acceptable)
  - Scope pills: 38x22px (too narrow)
  - Delete/rename buttons: 24x24px

**Contrast (dark theme):**
- `Theme.textPrimary` (#e8edf6) on `Theme.bgBase` (#0d0f12): ~14:1 -- excellent
- `Theme.textSecondary` (#a0abbe) on `Theme.bgPanel` (#1a1e28): ~7:1 -- good
- `Theme.textTertiary` (#7f90a6) on `Theme.bgPanel` (#1a1e28): ~4.8:1 -- barely passes AA
- `Theme.textDisabled` (#566070) on `Theme.bgPanel` (#1a1e28): ~2.8:1 -- FAILS AA (but disabled text is exempt)

**Keyboard navigation:**
- PreparePage has comprehensive keyboard shortcuts -- good
- PreviewPage has keyboard shortcuts -- good
- Tab navigation is default Qt Quick behavior -- adequate
- Focus indicators are default Qt Quick -- may not be visible enough in dark theme

### 7. Recommended Token Additions

Based on the audit, these new Theme tokens would improve consistency:

```qml
// Tooltip background (appears ~8 times)
readonly property color bgTooltip:    "#1a2332"

// Muted text color (appears ~10 times, between tertiary and disabled)
readonly property color textMuted:    "#8b949e"

// Input field border (appears ~6 times)
readonly property color borderInput:  "#2e3848"

// Warning/error subtle backgrounds (appears ~4 times)
readonly property color bgErrorSubtle:   "#4a1c1c"
readonly property color bgWarningSubtle: "#3a3420"

// Radius 16 (appears ~6 times for panel corners)
readonly property int radiusXXL:  16
```

### 8. Execution Priority

**Wave 1 (highest visual impact):**
- SliceProgress.qml -- smallest file, most visible to user during slicing
- PreviewPage.qml -- small file, 6 hardcoded colors
- StatsPanel.qml -- 2 hardcoded colors
- Legend.qml -- 0 hardcoded colors (already clean)
- LayerSlider.qml -- 19 hardcoded colors, high visibility
- MoveSlider.qml -- 5 hardcoded colors

**Wave 2 (core workflow):**
- PrintSettings.qml -- 97 hardcoded colors, large file
- CollapsibleSection.qml -- 1 hardcoded color
- Sidebar.qml -- 19 hardcoded colors

**Wave 3 (largest files):**
- PreparePage.qml -- 172 hardcoded colors, largest file
- main.qml -- 49 hardcoded colors in title bar

**Wave 4 (supporting components):**
- ToolPositionTooltip.qml -- 13 hardcoded colors
- SearchDialog.qml, NotificationCenter.qml, other dialogs
