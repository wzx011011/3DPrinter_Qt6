# Pages + Shell — UI Review (v5.1 retroactive)

**Audited:** 2026-07-17
**Baseline:** abstract 6-pillar standards + Theme.qml token system (E:/ai/3DPrinter_Qt6/src/qml_gui/Theme.qml)
**Scope:** main.qml shell + BBLTopbar + 10 page files (excluding PreparePage — separately audited)
**Method:** Code-only (no dev server / Playwright; QML adaptation of web grep patterns)
**Screenshots:** Not captured (QML desktop app, no dev server reachable)

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 2/4 | Jarring zh/EN mixing per-page (MultiMachine all-EN, Monitor/Home all-zh); brand literal "OWzx Slicer" untranslated |
| 2. Visuals | 2/4 | Massive Rectangle+MouseArea pseudo-button bypass of Cx* library (MonitorPage 92, MultiMachinePage 51) |
| 3. Color | 1/4 | 257 hardcoded hex colors; **PreferencesPage has ZERO Theme.* refs with 129 hex colors** |
| 4. Typography | 2/4 | 17 distinct numeric font.pixelSize values; only 46% use Theme.fontSize* tokens |
| 5. Spacing | 2/4 | main.qml / BBLTopbar / PreviewPage / PreferencesPage use 0 Theme.spacing* tokens |
| 6. Experience Design | 3/4 | Strong state coverage; missing destructive-action confirmations on 4+ actions |

**Overall: 12/24**

---

## Top 3 Priority Fixes

1. **PreferencesPage.qml is a token-system island** (0 Theme.* refs, 129 hardcoded hex colors, 0 Theme.spacing*, hardcoded font sizes throughout) — every page except this one uses Theme.* tokens; this file silently bypasses the design system entirely. **Fix:** mechanical pass replacing `color: "#a0abbe"` → `color: Theme.textSecondary`, `color: "#566070"` → `Theme.textDisabled`, `color: "#18c75e"` → `Theme.accent`, etc. The color hex values already match Theme tokens (a0abbe=textSecondary, 566070=textDisabled, 18c75e=accent, c8d4e0≈chromeText) — this is pure drift, not a design intent.

2. **Massive Rectangle+MouseArea pseudo-button pattern** (92 in MonitorPage, 51 in MultiMachinePage, 27 in CalibrationPage, 19 in PreferencesPage, 16 in HomePage, 14 in PreviewPage) bypasses the Cx* control library and Theme tokens, so hover/disabled/tooltip states are hand-rolled inconsistently (e.g., MonitorPage.qml:1208-1244 uses 4 different button-color families, none from Theme). **Fix:** replace `Rectangle{ MouseArea{ onClicked } }` button-like blocks with `CxButton`/`CxIconButton`; replace `Text { HoverHandler{} TapHandler{} }` icons with `CxIconButton` for tooltip/aria/consistent hover.

3. **Destructive actions fire without confirmation dialogs** — `cloudUnbindDevice(index)` (HomePage.qml:241), `removeDevice(index)` (MultiMachinePage.qml:661, an "x" button on hover), `stopAllLocalTasks()` (MultiMachinePage.qml:740), `stopAllCloudTasks()` (MultiMachinePage.qml:1064), `disconnectDevice(0)` (MonitorPage.qml:1223) all execute immediately on click/tap. **Fix:** add `CxDialog` confirm step on destructive ops, mirroring the existing `newProjectDialog` pattern in main.qml:255.

---

## Detailed Findings

### Pillar 1: Copywriting (2/4)

**Language mixing inconsistency (the dominant defect).** The shell is bilingual by intent but the page-level copy is split by file, not by user setting:

- **MultiMachinePage.qml** — entirely English UI strings (55 EN qsTr, 0 zh): `qsTr("Multi-Device Print")` (line 39), `qsTr("Device")`/`"Task Sending"`/`"Task Sent"` (lines 78-80), `qsTr("Search devices...")` (line 201), `qsTr("Send All")` (line 704), `qsTr("Pause All")`/`"Resume All"`/`"Stop All"` (lines 1029/1043/1057), `qsTr("Select Device to Send")` (line 1397), `qsTr("No sending tasks")` (line 793), `qsTr("No sent tasks")` (line 1117).
- **CalibrationPage.qml** — 25 EN qsTr, 9 zh qsTr, mixed within the same screen: header `qsTr("Calibration Center")` (line 98, EN) but `qsTr("历史记录")` (line 127, zh) on the same row; filter pills `qsTr("All")`/`"Slice Calibration"`/`"Hardware Calibration"` (lines 142-144, EN) sit next to zh sidebar headers `qsTr("SLICE CALIBRATION")` (line 217, EN) and zh column labels `qsTr("耗材预设")` (line 781, zh). User-visible confusion.
- **MonitorPage / HomePage / main.qml / BBLTopbar / ProjectPage** — entirely Chinese strings.
- **AssemblePage.qml** — entirely Chinese strings.

