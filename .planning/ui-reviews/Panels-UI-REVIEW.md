# Panels + Sidebar — UI Review (v5.1 retroactive)

**Audited:** 2026-07-17
**Baseline:** Theme.qml token system (sidebarWidth=240, rightPanelWidth=300, panelPadding=12) + v3.9 UI-SPEC (Phase 74) sidebar requirements
**Scope:** `src/qml_gui/panels/` (DockableSidebar, LeftSidebar, ObjectList, SliceProgress) + sidebar consumers in PreparePage / PreviewPage
**Screenshots:** Not captured (code-only audit — no dev server)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 3/4 | User-facing labels are localized and upstream-mapped; a few raw English/glyph buttons remain |
| 2. Visuals | 2/4 | Hierarchy OK but card-in-card nesting and 12px pill chrome violate the Phase 74 contract |
| 3. Color | 1/4 | LeftSidebar ships its own 6-color private palette bypassing Theme; 25+ hardcoded hex literals across panels |
| 4. Typography | 2/4 | 4 distinct raw pixel sizes (9/10/11/12) inside LeftSidebar alone; Theme font tokens barely used |
| 5. Spacing | 1/4 | `Theme.panelPadding=12` is referenced **zero times**; margins are 4/5/6/8/10/12/14 with no system |
| 6. Experience Design | 2/4 | SliceProgress missing explicit error/canceled/indeterminate states; sidebar resize handle is a no-op |

**Overall: 11/24**

---

## Top 3 Priority Fixes

1. **Sidebar width system is broken end-to-end** — `Theme.sidebarWidth=240` is dead. Real width is hard-locked to 392 in 4 QML pages *and* 3 C++ constexpr constants where `kSidebarMinWidth == kSidebarMaxWidth == kSidebarDefaultWidth == 392` (`BackendContext.h:467-469`). The DockableSidebar drag handle (`DockableSidebar.qml:106-147`) fires `widthChanged(newW)` but the backend `qBound(392, w, 392)` clamp (`BackendContext.cpp:227`) discards every value, so the handle is a visible no-op — a dishonest control, which Phase 74 explicitly forbids ("visible controls must not be no-op"). **Fix:** either (a) commit to 392 as the truth and update `Theme.sidebarWidth` to 392 with a real min/max range (e.g. 320–520) so the drag handle works, or (b) restore 240 as the truth and redesign `LeftSidebar` to fit. Pick one and delete the other.

2. **LeftSidebar defines a parallel theme** — `LeftSidebar.qml:17-22` declares `panelSurface #303236`, `sectionSurface #33363a`, `controlSurface #3b3e43`, `fieldSurface #2c2f33`, `dividerColor #45484d`, `mutedText #aeb4b9`. These six colors do not exist in `Theme.qml` and do not match its palette (`Theme.bgPanel=#161a23`, `Theme.bgElevated=#2a3140`). Result: the Prepare sidebar renders in a completely different visual register than ObjectList/SliceProgress, which *do* use `Theme.bgPanel`. **Fix:** delete the private palette and replace with `Theme.bgPanel` / `Theme.bgElevated` / `Theme.borderSubtle` / `Theme.textSecondary` etc., adding tokens only if a genuinely new surface is needed.

3. **`Theme.panelPadding=12` is unused — spacing is arbitrary** — `grep -rn panelPadding src/qml_gui` returns only the definition. Actual margins in the audited panels: `anchors.leftMargin: 8` (LeftSidebar:54), `anchors.topMargin: 10` (LeftSidebar:55), `anchors.margins: 14` (SliceProgress:39), `anchors.margins: 8` (ObjectList:1219), `anchors.margins: 5` (LeftSidebar:412). Five different padding values across four panels with no system. **Fix:** route every panel's outer padding through `Theme.panelPadding` (and consider a `Theme.panelPaddingSM=8` for scroll-view gutters), then sweep the panels to use it.

---

## Sidebar Width Investigation

The 392 px hardcode is not a single line — it is a **seven-layer lock** that defeats every escape hatch:

| Layer | File:Line | Value | Role |
|---|---|---|---|
| Theme token (dead) | `Theme.qml:102` | `sidebarWidth: 240` | Defined, never read |
| LeftSidebar target (dead) | `LeftSidebar.qml:16` | `targetSidebarWidth: 392` | Defined, never read |
| PreparePage width | `PreparePage.qml:29` | `sidebarWidth: 392` | Bound into DockableSidebar |
| PreparePage min/max | `PreparePage.qml:30-31` | `392 / 392` | Collapse to constant |
| Plater width + min/max | `Plater.qml:54-56` | `392 / 392 / 392` | Duplicate of PreparePage |
| AssemblePage width | `AssemblePage.qml:34` | `sidebarWidth: 392` | Third copy |
| PreviewPage left width | `PreviewPage.qml:19,23` | `targetPreviewLeftWidth: 392` | Fourth copy |
| BackendContext min | `BackendContext.h:467` | `kSidebarMinWidth = 392` | qBound lower |
| BackendContext max | `BackendContext.h:468` | `kSidebarMaxWidth = 392` | qBound upper |
| BackendContext default | `BackendContext.h:469` | `kSidebarDefaultWidth = 392` | Persistence seed |

Trace of the rendered width: `main.qml:506` binds `sidebarWidth: backend.sidebarWidth` → `backend.sidebarWidth_` is initialized via `qBound(kSidebarMinWidth, savedWidth, kSidebarMaxWidth)` (`BackendContext.cpp:227`) → because min==max==392, every persisted or dragged value collapses to 392. PreparePage then redundantly sets its own `sidebarWidth: 392` and `sidebarMinWidth/sidebarMaxWidth: 392`, passes them into DockableSidebar, which exposes a drag handle — but `widthChanged(newW)` round-trips through `backend.requestSetSidebarWidth(w)` and is re-clamped to 392 on the way back. **Net user-visible behavior: the drag cursor changes to `Qt.SplitHCursor`, the user drags, nothing moves.** This is a BLOCKER under Phase 74's "visible controls must not be no-op" rule.

`LeftSidebar.targetSidebarWidth: 392` (`LeftSidebar.qml:16`) is a second dead reference — declared and never bound anywhere. The Phase 74 spec ("left sidebar must be dense but readable") does not name a pixel width; 392 comes from a screenshot measurement (`BackendContext.h:467` comment: "Screenshot Prepare sidebar width"). The Phase 74 UI-SPEC itself does **not** mandate 392 — it mandates "compact", which 392 px arguably violates on smaller laptop screens. The PreparePage audit's flag is correct: there is no compact-layout fallback as the spec implies.

`Theme.rightPanelWidth=300` fares slightly better — PreviewPage honors the *value* but bypasses the token, re-declaring `targetPreviewRightWidth: 300` (`PreviewPage.qml:20`) and deriving `rightPanelWidth` from it. Same indirection disease.

---

## Detailed Findings

### Pillar 1: Copywriting (3/4)

**Good:**
- All structural labels are localized with `qsTr()`: 打印机 / 耗材 / 工艺 / 全局 / 对象 / 盘 / 质量 / 强度 / 支撑 / 材料 / 其他 (`LeftSidebar.qml:66,184,268,283,290,300,421-426`).
- ObjectList upstream-mapped action labels are present and conditionally pluralized: "删除已选对象" vs "删除对象" (`ObjectList.qml:557-559`), "设为独立对象" vs "拆分为独立对象" (`ObjectList.qml:543-546`).
- Empty state in ObjectList is specific and actionable: "场景中无对象\n请从顶部菜单导入模型" (`ObjectList.qml:1206-1207`).
- SliceProgress hints (`actionHint`, `previewHint`, `exportHint`) are sourced from the ViewModel, so copy stays consistent with state.

