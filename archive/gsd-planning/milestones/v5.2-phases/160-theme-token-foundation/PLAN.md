# Phase 160: Theme Token Foundation

**Status:** Ready to execute
**Workstream:** DS (Design-System Foundation)
**Requirement:** DS-01

## Goal

Make Theme.qml the single source of truth by adding all missing tokens + resolving dead ones. Foundation for all later v5.2 phases.

## Evidence (from 6 audits + direct token analysis)

**1. Undefined token (silent `undefined` at runtime):**
- `Theme.borderActive` — referenced in src/qml_gui/ (multiple sites) but NOT in Theme.qml

**2. Dead tokens (defined but never read in any *.qml):**
- bgCard, chromeSurfaceAlt, controlHeightLG, iconButtonSizeLG, iconButtonSizeSM, panelPadding, radiusXXL, rightPanelWidth, sidebarWidth, statusBarHeight, switchTrackOn, tabBarHeight

**3. New tokens needed (from audit findings — current hardcodes):**

Color:
- `statusErrorDark`, `statusErrorPressed` — replace Qt.darker() in CxButton.qml:31-32
- `accentSubtlePressed` — replace Qt.darker() in CxIconButton.qml:48
- `scrollBarColor` — used across CxScrollView/etc (currently hardcoded)
- `borderActive` — DEFINE this (currently silent undefined)

Typography:
- `fontMono` — replace 26 `font.family: "Consolas"` hardcodes
- `fontSize13` — used 17× in pages, not in scale

Sizing:
- `sliderTrackHeight`, `switchTrackWidth` — used in CxSlider/CxSwitch hand-rolled values
- `dialogHeaderHeight`, `dialogFooterHeight` — currently magic 44px in CxDialog
- `controlHeightXL` — extend scale above LG (currently 28/34/40)
- `panelPaddingSM` — for scroll gutters (8px) vs panelPadding (12px)

Sidebar sizing (Phase 164 will consume):
- `sidebarWidthMin`, `sidebarWidthMax`, `sidebarWidthDefault` — replace 7-layer 392 hardcode

Notification severity palette (Phase 167 will consume):
- `statusSeverity0..9` — collapses 3 private 10-level tables

## Plan

### Wave 1 — Theme.qml additions + cleanups

1. `src/qml_gui/Theme.qml`: add the missing tokens. For severity palette, expose as a 10-element list property `severityColors` (one source of truth). Add a header comment documenting the canonical token list.

### Wave 1 — Resolve undefined + dead tokens

2. For `Theme.borderActive`: define it (audit found it referenced — choose a value consistent with borderStrong/borderFocus). Keep all existing call sites working.
3. For dead tokens: keep ones that are semantically meaningful (controlHeightLG, panelPadding, rightPanelWidth, sidebarWidth — these will be consumed by Phases 161/164). Remove pure-junk ones if any (none confirmed — all 12 are kept, most have future consumers).

### Wave 2 — Regression anchor

4. `tests/QmlUiAuditTests.cpp`: add `v52ThemeTokenFoundationWired` slot asserting:
   - No undefined tokens (every Theme.X referenced in *.qml is defined in Theme.qml)
   - New tokens added (statusErrorDark/Pressed, accentSubtlePressed, scrollBarColor, fontMono, fontSize13, severity palette, etc.)
   - Header documentation exists

## Verification

- Canonical build exits 0
- 5/5 ctest groups PASS
- New `v52ThemeTokenFoundationWired` slot passes
- Zero undefined Theme.* references across src/qml_gui/ (assertable)
