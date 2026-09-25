# Components — UI Review (v5.1 retroactive, 18 components)

**Audited:** 2026-07-17
**Baseline:** Theme.qml token system + cross-component consistency
**Method:** Code-only audit (no dev server, no screenshots). All 18 files in `src/qml_gui/components/` read in full; usage counts derived from `grep -rln` across `src/qml_gui` + cross-check against `qml.qrc` and `src/core`.

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | i18n coverage is good (qsTr used in 16/18 files); two files (CxPanel, CxSectionHeader) have no strings at all (dead); English/mixed labels leak in a few places ("OK"/"Cancel" alongside Chinese CTA elsewhere). |
| 2. Visuals | 2/4 | Icon-only widgets are well-tooltipped in GLToolbars but several icon-glyph buttons in NotificationCenter/MoveSlider/PreviewLayerRail use text-as-icon ("<", ">", "<<") with no aria-equivalent; clear focal points exist per component. |
| 3. Color | 2/4 | 4 components are fully Theme-compliant; 14 leak at least one hardcoded color. ErrorToast/NotificationCenter/ErrorBanner each maintain their own private severity→color tables (~50 duplicated magic colors total); GLToolbars uses translucent hex literals for viewport surfaces. |
| 4. Typography | 2/4 | font.pixelSize is consistently Theme-tokens; but `font.family: "Consolas"` is hardcoded in 8 files / 26 sites instead of a `Theme.fontMono` token. |
| 5. Spacing | 3/4 | Theme.spacing* is used broadly; small leaks of literal margins (`anchors.leftMargin: 12`, `10`, `20`) in ErrorBanner, StatusBar, GLToolbars, GroupNavSidebar — minor but inconsistent with the spacing scale. |
| 6. Experience Design | 2/4 | Notification trio lacks coherence; 4 components are orphaned (no consumers — file waste); state coverage (loading/empty/disabled) is uneven — empty state exists only in NotificationCenter & Legend; no loading state anywhere; disabled states are good where present. |

**Overall: 14/24**

---

## Top 3 Priority Fixes

1. **Collapse the notification trio into a shared base / token module.** ErrorBanner.qml, ErrorToast.qml, and NotificationCenter.qml each hand-roll a severity→color and severity→icon map (`ErrorToast.qml:41-44`, `NotificationCenter.qml:13-37`, `ErrorBanner.qml` lines 13-16, 24, 28). That is three private copies of the same 10-level table with ~50 hardcoded hex strings and zero shared contract — a guaranteed drift surface. **Fix:** extract a `Theme`-level `severityColor(sev)` / `severityIcon(sev)` (or a small `NotificationPalettes.qml` singleton) and have all three components consume it. Promote severity backgrounds (e.g. `#251a08`, `#2e1a1a`, `#1a2e20`) into Theme as `bgStatusWarningSubtle`, `bgStatusErrorSubtle`, `bgStatusSuccessSubtle` (Theme already has `bgErrorSubtle` / `bgWarningSubtle` — extend the pattern rather than fork it).

2. **Delete or wire up the 4 orphan components.** `CxPanel.qml`, `CxSectionHeader.qml`, `FilamentSlot.qml`, and `GroupNavSidebar.qml` are registered in `qml.qrc` but have **zero consumers** anywhere in `src/qml_gui` or `src/core` (verified by `grep -rln` across all `.qml/.cpp/.h/.qrc`). Per AGENTS.md ("If an existing page is materially off-design, replace it and remove the old files, routes, registrations, tests, imports, and resources instead of keeping deprecated UI code"), dead UI files should be deleted, not stockpiled. **Fix:** remove all four files and their `qml.qrc` entries in one cleanup commit; if any are slated for future use, gate them behind a TODO with an owner and a phase, not in the active component layer.

3. **Add a `Theme.fontMono` token and eliminate the 26 "Consolas" hardcodes.** Eight components (`ToolPositionTooltip`, `StatsPanel`, `MoveSlider`, `PreviewLayerRail`, `Legend`, `NotificationCenter`, `ErrorToast`, `CustomGcodeDialog`) hardcode `font.family: "Consolas"` or `"Consolas, monospace"`. On a non-Windows target (macOS/Linux are explicitly in scope per the upstream lock — AGENTS.md "Platform: 当前仅 Windows，上游同时支持 macOS/Linux") Consolas does not exist and falls back silently. **Fix:** add `readonly property string fontMono: "Consolas, Menlo, monospace"` to Theme.qml and replace every hardcoded family reference; also add `fontSans`/`fontDefault` while you are in there for symmetry.