**Issues:**
- Raw English leaking through in otherwise-Chinese UI: "No data" in PreviewPage view-mode pill (`PreviewPage.qml:124` — note: outside panels dir but in scope as a sidebar consumer flag) and "Fit" (`PreviewPage.qml:187`). Both should be `qsTr("无数据")` / `qsTr("适配")`.
- Glyph-only button labels: `✕` for delete (`ObjectList.qml:815,1086`), `+`/`-` for expand (`ObjectList.qml:591,673`), `⚙ ✔ ☁ ✓` in SliceProgress (`SliceProgress.qml:56,117`). These have no `ToolTip.text` on the expand/delete glyphs in ObjectList (the `✕` delete rectangles at `ObjectList.qml:810-833` and `1078-1101` carry no tooltip; only the menu items do). Phase 74 requires icon-only buttons be paired with tooltips.
- "No data" appearing as the literal English string in a Chinese-localized build is a copywriting BLOCKER for that surface.
- Import button label "+ 导入模型" (`ObjectList.qml:1228`) mixes a Latin glyph prefix with Chinese — minor but inconsistent with the icon-first Phase 74 rule ("Use icons for tool actions where an icon exists").

**Score rationale:** 3 — strings are mostly upstream-mapped and localized, but the raw "No data"/"Fit" and unkeyed `✕`/`+`/`-` glyphs without tooltips are real copy/affordance gaps.

### Pillar 2: Visuals (2/4)

**Issues:**
- **Card-in-card nesting** — LeftSidebar nests `printerHeroCard` (`radius:4` sectionSurface) inside the outer sidebar `Rectangle` (`panelSurface`) and then nests `printerThumbnail`, three `InfoTile`s, and two `PixelIconButton`s inside that card (`LeftSidebar.qml:73-180`). Phase 74 explicitly says "Do not nest cards inside cards in the Prepare sidebar." Direct violation.
- **12 px pill/card chrome persists** — ObjectList toolbar `radius: 12` (`ObjectList.qml:28`), import button `radius: 12` (`ObjectList.qml:1221`); Phase 74 says "Keep card radius at 8px or less. The current 12px pill/card style is a visual mismatch." Direct violation.
- **Inconsistent radius system** — radii across the audited surface: 3 (InfoTile), 4 (most LeftSidebar cards), 5 (SliceProgress action buttons via `Theme.radiusMD`), 6 (SliceProgress status icon), 7 (ObjectList pill buttons), 8 (Theme.radiusLG, unused here), 12 (ObjectList chrome). Six different radii with no hierarchy. Theme already defines `radiusSM=3 / radiusMD=5 / radiusLG=8 / radiusXL=12 / radiusXXL=16` — only SliceProgress uses any of them.
- **Focal-point drift** — the sidebar's printer hero card (`LeftSidebar.qml:73`) is 76 px tall with a 52×52 thumbnail, three info tiles, AND two icon buttons — visually competing with the parameter list below. Phase 74 wants the sidebar "visually quiet".
- **Two different sidebar visuals exist** — LeftSidebar (panelSurface `#303236`, warm grey, dense) vs DockableSidebar's chrome (`Theme.bgPanel=#161a23`, near-black). When PreparePage wraps LeftSidebar in DockableSidebar (`PreparePage.qml:1649`), the inner warm-grey panel sits inside a near-black frame — two surface tones abutting.
- **Stable-dimensions violation** — ObjectList's selection pill `implicitWidth: selectedCountText.implicitWidth + 10` (`ObjectList.qml:92`) grows/shrinks as the count changes, shifting toolbar layout on every selection change. Phase 74: "Toolbars and floating panels must have stable dimensions."

**Score rationale:** 2 — hierarchy exists but Phase 74's explicit anti-nesting and ≤8px-radius rules are violated, and the sidebar visual register is internally contradictory.

### Pillar 3: Color (1/4)

**Hardcoded color census in audited files:**

