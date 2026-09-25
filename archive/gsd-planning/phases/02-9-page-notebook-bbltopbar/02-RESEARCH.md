# Phase 2: 9-Page Notebook + BBLTopbar - Research

**Researched:** 2026-06-16
**Domain:** Qt6/QML top-level framework rewrite (TabBar + StackLayout + BBLTopbar menu system)
**Confidence:** HIGH (upstream source code read directly; current baseline inspected)

## Summary

Phase 2 rewrites the `main.qml` top-level framework to align with upstream OrcaSlicer's 9-page Notebook + BBLTopbar architecture. The current code already has a 12-page StackLayout (currentIndex = `backend.currentPage`) and a hand-built `titleBar` with Logo/File-menu/Dropdown/Save/Undo/Redo buttons + 4 workflow tabs + window controls. Phase 2 must: (1) replace the 4-tab `workflowTabs` array with a 9-tab `TabBar` model matching upstream `MainFrame::TabPosition` enum, (2) broadcast tab switches through `request_select_tab(TabPosition)` semantics, (3) extend the existing partial `[File ▾]` and `[▾]` menus to full upstream coverage, (4) add a `side_tools` region (Slice/Print dropdowns + FilamentGroupPopup placeholder) on the right of the tab strip.

The upstream source truth is concentrated in three files: `MainFrame.hpp:218-229` (the `TabPosition` enum), `MainFrame.cpp:495-498` (the `EVT_SELECT_TAB` event binder that calls `m_tabpanel->SetSelection(pos)`), and `BBLTopbar.cpp:29-41` (the `CUSTOM_ID` enum for topbar tool IDs). The Qt6 baseline already implements ~70% of BBLTopbar visuals — the main work is *completion and reorganization*, not greenfield.

**Primary recommendation:** Implement a single `BBLTopbar.qml` component that owns the entire top strip (file menu, dropdown, tool buttons, centered title, tab strip, side_tools, window controls), backed by a new `TabPosition` enum exposed from `BackendContext` and a `requestSelectTab(int)` signal. Reuse existing `CxMenu`/`CxMenuItem`/`CxIconButton` infrastructure unchanged.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **TabBar implementation:** Qt Quick Controls `TabBar` + `StackLayout` (NOT custom-drawn). Stay consistent with existing Cx* control style.
- **9 Tab order (LOCKED):** `tpHome=0, tp3DEditor=1, tpPreview=2, tpDevice=3, tpMultiDevice=4, tpProject=5, tpCalibration=6, tpPlaceholder1=7, tpPlaceholder2=8`
- **TabPosition constants (ARCH-02):** Align values with upstream `Notebook.cpp` / `MainFrame.hpp:218-229` `TabPosition` enum.
- **Tab switch event (ARCH-03):** Implement `request_select_tab(TabPosition)` signal broadcast through `BackendContext`; replaces current local `pendingSwitchToken` / `pendingSwitchTargetPage` state for cross-component coordination.
- **side_tools region (ARCH-04):** TabBar right side hosts a `side_tools` container with Slice dropdown + Print dropdown + FilamentGroupPopup placeholder (v2.0 = placeholder only).
- **BBLTopbar integration (LOCKED):** Title bar + menu + tool buttons in one strip at top of `ApplicationWindow`, with `flags: Qt.FramelessWindowHint`.
- **[File ▾] menu (LOCKED):** New / Open / Recent (submenu) / Save / Save As / Import (submenu: 3MF/STL/OBJ/STEP/AMF) / Export (submenu: G-code/3MF/Model) / Quit.
- **[▾] second-level menu (LOCKED):** Edit (Undo/Redo/Cut/Copy/Paste/Delete/Select All/Invert Selection) / View (Show-Hide Gizmo/Reset View/Show Layers/Hide Layers) / Preferences (open PreferencesDialog) / Calibration (submenu placeholder, wired to Phase 7) / Help (Documentation/Check for Updates/About/Shortcut Overview).
- **Tool buttons (LOCKED):** Save / Undo / Redo / Calibration (quick button); conditional buttons Account / ModelStore / Publish are placeholder-only in v2.0.
- **CenteredTitle:** Project name centered horizontally; default "OWzx Slicer" when no project loaded.
- **Window controls:** Min / Max / Close using CxIconButton custom-drawn style. Linux-style skipped in v2.0 (Windows-only build).
- **Platform branching (TOPBAR-07):** `#ifdef Q_OS_MACOS` → system MenuBar; Win/Linux → custom BBLTopbar. Currently Windows-only build, MACOS branch reserved but inactive.