Result: switching tabs produces a visible language flip. This violates the AGENTS.md convention "all user-visible strings `qsTr()`" — the strings ARE wrapped, but the source-language is inconsistent so the .ts translation catalogs cannot compensate.

**Other defects:**

- **Brand literal "OWzx Slicer" untranslated** in 2 places — main.qml:217 (`text: "OWzx Slicer"` inside AboutDialog) and HomePage.qml:63 (`text: "OWzx Slicer"`). Minor but consistent (brand names are usually left untranslated; the inconsistency is the duplicated label rather than wrapping). This is borderline acceptable.
- **Untranslated UI strings** "LAN" (MonitorPage.qml:1362) and "CP……" (MonitorPage.qml:1445) — likely should be `qsTr("LAN")`/`qsTr("CP……")`.
- **Generic placeholder texts** ("No data" PreviewPage.qml:124, "OK"/"Cancel" patterns throughout MultiMachinePage.qml:1482/1488 and HomePage.qml:336/340) — acceptable as dialog convention but not aligned to a copy contract.
- **Developer-jargon strings** visible in user-facing copy: `"Mock 模式，更新检查功能需要连接更新服务器后启用"` (PreferencesPage.qml:637), `"Backend unavailable"` (BBLTopbar.qml:555), `"未实现"`/`"Hardware calibration pending"` (BBLTopbar.qml:929, EN). "Mock mode" is internal terminology that should not surface.
- **Status bar literal concatenation** in main.qml:575 — `"就绪  |  Qt 6.10  |  页面 " + (backend.currentPage + 1) + " / 9  |  " + backend.latencyBrief` — hardcoded page count "9" will drift if pages added; should use `backend.tabCount`.

**Strengths:** `qsTr()` is used consistently (no obvious naked user-visible strings).

---

### Pillar 2: Visuals (2/4)

**Hand-rolled `Rectangle + MouseArea/TapHandler` pseudo-buttons bypass the Cx* library** at industrial scale. Per-file counts:

| File | MouseArea | TapHandler | CxButton | CxIconButton |
|------|-----------|-----------|----------|--------------|
| MonitorPage.qml | 25 | 5 | 0 | 0 |
| MultiMachinePage.qml | 7 | 26 | 0 | 0 |
| CalibrationPage.qml | 0 | 9 | 4 | 0 |
| PreferencesPage.qml | 10 | 1 | 2 | 0 |
| main.qml | 12 | 0 | 0 | 0 |
| HomePage.qml | 4 | 0 | 5 | 0 |
| BBLTopbar.qml | 6 | 1 | 0 | 14 |
| PreviewPage.qml | 4 | 0 | 0 | 0 |
| ProjectPage.qml | 0 | 0 | 1 | 0 |
| AssemblePage.qml | 0 | 0 | 5 | 0 |

`Rectangle` total per file (many are pseudo-buttons): MonitorPage 92, MultiMachinePage 51, CalibrationPage 27, PreferencesPage 19, HomePage 16, PreviewPage 14, BBLTopbar 14, ProjectPage 10, main.qml 9.

**Concrete defects:**

- **Icon-only buttons with empty `toolTipText`** — BBLTopbar.qml:255-258 (Account placeholder), 266-269 (ModelStore), 277-280 (Publish). These have `toolTipText: ""` and `enabled: false; visible: false`, but the pattern shows the CxIconButton tooltip affordance is not enforced as a project rule.
- **Inconsistent button-color families on MonitorPage** — destructive `disconnect` uses `#dc2626`/`#ef4444` (MonitorPage.qml:1210), `startPrint` uses `#16a34a`/`#22c55e` (1230), `pause` uses `#d97706`/`#f59e0b` (1250), `resume` uses `#16a34a`/`#22c55e` (1270) — six hex colors, none from Theme, all Tailwind/Bootstrap palette instead of OWzx Theme.status* tokens.
- **Inconsistent icon sizing in BBLTopbar** — chrome buttons use `buttonSize: 30; iconSize: 16` (Save/Undo/Redo/Calibration), but window controls use `buttonSize: 30; iconSize: 14` (Min/Max/Close:459/467/478), and bell uses `iconSize: 14` while slice/print use 16. The chrome-text-icon density is uneven.
- **No clear focal point on some pages** — HomePage stacks 5 panels vertically (banner + cloud-devices + daily-tip + recent-projects + quick-actions) with no size/weight differentiation; the focal point is unclear.
- **Focal-point issue on PreviewPage** — empty-state pill (line 271) appears on the canvas, but the GLViewport underneath still receives keyboard input (PreviewPage.qml:36-75 `Keys.onPressed`); visual focus ≠ input focus.

