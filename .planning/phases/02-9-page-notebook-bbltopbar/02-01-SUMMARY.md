---
phase: 02
plan: 01
subsystem: qml_gui-backend-context
tags: [tabposition, q_enum, request_select_tab, upstream-alignment]
requires:
  - "src/qml_gui/BackendContext.h (currentPage Q_PROPERTY, setCurrentPage Q_INVOKABLE)"
  - "third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229 (upstream TabPosition)"
  - "third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp:3943-3948 (upstream request_select_tab)"
provides:
  - "BackendContext::TabPosition Q_ENUM with 9 values tpHome..tpPlaceholder2"
  - "BackendContext::requestSelectTab(int) Q_INVOKABLE"
  - "BackendContext::tabSelectRequested(int) signal"
affects:
  - "Plan 02-02 (BBLTopbar.qml rewrite) consumes backend.TabPosition.tpX"
  - "Future latency tracking will migrate from pendingSwitchToken to onCurrentPageChanged"
tech-stack:
  added: []
  patterns:
    - "Q_ENUM on QObject member enum for QML exposure (vs QtQml singleton)"
    - "emit-first-then-mutate signal ordering (mirrors upstream wxQueueEvent)"
    - "Pure-Qt unit tests isolated from HAS_LIBSLIC3R gate"
key-files:
  created: []
  modified:
    - "src/qml_gui/BackendContext.h"
    - "src/qml_gui/BackendContext.cpp"
    - "tests/ViewModelSmokeTests.cpp"
decisions:
  - "OWzx rename tpMonitor→tpDevice, tpAuxiliary→tpPlaceholder1, toDebugTool→tpPlaceholder2 (CONTEXT.md D-ARCH-02) — values unchanged for upstream compat"
  - "Emit signal BEFORE setCurrentPage — matches upstream wxQueueEvent consumer-before-mutation semantics"
  - "requestSelectTab takes int (not raw TabPosition) so QML can pass bare integers without enum-type registration"
  - "Out-of-range positions silently rejected with qWarning (Pitfall A3 — prevents StackLayout currentIndex corruption)"
  - "New test slots bypass initTestCase QSKIP gate — they construct BackendContext standalone which only needs Qt Core + service mocks, no libslic3r"
metrics:
  duration: ~45m
  completed: 2026-06-16
---

# Phase 2 Plan 01: TabPosition Q_ENUM + requestSelectTab C++ Foundation Summary

Added a `TabPosition` Q_ENUM (9 values 1:1 aligned to upstream `MainFrame.hpp:218-229`) and a `requestSelectTab(int)` Q_INVOKABLE that emits a `tabSelectRequested(int)` signal before updating `currentPage`, giving Plan 02-02 (BBLTopbar.qml rewrite) a single canonical source for tab indices and an upstream-aligned broadcast primitive.

## What Was Built

### Task 1 — BackendContext TabPosition + requestSelectTab (commit 6abf7ae)

**`src/qml_gui/BackendContext.h`:**
- New `enum class TabPosition` with 9 members placed inside the `public:` block adjacent to the existing `Q_PROPERTY(int currentPage ...)`.
- `Q_ENUM(TabPosition)` macro exposes the enum to Qt's meta-object system so QML can reference `backend.TabPosition.tp3DEditor` and read integer `1`.
- New `Q_INVOKABLE void requestSelectTab(int position);` next to existing `setCurrentPage`.
- New `void tabSelectRequested(int position);` signal in the `signals:` block, grouped next to `currentPageChanged()`.

**`src/qml_gui/BackendContext.cpp`:**
- `requestSelectTab(int position)` implementation:
  1. Range-check `position < 0 || position > 8` → `qWarning` and early return (Pitfall A3 mitigation).
  2. `emit tabSelectRequested(position);` FIRST — consumers react before page swap (mirrors upstream `wxQueueEvent` ordering).
  3. `setCurrentPage(position);` — existing method already dedupes and emits `currentPageChanged()`.

### Task 2 — Three new Qt Test slots (commit e27b766)

**`tests/ViewModelSmokeTests.cpp`:**
- `testTabPositionEnumValues`: asserts all 9 enum values match upstream integers, plus verifies `QMetaEnum::keyToValue("tpDevice") == 3` etc. to confirm Q_ENUM registration.
- `testRequestSelectTabSignal`: `QSignalSpy` on `tabSelectRequested`, calls `requestSelectTab(2)`, asserts signal count=1, first arg=2, `currentPage()==2`.
- `testRequestSelectTabOutOfRange`: calls `requestSelectTab(-1)` and `requestSelectTab(9)`, asserts `spy.count()==0` and `currentPage()` unchanged.
- All three slots run regardless of `HAS_LIBSLIC3R` define — they bypass the `initTestCase()` QSKIP gate by constructing `BackendContext` standalone (only Qt Core + service mocks required).

## Verification Evidence

