---
phase: 02
status: issues_found
total_findings: 11
critical: 1
warning: 7
info: 3
reviewed_at: 2026-06-16
depth: standard
files_reviewed: 5
files_reviewed_list:
  - src/qml_gui/BackendContext.h
  - src/qml_gui/BackendContext.cpp
  - src/qml_gui/BBLTopbar.qml
  - src/qml_gui/main.qml
  - src/qml_gui/qml.qrc
  - tests/ViewModelSmokeTests.cpp
---

# Phase 02: Code Review Report

**Reviewed:** 2026-06-16
**Depth:** standard
**Files Reviewed:** 6
**Status:** issues_found

## Summary

Phase 02 adds a 9-page Notebook + BBLTopbar architecture on top of Phase 1's `BackendContext`. The C++ foundation (`TabPosition` Q_ENUM + `requestSelectTab` + 9 `Q_PROPERTY(int tpX CONSTANT)` accessors) is sound and well-tested. However, the QML layer in `BBLTopbar.qml` and `main.qml` contains several real defects: a duplicate-signal-dispatch feedback issue (each tab click emits `tabSelectRequested` twice), a runtime-language-switch regression for tab labels, the Account placeholder button reusing the Logo's `printer.svg` icon (visual confusion), and a non-functional `macOS MenuBar` Loader that is anchored to `fill: parent` even while inactive (over-rendered). The C++ layer has a latent maintainability trap (`requestSelectTab` hardcodes the upper bound `8`, which will silently truncate when a future phase adds a 10th tab). One `topbarImportModel` / `topbarOpenProject` mismatch in the Recent submenu passes a raw `QString` from QML to a `QString` parameter, but the C++ side does `QUrl(filePath)` parsing — for `QStringList` entries already containing Windows backslash paths this works, but it is fragile and undocumented.

The adversarial stance surfaced what was NOT verified by the implementation summary: the SUMMARY claims the macOS branch is "inactive on Windows" but the Loader with `anchors.fill: parent` still participates in the layout graph even when `active: false`, and the `Loader` carries a `MenuBar` sourceComponent that allocates child objects lazily — there is no test verifying this branch does nothing problematic on Windows.

---

## Critical Issues

### CR-01: `requestSelectTab` hardcoded upper bound `8` will silently break when adding a 10th tab

**File:** `src/qml_gui/BackendContext.cpp:205`
**Category:** bug / maintainability
**Issue:**

```cpp
if (position < 0 || position > 8) {
  qWarning("[Backend] requestSelectTab: invalid position %d", position);
  return;
}
```

The upper bound is the literal `8` — the integer value of `TabPosition::tpPlaceholder2`. The same literal `8` is also hard-coded in `BBLTopbar.qml:267`:

```qml
if (currentIndex !== backend.currentPage
        && currentIndex >= 0 && currentIndex <= 8) {
```

If a future phase adds `tpPlaceholder3 = 9` (a very plausible extension given the upstream `toDebugTool=8` is "reserved for future use"), the C++ side will silently drop the click with a `qWarning` and the QML side will silently swallow it without warning. Both checks should reference the enum's last value, not a magic number. The Plan 02-01 SUMMARY explicitly acknowledges this trade-off ("Acceptable trade-off vs. magic-number risk of accepting position = 99") but the real risk is the opposite — a future addition being silently dropped — and that risk is concrete, not hypothetical.

**Recommendation:**

```cpp
// BackendContext.cpp
void BackendContext::requestSelectTab(int position)
{
  constexpr int kLastTab = static_cast<int>(TabPosition::tpPlaceholder2);
  if (position < 0 || position > kLastTab) {
    qWarning("[Backend] requestSelectTab: invalid position %d (valid 0..%d)",
             position, kLastTab);
    return;
  }
  ...
}
```

```qml
// BBLTopbar.qml
&& currentIndex >= 0 && currentIndex <= backend.tpPlaceholder2
```

A unit test (`testRequestSelectTabBoundIsEnum`) asserting `BackendContext::tpPlaceholder2() == 8` would also catch enum-grows-but-bound-doesn't regressions at compile time.

---

## Warnings

### WR-01: Tab click emits `tabSelectRequested` twice per user click

**File:** `src/qml_gui/BBLTopbar.qml:265-269` (TabBar `onCurrentIndexChanged`) **and** `322-327` (TabButton `onClicked`)
**Category:** bug
**Issue:**