**Strengths:**
- BBLTopbar uses CxIconButton consistently for chrome controls (14 instances), each with `cxStyle: CxIconButton.Style.Chrome`.
- AssemblePage is clean — uses CxButton for all primary actions (5/5), zero pseudo-buttons.
- TabBar (BBLTopbar.qml:287-360) has proper visual hierarchy with accent on checked, accentLight on hovered, and ColorAnimation transitions.

---

### Pillar 3: Color (1/4)

**Total: 257 hardcoded hex colors across scope.** Distribution:

| File | Hex count | Theme.* refs | Coverage |
|------|-----------|--------------|----------|
| PreferencesPage.qml | 129 | **0** | **0%** |
| MonitorPage.qml | 89 | 307 | 78% |
| HomePage.qml | 8 | 69 | 90% |
| main.qml | 4 | 16 | 80% |
| BBLTopbar.qml | 4 | 25 | 86% |
| MultiMachinePage.qml | 2 | 297 | 99% |
| AssemblePage.qml | 2 | 46 | 96% |
| ProjectPage.qml | 0 | 28 | 100% |
| CalibrationPage.qml | 0 | 157 | 100% |
| Plater.qml | 0 | 0 | N/A (wrapper) |
| PreviewPage.qml | 0 | 28 | 100% |

**PreferencesPage.qml is the BLOCKER.** Zero Theme.* references across all 860 lines. Top hardcoded colors that **literally match existing Theme tokens** (silent redundancy):

- `color: "#a0abbe"` (29 occurrences) = `Theme.textSecondary` (Theme.qml:25)
- `color: "#566070"` (11) = `Theme.textDisabled` (Theme.qml:27)
- `color: "#c8d4e0"` (9) ≈ `Theme.chromeText` (Theme.qml:44)
- `color: "#18c75e"` (multiple) = `Theme.accent` (Theme.qml:18)
- `color: "#0d0f12"` (2) = `Theme.bgBase` (Theme.qml:6)
- `color: "#1e2430"` / `"#151c28"` / `"#1a1e28"` — hardcoded backgrounds with no Theme equivalent, drift.

Every accent text (`"#18c75e"`), every primary text (`"#e8edf6"`), every secondary text (`"#a0abbe"`) in PreferencesPage is a hardcoded copy of a Theme token. This is pure drift, not design intent.

**Other color defects:**

- **main.qml uses `"#0d0f12"`** (== Theme.bgBase) 3 times — line 24 (ApplicationWindow.color), 166 (shortcut keycap), 366 (fallback for shell Rectangle). Shell should anchor on Theme.
- **BBLTopbar.qml:561** `prepareSliceButton` uses `color: enabled ? "#4a7f76" : "#33433f"` — a teal-grey slice button with no Theme equivalent; BBLTopbar.qml:571 `color: "#e7f4f0"` text on it; BBLTopbar.qml:596 `enabled ? Theme.accent : "#31504b"` — mix of Theme + non-Theme for the same control.
- **MonitorPage.qml:1208-1250** — 6 unique Tailwind palette hex colors for 4 different action button states, none from Theme.
- **HomePage.qml** — `color: "#fef2f2"` (line 150), `color: "#ef4444"` (lines 155/233/326/420), `color: "#22c55e"` (line 204), `color: "#6b7280"` (line 204), `color: "#1018c75e"` (line 515, accent at 6% alpha). All status/destructive colors are Tailwind palette, not Theme.status* / Theme.accent.
- **CalibrationPage.qml:431** `color: Qt.rgba(0.96, 0.65, 0.14, 0.12)` — equivalent to Theme.statusWarning at low alpha but hand-rolled.
- **No undefined Theme.* token references found** in scope (silent-undefined risk is clean here — only PreparePage.qml uses an undefined `Theme.borderActive`, which is out of scope).