- `LeftSidebar.qml:17-22` — 6 colors in a private palette (`#303236`, `#33363a`, `#3b3e43`, `#2c2f33`, `#45484d`, `#aeb4b9`). Plus `#d9dee3` and `#9ea6ad` for the printer thumbnail (`:93,95`) and `#44484d` for button hover (`:642`) and `#f05545` for incompatibility (`:202,258`).
- `ObjectList.qml` — 13+ literals: `#7d2020` (danger hover, `:186,812,1082`), `#ffaaaa` (danger text, `:816,1088`), `#bbc7d4` (object name `:752`), `#111722` (group header `:580`), `#243247` / `#1f2937` / `#1d2735` (status pills `:766,786,1064`), and a 5-color volume-type palette `#e74c3c / #f39c12 / #9b59b6 / #2ecc71 / #3498db / #7b8794` (`:1043-1048`).
- `SliceProgress.qml` — 12+ literals for button states: `#7d2020 / #19a84e / #157a39 / #5e1818` (slice button `:503`), `#7c3aed / #6d28d9` (preview `:542`), `#2563eb / #1d4ed8` (export `:569`), and per-extruder palette `#18c75e / #3b82f6 / #f59e0b / #ef4444` (`:418`). Note `#18c75e` is the *literal expansion* of `Theme.accent` — proving the author knew the token existed.

**Accent (60/30/10) distribution:**
- `Theme.accent` is used appropriately for selection indicators, the dirty-preset dot, and active tab fills — that part is fine.
- But the three colored action buttons in SliceProgress (green/purple/blue, `:496-587`) introduce **two entirely new accent hues** (`#7c3aed` purple, `#2563eb` blue) with no Theme backing. These compete with `Theme.accent` for focal attention, breaking any 60/30/10 split.
- LeftSidebar's `accent` use on the `F` filament icon chip (`LeftSidebar.qml:568`) and active tabs (`:433,435`) is fine; the problem is the *base palette* it sits on is off-Theme.

**Score rationale:** 1 — the LeftSidebar private palette is a complete Theme bypass, two new accent hues are invented in SliceProgress, and 25+ hex literals have no token backing. This is the worst pillar.

### Pillar 4: Typography (2/4)

**Font size distribution in audited files:**

| Size | Where | Token? |
|---|---|---|
| 8 | ObjectList plate/module pill text (`:778,798,1074`), SliceProgress plate pill (`:170,176`) | raw |
| 9 | ObjectList toolbar pills (`:48,58,68,78,103,112,127,143,158,172,187,199,614`), volume row name (`:1056`) | raw |
| 10 | LeftSidebar InfoTile body (`:614,622`), tab labels (`:442`), search icon-related; ObjectList group header (`:599,614`), status text (`:805`); SliceProgress hint/extruder labels (`:424,439,469,623`) | raw |
| 11 | LeftSidebar most text (`:120,223,378,389,441,568,578,588,686`); ObjectList object name (`:728,753`); SliceProgress many labels | raw |
| 12 | LeftSidebar headers (`:120,577`); ObjectList title (`:43`); SliceProgress status (`:67`), result values (`:204,246,299,314,346,365,381`) | raw |
| 13 | ObjectList text-field dialog (`:1305`) | raw |
| 14 | — | — |
| 16 | — | — |
| 20 | — | — |

**Token use:**
- `Theme.fontSizeSM=11`, `Theme.fontSizeMD=12`, `Theme.fontSizeLG=14` together appear exactly **5 times** across all four panels (ObjectList:43, SliceProgress:67,82,88,125). Everything else is a raw literal — including `font.pixelSize: 9`, which does not exist in the Theme scale at all (Theme jumps from XS=10 to SM=11). Size 9 is an **off-scale** value used 15+ times.
- No `Theme.fontSizeXS=10` use anywhere in panels despite being the obvious token for the 10px labels.
- Bold weight is applied inconsistently: LeftSidebar uses `font.bold: true` on tabs only when active (`:443`) — good — but ObjectList uses bold on every toolbar pill (`:43`) and every status pill, flattening hierarchy.

**Score rationale:** 2 — a clear type hierarchy exists (9/10/11/12), but it bypasses Theme tokens almost entirely and introduces an off-scale 9 px size. Phase 74 ("Do not scale font size with viewport width") is satisfied, but the *consistency* rule is not.

### Pillar 5: Spacing (1/4)

**`Theme.panelPadding=12` usage in audited scope: zero.** Confirmed by `grep -rn panelPadding src/qml_gui` returning only the definition.

**Outer padding census:**