---

## Component Role Matrix

| Component | Role | Used by (file count) | Theme-compliant? | Notes |
|---|---|---|---|---|
| CollapsibleSection | Fold/unfold card container | 1 (VisibilityFilter) | Mostly — uses Theme tokens, but mixes `8` literal radius with `Theme.radiusSM` (`CollapsibleSection.qml:29,44`) | Reused only once; could be promoted to broader use OR inlined. |
| CustomGcodeDialog | Modal G-code input | 1 (PreviewLayerRail, ×2 instances) | Good — CxDialog base + Theme tokens + qsTr | Self-contained, correctly built on Cx primitives. |
| CxPanel | Surface-background rectangle (Panel/Floating/Card/Elevated/Inset/Transparent enum) | **0 — ORPHAN** | Fully Theme-driven | Never instantiated anywhere. Delete or wire. |
| CxSectionHeader | Title + subtitle + trailing actions header | **0 — ORPHAN** | Fully Theme-driven | Never instantiated. Delete or wire. |
| ErrorBanner | Full-width severity=1 warning banner | 1 (main.qml) | **Poor** — every color is a hardcoded amber literal (`#251a08`, `#c87840`, `#e8b870`, `#3a2412`, `#5a3418`, `#c0824a`) | Hardcoded to severity=1; cannot render other severities. |
| ErrorToast | Bottom-center transient toast, severity 0-9, progress, hint nav, confirm | 1 (main.qml) | **Poor** — lines 41-43 hand-roll ~30 hex colors; icon/text/bg triple per severity | Most feature-rich notification; also most color-leaky. |
| FilamentSlot | Per-extruder preset selector + color swatch | **0 — ORPHAN** (cpp matches are unrelated AMS slot methods) | Mixed — Theme tokens plus a hardcoded 5-color palette (`FilamentSlot.qml:16`) and `#f05545` error color | Dead; the inline color-picker Popup is also `active: false` permanently. |
| GLToolbars | Viewport action/gizmo/view toolbars | 1 (PreparePage via editorVm) | Mixed — buttons themselves are clean; toolbar chrome uses translucent literals (`#3b3e46aa`, `#71757d8466`, `#7a7d8466`, `#c3c7cd77`, `#9097a066`) | Excellent tooltip coverage; icon set has known reuses (Hollow = layers-subtract). |
| GroupNavSidebar | Settings option-group navigation rail | **0 — ORPHAN** | Mostly Theme tokens + `#1c2a3e`/`#161d28`/`#1e2535`/`#1e3828` literals | Never instantiated — SettingsDialog/LeftSidebar use OptionRow directly. |
| Legend | Preview color legend (gradient or item list) | 1 (PreviewPage) | Mixed — text tokens good; container bg `#24272e` literal; gradient stops are 10 inline hex colors (`Legend.qml:52-61`) | Gradient colors are inherently data-defined; container should be `Theme.bgInset`. |
| MoveSlider | Preview timeline slider with tool-change band + hover scrub tooltip | 1 (PreviewPage) | Good — Theme tokens throughout | Step buttons use `<`/`>`/`<<`/`>>` glyphs without tooltips. |
| NotificationCenter | Bell-popup notification history list | 1 (main.qml) | **Poor** — ~15 hardcoded colors (`#0f1217`, `#242a33`, `#151a22`, `#1e2430`, `#b0b8c4`, `#505860`, `#e0e6ed`, `#f05545`, `#58a6ff`, `#808890`, `#363d4e`); severity map duplicates ErrorToast | Owns the only proper empty state ("暂无通知记录"). |
| OptionRow | Typed option renderer (bool/int/double/percent/enum/string/color) | 2 (SettingsDialog, LeftSidebar) | Mostly Theme — zebra stripe `#080b10` and badge text size 9 are the only leaks | Most complex component (~530 lines); badge sub-components are good; inline `NumericEdit` should be promoted to `controls/`. |
| PreviewLayerRail | Vertical layer range slider + tick marks + add/edit menus | 1 (PreviewPage) | Good — Theme tokens, instantiates CustomGcodeDialog ×2 | Excellent state coverage (disabled, drag, right-click menus, 5 tick types). |
| StatsPanel | Preview statistics panel with extruder/role/layer breakdowns | 1 (PreviewPage) | Mixed — Theme tokens + `#24272e`/`#1c2027` literals for inner cards | Heavy CTA copy ("显示空驶", "显示热床") properly wrapped in qsTr. |
| StatusBar | Bottom status bar (status / objects / slice / coords / clock) | 1 (main.qml) | **Poor** — every color hardcoded (`#0a0c10`, `#242a33`, `#7a8898`, `#566070`, `#2e3444`); ignores Theme.statusBarHeight token (uses literal `24`) | Simplest component, worst Theme adherence. |
| ToolPositionTooltip | Hover tooltip showing tool X/Y/Z/feedrate/layer/fan/temp/etc. | 1 (PreviewPage) | Mostly — Theme tokens + bg `#11151dcc` literal + 11 `font.family: "Consolas"` hardcodes | Nine conditional rows; each line duplicates the font assignment. |
| VisibilityFilter | Collapsible role-line visibility toggles | 1 (PreviewPage) | Fully Theme-compliant | Cleanest component in the layer; only consumer of CollapsibleSection. |

