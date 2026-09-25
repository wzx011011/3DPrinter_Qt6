---
phase: 2
slug: 9-page-notebook-bbltopbar
status: draft
nyquist_compliant: true
wave_0_complete: true
created: 2026-06-16
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Qt Test (`QtTest` module) — already wired in `tests/ViewModelSmokeTests.cpp` |
| **Config file** | `CMakeLists.txt` (root) — `enable_testing()` + `add_executable(ViewModelSmokeTests ...)` + `add_test(...)` |
| **Quick run command** | `ctest --test-dir build -C Release -R ViewModelSmokeTests --output-on-failure` (incremental — reuses built binary if unchanged) |
| **Full suite command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (canonical — configures, builds, smoke-tests) |
| **Estimated runtime** | Quick: ~2-5 s | Full: ~5-15 min (rebuild from cold cache) |

---

## Sampling Rate

- **After every task commit:** Run quick command (`ctest -R ViewModelSmokeTests`)
- **After every plan wave:** Run full canonical build via `scripts/auto_verify_with_vcvars.ps1`
- **Before `/gsd:verify-work`:** Full canonical build green AND no new QML warnings in `startup_diagnostics.log`
- **Max feedback latency:** 5 s (quick) / 15 min (full)

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 02-01-01 | 02-01 | 1 | ARCH-02 | — | TabPosition enum exposed as Q_ENUM with 9 values aligned to upstream `MainFrame.hpp:218-229` | unit | `ctest --test-dir build -C Release -R ViewModelSmokeTests -R testTabPositionEnumValues --output-on-failure` | ✅ | ⬜ pending |
| 02-01-01 | 02-01 | 1 | ARCH-03 | — | `requestSelectTab(int)` Q_INVOKABLE emits `tabSelectRequested(int)` signal AND calls `setCurrentPage(int)` synchronously | unit | `ctest --test-dir build -C Release -R ViewModelSmokeTests -R testRequestSelectTabSignal --output-on-failure` | ✅ | ⬜ pending |
| 02-01-02 | 02-01 | 1 | ARCH-02 | — | Smoke build of test binary (BackendContext.h/cpp compile without libslic3r linkage) | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | ARCH-01 | — | `BBLTopbar.qml` integrated component registered in `qml.qrc`, loads without QML errors | smoke | `scripts/quick_run.ps1` (manual start) + inspect `startup_diagnostics.log` for `BBLTopbar` QML warnings | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | ARCH-04 | — | `side_tools` RowLayout region present in BBLTopbar.qml with Slice/Print dropdowns + FilamentGroupPopup placeholder (visible-disabled) | source-assertion | `grep -c 'side_tools' src/qml_gui/BBLTopbar.qml` returns >= 1 | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-01 | — | `BBLTopbar.qml` exists as single integrated top bar with title + menu + tools | source-assertion | `grep -c 'ApplicationWindow' src/qml_gui/main.qml` returns 0 (header replaced) AND `grep -E 'BBLTopbar\s*\{' src/qml_gui/main.qml` matches | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-02 | — | `[File ▾]` menu contains New/Open/Recent/Save/Save As/Import(3MF/STL/OBJ/STEP/AMF)/Export(G-code/3MF/Model)/Quit | source-assertion | `grep -E 'qsTr\("Import 3MF|Import STL|Import OBJ|Import STEP|Import AMF|Export G-code|Export 3MF|Export Model|Save As|Recent' src/qml_gui/BBLTopbar.qml` returns >= 9 lines | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-03 | — | `[▾]` menu contains Edit/View/Preferences/Calibration(9 entries)/Help | source-assertion | `grep -E 'qsTr\("Edit|View|Preferences|Calibration|Help' src/qml_gui/BBLTopbar.qml` returns >= 5 lines AND grep for 9 calibration entries | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-04 | — | Tool buttons: Save/Undo/Redo/Calibration enabled; Account/ModelStore/Publish disabled with `v2.1 实现` tooltip | source-assertion | `grep -c 'enabled: false' src/qml_gui/BBLTopbar.qml` returns >= 3 (3 placeholder buttons) | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-05 | — | CenteredTitle Text horizontally centered, bound to `backend.displayProjectTitle`, ElideRight | source-assertion | `grep -E 'displayProjectTitle|Layout.alignment: Qt\.AlignHCenter|elide: Text\.ElideRight' src/qml_gui/BBLTopbar.qml` matches | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-06 | — | Min/Max/Close window controls as CxIconButton (Chrome/ChromeDanger variants) | source-assertion | `grep -E 'Qt\.WindowMinimizeButtonRequested|window\.showMaximized|Qt\.Quit|CxIconButton' src/qml_gui/BBLTopbar.qml` matches | ✅ | ⬜ pending |
| 02-02-01 | 02-02 | 2 | TOPBAR-07 | — | macOS MenuBar Loader conditionally active only when `Qt.platform.os === "osx"` | source-assertion | `grep -E 'Qt\.platform\.os === "osx"' src/qml_gui/BBLTopbar.qml` matches | ✅ | ⬜ pending |
| 02-02-02 | 02-02 | 2 | ARCH-01 | — | `main.qml` no longer references `workflowTabs` array; uses TabBar with 9 tabs bound to `backend.TabPosition` enum values | source-assertion | `grep -c 'workflowTabs' src/qml_gui/main.qml` returns 0 AND `grep -c 'TabBar' src/qml_gui/main.qml` returns >= 1 | ✅ | ⬜ pending |
| 02-02-02 | 02-02 | 2 | ARCH-03 | — | TabBar.onCurrentIndexChanged calls `backend.requestSelectTab(currentIndex)` — no hardcoded integer page indices | source-assertion | `grep -E 'backend\.requestSelectTab|pendingSwitchToken|pendingSwitchTargetPage' src/qml_gui/main.qml` — `requestSelectTab` matches; `pendingSwitchToken` count must be 0 (replaced) | ✅ | ⬜ pending |
| 02-02-03 | 02-02 | 2 | TOPBAR-01..07 | — | Full canonical build green (compile + smoke test) + 0 new QML warnings in startup_diagnostics.log | build + smoke | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ | ⬜ pending |
| 02-02-03 | 02-02 | 2 | TOPBAR-01..07 | — | zh_CN.ts + en.ts contain new menu translation keys (`menu.file.new`, `menu.file.open`, `menu.edit.undo`, etc.) | source-assertion | `grep -c 'menu\.file\.\|menu\.edit\.\|menu\.view\.\|menu\.help\.' i18n/zh_CN.ts` returns >= 10 | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [x] `tests/ViewModelSmokeTests.cpp` — already exists with QSignalSpy harness; extend with `testTabPositionEnumValues` and `testRequestSelectTabSignal` test cases (covered by Plan 02-01 Task 2)
- [x] `scripts/auto_verify_with_vcvars.ps1` — already exists from Phase 1
- [x] `CMakeLists.txt` — `enable_testing()` already wired