**60/30/10 distribution:** Not analyzable from code — needs screenshots. But the inconsistency between Theme-based pages (CalibrationPage, ProjectPage, PreviewPage at 100% Theme) and hex-laden pages (PreferencesPage 0%, MonitorPage 22% drift) means the visual brand identity cannot hold across the app.

---

### Pillar 4: Typography (2/4)

**17 distinct numeric `font.pixelSize` values in scope** (target: ≤4 from token system):

| Size | Count | Theme token? |
|------|-------|--------------|
| 10 (fontSizeXS) | 44 | token used 11×, raw 33× |
| 11 (fontSizeSM) | 52 | token used 12×, raw 40× |
| 12 (fontSizeMD) | 63 | token used 22×, raw 41× |
| 13 | 17 | **not in token system** |
| 14 (fontSizeLG) | 6 | token used 5×, raw 1× |
| 16 (fontSizeXL) | 7 | token used 5×, raw 2× |
| 9 | 6 | **off-scale** |
| 8 | 3 | **off-scale** |
| 48 | 3 | **off-scale** (emoji icons) |
| 64 | 2 | **off-scale** (emoji icons) |
| 36 | 2 | **off-scale** |
| 24 | 2 | **off-scale** |
| 20 (fontSizeXXL) | 2 | token used 1×, raw 1× |
| 18 | 2 | **off-scale** |
| 56 | 1 | **off-scale** (emoji icon CalibrationPage:343) |
| 34 | 1 | **off-scale** (emoji icon HomePage:508) |
| 22 | 1 | **off-scale** |

Total `font.pixelSize:` occurrences: **396**. Theme.fontSize* usage: **181** (46%). The other 54% are raw numerics.

**Concrete defects:**

- **`font.pixelSize: 13` is used 17 times** but is NOT in the Theme token system (Theme has XS=10/SM=11/MD=12/LG=14/XL=16/XXL=20 — no 13). Examples: HomePage.qml:64-65, ProjectPage.qml:203, MonitorPage.qml (multiple), MultiMachinePage.qml:234/258/280/297. This is a gap between observed need and the declared scale.
- **`font.pixelSize: 9`** appears 6 times (off-scale, smaller than the declared XS=10 minimum): MonitorPage.qml:935 ("Preview area" caption), MultiMachinePage.qml:241/263 (sort arrow size), CalibrationPage.qml:833 ("点击下方按钮切换预设"), PreferencesPage.qml (multiple via `font.pixelSize: 9`).
- **`font.pixelSize: 8`** appears 3 times — MultiMachinePage.qml:241/263/280 sort arrows and fontSize=8 column.
- **HomePage.qml is the worst offender** — 10 distinct sizes in one file (10/11/12/13/14/16/20/22/24/34), only 1-2 use Theme tokens.

**Weight audit:** `font.bold: true` count is high (94 total: MultiMachinePage 23, MonitorPage 19, HomePage 14, CalibrationPage 12, PreferencesPage 11) but all use the single `bold: true` binary — no `font.weight: Font.Medium`/`DemiBold`/`Light` mixed in. Only 2 weights (normal + bold) — within the ≤2 weight budget.

---

### Pillar 5: Spacing (2/4)

**Theme.spacing* usage by file:**

| File | Theme.spacing* count |
|------|---------------------|
| main.qml | **0** |
| BBLTopbar.qml | **0** |
| PreviewPage.qml | **0** |
| PreferencesPage.qml | **0** |
| Plater.qml | **0** |
| ProjectPage.qml | 1 |
| AssemblePage.qml | 3 |
| HomePage.qml | 6 |
| CalibrationPage.qml | 11 |
| MonitorPage.qml | 20 |
| MultiMachinePage.qml | 26 |

**4 of 11 files use 0 Theme.spacing* tokens** — including the two largest (main.qml shell, BBLTopbar title bar). The `main.qml` shortcut dialog uses `spacing: 4` and `spacing: 16` (main.qml:131, 163) instead of Theme.spacingXS/spacingXL.

**Aggregate numeric spacing/margin values** (across all scope files):