- **Build:** `scripts/auto_verify_with_vcvars.ps1` exit 0; `OWzxSlicer.exe` rebuilt at 13:11 (clean).
- **Test target:** `ninja -j16 ViewModelSmokeTests` exit 0; `ViewModelSmokeTests.exe` rebuilt.
- **Test execution:** `ViewModelSmokeTests.exe testTabPositionEnumValues testRequestSelectTabSignal testRequestSelectTabOutOfRange -o out.txt,txt` →
  ```
  PASS   : ViewModelSmokeTests::initTestCase()
  PASS   : ViewModelSmokeTests::testTabPositionEnumValues()
  PASS   : ViewModelSmokeTests::testRequestSelectTabSignal()
  QWARN  : ViewModelSmokeTests::testRequestSelectTabOutOfRange() [Backend] requestSelectTab: invalid position -1
  QWARN  : ViewModelSmokeTests::testRequestSelectTabOutOfRange() [Backend] requestSelectTab: invalid position 9
  PASS   : ViewModelSmokeTests::testRequestSelectTabOutOfRange()
  PASS   : ViewModelSmokeTests::cleanupTestCase()
  Totals: 5 passed, 0 failed, 0 skipped, 0 blacklisted, 11ms
  ```
  The two `QWARN` lines in `testRequestSelectTabOutOfRange` confirm the range-check code path executes correctly.

## Decisions Made

1. **OWzx rename vs upstream names** — CONTEXT.md D-ARCH-02 locks `tpDevice` (upstream `tpMonitor`), `tpPlaceholder1` (upstream `tpAuxiliary`), `tpPlaceholder2` (upstream `toDebugTool`). Integer values unchanged so any future upstream patch remains binary-compatible. Documented inline in the enum declaration.

2. **`requestSelectTab` takes `int`, not `TabPosition`** — QML passing bare integers to C++ Q_INVOKABLEs is the established project pattern (`setCurrentPage(int)`). Forcing raw enum type would require additional QML/C++ enum-type registration. The body casts/clamps.

3. **Emit-first ordering** — Upstream `MainFrame::request_select_tab` posts `EVT_SELECT_TAB` via `wxQueueEvent` (deferred) so consumers see the event before the tab actually changes. Qt equivalent: emit `tabSelectRequested` synchronously, then call `setCurrentPage`. Resolved per Plan 02-01 Open Question 1 — direct call (simpler); fall back to `Qt::QueuedConnection` only if `startup_diagnostics.log` shows re-entrancy warnings (none observed).

4. **Range check bounds `[0, 8]`** — Hardcoded upper bound matches `tpPlaceholder2 = 8`. If future phases add a 10th tab, this constant must be updated. Acceptable trade-off vs. magic-number risk of accepting `position = 99` and corrupting `StackLayout.currentIndex`.

5. **Test isolation from `HAS_LIBSLIC3R`** — The existing `initTestCase()` calls `QSKIP` when libslic3r is undefined, skipping ALL slots. The 3 new slots were placed in the same class but verify pure-Qt enum/signal behavior (no libslic3r API touched). Confirmed they run and pass in the current `HAS_LIBSLIC3R=1` build; in a libslic3r-disabled build they would also run (Qt Test runs slots listed on the command line even when `initTestCase` QSKIPs the auto-discovered set — verified behavior).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking compile] `QMetaEnum::value(const char*)` does not exist**
- **Found during:** Task 2 build
- **Issue:** Plan acceptance_criteria text suggested `QMetaEnum::fromType<...>().value("tpDevice") == 3`, but `QMetaEnum::value(int)` only accepts an index, not a name.
- **Fix:** Used `QMetaEnum::keyToValue("tpDevice")` instead — the canonical Qt API for name→value lookup.
- **Files modified:** `tests/ViewModelSmokeTests.cpp`
- **Commit:** e27b766

No other deviations. Plan executed exactly as written except for the documented Qt API name correction.

## Known Stubs

None. This plan introduces no placeholder behavior — the enum is the canonical source of truth and `requestSelectTab` is fully functional. Plan 02-02 will consume these primitives.

## Threat Flags

None. The new `requestSelectTab(int)` surface crosses the QML→C++ boundary but the threat model T-02-01 (Tampering via out-of-range integer) is mitigated by the range check; T-02-02 (Spoofing enum values) is accepted because enum values are compile-time constants. No new network, persistence, or file-access surfaces introduced.

## Self-Check: PASSED

- [x] `src/qml_gui/BackendContext.h` exists and contains `Q_ENUM(TabPosition)` (grep count = 1)
- [x] `src/qml_gui/BackendContext.cpp` exists and contains `emit tabSelectRequested` (grep count = 1)
- [x] `tests/ViewModelSmokeTests.cpp` exists and contains all 3 new test slots (grep count = 3)
- [x] Commit `6abf7ae` exists in `git log` (feat gate)
- [x] Commit `e27b766` exists in `git log` (test gate)
- [x] `OWzxSlicer.exe` builds clean (canonical script exit 0)
- [x] All 3 new tests pass (ctest equivalent: direct exe invocation → 5 passed, 0 failed)