When the user clicks an enabled tab, two code paths fire:

1. The TabButton delegate's `onClicked` (line 322) calls `backend.requestSelectTab(modelData.pos)` → emits `tabSelectRequested(pos)` once.
2. Because `currentIndex` of `TabBar` is bound to `backend.currentPage` and the click also updates `navTabBar.currentIndex`, `onCurrentIndexChanged` (line 265) fires, and since `currentIndex !== backend.currentPage` is momentarily true (before setCurrentPage completes), it calls `backend.requestSelectTab(currentIndex)` → emits `tabSelectRequested(pos)` again.

`setCurrentPage`'s dedupe prevents the duplicate `currentPageChanged`, but `tabSelectRequested` has NO dedupe — every future consumer (Phase 3 Plater sharing, latency tracking, analytics) will receive the event twice. The implementation's own Pitfall-2 mitigation (the `if (currentIndex !== backend.currentPage)` guard) does NOT prevent this — it only suppresses the THIRD and later calls.

**Recommendation:** Pick a single dispatch site. Either:
- Remove `onCurrentIndexChanged` entirely (since `onClicked` already drives the request) — but then keyboard-arrow navigation through TabBar would not work; or
- Remove `onClicked` and rely solely on `onCurrentIndexChanged` — but then `beginLatency("tab-switch", label)` cannot be triggered because `onCurrentIndexChanged` doesn't have a label/context to pass to `beginLatency`.

The cleanest fix is to gate `onClicked` to skip when the click is already reflected in `currentIndex`:

```qml
delegate: TabButton {
    ...
    onClicked: {
        if (navTabBar.currentIndex === modelData.pos) return  // TabBar already moved
        if (backend.currentPage === modelData.pos) return
        root.lastTabSwitchToken = backend.beginLatency("tab-switch", modelData.label)
        // requestSelectTab will be invoked by onCurrentIndexChanged
    }
}
```

Or, more robustly, install a `QSignalSpy`-style guard in C++: dedupe `tabSelectRequested` if the same `position` was emitted within the last 50 ms.

### WR-02: Tab labels do not retranslate at runtime when language changes

**File:** `src/qml_gui/BBLTopbar.qml:276-286`
**Category:** bug / i18n
**Issue:**

```qml
Repeater {
    model: [
        { label: qsTr("首页"),     pos: backend.tpHome },
        ...
    ]
```

The `model` is a plain JavaScript array literal evaluated once at component initialization. The `qsTr("首页")` etc. calls are resolved to a `QString` at load time and stored as a plain JS string in the array element. When `BackendContext::applyLanguage` calls `engine->retranslate()` (the documented mechanism in `CLAUDE.md`), QML's retranslate only re-evaluates bindings that reference `qsTr()` directly in property bindings — NOT `qsTr()` calls inside JavaScript array literals whose result was already captured.

The result: clicking "切换为英文" via the Settings page will update every other `qsTr()` string in the app (file menu items, dialog labels) but the 9 tab labels will stay in Chinese. This is a regression vs. the previous `main.qml` workflow tabs (which used a Repeater over a property `model: workflowTabs` whose elements were bound directly to TabButton.text).

**Recommendation:** Either bind each tab's `text` directly via `qsTr()` (so retranslate picks it up):

```qml
Repeater {
    model: 9
    delegate: TabButton {
        text: {
            switch (index) {
                case 0: return qsTr("首页")
                case 1: return qsTr("准备")
                ...
            }
        }
        ...
    }
}
```

Or hold the labels in a `BackendContext` Q_PROPERTY (`Q_PROPERTY(QStringList tabLabels ...)` populated from `tr()`), so retranslate re-emits a list change.

### WR-03: Account placeholder button reuses Logo's `printer.svg` icon

**File:** `src/qml_gui/BBLTopbar.qml:229`
**Category:** quality / UX
**Issue:**

```qml
// Account 占位按钮
CxIconButton {
    ...
    iconSource: "qrc:/qml/assets/icons/printer.svg"  // ← same icon as Logo (line 118)
    toolTipText: qsTr("v2.1 实现")
    enabled: false
}
```

The Logo (line 118) and the Account placeholder button both use `printer.svg`. Visually, two identical icons appear in the top-left and tool-strip, which will confuse users into thinking they are the same control. The upstream OrcaSlicer `BBLTopbar` uses distinct icons (`user.svg` for account). There is no `user.svg` asset in `qml.qrc` — this is why the implementer reused `printer.svg` — but the correct fix is to add a `user.svg` asset, not to copy the logo.