| Value | Count | Theme token? |
|-------|-------|--------------|
| 0 | 32 | (none) |
| 4 (spacingXS) | 35 | mixed |
| 6 (spacingSM) | 13 | mixed |
| 8 (spacingMD) | 38 | mixed |
| 12 (spacingLG) | 28 | mixed |
| 16 (spacingXL) | 44 | mixed |
| 24 (spacingXXL) | 1 | (used raw) |
| 1 | 7 | off-scale |
| 2 | 16 | off-scale |
| 5 | 4 | off-scale |
| 7 | 1 | off-scale |
| 10 | 9 | off-scale |
| 14 | 7 | off-scale |
| 18 | 3 | off-scale |
| 80 | 1 | off-scale |
| 196 | 1 | off-scale |

The on-scale values are well-represented (4/6/8/12/16 dominate) but off-scale values (1, 2, 5, 7, 10, 14, 18) appear ~50 times — these are not in the Theme spacing scale (Theme has only XS=4/SM=6/MD=8/LG=12/XL=16/XXL=24).

**Concrete arbitrary-spacing defects:**
- **`anchors.leftMargin: 80`** — AssemblePage.qml:108 (Move/Rotate/Scale selector offset from left edge) — arbitrary.
- **`width: 196` / `width: 196; height: 164`** — HomePage.qml:504 (recent-project card) — arbitrary fixed dimension.
- **`height: 52`** — main.qml (and elsewhere) — the `prepareChromeHeight: 70` token exists; not all chrome heights use it.
- **`Layout.preferredHeight: 52`** MonitorPage.qml:81 / `Layout.preferredHeight: 64` MonitorPage.qml:633 / `Layout.preferredHeight: 36` etc. — these card heights don't map to Theme tokens (Theme has controlHeightSM=28/MD=34/LG=40, no card-height token).
- **`spacing: 7`** — BBLTopbar.qml:528 (icon-label spacing inside tab button) — between spacingSM(6) and spacingMD(8).

---

### Pillar 6: Experience Design (3/4)

**Strengths — state coverage is genuinely strong:**

- **MonitorPage.qml** has a complete 4-state machine (`monitorState === 0/1/2/3` — NoPrinter / Connecting / Disconnected / Normal) at lines 472-629, each with dedicated icon + headline + subtext + CTA. The Connecting state even has an animated spinner (lines 530-551).
- **PreviewPage.qml** has `hasPreviewData` empty state (line 271) showing `previewStatusText` ("请先切片或载入 G-code") and a "No data" status pill for unavailable view modes (line 113-141).
- **CalibrationPage.qml** has selectedIndex < 0 empty state (line 338), isRunning progress bar (line 879-907), unavailable-reason warning banner (line 426-445), and a status badge system (InProgress/Completed/Failed).
- **HomePage.qml** has `cloudSyncing` loading state (line 144), `cloudLoginFailed` error display (line 324-329), cloud-bound-device-count conditional sections (line 174).
- **MultiMachinePage.qml** has empty states for all 3 tabs (lines 375-408, 784-799, 1108-1123) plus pagination (visible only when totalPages > 1).
- **Async feedback** via `Connections` is consistent: `onSelectionChanged`/`onStatusChanged`/`onStepChanged` in CalibrationPage.qml:60-66, `onCloudLoginFailed`/`onCloudStateChanged` in HomePage.qml:349-358.
- **Disabled states are generally good** — BBLTopbar gates Save/Undo/Redo on `backend.canSave`/`canUndo`/`canRedo` (lines 211/225/236), Import/Export on `backend.canImport`/`canExport` (BBLTopbar.qml:748/753/758/763/768/778/783/788/793).
- **Keyboard shortcuts** are comprehensive — main.qml:294-350 covers Undo/Redo/Save/Delete/Copy/Cut/Paste/Duplicate/Import/Open. BBLTopbar workflow tabs support `Keys.onPressed` for Return/Enter/Space (line 511).

**Defects:**

- **No destructive-action confirmation dialogs.** Five immediate-fire destructive paths:
  1. `cloudUnbindDevice(index)` — HomePage.qml:241 (unbind cloud device, single click)
  2. `removeDevice(index)` — MultiMachinePage.qml:661 (tiny "x" on hover, single tap)
  3. `stopAllLocalTasks()` — MultiMachinePage.qml:740
  4. `stopAllCloudTasks()` — MultiMachinePage.qml:1064
  5. `disconnectDevice(0)` — MonitorPage.qml:1223

  None has a `CxDialog` confirm step. The existing pattern (`newProjectDialog` at main.qml:255) is the template; these destructive ops should mirror it. The "x" remove button in MultiMachinePage.qml:646-662 is 16x16 px, font.pixelSize 9 — usability hazard (easy to mis-tap, no undo, no confirm).

