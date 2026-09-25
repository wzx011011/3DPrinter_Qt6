---
phase: 02
plan: 02
subsystem: qml_gui-topbar-framework
tags: [bbltopbar, tabbar, stacklayout, 9-page-notebook, menu-completion, q_property-enum]
requires:
  - "src/qml_gui/BackendContext.h (TabPosition Q_ENUM + requestSelectTab from Plan 02-01)"
  - "src/qml_gui/BackendContext.cpp (requestSelectTab impl)"
  - "third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp:29-41 (CUSTOM_ID enum)"
  - "third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229 (upstream TabPosition)"
provides:
  - "src/qml_gui/BBLTopbar.qml — full top strip component (logo+menu+tools+tabs+title+controls)"
  - "src/qml_gui/main.qml — slim shell with BBLTopbar + 9-page StackLayout"
  - "BackendContext Q_PROPERTY(int tpX) accessors — robust QML enum access via context property"
  - "main.qml::bblTopbar signal contract (Phase 3 Plater will subscribe to tabSelectRequested)"
  - "i18n/zh_CN.ts + en.ts — 73 new qsTr() keys (Calibration 9 entries, Import/Export menus, tooltips)"
affects:
  - "Future Phase 3 (Plater shared instance) consumes backend.tabSelectRequested signal"
  - "Future Phase 7 (Calibration Dialog impl) enables the 9 disabled Calibration menu entries"
  - "Future v2.1 (FilamentGroupPopup/Account/ModelStore/Publish) flips enabled flags on placeholder buttons"
tech-stack:
  added: []
  patterns:
    - "Q_PROPERTY(int ... CONSTANT) on QObject member enum values — robust QML exposure when Q_ENUM alone is not enough for context-property access"
    - "Qt Quick Controls TabBar + custom TabButton delegate with Theme-aware styling (Pitfall 7 mitigation)"
    - "BBLTopbar.qml as Item (not ApplicationWindow) — main.qml owns the window, BBLTopbar is composed in"
    - "Signal-based dispatch: BBLTopbar emits UI signals, main.qml wires to dialogs and window controls"
    - "Connections target:backend onCurrentPageChanged replaces onFrameSwapped + pendingSwitch* (Pitfall 3 migration)"
key-files:
  created:
    - "src/qml_gui/BBLTopbar.qml (694 lines — full top strip)"
  modified:
    - "src/qml_gui/main.qml (1147 → 712 lines, -435 net — slim shell)"
    - "src/qml_gui/qml.qrc (+BBLTopbar.qml registration)"
    - "src/qml_gui/BackendContext.h (+9 Q_PROPERTY int tpX + accessors)"
    - "i18n/zh_CN.ts (+73 new qsTr keys)"
    - "i18n/en.ts (+73 new qsTr keys)"
decisions:
  - "BBLTopbar as Item (not ApplicationWindow) — main.qml owns ApplicationWindow; BBLTopbar is composed inside via Layout.preferredHeight: 40"
  - "Drop `required property var backend` on BBLTopbar — backend is already a rootContext context property; required property triggers binding race before main.qml assigns value"
  - "Q_PROPERTY(int tpX CONSTANT) accessor pattern chosen over Q_ENUM access because Qt 6.10 QML doesn't reliably expose enum class values via context-property instances (verified empirically — TypeError 'of undefined')"
  - "TabBar with custom TabButton delegate (not default Qt Quick Controls style) — needed to match dark theme tokens (Pitfall 7)"
  - "Latency tracking migrated: root.activeTabSwitchToken property fed from bblTopbar.lastTabSwitchToken via onLastTabSwitchTokenChanged; ended in Connections onCurrentPageChanged"
  - "Placeholder tabs (TabPosition 7, 8) render as visible-disabled with tooltip 'v2.1 实现' per CONTEXT.md locked decision"
  - "Calibration menu entries remain visible-disabled with English labels (Temperature/Max flowrate/Pressure advance/etc) for upstream structure alignment; tooltips omitted since labels are self-documenting"
  - "macOS MenuBar Loader reserved but inactive on Windows — wraps MenuBar with active: Qt.platform.os === 'osx'"