| Panel | Outer margin | Source |
|---|---|---|
| LeftSidebar scroll view | left/right 8, top 10, bottom 8 (`:53-56`) | raw |
| LeftSidebar paramsInlinePanel | margins 5 (`:412`) | raw |
| LeftSidebar hero card | margins 8 (`:85`) | raw |
| ObjectList toolbar | left 10, right 8 (`:36-37`) | raw |
| ObjectList import button | margins 8 (`:1219`) | raw |
| SliceProgress root | margins 14 (`:39`) | raw |
| SliceProgress info card | margins 10 (`:197`) | raw |
| SliceProgress hint card | margins 9 (`:487`) | raw |
| DockableSidebar drag handle | width 6 (`:113`) | raw |

**Spacing values in use:** 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 18. Theme defines `spacingXS=4 / spacingSM=6 / spacingMD=8 / spacingLG=12 / spacingXL=16 / spacingXXL=24` — half of those (4/6/8/12/16) are available, but the panels use off-token 5, 7, 9, 10, 14 instead. The `spacingXXL=24` token is unused.

**Information density:**
- LeftSidebar at 392 px is dense and scannable — within spec.
- SliceProgress at 300 px (rightPanelWidth) is **too sparse for the right-panel width**: each result row is a single label/value pair with 6 px spacing (`:197`), wasting vertical space; the per-extruder breakdown alone (`:396-444`) can consume 80+ px.
- ObjectList toolbar packs 10+ pill buttons into a single 36 px row (`:21-202`) — **too cramped**; with selection pill + count text + 8 action pills, the row overflows at narrow widths and pills clip.

**Score rationale:** 1 — the headline token (`panelPadding=12`) is dead, 5/9/10/14 are off-scale, and density is uneven (cramped toolbar, sparse SliceProgress). Phase 74's "compact but readable" goal is met only by LeftSidebar.

### Pillar 6: Experience Design (2/4)

**SliceProgress state coverage:**

| State | Covered? | Evidence |
|---|---|---|
| Idle / waiting | yes | `statusLabel: qsTr("等待切片")` (`SliceProgress.qml:15`), `☁` glyph (`:56`) |
| Slicing (determinate) | yes | `slicingNow` + `pct` + `CxProgressBar` (`:93-98`) |
| Slicing (indeterminate) | **no** | No spinner; `⚙` glyph is static (`:56`). When `pct=0` at slice start, the bar sits empty with no motion. |
| Canceled | **no** | No `canceled` visual state. `cancelSlice()` (`:522`) just flips back to idle; user gets no "已取消" confirmation. |
| Error | **partial** | No error-specific UI. Errors surface only as `actionHint` text in the hint card (`:488`), and the card turns `bgErrorSubtle` only when `!canRequestSlice` (`:481`) — but `actionHint` is a ViewModel string, not a structured error path. |
| Complete | yes | `pct >= 100` → `✔` glyph (`:56`), result card visible (`:191`), action row enabled (`:534`) |
| Multi-plate | yes | `plateCount > 1` plate-status row (`:131-183`) and "所有平板已切片完成" banner (`:101-129`) |

**Other interaction issues:**
- **Dishonest resize handle** — `DockableSidebar.qml:106-147` drag handle changes cursor to `SplitHCursor` and animates accent color on hover (`:115`), implying resizability, but the backend clamp makes it a no-op (see Sidebar Investigation). Phase 74 forbids no-op visible controls.
- **No disabled-state reason** — SliceProgress "全部切片" button (`:594-614`) and "预览" (`:541-560`) go disabled but only show a hint *below* the buttons (`:618-625`); the buttons themselves give no inline reason. Phase 74: "Disabled/blocked controls must be honestly disabled with a reason."
- **Destructive action without confirmation** — ObjectList inline `✕` delete (`:810-833`, `:1078-1101`) and the "删除" toolbar pill (`:181-195`) both call `deleteSelection()` directly with no confirmation dialog. Object selection is reversible via undo, but the toolbar "删除" pill on a multi-selection is a bulk destructive action with zero confirmation gate.
- **Good:** ObjectList inline rename on double-click (`:1174-1181`), drag-reorder with insertion-line indicator (`:262-270`), and context-menu breadth (`:281-569`) are all solid interaction patterns aligned with upstream.
- **Good:** DockableSidebar's collapsed-handle expand affordance (`:80-104`) and NumberAnimation width transition (`:44`) are polished.

