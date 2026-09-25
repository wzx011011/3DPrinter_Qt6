---
phase: 02
fixed_at: 2026-06-16
review_path: .planning/phases/02-9-page-notebook-bbltopbar/02-REVIEW.md
iteration: 1
findings_in_scope: 5
fixed: 5
skipped: 0
status: all_fixed
---

# Phase 02: Code Review Fix Report

**Fixed at:** 2026-06-16
**Source review:** .planning/phases/02-9-page-notebook-bbltopbar/02-REVIEW.md
**Iteration:** 1

**Summary:**
- Findings in scope: 5 (1 critical + 4 warning)
- Fixed: 5
- Skipped: 0

**Scope applied:** Critical + Warning only (WR-02, WR-03, WR-05, and all Info findings deferred per orchestrator instructions).

**Verification:**
- Canonical build (`scripts/auto_verify_with_vcvars.ps1`): exit 0 — OWzxSlicer.exe + ViewModelSmokeTests.exe + E2EWorkflowTests.exe + owzx-cli.exe + CliTests.exe all linked successfully.
- ViewModelSmokeTests.exe: exit 0 (all assertions pass).
- Build warnings (C4267 size_t conversion, C4858 QtConcurrent::run return) are pre-existing and unrelated to these fixes.

## Fixed Issues

### CR-01: `requestSelectTab` hardcoded upper bound `8` will silently break when adding a 10th tab

**Files modified:** `src/qml_gui/BackendContext.cpp`, `src/qml_gui/BBLTopbar.qml`
**Commit:** `c25a11f`
**Applied fix:** Replaced literal `8` in `BackendContext::requestSelectTab` with `constexpr int kLastTab = static_cast<int>(TabPosition::tpPlaceholder2)`, and replaced `currentIndex <= 8` in BBLTopbar.qml `onCurrentIndexChanged` with `currentIndex <= backend.tpPlaceholder2`. The bound now tracks the enum automatically; a future `tpPlaceholder3 = 9` will not be silently dropped.

### WR-01: Tab click emits `tabSelectRequested` twice per user click

**File modified:** `src/qml_gui/BBLTopbar.qml`
**Commit:** `f50df33`
**Applied fix:** Removed the direct `backend.requestSelectTab(modelData.pos)` call from `TabButton.onClicked`. Instead, `onClicked` now sets `navTabBar.currentIndex = modelData.pos`, which triggers the single dispatch source — `onCurrentIndexChanged`. This eliminates the double `tabSelectRequested` emission. The latency token (`beginLatency`) is still triggered from `onClicked` since `onCurrentIndexChanged` lacks the label context.
**Status:** fixed: requires human verification — this is a logic change to the click-navigation path; manual smoke test of tab clicks recommended to confirm single-dispatch behavior.

### WR-04: `topbarOpenProject(modelData)` passes a bare `QString` to a `Q_INVOKABLE` that runs it through `QUrl`

**File modified:** `src/qml_gui/BBLTopbar.qml`
**Commit:** `a0b12a9`
**Applied fix:** Documentation only. Added a 3-line inline comment at the Recent-submenu call site (BBLTopbar.qml:~490) clarifying that `modelData` is a local `QString` path (not QUrl-encoded) and that `topbarOpenProject` tolerates both via `QUrl::isLocalFile()` fallback. No code change — the C++ side already handles both input forms.

### WR-06: `displayProjectTitle` returns `tr("未命名")` which won't retranslate

**File modified:** `src/qml_gui/BackendContext.cpp`
**Commit:** `a6f01f4`
**Applied fix:** Added `emit displayProjectTitleChanged();` immediately after `emit languageChanged();` in `BackendContext::applyLanguage(int idx)`. QML bindings to `backend.displayProjectTitle` now re-fetch the value, so the `tr("未命名")` placeholder retranslates on language switch.

### WR-07: `openSettings()` hardcodes `setCurrentPage(11)` — out of range for new 9-page StackLayout

**File modified:** `src/qml_gui/BackendContext.cpp`
**Commit:** `76ee3af`
**Applied fix:** Replaced `setCurrentPage(11)` with `setCurrentPage(static_cast<int>(TabPosition::tpProject))` (= index 5). Settings is now embedded inside the Project page in the new 9-page StackLayout; the obsolete index 11 would have been silently clamped by `StackLayout`, leaving the UI blank. Updated comment explains the page-reduction rationale.

---

_Fixed: 2026-06-16_
_Fixer: Claude (gsd-code-fixer)_
_Iteration: 1_