**Recommendation:** Either add a `user.svg` (or `account.svg`) to `src/qml_gui/assets/icons/` and register in `qml.qrc`, or use `layout-grid.svg`-style generic placeholder icon for all three placeholder buttons until proper assets exist.

### WR-04: `topbarOpenProject(modelData)` passes a bare `QString` to a `Q_INVOKABLE` that runs it through `QUrl`

**File:** `src/qml_gui/BBLTopbar.qml:490` → `src/qml_gui/BackendContext.cpp:300-304`
**Category:** bug / robustness
**Issue:**

`ProjectViewModel::recentProjects` is a `QStringList` of plain local-path strings (verified in `ProjectViewModel.h:14`). The `Instantiator` delegate passes `modelData` (a `QString` like `C:/foo/bar.3mf`) to `backend.topbarOpenProject(modelData)`.

In `BackendContext::topbarOpenProject`:

```cpp
const QUrl url(filePath);
const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
```

For a plain Windows path `C:/foo/bar.3mf`, `QUrl("C:/foo/bar.3mf")` parses this as a *relative* URL with scheme interpretation ambiguity — `QUrl::isLocalFile()` returns `false` for non-`file://` URLs, so the fallback `filePath` is used, which works. **BUT** if a path ever contains a `#` or `?` (Windows allows these on NTFS), `QUrl` will mangle it (treating `#` as fragment separator) and the fallback will be the mangled path. This is a latent bug.

**Recommendation:** Either:
- Document in `topbarOpenProject`'s docstring that it accepts both QUrl-string and local-path-string inputs (already partially true); or
- Stop using `QUrl` parsing for the Recent-submenu path (which is guaranteed to be a local path) — add a separate `Q_INVOKABLE bool openRecentProject(const QString &localPath)` that bypasses `QUrl`.

### WR-05: `Loader` for macOS MenuBar uses `anchors.fill: parent` even when `active: false`

**File:** `src/qml_gui/BBLTopbar.qml:58-81`
**Category:** quality / minor perf
**Issue:**

```qml
Loader {
    id: macOSMenuBarLoader
    active: Qt.platform.os === "osx"
    anchors.fill: parent                  // ← anchors on Loader itself
    sourceComponent: MenuBar { ... }
}
```

On Windows, `active: false` means the Loader has no `item`, but the Loader itself is still anchored to fill the root Item. While this doesn't render anything visible (inactive Loader has no item), it adds the Loader to the layout graph and forces QML to evaluate `anchors.fill` binding — wasteful, and more importantly it implies the Loader is "in the visual tree" which can confuse future maintainers reading the code. Additionally, `MenuBar` is not a visual Item (it's `QtQuick.Controls` MenuBar attached to native platform menu) — anchoring a Loader whose source is a MenuBar is conceptually wrong.

**Recommendation:** Drop `anchors.fill: parent` from the Loader:

```qml
Loader {
    id: macOSMenuBarLoader
    active: Qt.platform.os === "osx"
    sourceComponent: MenuBar { ... }
}
```

A `MenuBar` has no visual extent; anchoring is meaningless.

### WR-06: `displayProjectTitle` returns `tr("未命名")` which won't retranslate

**File:** `src/qml_gui/BackendContext.cpp:1169`
**Category:** bug / i18n
**Issue:**

```cpp
QString BackendContext::displayProjectTitle() const
{
  ...
  return tr("未命名");
}
```

This `tr("未命名")` is evaluated lazily (good), but the property `displayProjectTitle` has `NOTIFY displayProjectTitleChanged`. When `applyLanguage()` runs, it emits `languageChanged()` but NOT `displayProjectTitleChanged`. QML's binding to `backend.displayProjectTitle` will not re-fetch the value, so the "未命名" placeholder stays in the old language even after a language switch.

Also note: line 71 of `BackendContext.cpp`:

```cpp
connect(editorViewModel_, &EditorViewModel::stateChanged, this, &BackendContext::displayProjectTitleChanged);
```

This wires `EditorViewModel::stateChanged` → `displayProjectTitleChanged`. That's reasonable for project-name changes, but it does NOT cover language changes.

**Recommendation:** In `applyLanguage()`, also emit `displayProjectTitleChanged()`:

```cpp
emit languageChanged();
emit displayProjectTitleChanged();  // tr("未命名") may have changed language
```

### WR-07: `openSettings()` hardcodes `setCurrentPage(11)` — out of range for new 9-page StackLayout

**File:** `src/qml_gui/BackendContext.cpp:214-217`
**Category:** bug
**Issue:**

```cpp
void BackendContext::openSettings()
{
  setCurrentPage(11);
}
```

The Phase 2 rewrite reduced `StackLayout` from 12 pages to 9 pages (indices 0..8). `setCurrentPage(11)` now points at a non-existent page. While `StackLayout` clamps `currentIndex` silently (so this won't crash), `openSettings()` — if invoked from anywhere — will navigate to a non-existent page, leaving the UI blank.

**Recommendation:** Either delete `openSettings()` if Settings now lives in the [▾] → 偏好设置 dialog (which seems to be the case — see `BBLTopbar.qml:656 preferencesRequested`), or update it to the new index:

```cpp
void BackendContext::openSettings()
{
  // Settings is now accessed via BBLTopbar [▾] → 偏好设置 → preferencesRequested signal
  // No dedicated tab page exists in the 9-page StackLayout
  emit preferencesSettingsRequested();  // or simply remove this method
}
```

Grep to verify callers — if none, delete it.

---

## Info

### IN-01: Latency token flow depends on `onLastTabSwitchTokenChanged` re-firing per click

**File:** `src/qml_gui/main.qml:389-393`, `src/qml_gui/BBLTopbar.qml:325`
**Category:** maintainability
**Issue:**

The latency-token plumbing works like this: TabButton `onClicked` sets `root.lastTabSwitchToken = backend.beginLatency(...)` → `onLastTabSwitchTokenChanged` in main.qml copies to `root.activeTabSwitchToken` → `Connections onCurrentPageChanged` calls `backend.endLatency(root.activeTabSwitchToken)`.

If two consecutive tab clicks return the same integer token (impossible in current implementation because `m_latencyNextToken` is monotonically incremented), the `onLastTabSwitchTokenChanged` would NOT fire. This is robust today, but the contract is fragile — anyone refactoring `beginLatency` to reuse token 0/1 would silently break tab-switch latency tracking with no compile error.

**Recommendation:** Document the assumption in `BackendContext::beginLatency` ("tokens are monotonically increasing; never reuse a value").

### IN-02: `topMenu.popup()` and `fileMenu.popup()` rely on cursor-position default behavior

**File:** `src/qml_gui/BBLTopbar.qml:149, 173`
**Category:** quality
**Issue:**

`fileMenu.popup()` and `topMenu.popup()` are called with no arguments from MouseArea `onClicked`. In QtQuick Controls 6, `Menu.popup()` with no args pops up at `(0, 0)` of the Menu's parent item if it has one, or at the screen origin otherwise. Since `CxMenu` instances here are children of the BBLTopbar root Item, the popup may appear at the top-left corner of the topbar, not under the cursor — inconsistent with the original main.qml behavior.

**Recommendation:** Pass cursor coords explicitly:

```qml
onClicked: fileMenu.popup(fileBtnMouse.mouseX, fileBtnMouse.mouseY)
// or
onClicked: fileMenu.popup(fileBtn, 0, fileBtn.height)
```

Verify visually that the menu appears under the `[File ▾]` button.

### IN-03: `qsTr("占位 Tab (v2.1 实现)")` placeholder label rendered on placeholder pages 7 and 8

**File:** `src/qml_gui/main.qml:497, 511`
**Category:** quality / UX
**Issue:**

Pages 7 and 8 in `StackLayout` are `Item { visible: false }` containing a `Text { text: qsTr("占位 Tab (v2.1 实现)") }`. The `visible: false` means the placeholder label is never shown to the user. The text and Rectangle subtree are dead UI — they consume QML allocation but render nothing. This is intentional (placeholder tabs are `enabled: false` so `currentPage` can never reach 7 or 8 in current code) but the dead-UI subtree is a quality smell.

**Recommendation:** Either delete the placeholder page subtrees entirely (leaving empty `Item {}`), or wire the TabPosition placeholder tabs to a future enable-flag so the dead-UI becomes reachable when v2.1 flips `enabled: true`. As written, this is unreachable code.

---

_Reviewed: 2026-06-16_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