### Claude's Discretion

- Exact pixel heights for BBLTopbar (upstream uses `FromDIP(30)` for toolbar height — verify DIP equivalent in Qt6 — recommend `36px` logical on Qt to match upstream visual ratio).
- Internal QML file decomposition (single `BBLTopbar.qml` vs split into `BBLTopbarMenuBar.qml` + `BBLTopbarTabBar.qml` + `BBLTopbarWindowControls.qml`).
- Whether `TabPosition` enum is exposed as `Q_ENUM` on `BackendContext` (RECOMMENDED — enables QML access via `backend.TabPosition.tp3DEditor`) or as a standalone `QtQml` singleton.

### Deferred Ideas (OUT OF SCOPE)

- FilamentGroupPopup full multi-filament group slicing logic → v2.1
- Account / ModelStore / Publish tool buttons full impl → v2.1 (depends on QtWebEngine / Network)
- macOS system menubar full testing → after cross-platform build support
- Linux custom-drawn window control style → after cross-platform build support
- Tab drag-and-drop reordering → upstream does not support, will not implement
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| ARCH-01 | main.qml 12-page StackLayout → 9-page TabBar + StackLayout | Upstream `MainFrame.hpp:218-229` TabPosition enum; Qt Quick Controls `TabBar` + `StackLayout` (currentIndex binding) |
| ARCH-02 | TabPosition constants match upstream | `MainFrame::TabPosition { tpHome=0, tp3DEditor=1, tpPreview=2, tpMonitor=3, tpMultiDevice=4, tpProject=5, tpCalibration=6, tpAuxiliary=7, toDebugTool=8 }` |
| ARCH-03 | `request_select_tab(TabPosition)` event broadcast | Upstream `MainFrame.cpp:3943-3948` `request_select_tab` posts `EVT_SELECT_TAB` wxCommandEvent; Qt equivalent: signal on BackendContext |
| ARCH-04 | side_tools region (Slice/Print dropdowns + FilamentGroupPopup placeholder) | Upstream `Notebook.cpp:45-55` shows side_tools as `wxBoxSizer` injected via constructor |
| TOPBAR-01 | BBLTopbar title+menu+toolbuttons combined | `BBLTopbar.cpp:29-41` CUSTOM_ID enum; current `main.qml:583-854` already has skeleton |
| TOPBAR-02 | [File ▾] full menu | Upstream `MainFrame.cpp:448-452` wxID_NEW/OPEN/SAVE/SAVEAS bindings; current `main.qml:115-157` has partial — needs Import/Export submenus |
| TOPBAR-03 | [▾] second-level menu with Edit/View/Preferences/Calibration/Help | Current `main.qml:160-282` already implements — extend Calibration submenu to 9 entries placeholder |
| TOPBAR-04 | Tool buttons Save/Undo/Redo/Calibration + conditional Account/ModelStore/Publish | Upstream `BBLTopbar.cpp:300-364`; current `main.qml:680-713` has Save/Undo/Redo — add Calibration button + 3 conditional placeholders |
| TOPBAR-05 | CenteredTitle | Upstream `BBLTopbar.cpp:43-94` CenteredTitle class with ellipsizing; current `main.qml:762-774` already has projectTitleLabel |
| TOPBAR-06 | Window controls Min/Max/Close | Current `main.qml:819-844` already has all three; Linux style deferred |
| TOPBAR-07 | macOS MenuBar / Win+Linux BBLTopbar conditional compile | Upstream `MainFrame.cpp:526-530` `#ifndef __APPLE__` adds topbar; Qt equivalent `#ifdef Q_OS_MACOS` (reserved branch only) |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| TabPosition enum definition | core/services (BackendContext Q_ENUM) | — | Single source of truth shared between QML and C++; prevents desync between index values |
| Tab strip rendering | QML (TabBar + custom delegate) | — | Pure presentation; no business logic |
| Tab switch broadcast | core/services (BackendContext signal) | QML (Connections) | Mirrors upstream EVT_SELECT_TAB; BackendContext is the composition root for cross-component signals |
| [File ▾] / [▾] menus | QML (CxMenu + CxMenuItem) | core/services (Q_INVOKABLE handlers) | Menu structure is UI; behavior dispatches to backend via existing `topbar*` invokables |
| CenteredTitle text source | core/services (ProjectServiceMock via BackendContext.displayProjectTitle) | QML (read-only binding) | Business state lives in service; QML displays |
| Window controls (Min/Max/Close) | QML (CxIconButton) | ApplicationWindow API | Pure presentation; calls `root.showMinimized()` etc. |
| Undo/Redo state | core/services (UndoRedoManager) | — | Already wired; BBLTopbar just binds `enabled` to viewmodel `hasSelection`/can-undo |
| Slice/Print side_tools actions | QML (CxMenu popup) | core/services (EditorViewModel) | Already wired via `sliceTopMenu`/`printTopMenu` |
| Side_tools region layout | QML (RowLayout inside TabBar parent) | — | Pure layout composition |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick Controls | 6.10 | TabBar, StackLayout, Menu, MenuSeparator | Already in vcpkg.json / CMake; project standard |
| Qt QML | 6.10 | ApplicationWindow, signals, components | Project standard |
| Qt LinguistTools | 6.10 | qsTr() translation keys for new menu items | Project standard, i18n/i18n/*.ts |
| CxMenu / CxMenuItem | internal | Custom menu infrastructure | Already exists at `src/qml_gui/controls/CxMenu.qml`; reuse |
| CxIconButton | internal | Tool button base | Already exists; Chrome + ChromeDanger styles |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Theme singleton | internal (qmldir) | Color tokens (topbarHover/Pressed/Text, accent) | All BBLTopbar colors must come from Theme — already established |
| HoverHandler / TapHandler | Qt Quick (built-in) | Button hover/tap detection | Existing pattern in current main.qml |
| Instantiator | QtQuick (built-in) | Dynamic Recent submenu items | Existing pattern at `main.qml:136-147` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Qt Quick Controls `TabBar` | Custom Repeater (current approach) | Current Repeater works but skips built-in keyboard nav, focus handling, accessibility. CONTEXT.md locks `TabBar` choice. |
| `BackendContext` Q_ENUM for TabPosition | QtQml singleton `TabPosition {}` | Q_ENUM is preferable — keeps enum near composition root, single source of truth; C++ side already passes ints via `setCurrentPage(int)` |

**Installation:** No new packages. Phase 2 is pure QML + small C++ additions on `BackendContext`.

## Package Legitimacy Audit

Phase 2 installs **zero** external packages — all work uses existing Qt 6.10 modules and in-repo QML components. Audit skipped (no candidates).

## Architecture Patterns

### System Architecture Diagram

```
ApplicationWindow (frameless)
   └─ shell Rectangle (rounded border, clip)
      └─ ColumnLayout
         ├─ BBLTopbar.qml  (top strip, height 36-40px)
         │   ├─ LEFT:  Logo | Divider | [File ▾] | [▾] | Divider | Save | Undo | Redo | Calibration | placeholder[Account/ModelStore/Publish]
         │   ├─ CENTER: TabBar (9 tabs) ──┬── broadcast requestSelectTab(TabPosition)
         │   │                              └── binds currentIndex ↔ backend.currentPage
         │   ├─ RIGHT: side_tools (Slice ▾ | Print ▾ | FilamentGroupPopup placeholder)
         │   ├─ CENTERED TITLE: projectTitleLabel (binds backend.displayProjectTitle)
         │   ├─ Bell (notification center) + Divider
         │   └─ Window controls: Minimize | Maximize/Restore | Close
         │
         ├─ ErrorBanner (severity=1)  [existing — keep]
         ├─ StackLayout (currentIndex = backend.currentPage)
         │   └─ 9 pages mapped to TabPosition enum (was 12)
         ├─ StatusBar  [existing — keep]
         └─ ErrorToast (severity=0, z-stack overlay)  [existing — keep]
```

Data flow for tab switch:
1. User clicks tab in TabBar
2. TabBar delegate `onClicked` → `backend.requestSelectTab(TabPosition.tpX)` (new Q_INVOKABLE)
3. BackendContext emits `requestSelectTab` signal (queued or direct — see Pitfalls)
4. BackendContext updates `currentPage` property (existing)
5. StackLayout `currentIndex` binding triggers page swap
6. All other components listening to `onCurrentPageChanged` react (latency tracking, undo/redo enable state, etc.)

### Recommended Project Structure

```
src/qml_gui/
├── main.qml                     [modify — slim down, delegate to BBLTopbar.qml]
├── BBLTopbar.qml                [new — top strip component]
├── controls/
│   ├── CxMenu.qml               [reuse]
│   ├── CxMenuItem.qml           [reuse]
│   └── CxIconButton.qml         [reuse]
└── (dialogs, pages, etc. unchanged)

src/qml_gui/BackendContext.h     [modify — add TabPosition Q_ENUM + requestSelectTab signal/slot]
src/qml_gui/BackendContext.cpp   [modify — implement requestSelectTab]
```

### Pattern 1: Q_ENUM exposure to QML

```cpp
// BackendContext.h
class BackendContext : public QObject {
  Q_OBJECT
  Q_ENUM(TabPosition)
public:
  enum class TabPosition {
    tpHome = 0,
    tp3DEditor = 1,
    tpPreview = 2,
    tpDevice = 3,        // upstream calls tpMonitor — rename for OWzx
    tpMultiDevice = 4,
    tpProject = 5,
    tpCalibration = 6,
    tpPlaceholder1 = 7,  // upstream tpAuxiliary — reserved
    tpPlaceholder2 = 8,  // upstream toDebugTool — reserved
  };
  Q_INVOKABLE void requestSelectTab(int position);  // takes TabPosition as int
signals:
  void tabSelectRequested(int position);
};
```

In QML: `backend.TabPosition.tp3DEditor` returns `1`.

**Source:** Upstream `MainFrame.hpp:218-229` for enum values; Qt documentation for Q_ENUM (CITED: doc.qt.io/qt-6/qobject.html#Q_ENUM — verified via Qt 6 project pattern, established across existing BackendContext usage).

### Pattern 2: TabBar + StackLayout binding

```qml
TabBar {
    id: navTabBar
    currentIndex: backend.currentPage
    onCurrentIndexChanged: {
        if (currentIndex !== backend.currentPage)
            backend.requestSelectTab(currentIndex)
    }
    Repeater {
        model: [
            { label: qsTr("Home"),        pos: backend.TabPosition.tpHome },
            { label: qsTr("Prepare"),     pos: backend.TabPosition.tp3DEditor },
            { label: qsTr("Preview"),     pos: backend.TabPosition.tpPreview },
            { label: qsTr("Device"),      pos: backend.TabPosition.tpDevice },
            { label: qsTr("Multi-device"),pos: backend.TabPosition.tpMultiDevice },
            { label: qsTr("Project"),     pos: backend.TabPosition.tpProject },
            { label: qsTr("Calibration"), pos: backend.TabPosition.tpCalibration },
            { label: qsTr("Placeholder"), pos: backend.TabPosition.tpPlaceholder1, enabled: false, tooltip: qsTr("v2.1 实现") },
            { label: qsTr("Placeholder"), pos: backend.TabPosition.tpPlaceholder2, enabled: false, tooltip: qsTr("v2.1 实现") },
        ]
        delegate: TabButton {
            text: modelData.label
            enabled: modelData.enabled !== false
            ToolTip.text: modelData.tooltip || ""
            onClicked: backend.requestSelectTab(modelData.pos)
        }
    }
}
```

**Caveat:** `TabBar.currentIndex` two-way binding with `backend.currentPage` can create feedback loops — prefer explicit signal-based dispatch (Pattern 1).

### Anti-Patterns to Avoid

- **Hardcoded index literals in QML** (`backend.setCurrentPage(1)`): use `backend.TabPosition.tp3DEditor` for self-documentation and refactor safety. Current code has literals scattered (e.g., `main.qml:256 setCurrentPage(8)`, `main.qml:263-265 setCurrentPage(5)`) — Phase 2 should clean these up.
- **Inline business logic in tab delegate** (e.g., conditional page eligibility checks): move to BackendContext as `Q_INVOKABLE bool canSelectTab(TabPosition)`.
- **Duplicating menu items** in both `[File ▾]` and shortcut handlers: each action should have a single Q_INVOKABLE entry point.
- **Mixing the old `pendingSwitchToken`/`pendingSwitchTargetPage` with the new broadcast pattern**: ARCH-03 replaces these — `onFrameSwapped` latency hook should connect to `onCurrentPageChanged` instead.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tab strip with keyboard nav + focus + a11y | Custom Repeater with manual key handling | Qt Quick Controls `TabBar` + `TabButton` | Built-in keyboard navigation, screen-reader roles, RTL support — all already battle-tested |
| Dynamic Recent submenu items | Manual model rebuilding on every menu open | `Instantiator` with `model:` binding (existing pattern at main.qml:136) | Already works; just keep |
| Ellipsized centered title | Manual text clipping in QML JS | QML `Text.elide: Text.ElideRight` (current approach) + `horizontalAlignment: Qt.AlignHCenter` | Standard QML primitive |
| Frameless window dragging | Manual mouse tracking across whole title bar | `startSystemMove()` (current pattern) | OS-native, supports snap layouts on Win11 |
| Window edge resize | Manual 8-border MouseArea (current approach) | `startSystemResize(edges)` (current pattern) | Already correct |
| Menu popup positioning | Manual x/y math | `CxMenu.popup()` (current) — popup auto-positions under parent | Already correct |

**Key insight:** This phase is mostly *completion and reorganization* of an existing skeleton. The infrastructure (CxMenu, CxIconButton, HoverHandler, startSystemMove/Resize, Instantiator) all exists. New work is: 9-tab data model, enum exposure, complete menu structure, side_tools region.

## Common Pitfalls

### Pitfall 1: TabPosition value desync between C++ and QML
**What goes wrong:** QML uses literal `1` while C++ enum says `tp3DEditor = 1` — then a refactor changes one side and silently breaks tab switching.
**Why it happens:** Current code mixes both styles (`pagePrepare: 1` constant in main.qml vs `backend.currentPage === 1` checks scattered throughout).
**How to avoid:** Expose `TabPosition` as `Q_ENUM(TabPosition)` on BackendContext and reference exclusively as `backend.TabPosition.tpX` in QML. Keep one canonical integer source.
**Warning signs:** Searching for `setCurrentPage(N)` literal calls in QML — any matches outside TabBar itself are violations.

### Pitfall 2: requestSelectTab feedback loop
**What goes wrong:** TabBar binds `currentIndex: backend.currentPage` AND emits `requestSelectTab(currentIndex)` on `onCurrentIndexChanged` → infinite signal loop or skipped updates.
**Why it happens:** Two-way binding semantics between a `QtProperty` and a QML property are subtle.
**How to avoid:** Use a guard (`if (currentIndex !== backend.currentPage)`) at every emit site, OR push the change to backend first then let the binding echo back. Prefer the latter — backend is the single source of truth.
**Warning signs:** Console spam "currentPage changed" or unbounded CPU during tab click.

### Pitfall 3: Frame swap latency hook breaks
**What goes wrong:** Current `onFrameSwapped` hook (main.qml:72-78) ends latency tracking when `backend.currentPage === pendingSwitchTargetPage`. If ARCH-03 removes `pendingSwitchTargetPage` without re-routing latency tracking, the [Latency] logging breaks silently.
**Why it happens:** Latency instrumentation and page-switch logic are tightly coupled in current code.
**How to avoid:** Keep latency token generation at the click site (`backend.beginLatency("tab-switch", label)`) and end it on `backend.currentPage` change (via `Connections onCurrentPageChanged`). Drop the `pendingSwitch*` properties entirely.
**Warning signs:** Missing `[Latency]` log entries during tab switches in `startup_diagnostics.log`.

### Pitfall 4: Menu order doesn't match upstream `[File ▾]`
**What goes wrong:** Implementing Import/Export as flat items in the wrong order confuses users coming from upstream OrcaSlicer.
**Why it happens:** CONTEXT.md locks the order but it's easy to deviate during implementation.
**How to avoid:** Follow exact upstream order from `MainFrame.cpp:448-452` and CONTEXT.md lock: New / Open / Recent / Save / Save As / Import(3MF/STL/OBJ/STEP/AMF) / Export(G-code/3MF/Model) / Quit.
**Warning signs:** User-facing translation review notes.

### Pitfall 5: FilamentGroupPopup placeholder leaks business logic
**What goes wrong:** v2.0 only needs a placeholder, but a developer adds partial logic that breaks Phase 5+ when actual filament groups are introduced.
**Why it happens:** Easy to add "just a tiny bit of behavior."
**How to avoid:** Placeholder must be purely visual: a disabled button with tooltip "v2.1 实现" and a `// TODO(v2.1)` comment. No state, no Q_INVOKABLE calls.
**Warning signs:** Any new Q_INVOKABLE on BackendContext referencing "filamentGroup" or "multiFilamentSlice".

### Pitfall 6: StackLayout currentIndex off-by-one after page count change
**What goes wrong:** Current code uses 12 pages (indices 0-11) with Page 7 = Device, Page 9 = Online. Phase 2 reduces to 9 pages (indices 0-8). Any leftover reference to `pageDevice=7` (old) vs `tpDevice=3` (new) silently shows the wrong page.
**Why it happens:** Multiple page-index constants across QML files.
**How to avoid:** Grep for every `setCurrentPage(N)` and `currentPage === N` after migration and verify against new enum. Run `scripts/auto_verify_with_vcvars.ps1` smoke test.
**Warning signs:** Clicking "Device" tab opens wrong page.

### Pitfall 7: TabBar styling diverges from current hand-drawn tabs
**What goes wrong:** Qt Quick Controls default TabBar style (rounded blue accent) doesn't match existing dark theme tokens.
**Why it happens:** Default Qt Quick Controls style ≠ project Theme.
**How to avoid:** Provide custom `TabButton` delegate that uses `Theme.topbarHover` / `Theme.accent` / `Theme.topbarText` tokens (same colors as current hand-drawn tabs at main.qml:730-743).
**Warning signs:** Visual regression in side-by-side screenshot.

## Code Examples

### requestSelectTab implementation (C++)

```cpp
// Source pattern: MainFrame.cpp:3943-3948 (upstream request_select_tab posts EVT_SELECT_TAB)
// BackendContext.cpp
void BackendContext::requestSelectTab(int position)
{
    if (position < 0 || position > 8) {
        qWarning("[Backend] requestSelectTab: invalid position %d", position);
        return;
    }
    // Emit signal first so listeners can react before page change
    emit tabSelectRequested(position);
    // Update currentPage (existing property with NOTIFY)
    setCurrentPage(position);
}
```

### Latency-aware tab switch (QML)

```qml
// Replaces current onFrameSwapped + pendingSwitch* pattern
TabButton {
    text: modelData.label
    onClicked: {
        if (backend.currentPage === modelData.pos) return
        var token = backend.beginLatency("tab-switch", modelData.label)
        backend.requestSelectTab(modelData.pos)
        // Token ends on backend.currentPage change via Connections
    }
}

Connections {
    target: backend
    function onCurrentPageChanged() {
        // End any in-flight latency token here
        // (BackendContext may track the pending token internally for cleaner code)
    }
}
```

### side_tools region (Slice/Print dropdowns)

```qml
// Source pattern: Notebook.cpp:45-55 (side_tools injected into ButtonsListCtrl)
RowLayout {
    Layout.alignment: Qt.AlignVCenter
    spacing: 4

    // Slice dropdown (existing sliceTopMenu)
    CxIconButton {
        cxStyle: CxIconButton.Style.Chrome
        iconSource: "qrc:/qml/assets/icons/slice.svg"
        toolTipText: qsTr("切片")
        onClicked: sliceTopMenu.popup()
    }

    // Print dropdown (existing printTopMenu)
    CxIconButton {
        cxStyle: CxIconButton.Style.Chrome
        iconSource: "qrc:/qml/assets/icons/printer.svg"
        toolTipText: qsTr("打印")
        onClicked: printTopMenu.popup()
    }

    // FilamentGroupPopup placeholder (v2.0 = visual only)
    CxIconButton {
        cxStyle: CxIconButton.Style.Chrome
        iconSource: "qrc:/qml/assets/icons/filament.svg"
        toolTipText: qsTr("多耗材分组切片 (v2.1 实现)")
        enabled: false
        opacity: 0.4
        // TODO(v2.1): implement FilamentGroupPopup
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Custom Repeater for tab strip | Qt Quick Controls `TabBar` + `TabButton` | Phase 2 (this) | Better keyboard nav, a11y, focus handling; need custom delegate to match theme |
| Hardcoded page indices (1, 2, 7, 9) in QML | `Q_ENUM(TabPosition)` on BackendContext | Phase 2 (this) | Refactor-safe; single source of truth |
| `pendingSwitchToken`/`pendingSwitchTargetPage` local state | `backend.currentPage` change broadcast | Phase 2 (this) | Simpler latency tracking; aligns with upstream EVT_SELECT_TAB |
| 12-page StackLayout | 9-page StackLayout | Phase 2 (this) | Page index remapping; matches upstream TabPosition |

**Deprecated/outdated:**
- `workflowTabs` array builder in `main.qml:50-58` (replaced by 9-tab TabBar model)
- `hiddenTabs` toggling via `toggleTabVisibility` (deferred — upstream does not support runtime tab hide/show)

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `BackendContext` is the right home for `TabPosition` Q_ENUM (vs separate singleton) | Architecture Patterns Pattern 1 | Low — if singleton preferred, refactor is mechanical; behavior identical |
| A2 | BBLTopbar height should be ~36-40px logical (upstream uses `FromDIP(30)`) | Claude's Discretion | Low — visual tuning only |
| A3 | `requestSelectTab` should both emit signal AND call `setCurrentPage` directly (no deferred event loop like upstream `wxQueueEvent`) | Code Examples | Medium — if direct call causes re-entrancy issues, switch to queued signal (`QMetaObject::invokeMethod(... Qt::QueuedConnection)`) |
| A4 | Existing `CxMenu` supports submenus via nested `CxMenu` (current code at main.qml:133-154 already nests — assumed still works in Qt 6.10) | TOPBAR-02 (Import/Export submenus) | Low — already in use; if broken, fallback is flat list |
| A5 | TabButton styling via custom delegate is sufficient to match current dark-theme tabs (vs needing a full custom `TabBar` style) | Pitfall 7 | Low — Theme tokens already defined |

**All [VERIFIED] / [CITED] claims:** TabPosition enum values `[VERIFIED: third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229]`, request_select_tab `[VERIFIED: MainFrame.cpp:3943-3948]`, EVT_SELECT_TAB binding `[VERIFIED: MainFrame.cpp:495-498]`, BBLTopbar CUSTOM_ID enum `[VERIFIED: BBLTopbar.cpp:29-41]`, CenteredTitle ellipsizing `[VERIFIED: BBLTopbar.cpp:43-94]`, side_tools injection pattern `[VERIFIED: Notebook.cpp:45-55]`, current main.qml baseline `[VERIFIED: src/qml_gui/main.qml]`.

## Open Questions (RESOLVED)

1. **Should `requestSelectTab` defer the page change to next event loop iteration?**
   - What we know: Upstream uses `wxQueueEvent` (deferred). Current code does direct `setCurrentPage`.
   - What's unclear: Whether Qt6 QML StackLayout handles synchronous page swap cleanly during signal dispatch (risk of UI tear).
   - Recommendation: Start with direct call (simpler), watch startup_diagnostics.log for warnings, fall back to `QueuedConnection` if issues.
   - **RESOLVED (Plan 02-01 Task 1):** Use direct call — `emit tabSelectRequested(position)` followed by `setCurrentPage(position)` in the same slot. Fall back to `QMetaObject::invokeMethod(... Qt::QueuedConnection)` only if `startup_diagnostics.log` shows re-entrancy warnings during execution verification.

2. **Should the Calibration submenu in [▾] wire to Phase 7 dialog IDs now (placeholders) or wait for Phase 7?**
   - What we know: CONTEXT.md says "Phase 2 仅占位".
   - What's unclear: Whether the 9 calibration entries should be visible-but-disabled, or hidden.
   - Recommendation: Visible but disabled with tooltip "Phase 7" — preserves menu structure alignment without promising functionality.
   - **RESOLVED (Plan 02-02 Task 1):** Visible-but-disabled with tooltip "Phase 7 实现" (visibleDisabled + tooltip). 9 calibration entries preserved in menu structure, all `enabled: false`. No dialog wiring in Phase 2 — Phase 7 will enable them.

3. **Are Account/ModelStore/Publish tool buttons rendered (disabled) or hidden in v2.0?**
   - What we know: CONTEXT.md says "v2.0 仅占位".
   - What's unclear: "占位" can mean visible-disabled or hidden.
   - Recommendation: Visible but disabled — preserves visual parity with upstream; easier to enable in v2.1.
   - **RESOLVED (Plan 02-02 Task 1):** Visible-but-disabled per locked recommendation. `enabled: false` + tooltip "v2.1 实现". Renders alongside enabled Save/Undo/Redo/Calibration buttons.

## Environment Availability

Phase 2 has no new external dependencies. All required tooling is already verified by Phase 1 build:

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Qt 6.10 (Quick, Controls, Layouts) | All QML work | ✓ | 6.10 | — |
| Qt LinguistTools | i18n for new menu strings | ✓ | 6.10 | — |
| Ninja + MSVC | Build | ✓ | (per Phase 1) | — |
| `scripts/auto_verify_with_vcvars.ps1` | Verification | ✓ | (Phase 1) | — |

**Missing dependencies with no fallback:** None.
**Missing dependencies with fallback:** N/A.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Qt Test (`QtTest`), CTest integration |
| Config file | root `CMakeLists.txt` (`include(CTest)`) |
| Quick run command | `cmake --build build --config Release --target ViewModelSmokeTests && ctest --test-dir build -R ViewModel -V` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| ARCH-01 | 9 tabs visible in TabBar; currentIndex follows backend.currentPage | manual + smoke | Launch app, click each tab, verify page swap | N/A (UI smoke) |
| ARCH-02 | TabPosition enum values match upstream (0-8) | unit | `tests/ViewModelSmokeTests.cpp` — add `testTabPositionEnumValues` | ❌ Wave 0 |
| ARCH-03 | requestSelectTab updates currentPage and emits tabSelectRequested | unit | `tests/ViewModelSmokeTests.cpp` — add `testRequestSelectTabSignal` using QSignalSpy | ❌ Wave 0 |
| ARCH-04 | side_tools region visible with 3 buttons | manual | Visual smoke | N/A |
| TOPBAR-01..07 | Menu items present; correct order; enabled states | manual | Click through menus, verify dispatch | N/A |

### Sampling Rate

- **Per task commit:** `cmake --build build --config Release --target ViewModelSmokeTests && ctest --test-dir build -R ViewModel -V`
- **Per wave merge:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Phase gate:** Full build + smoke test green before `/gsd:verify-work`

### Wave 0 Gaps

- [ ] `tests/ViewModelSmokeTests.cpp` — add `testTabPositionEnumValues` covering ARCH-02 (verify `BackendContext::TabPosition::tpHome == 0` ... `tpPlaceholder2 == 8`)
- [ ] `tests/ViewModelSmokeTests.cpp` — add `testRequestSelectTabSignal` covering ARCH-03 (QSignalSpy on `tabSelectRequested`, call `requestSelectTab(2)`, assert signal emitted with arg=2 AND `currentPage == 2`)
- [ ] Existing smoke test infrastructure already covers app launch — extend to assert 9 tabs in `startup_diagnostics.log` if feasible

## Security Domain

Phase 2 introduces no new external input surfaces, network calls, or persistence changes. The menu actions dispatch to existing Q_INVOKABLE handlers that were already audited in Phase 1.

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes (file paths from FileDialog) | Existing `topbarOpenProject/topbarImportModel` already sanitize paths; no new surface |
| V6 Cryptography | no | — |

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malicious file path in Recent submenu | Tampering | ProjectServiceMock already validates paths before opening; Phase 2 only reads the list |
| Menu item ID injection | Tampering | N/A — menu structure is static QML |

## Sources

### Primary (HIGH confidence)
- `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229` — TabPosition enum definition (read directly)
- `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp:495-498` — EVT_SELECT_TAB binder (read directly)
- `third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp:3869-3948` — select_tab / request_select_tab impl (read directly)
- `third_party/OrcaSlicer/src/slic3r/GUI/Notebook.cpp:17-61, 121-191` — ButtonsListCtrl / side_tools injection (read directly)
- `third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp:29-41, 43-94, 285-395` — CUSTOM_ID enum, CenteredTitle, tool setup (read directly)
- `src/qml_gui/main.qml:1-1147` — Current Qt6 baseline (read directly)
- `src/qml_gui/BackendContext.h:150, 204-208` — Existing setCurrentPage + topbar* Q_INVOKABLEs (grep verified)

### Secondary (MEDIUM confidence)
- Qt 6 Q_ENUM pattern — well-established project convention, consistent with existing BackendContext Q_PROPERTY/Q_INVOKABLE usage

### Tertiary (LOW confidence)
- None — all claims were verified by direct source inspection

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — Qt 6.10 is the locked runtime; all components already in use
- Architecture: HIGH — upstream MainFrame/Notebook/BBLTopbar source code read directly; current baseline inspected
- Pitfalls: HIGH — derived from code reading of both upstream and current code, not hypothetical

**Research date:** 2026-06-16
**Valid until:** 2026-07-16 (30 days — upstream source is locked to OrcaSlicer v7.0.1; current Qt6 baseline changes only via this project's own commits)