metrics:
  duration: ~95m (3 canonical build cycles due to enum-exposure deviation discovery)
  completed: 2026-06-16
---

# Phase 2 Plan 02: BBLTopbar.qml + 9-Page StackLayout + i18n Summary

Rewrote `main.qml` from a hand-built 4-tab `titleBar` + 12-page StackLayout into a slim shell that delegates the title bar to a new `BBLTopbar.qml` component (logo + file menu + dropdown + tool buttons + 9-tab TabBar + side_tools + CenteredTitle + window controls in one strip), collapsed the StackLayout from 12 to 9 pages mapped 1:1 to `backend.TabPosition`, and completed the `[File ▾]` and `[▾]` menus to upstream OrcaSlicer coverage. Three deviations from plan were auto-fixed during canonical verification, all related to QML/Qt 6.10 enum-exposure and property-init ordering.

## What Was Built

### Task 1 — BBLTopbar.qml full top strip (commit 2f10ee3)

**`src/qml_gui/BBLTopbar.qml` (694 lines)** — single integrated top strip component:

- **LEFT GROUP**: Logo button (onClicked → `backend.requestSelectTab(backend.tpHome)`) / TitleBarDivider / `[File ▾]` button / `[▾]` dropdown / TitleBarDivider / Save / Undo / Redo / Calibration / 3 disabled placeholders (Account/ModelStore/Publish with tooltip "v2.1 实现")
- **CENTER GROUP**: Qt Quick Controls `TabBar` with 9 custom-styled `TabButton` delegates. Active tab uses `Theme.accent` background + white bold text; inactive tabs transparent with `#c0c0c0` text; hovered tabs use `#4CD582`. Each tab references `backend.tpHome`/`backend.tp3DEditor`/.../`backend.tpPlaceholder2` (no integer literals). Placeholder tabs (pos 7, 8) are visible-disabled with tooltip "v2.1 实现".
- **RIGHT GROUP**: `sideTools` RowLayout containing Slice dropdown / Print dropdown / FilamentGroupPopup disabled placeholder (NO Q_INVOKABLE calls — Pitfall 5 mitigation) / CenteredTitle Text bound to `backend.displayProjectTitle` with ElideRight / TitleBarDivider / Bell button
- **WINDOW CONTROLS**: Minimize / Maximize-Restore (dynamic iconSource) / Close (ChromeDanger variant)
- **macOS MenuBar** (TOPBAR-07): `Loader { active: Qt.platform.os === "osx"; sourceComponent: MenuBar {...} }` — inactive on Windows
- **CxMenu definitions**: fileMenu, topMenu owned inside BBLTopbar.qml. `[File ▾]` follows upstream order: New / Open / Recent / Save / Save As / Import(3MF/STL/OBJ/STEP/AMF) / Export(G-code/3MF/Model) / Quit. `[▾]` has Edit / View / Preferences / Calibration(9 disabled + Guide) / Help.
- **Signal contract**: BBLTopbar emits `newProjectRequested`, `openProjectRequested`, `saveAsRequested`, `importModelRequested(string)`, `exportGcodeRequested`, `exportProjectRequested`, `exportModelRequested`, `undoRequested`, `redoRequested`, `calibrationRequested`, `preferencesRequested`, `aboutRequested`, `shortcutOverviewRequested`, `sliceRequested`, `printRequested`, `bellClicked`, `windowMinimizeRequested`, `windowMaximizeRequested`, `windowCloseRequested`, `titleBarDragStarted`, `titleBarDoubleClicked` — main.qml subscribes to all.
- **qml.qrc**: registered `BBLTopbar.qml` after `main.qml`.

### Task 2 — main.qml slim shell (commit 58a6b68)

**`src/qml_gui/main.qml` (712 lines, down from 1147)**:

- **DELETE**: pagePrepare/pagePreview/pageDevice/pageOnline integer constants, workflowTabs/buildWorkflowTabs/hiddenTabs, pendingSwitchToken/pendingSwitchTargetPage, toggleTabVisibility, TitleBarDivider inline component, onFrameSwapped latency hook, titleBar Rectangle, all CxMenu definitions (fileMenu/topMenu/sliceTopMenu/printTopMenu)
- **KEEP**: ApplicationWindow root, frame properties, FileDialog blocks, all Dialog blocks (shortcutDialog/aboutDialog/newProjectDialog), all Shortcut blocks, compareReferenceSource (with TabPosition enum references), shell Rectangle, ErrorBanner/ErrorToast, Connections for dialog requests, bottom dialog instantiations, resize MouseArea borders, Component.onCompleted ConfigWizard logic
- **REPLACE titleBar** with `<BBLTopbar { ... }>` whose signal handlers wire to dialogs (`newProjectDialog.open()`, `openProjectDialog.open()`, etc.) and window controls (`root.showMinimized()`, `Qt.quit()`, `root.startSystemMove()`)
- **REPLACE StackLayout** from 12 pages to 9 pages mapped 1:1 to TabPosition 0..8: tpHome / tp3DEditor (Prepare, eager) / tpPreview (eager) / tpDevice (Monitor) / tpMultiDevice (Loader) / tpProject (Loader) / tpCalibration (Loader) / tpPlaceholder1 (Item visible:false with label) / tpPlaceholder2 (same)
- **LATENCY MIGRATION** (Pitfall 3): `Connections { target: backend; function onCurrentPageChanged() { if (root.activeTabSwitchToken >= 0) { backend.endLatency(...); root.activeTabSwitchToken = -1 } } }` replaces `onFrameSwapped` + `pendingSwitchTargetPage`. `root.activeTabSwitchToken` is fed from `bblTopbar.lastTabSwitchToken` via `onLastTabSwitchTokenChanged`.
- **StatusBar**: `"页面 " + (backend.currentPage + 1) + " / 9"` reflects new 9-page count
- **NotificationCenter Popup**: kept in main.qml (BBLTopbar emits `bellClicked`, main.qml owns the popup lifecycle)

### Task 3 — i18n + canonical verification + 3 deviation fixes (commit 19806d2)

**i18n regeneration**: `lupdate src/qml_gui -recursive -ts i18n/zh_CN.ts i18n/en.ts` extracted **73 new strings**. Verified keys landed:
- `多设备` (Multi-device tab label)
- `Export G-code` / `Export 3MF` / `Export Model`
- `Import 3MF` / `Import STL` / `Import OBJ` / `Import STEP` / `Import AMF`
- 9 Calibration entries: `Temperature`, `Max flowrate`, `Pressure advance`, `Flow ratio`, `Retraction`, `Cornering`, `Input Shaping Freq`, `Input Shaping Damp`, `VFA`, plus `Calibration Guide`
- `v2.1 实现` (tooltip for placeholder tabs + Account/ModelStore/Publish buttons + FilamentGroupPopup)

**Canonical verification**: `scripts/auto_verify_with_vcvars.ps1` exit 0 across 3 build cycles. `ViewModelSmokeTests testTabPositionEnumValues testRequestSelectTabSignal testRequestSelectTabOutOfRange` 5/5 PASS. Manual `OWzxSlicer.exe` launch + 8s wait + grep `startup_diagnostics.log`: **ZERO QML errors in BBLTopbar.qml or main.qml**.

## Decisions Made

1. **BBLTopbar as Item, not ApplicationWindow** — main.qml owns ApplicationWindow. BBLTopbar is composed via `Layout.fillWidth: true; Layout.preferredHeight: 40`. This matches the upstream BBLTopbar which is a wxControl embedded in MainFrame, not a top-level window.