- **Error handling is silent in some paths.** ProjectPage.qml:33-37 (`loadFile`) and :57-61 (`saveProjectAs`) and :73-77 (`exportModel`) all check `if (root.editorVm)` but do not surface the return value `ok` to the user — only `console.log`. Save failures are silent to the user.

- **No loading skeleton on PreviewPage layer-rail / right-panel** — `previewVm.gcodeLines` (line 401) is empty until slice completes; the empty-state pill (line 271) covers the canvas but the right-panel `gcodeSourcePanel` (line 362) shows an empty list with no placeholder.

- **No error boundary for camera feed** in MonitorPage video tab (tab index 2) — beyond line 1272 (not read in this audit) but the camera infrastructure (CameraImageProvider) is known-stubbed per project context (FFmpeg unavailable).

- **HomePage "login" / "bind" dialogs have weak error display** — HomePage.qml:324-329 shows `loginError` in a single red Label but no error icon, no field-level validation (empty username/password), no "forgot password" affordance.

- **`forceActiveFocus()` is called inside `onOpened`** (HomePage.qml:364) which is correct, but the field validation doesn't disable the submit button when fields are empty — users can submit an empty form and only see a server error.

---

## Files Audited

- `src/qml_gui/main.qml` (795 lines)
- `src/qml_gui/BBLTopbar.qml` (949 lines)
- `src/qml_gui/pages/PreviewPage.qml` (553 lines)
- `src/qml_gui/pages/AssemblePage.qml` (471 lines)
- `src/qml_gui/pages/CalibrationPage.qml` (985 lines)
- `src/qml_gui/pages/HomePage.qml` (561 lines)
- `src/qml_gui/pages/ProjectPage.qml` (273 lines)
- `src/qml_gui/pages/MonitorPage.qml` (2280 lines)
- `src/qml_gui/pages/MultiMachinePage.qml` (1503 lines)
- `src/qml_gui/pages/PreferencesPage.qml` (860 lines)
- `src/qml_gui/pages/Plater.qml` (114 lines, wrapper)

## Per-File Verdicts

- **main.qml** — Functional shell with solid dialog/shortcut wiring; bypasses Theme for shell background (`#0d0f12` × 3) and uses 0 Theme.spacing* in shortcut dialog. StatusBar text hardcodes "/ 9" page count.
- **BBLTopbar.qml** — Best-in-scope file; uses CxIconButton consistently (14×) with proper `cxStyle: Chrome`. Local drift: slice/export buttons use hex `#4a7f76`/`#31504b`/`#e7f4f0` not in Theme.
- **PreviewPage.qml** — Token-clean (0 hardcoded hex), strong empty-state coverage. 0 Theme.spacing* (uses raw 4/6/8/12). Uses `font.family: "Consolas"` hardcoded.
- **AssemblePage.qml** — Token-clean, uses CxButton for all actions. Measure panel uses `color: Theme.bgFloating` correctly. Minor: `measureAccent: "#4ec9b0"` (line 58) is an off-Theme teal.
- **CalibrationPage.qml** — Token-clean (0 hex) but has heavy zh/EN copy mixing within the same screen. Uses CxSpinBox/CxButton/CxProgressBar correctly. Empty state at line 338 is good.
- **HomePage.qml** — Token-clean but uses Tailwind palette (`#ef4444`/`#22c55e`/`#6b7280`) for status indicators instead of Theme.status*. Login dialog error UX is weak.
- **ProjectPage.qml** — Token-clean, simplest page. Save/export paths swallow return values silently (only console.log).
- **MonitorPage.qml** — WARNING: 89 hardcoded hex colors, 25 MouseArea pseudo-buttons, 6 different button-color families in the action row. The 4-state machine (NoPrinter/Connecting/Disconnected/Normal) is excellent; the visual implementation is not.
- **MultiMachinePage.qml** — Entirely English UI strings (55 EN qsTr, 0 zh) — language drift from rest of app. 26 TapHandler pseudo-buttons. Destructive actions (remove/stop-all) have no confirmation.
- **PreferencesPage.qml** — BLOCKER: Zero Theme.* references across all 860 lines (129 hardcoded hex colors, all matching existing Theme tokens). This file is a separate design system from the rest of the app. CxSwitch/CxSlider/CxComboBox used correctly for controls.
- **Plater.qml** — Pure wrapper (114 lines, no Rectangle-as-button, no font/color logic). No findings.