*Existing infrastructure covers all phase requirements. No new Wave 0 work needed.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Tab switch latency tracking logs still appear in console (`[Latency]` prefix) after `onCurrentPageChanged` migration | ARCH-03 | Latency timing cannot be asserted deterministically in unit tests | 1. Run `scripts/quick_run.ps1`. 2. Click tabs in BBLTopbar. 3. Verify `[Latency] tabswitch token=...` logs appear in console / startup_diagnostics.log. |
| BBLTopbar visual parity with upstream BBLTopbar (logo, menu layout, tool button spacing) | TOPBAR-01 | Visual parity requires human judgment — pixel-perfect upstream match is subjective | 1. Run `scripts/quick_run.ps1`. 2. Compare against upstream OrcaSlicer screenshot. 3. Confirm BBLTopbar height, button arrangement, and menu icon placement match upstream. |
| macOS MenuBar branch is non-activating on Windows build | TOPBAR-07 | macOS build path cannot be tested on Windows host | 1. Verify `Qt.platform.os === "osx"` guard is present. 2. Verify Windows runtime does not show MenuBar Loader (system menu bar is Windows-style). |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify (all tasks have a source-assertion or build/smoke verify)
- [x] Wave 0 covers all MISSING references (no MISSING refs — infrastructure exists)
- [x] No watch-mode flags
- [x] Feedback latency < 15 min
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending (will be approved after first execution wave passes)