---

## Notification System Coherence Analysis

The three notification components live together in `main.qml` (lines 467-474, 580) and read from the same `backend` context (`lastErrorSeverity`, `lastErrorMessage`, `lastErrorTitle`, `pendingNotificationCount`, `currentNotification*`). Their roles are *intended* to be distinct, but the implementation does not enforce the distinction and triplicates the underlying palette logic.

**Intended roles (deduced from code):**
- **ErrorBanner** — full-width, in-flow, persistent warning (severity=1 only). Lives below the title bar inside the layout.
- **ErrorToast** — bottom-center transient overlay, auto-dismiss with hover-pause, handles the full 0-9 severity range, persistent confirm dialogs, slicing progress, hint navigation, export/preview actions. The "kitchen sink" of notifications.
- **NotificationCenter** — bell-triggered history popup, scrollable list of past notifications with unread badge and mark-read/clear actions.

**Coherence problems:**

1. **Triplicated severity palette.** ErrorToast.qml:41-43, NotificationCenter.qml:13-37, and ErrorBanner.qml (hardcoded amber) each maintain their own severity→{icon, text, background, accent} table. The hex values mostly agree but drift in places (e.g. ErrorToast uses `#f05545` for sev=3; NotificationCenter also uses `#f05545`; ErrorBanner's amber `#c87840` matches both — but the subtle backgrounds `#2e1a1a` vs `#251a08` vs `#151a22` are all different). There is no single source of truth, so a future severity recolor requires editing three files.

2. **Severity routing is implicit and asymmetric.** ErrorBanner hardcodes `backend.lastErrorSeverity === 1` (ErrorBanner.qml:11). ErrorToast shows for `sev >= 0 && lastErrorMessage !== ""` (ErrorToast.qml:14) — which includes sev=1, the same condition as the banner. **Both render simultaneously for a severity=1 warning**, which means a warning notification is shown twice: once as a banner and once as a toast. There is no mutual-exclusion logic and no shared "current surface" contract. This is the strongest evidence that the trio was built incrementally rather than designed as a system.

3. **No shared base / no abstract notification primitive.** All three are top-level `Rectangle` or `Item` roots that re-implement icon glyph, title, message, pending-count badge, and dismiss button from scratch. A `NotificationCard` primitive (taking `severity`, `title`, `message`, `pending`, optional action buttons) would let the banner = single-row card, the toast = floating card with auto-dismiss, and the center list = repeater of compact cards. As built, the pending-count badge alone is implemented twice (ErrorBanner.qml:42-54, ErrorToast.qml:116-130) with slightly different sizes (20×20 vs 18×18).

4. **Action surfaces are not unified.** ErrorToast alone implements hint navigation, slicing-complete export/preview, and confirm/cancel — all as privately-styled mini-rectangles with their own hover colors. None of these action styles are reusable by the banner or the center list.

5. **What is correct.** The data layer is coherent: a single `backend` notification queue drives all three. The split into "banner / toast / history" maps cleanly onto upstream OrcaSlicer's NotificationManager (which the comments cite). The problem is purely at the presentation layer — duplicated palette, duplicated badge, no mutual exclusion.

**Recommended end state:** Introduce `components/NotificationCard.qml` (or push severity palette into Theme). Refactor ErrorBanner and ErrorToast to instantiate NotificationCard with a `surface: "banner"|"toast"` prop. Have BackendContext (or a thin QML controller) decide which surface a given severity renders on, so sev=1 cannot appear in two places at once.

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

- **Good:** 16/18 files use `qsTr()` for user-visible strings; GLToolbars (47 calls), OptionRow (25), StatsPanel (23), PreviewLayerRail (15) have thorough coverage including disabled-state copy ("Select one or more objects", "Clipboard is empty").
- **WARNING — orphan files have no copy:** CxPanel.qml and CxSectionHeader.qml contain zero user-visible strings. This is consistent with them being dead code (see Experience Design).
- **WARNING — mixed CTA language:** ErrorToast.qml mixes Chinese CTAs ("预览", "导出", "关闭", "取消", "确认", "不再提示", "文档") with no English fallback, while CustomGcodeDialog.qml uses English CTAs ("Cancel", "OK"). StatsPanel/Legend/NotificationCenter are Chinese-only. This is internally inconsistent — pick one source language and let `qsTr()` + the i18n `.ts` files handle translation. The convention in AGENTS.md ("All new or modified source comments must be English and ASCII-only") implies source strings should be English; the Chinese source strings here will not round-trip through `lupdate` cleanly for the `en.ts` target.
- **Minor:** StatusBar.qml:12 has `qsTr("就绪")` (Chinese source) and the clock format `"hh:mm"` is not localized (24-hour only).

### Pillar 2: Visuals (2/4)

- **Good:** GLToolbars provides a `toolTipText` on every `ActionToolButton`, `ViewToolButton`, and `GizmoToolButton`, plus a dynamic `gizmoTip()` that appends gizmo status ("Move - Ready" vs "Move - select an object first"). This is the gold standard for the layer.
- **BLOCKER — icon-glyph buttons have no aria-equivalent:** MoveSlider's `MoveStepButton` (lines 181-212) renders `<`, `>`, `<<`, `>>` as text labels with no `ToolTip`. PreviewLayerRail's `RailButton` (lines 357-393) *does* have a tooltip — good — but MoveSlider does not follow the same pattern despite using the identical idiom. The two components should share a `StepButton` primitive.
- **WARNING — text-as-icon inconsistency:** NotificationCenter uses `"✓"`, `"🗑"`, `"✕"` glyph buttons (lines 90, 107, 122) — only some have tooltips (none do). The emoji `"🗑"` will render differently per platform font.
- **Good focal points:** Each component has a clear primary region. StatsPanel and Legend use a bold header label + bordered card to establish hierarchy.

### Pillar 3: Color (2/4)

- **Fully Theme-compliant (4):** CxPanel, CxSectionHeader, VisibilityFilter, CustomGcodeDialog.
- **Mostly compliant (8):** CollapsibleSection, OptionRow, GLToolbars, MoveSlider, PreviewLayerRail, FilamentSlot, StatsPanel, ToolPositionTooltip — each leaks 1-5 literals.
- **Non-compliant (4):** ErrorBanner, ErrorToast, NotificationCenter, StatusBar.
- **Worst offenders:**
  - ErrorToast.qml:41-43 — three 10-entry severity tables inline (`iconColor`, `bgColor`, `textColor`), ~30 distinct hex literals.
  - NotificationCenter.qml — ~15 hex literals including the only-slightly-off-from-Theme `#0f1217` (≈ `bgBase` `#0d0f12`), `#242a33` (≈ `borderDefault` `#363d4e`), `#151a22` (≈ `bgPanel` `#161a23`). These are close enough to Theme tokens that the hardcoded versions look like drift, not design.
  - StatusBar.qml:8,25,29,34,40,49,56 — every color is a literal; `#0a0c10` is ~2 RGB values from `Theme.bgBase`.
  - GLToolbars.qml:25-26,99-101,260-262,357 — translucent literals for viewport chrome (`#3b3e46aa`, `#7a7d8466`, etc.). These arguably *should* be theme tokens (`Theme.chromeOverlaySurface` or similar) since the same translucency pattern recurs across the GL layer.
- **BLOCKER pattern:** No component uses `Theme.statusSuccess` / `statusWarning` / `statusError` / `statusInfo` directly — they reinvent the same colors (`#18c75e`, `#c87840`, `#f05545`, `#58a6ff`). Note `Theme.statusWarning` is `#f5a623` but ErrorToast uses `#c87840` for warning — **the warning hue is inconsistent between Theme and the notification components.** Pick one.

### Pillar 4: Typography (2/4)

- **Good:** `font.pixelSize` is consistently `Theme.fontSizeXS/SM/MD/LG/XL` across all 18 files; no raw pixel sizes for body text.
- **WARNING — font family not tokenized:** 26 sites across 8 files hardcode `font.family: "Consolas"` or `"Consolas, monospace"`. There is no `Theme.fontMono`. ToolPositionTooltip alone has 11 such sites (one per data row). CustomGcodeDialog also uses bare `"monospace"`.
- **WARNING — magic font sizes:** A handful of components use raw `font.pixelSize: 9` (OptionRow.qml:494, StatsPanel.qml:269,277, ErrorToast.qml:128,168,200,290, ErrorBanner.qml:51) and `font.pixelSize: 10`/`11`/`13`/`14` literals alongside Theme tokens. 9 is smaller than `Theme.fontSizeXS` (10) — this is an off-scale size that should either be added to Theme or removed.
- **Good:** Bold/weight usage is consistent — bold for titles, statuses, and dirty markers; regular for body.

### Pillar 5: Spacing (3/4)

- **Good:** Theme.spacingXS/SM/MD/LG/XL/XXL is the dominant spacing source. VisibilityFilter, OptionRow, GroupNavSidebar use the scale religiously.
- **WARNING — literal margins:** ErrorBanner.qml:21-22 (`12`, `8`), StatusBar.qml:19-20 (`10`, `16` spacing), GLToolbars.qml:91,252,302 (`598`, `392`, `20` — pixel-perfect layout numbers, which is acceptable for viewport anchoring but not for padding), GroupNavSidebar.qml:38,66 (`Theme.spacingSM` alongside literal `12`). The mix of token and literal within the same file is the smell.
- **Minor:** CollapsibleSection.qml:57-58 uses `10`/`8` literals for title-bar padding while the rest of the file uses Theme tokens.

### Pillar 6: Experience Design (2/4)

- **BLOCKER — 4 orphan components:** CxPanel, CxSectionHeader, FilamentSlot, GroupNavSidebar are registered in `qml.qrc` and have zero consumers in `src/qml_gui` or `src/core` (verified by grep). They inflate the layer without delivering value and confuse the "what is reusable here?" question. Per AGENTS.md, dead UI code should be removed.
- **BLOCKER — notification system incoherence:** See dedicated section above. The trio renders duplicate UI for severity=1, triplicates the palette, and shares no base primitive.
- **WARNING — uneven state coverage:**
  - *Empty state:* Only NotificationCenter ("暂无通知记录") and Legend ("暂无图例数据") handle empty. StatsPanel always renders rows of `"--"` placeholders (acceptable but inconsistent). VisibilityFilter shows nothing if `roleVisibilities` is empty (should show an empty state).
  - *Loading state:* **No component has a loading state.** StatsPanel/Legend/ToolPositionTooltip render `"--"` while `previewVm` is null, which is a reasonable null-guard but is not branded as "Loading…". For a slicer where statistics compute over seconds, this is a real gap.
  - *Disabled state:* Good — GLToolbars, MoveSlider, PreviewLayerRail, OptionRow all set `enabled: root.previewVm && root.totalMoves > 0` style guards with `opacity: 0.45` feedback.
  - *Error state:* Only the notification trio surfaces errors; no component has its own error boundary.
- **Good — required-property discipline (partial):** CollapsibleSection (`required property string title`), OptionRow (`required property var optionModel/optIdx/rowIndex`), FilamentSlot (`required property int slotIndex; required property var configVm`), GLToolbars (`required property var editorVm; required property var viewport3d`), PreviewLayerRail/Legend/MoveSlider/StatsPanel/ToolPositionTooltip/VisibilityFilter (`required property var previewVm`) all declare required props.
- **WARNING — loose `var` for viewmodels:** Every viewmodel reference is `property var previewVm` / `editorVm` / `configVm`. This is the QML idiom but it forfeits compile-time checking. Acceptable trade-off, but combined with `required property var` it is easy to pass the wrong VM type. Consider documenting the expected VM type in a comment per component (some already do).
- **WARNING — one-off reuse candidates:**
  - CollapsibleSection has exactly 1 consumer (VisibilityFilter). It is well-built and should either be promoted (Settings/Config pages would benefit) or moved closer to its consumer.
  - CustomGcodeDialog has 1 consumer (PreviewLayerRail, instantiated twice). Fine as-is.
  - OptionRow's inline `NumericEdit` and `Badge` sub-components (lines 477-530) are generally useful and should be promoted to `controls/` for reuse.
  - MoveSlider's `MoveStepButton` and PreviewLayerRail's `RailButton` are the same idiom (Rectangle + Text glyph + MouseArea + disabled opacity) and should be unified into a single `GlyphStepButton` control.

---

## Per-Component Verdicts (one line each, 18 rows)

| # | Component | Verdict |
|---|---|---|
| 1 | CollapsibleSection | OK with nits — Theme tokens dominant, but `8` literal radius and only 1 consumer; promote or inline. |
| 2 | CustomGcodeDialog | Good — clean CxDialog-based modal, full qsTr, no leaks; "OK"/"Cancel" should match app CTA language. |
| 3 | CxPanel | **DELETE** — zero consumers; Theme-correct but dead code. |
| 4 | CxSectionHeader | **DELETE** — zero consumers; Theme-correct but dead code. |
| 5 | ErrorBanner | Needs work — every color hardcoded, severity=1 only, duplicates sev=1 with ErrorToast; refactor into shared NotificationCard. |
| 6 | ErrorToast | Needs work — feature-rich but ~30 hardcoded colors, duplicate severity table; extract palette to Theme. |
| 7 | FilamentSlot | **DELETE or WIRE** — zero QML consumers; inline color-picker Popup is permanently `active: false`; hardcoded 5-color palette. |
| 8 | GLToolbars | Good — best tooltip coverage in the layer; only leak is viewport-chrome translucents that should be Theme tokens. |
| 9 | GroupNavSidebar | **DELETE or WIRE** — zero consumers; literal selection colors drift from Theme; Settings pages use OptionRow directly. |
| 10 | Legend | Mostly good — Theme-titled; container bg and 10 gradient stops are literals (gradient is data, container is not). |
| 11 | MoveSlider | Good — Theme tokens throughout; add tooltips to `<`/`>`/`<<`/`>>` step buttons; unify with PreviewLayerRail.RailButton. |
| 12 | NotificationCenter | Needs work — ~15 hardcoded colors, duplicate severity table, only proper empty state in the trio; refactor with ErrorToast. |
| 13 | OptionRow | Mostly good — most complex component, well-tokenized; promote inline `NumericEdit`/`Badge` to controls/; off-scale font size 9. |
| 14 | PreviewLayerRail | Good — excellent state coverage (disabled/drag/menus/5 tick types); minor literal `8`/`4` margin leaks. |
| 15 | StatsPanel | Mostly good — Theme-titled; inner card bgs `#24272e`/`#1c2027` are literals; Chinese source strings should be English for i18n. |
| 16 | StatusBar | Needs work — every color hardcoded, ignores `Theme.statusBarHeight`, Chinese source string `就绪`; smallest component, worst adherence. |
| 17 | ToolPositionTooltip | Mostly good — Theme tokens + 1 bg literal; 11 duplicate `font.family: "Consolas"` sites should use a Theme.fontMono token. |
| 18 | VisibilityFilter | **Excellent** — fully Theme-compliant, only consumer of CollapsibleSection; the model component for the layer. |

---

## Files Audited

All 18 components in `src/qml_gui/components/`:
- CollapsibleSection.qml, CustomGcodeDialog.qml, CxPanel.qml, CxSectionHeader.qml, ErrorBanner.qml, ErrorToast.qml, FilamentSlot.qml, GLToolbars.qml, GroupNavSidebar.qml, Legend.qml, MoveSlider.qml, NotificationCenter.qml, OptionRow.qml, PreviewLayerRail.qml, StatsPanel.qml, StatusBar.qml, ToolPositionTooltip.qml, VisibilityFilter.qml

Cross-referenced against:
- `src/qml_gui/Theme.qml` (token system)
- `src/qml_gui/main.qml:455-490,580` (notification trio wiring)
- `src/qml_gui/qml.qrc` (registration confirmations)
- `src/core/viewmodels/EditorViewModel.{h,cpp}` (FilamentSlot false-positive check)
- `AGENTS.md` (dead-code and i18n conventions)

**Screenshots:** not captured (no dev server detected; code-only audit).
**Registry safety audit:** skipped (no `components.json` — this is a QML/QtQuick project, not shadcn).