**Score rationale:** 2 — the core slice flow states (idle/slicing/complete/multi-plate) are covered, but indeterminate, canceled, and explicit error states are missing, the resize handle is dishonest, and bulk delete has no confirmation gate.

---

## Per-File Verdicts

- **`DockableSidebar.qml`** — WARNING. Wrapper logic (collapse, drag, dock-mirror) is clean, but exposes a drag handle that the backend clamp silently defeats; the `_isRightDocked`/`widthChanged`/`toggleRequested` callback properties are an ad-hoc signal system that should be real signals.
- **`LeftSidebar.qml`** — BLOCKER. Ships a 6-color private palette bypassing Theme, contains unused `targetSidebarWidth: 392`, nests cards-in-cards against Phase 74 rules, and uses raw font literals throughout. This is the single worst token-discipline offender.
- **`ObjectList.qml`** — WARNING. Functionally rich (drag-reorder, inline rename, deep context menu), but 13+ hardcoded hex literals, 12 px radius chrome against spec, off-scale 9 px font everywhere, cramped toolbar that overflows, and unconfirmed bulk delete.
- **`SliceProgress.qml`** — WARNING. Best Theme-citizen of the four (uses `Theme.radiusMD`, `Theme.fontSizeSM`, `Theme.bgPanel`), but invents two new accent hues (`#7c3aed`, `#2563eb`), has no indeterminate/canceled/explicit-error state, and mixes raw hex button colors with `Theme` colors in the same component.
- **`PreparePage.qml` (sidebar portion)** — BLOCKER. Hardcodes `sidebarWidth/sidebarMinWidth/sidebarMaxWidth: 392` (`:29-31`), duplicating BackendContext's constants; the inline `Left sidebar (280px)` comment (`:1645`) is a third contradictory width value (240/280/392). Comment lie.
- **`PreviewPage.qml` (sidebar portion)** — WARNING. Re-declares `targetPreviewLeftWidth: 392` and `targetPreviewRightWidth: 300` (`:19-20`) instead of binding to `Theme.sidebarWidth`/`Theme.rightPanelWidth`; uses LeftSidebar directly without DockableSidebar, so preview has no collapse/dock parity with prepare.
- **`Theme.qml` (as baseline)** — WARNING. Three of its tokens (`sidebarWidth=240`, `rightPanelWidth=300`, `panelPadding=12`) are either dead or bypassed in the audited scope. The token system exists but is not enforced.

---

## Files Audited

- `E:/ai/3DPrinter_Qt6/src/qml_gui/Theme.qml` (baseline)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/panels/DockableSidebar.qml`
- `E:/ai/3DPrinter_Qt6/src/qml_gui/panels/LeftSidebar.qml`
- `E:/ai/3DPrinter_Qt6/src/qml_gui/panels/ObjectList.qml`
- `E:/ai/3DPrinter_Qt6/src/qml_gui/panels/SliceProgress.qml`
- `E:/ai/3DPrinter_Qt6/src/qml_gui/pages/PreparePage.qml` (sidebar region: lines 27-35, 1637-1668)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/pages/PreviewPage.qml` (sidebar region: lines 18-25, 226-246, 303-443)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/pages/Plater.qml` (sidebar width duplication, lines 54-56)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/pages/AssemblePage.qml` (sidebar width duplication, line 34)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/BackendContext.h` (sidebar width constants, lines 466-472)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/BackendContext.cpp` (sidebar clamp, line 227)
- `E:/ai/3DPrinter_Qt6/src/qml_gui/main.qml` (sidebar binding, lines 506-511)
- `E:/ai/3DPrinter_Qt6/.planning/milestones/v3.9-phases/74-prepare-source-truth-gap-audit/74-UI-SPEC.md` (baseline contract)

**Note:** No shadcn `components.json` present; registry safety audit skipped per spec.