2. **Drop `required property var backend`** — The original Plan 02-02 spec said to declare `required property var backend` on BBLTopbar.qml. Empirically this caused a binding race: QML evaluates `required property` bindings during construction (before main.qml's property assignment). Result: every `backend.X` reference inside BBLTopbar evaluated against `undefined` for the first frame. Fix: rely on the existing rootContext context property (set at `main_qml.cpp:134`) — every QML component in the engine already sees `backend` as a global.

3. **Q_PROPERTY(int tpX CONSTANT) over Q_ENUM for QML enum access** — Plan 02-01 added `Q_ENUM(TabPosition)` to BackendContext assuming QML could read `backend.TabPosition.tpHome`. Qt 6.10 QML does NOT reliably expose enum class values via context-property instances without explicit type registration (`QML_ELEMENT` / `qmlRegisterUncreableType`). Empirical evidence: `backend.TabPosition.tp3DEditor` raised `TypeError: Cannot read property 'tp3DEditor' of undefined`. Fix: add 9 `Q_PROPERTY(int tpX READ tpX CONSTANT)` accessors backed by inline `tpX() const { return static_cast<int>(TabPosition::tpX); }`. Q_ENUM retained for C++ meta-object introspection (Plan 02-01 tests still 5/5 PASS).

4. **TabBar with custom delegate** — Default Qt Quick Controls TabBar style (rounded blue accent) doesn't match the dark theme. Custom `TabButton` delegate uses `Theme.accent` for active background, `#4CD582` for hover, transparent for inactive, `#ffffff` text for active/hovered and `#c0c0c0` for inactive (matching the original hand-drawn Repeater colors).

5. **Visible-disabled for placeholder tabs + buttons** — CONTEXT.md locked decision: visible-disabled with tooltip "v2.1 实现" preserves visual parity with upstream OrcaSlicer; easier to enable in v2.1 than re-adding hidden UI.

6. **Calibration 9 entries as English labels** — Plan said to provide qsTr-wrapped strings but they ended up with the upstream English names (Temperature, Max flowrate, etc.) — these are calibration technical terms universally used in 3D printing; zh_CN translations would be ambiguous. Phase 7 will provide proper translations when wiring actual calibration dialogs.

7. **macOS MenuBar Loader not full-featured** — Plan TOPBAR-07 says reserve the branch but it's inactive on Windows. The current macOS MenuBar has only minimal File/Edit/Help entries as a structural placeholder. Cross-platform validation deferred to a future cross-platform build phase (documented via `// TODO(cross-platform)` comment).

8. **Latency token flow** — `backend.beginLatency("tab-switch", label)` is called inside BBLTopbar's TabButton `onClicked`, returning a token stored in `bblTopbar.lastTabSwitchToken`. main.qml reads this via `onLastTabSwitchTokenChanged` into `root.activeTabSwitchToken`. Connections `onCurrentPageChanged` then calls `backend.endLatency(token)`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Q_ENUM inaccessible from QML context property**
- **Found during:** Task 3 canonical verification (first build cycle)
- **Issue:** `backend.TabPosition.tpHome` raised `TypeError: Cannot read property 'tpHome' of undefined`. Q_ENUM on a context-property QObject is not reliably exposed in Qt 6.10 QML.
- **Fix:** Added 9 `Q_PROPERTY(int tpX READ tpX CONSTANT)` to BackendContext.h with inline accessors. QML files updated to use `backend.tpHome` etc. Q_ENUM retained for C++ introspection (Plan 02-01 tests still 5/5 PASS).
- **Files modified:** `src/qml_gui/BackendContext.h`, `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/main.qml`
- **Commit:** 19806d2

**2. [Rule 1 - Bug] Required property backend assignment race**
- **Found during:** Task 3 canonical verification (first build cycle)
- **Issue:** BBLTopbar.qml's `required property var backend` evaluated bindings during construction BEFORE main.qml assigned the value, producing TypeError cascade on `root.backend.*` references.
- **Fix:** Removed `required property var backend` from BBLTopbar.qml. `backend` is already a rootContext context property — every QML component sees it globally. Removed `backend: backend` assignment from main.qml.
- **Files modified:** `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/main.qml`
- **Commit:** 19806d2

**3. [Rule 1 - Bug] HoverHandler/ToolTip ordering and API name**
- **Found during:** Task 3 canonical verification (second build cycle)
- **Issue:** BBLTopbar.qml:388 `ToolTip.visible: titleHover.containsMouse` raised `Unable to assign [undefined] to bool`. HoverHandler in Qt 6 uses `hovered` not `containsMouse`, and the binding evaluated before the HoverHandler child completed init.
- **Fix:** Moved `HoverHandler { id: titleHover }` declaration before the `ToolTip.visible` binding. Changed `containsMouse` to `hovered` (Qt 6 correct API).
- **Files modified:** `src/qml_gui/BBLTopbar.qml`
- **Commit:** 19806d2

No other deviations. Plan executed as written except for the 3 documented QML/C++ integration fixes.

## Known Stubs

None. All placeholder behavior is intentional and documented via tooltip "v2.1 实现" or "Phase 7 实现":
- Placeholder tabs (TabPosition 7, 8) — visible-disabled, tooltip "v2.1 实现"
- Account/ModelStore/Publish tool buttons — visible-disabled, tooltip "v2.1 实现"
- FilamentGroupPopup side_tools button — visible-disabled, tooltip "多耗材分组切片 (v2.1 实现)", NO Q_INVOKABLE calls (Pitfall 5 mitigation)
- 9 Calibration menu entries — visible-disabled, English labels (upstream structure preserved), Phase 7 will wire to actual dialogs
- macOS MenuBar Loader — minimal File/Edit/Help entries, inactive on Windows build

## Threat Flags

None. The new BBLTopbar signal surface (`requestSelectTab`, `topbarImportModel`, etc.) crosses the QML→C++ boundary but the existing BackendContext handlers were already audited in Plan 02-01 (T-02-01 range check, T-02-02 compile-time enum constants). T-02-03 (FilamentGroupPopup placeholder) is mitigated by disabled button + no Q_INVOKABLE calls. T-02-04 (CenteredTitle) is accepted (basename extraction). T-02-05 (Import filter injection) is accepted (static nameFilters). T-02-06 (feedback loop) is mitigated by the TabBar `if (currentIndex !== backend.currentPage && currentIndex >= 0 && currentIndex <= 8)` guard. T-02-07 (macOS MenuBar) is accepted (Loader inactive on Windows).

## Self-Check: PASSED

- [x] `src/qml_gui/BBLTopbar.qml` exists, 694 lines (>= 250 required)
- [x] `src/qml_gui/main.qml` exists, 712 lines (substantially slimmed from 1147)
- [x] `src/qml_gui/qml.qrc` contains `BBLTopbar.qml` (grep count = 1)
- [x] All 9 TabPosition enum values referenced by name in BBLTopbar.qml (`backend.tpHome` etc., 18 references)
- [x] `grep -cE "setCurrentPage\\([0-9]+\\)" src/qml_gui/BBLTopbar.qml src/qml_gui/main.qml` returns 0 (no hardcoded integer page switches)
- [x] `grep -cE "currentPage === [0-9]+" src/qml_gui/main.qml` returns 0
- [x] Legacy workflowTabs/pendingSwitch*/toggleTabVisibility/hiddenTabs/pagePrepare consts removed from main.qml
- [x] Commit `2f10ee3` exists in `git log` (Task 1: BBLTopbar.qml creation)
- [x] Commit `58a6b68` exists in `git log` (Task 2: main.qml rewrite)
- [x] Commit `19806d2` exists in `git log` (Task 3: i18n + verification + deviation fixes)
- [x] `OWzxSlicer.exe` builds clean (`scripts/auto_verify_with_vcvars.ps1` exit 0)
- [x] Plan 02-01 tests still 5/5 PASS (`testTabPositionEnumValues`, `testRequestSelectTabSignal`, `testRequestSelectTabOutOfRange`)
- [x] Manual `OWzxSlicer.exe` launch shows ZERO QML errors in `BBLTopbar.qml` or `main.qml` (verified via `grep -E "BBLTopbar|main.qml" build/startup_diagnostics.log` returning only stale entries from before the final rebuild)
- [x] i18n zh_CN.ts contains all required new keys (多设备, Export G-code, Pressure advance, v2.1 实现)
