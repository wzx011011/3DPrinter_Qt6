---
phase: 51-shell-and-navigation-restoration
plan: 03
subsystem: testing
tags: [qt-test, qml-audit, shell-gate, source-truth, cleanup, inventory]

# Dependency graph
requires:
  - phase: 51-01
    provides: "BackendContext 8 shell action gate Q_PROPERTY + 4 label Q_PROPERTY + stateChanged forwarding wiring"
  - phase: 51-02
    provides: "BBLTopbar.qml Undo/Redo/Save/Slice/Import/Export action bindings to the C++ gate properties"
provides:
  - "Automated C++ shell-state test (shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip) proving the 8 gates are Q_PROPERTY-registered, canUndo/canRedo reflect the empty stack, and the Prepare <-> Preview round-trip preserves state"
  - "Automated QML route/registration audit (shellActionsBindToBackendContextGates) locking the BBLTopbar Undo/Redo/Save/Slice gate bindings against regression"
  - "Notification-placement guard test (notificationSurfacesStayNonOverlapping) asserting the 3 notification surfaces keep non-overlapping anchoring (SHELL-04)"
  - "Removal of the stale unused components/Toolbar.qml + its qml.qrc entry (SHELL-05)"
  - "Inventory PREP-TOP qt_target reconciled to BBLTopbar.qml + §7 cleanup checklist gains the Toolbar.qml file line"
affects: [52-sidebar, 57-deprecated-ui-removal, 58-verify]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Source-grep QML audit: readSource() + QVERIFY2(contains(...)) to lock a binding contract against silent regression"
    - "Teardown-safe async test: wait for the authoritative completion state (modelCount >= 1), not just the intermediate signal, so a QtConcurrent worker thread finishes before the object graph is torn down"

key-files:
  created: []
  modified:
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp
    - src/qml_gui/qml.qrc
    - docs/v3.6-ui-inventory.md
  deleted:
    - src/qml_gui/components/Toolbar.qml

key-decisions:
  - "Extended the two existing QtTest files (ViewModelSmokeTests.cpp, QmlUiAuditTests.cpp) rather than adding a new test binary — they are already wired into CMake/CTest, matching the plan scope fence"
  - "The stateChanged-forwarding assertion waits for editor modelCount() >= 1 (the authoritative import-completion state), not just the first stateChanged signal — this lets the QtConcurrent import worker fully finish before BackendContext destruction, avoiding a dangling-thread segfault at process exit"
  - "Kept the SHELL-04 notification guard as a source-grep of anchoring code (ErrorBanner Layout.fillWidth / ErrorToast z:200 + anchors.bottom / NotificationCenter explicit placement) rather than a visual test — Phase 51 verifies the contract, it does not redesign placement"
  - "The 4 orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage, ConfigPage) were NOT removed — verified they still exist on disk (deferred to Phase 57 per CONTEXT scope fence)"

patterns-established:
  - "Shell gate test: assert Q_PROPERTY registration via metaObject()->indexOfProperty, gate value via property().toBool(), and round-trip state preservation via requestSelectTab"
  - "Defense-in-depth audit: assert a QML enabled binding contains BOTH the page-gate AND the C++ gate string verbatim, plus a minimum occurrence count (toolbar + menu)"

requirements-completed: [SHELL-02, SHELL-03, SHELL-04, SHELL-05]

# Metrics
duration: ~45min
completed: 2026-07-01
---

# Phase 51 Plan 03: Shell State Tests, Notification Guard, Toolbar.qml Removal, Inventory Reconciliation Summary

**Closed out Phase 51 with the verification + cleanup layer: an automated C++ shell-state test proving the 8 BackendContext gates are registered and the Prepare <-> Preview round-trip preserves state, two QML source-grep audits locking the BBLTopbar gate bindings and notification-surface placement, deletion of the stale unused components/Toolbar.qml, and the inventory PREP-TOP qt_target reconciliation to BBLTopbar.qml.**

## Performance

- **Duration:** ~45 min
- **Started:** 2026-07-01
- **Completed:** 2026-07-01
- **Tasks:** 5
- **Files modified:** 4 (+ 1 deleted)

## Accomplishments
- Added `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()` to ViewModelSmokeTests.cpp (SHELL-02 + SHELL-03): asserts all 8 gate Q_PROPERTY + 4 label Q_PROPERTY are registered on the BackendContext meta-object, canUndo/canRedo start false on a fresh context (empty undo stack), canSave is true while idle, the Prepare(1) -> Preview(2) -> Prepare(1) round-trip preserves page/viewMode state without reset, and the editor viewmodel stateChanged forwards to BackendContext::stateChanged (the SHELL-02 wiring from Plan 51-01 task 4).
- Added `shellActionsBindToBackendContextGates()` to QmlUiAuditTests.cpp (SHELL-03): locks the BBLTopbar binding contract — Undo/Redo bind `enabled` to BOTH the page-gate AND canUndo/canRedo, Save binds to canSave, Slice binds to canSlice, and canUndo/canRedo each appear >= 2 times (toolbar + Edit-menu). Prevents regression of the undo-stack-empty UX-bug fix.
- Added `notificationSurfacesStayNonOverlapping()` to QmlUiAuditTests.cpp (SHELL-04): asserts the non-overlapping placement contract — ErrorBanner stays inline Layout.fillWidth (no z=200 float), ErrorToast keeps z=200 + anchors.bottom, NotificationCenter keeps explicit placement, and main.qml mounts all 3 surfaces.
- Removed the stale unused `src/qml_gui/components/Toolbar.qml` (a 1257-byte stub nothing instantiates) and its `qml.qrc` entry — the ONLY shell-level legacy removal in Phase 51 (SHELL-05). GLToolbars.qml preserved.
- Reconciled `docs/v3.6-ui-inventory.md`: §2 PREP-TOP qt_target cell corrected from `main.qml + components/Toolbar.qml` to `main.qml + BBLTopbar.qml`, §6 PREP-TOP rationale updated for consistency, and §7 aggregate cleanup checklist gained the `file: src/qml_gui/components/Toolbar.qml` line under a Phase-51 shell-level-legacy-removal note.
- The 4 orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage, ConfigPage) were NOT removed — verified to still exist on disk, deferred to Phase 57 (scope fence respected).

## Task Commits

Each task was committed atomically:

1. **Task 1: shell gate round-trip + stateChanged forwarding test** - `28ec2a1` (test)
2. **Task 2: shell gate + notification placement QML audit tests** - `c098269` (test)
3. **Task 3: remove stale unused components/Toolbar.qml + qrc entry** - `ea3878c` (refactor)
4. **Task 4: reconcile inventory PREP-TOP qt_target to BBLTopbar.qml** - `79dcefa` (docs)
5. **Task 5: build + test verification** - (verification, no separate commit — the Task 1 teardown-safety fix was folded back into Task 1 via `git commit --fixup` + autosquash rebase)

**Plan metadata:** this summary commit (docs)

## Acceptance Criteria Verification

All task and plan-level acceptance criteria verified via grep / test runs:

| Check | Method | Expected | Actual | Status |
|-------|--------|----------|--------|--------|
| Task 1 slot name count | `grep -c` | >= 2 | 2 | PASS |
| Task 1 indexOfProperty gates | test body | canImport/canSave/isBusy present | present | PASS |
| Task 1 canUndo/canRedo start false | test body | `!canUndo` + canRedo | present | PASS |
| Task 1 round-trip | test body | tpPreview + tp3DEditor + currentPage/viewMode asserts | present | PASS |
| Task 1 QSignalSpy on stateChanged | test body | QSignalSpy + BackendContext::stateChanged | present | PASS |
| Task 2 slot declarations | `grep` | 2 | 1+1 | PASS |
| Task 2 BBLTopbar 4 gate bindings | test body | canUndo/canRedo/canSave/canSlice contains | present | PASS |
| Task 2 reads 3 components + main.qml | test body | all 4 readSource | present | PASS |
| Task 2 z:200 ErrorToast vs ErrorBanner | test body | distinguishes | present | PASS |
| Task 3 Toolbar.qml deleted | `test ! -f` | gone | gone | PASS |
| Task 3 qrc entry removed | `grep -c` | 0 | 0 | PASS |
| Task 3 GLToolbars preserved | `grep -c` | 1 | 1 | PASS |
| Task 3 no live import | `grep -rn` | nothing | nothing | PASS |
| Task 4 PREP-TOP cites BBLTopbar | `grep -c` | >= 1 | 1 | PASS |
| Task 4 PREP-TOP no Toolbar.qml | row check | absent | absent | PASS |
| Task 4 §7 Toolbar.qml file line | `grep -c` | 1 | 1 | PASS |
| Task 4 §6 rationale BBLTopbar | `grep` | present | present | PASS |
| Task 4 column schema unchanged | `grep -c` | >= 4 | 6 | PASS |
| Task 4 frozen snapshot untouched | `git status` | nothing | nothing | PASS |
| Orphaned pages exist (scope fence) | `test -f` | all 4 | all 4 | PASS |

## Files Created/Modified
- `tests/ViewModelSmokeTests.cpp` - added `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()` slot (declaration + implementation); asserts 8 gate + 4 label Q_PROPERTY registration, empty-stack canUndo/canRedo, canSave idle state, Prepare<->Preview round-trip preservation, and stateChanged forwarding (HAS_LIBSLIC3R-guarded model load).
- `tests/QmlUiAuditTests.cpp` - added `shellActionsBindToBackendContextGates()` (SHELL-03 BBLTopbar binding audit) and `notificationSurfacesStayNonOverlapping()` (SHELL-04 notification placement guard) slots.
- `src/qml_gui/qml.qrc` - removed the single `<file>components/Toolbar.qml</file>` entry; GLToolbars.qml entry preserved.
- `docs/v3.6-ui-inventory.md` - §2 PREP-TOP qt_target cell corrected to cite BBLTopbar.qml; §6 PREP-TOP rationale updated; §7 cleanup checklist gained the Toolbar.qml `file:` line + Phase 51 note.
- `src/qml_gui/components/Toolbar.qml` - DELETED (stale unused 1257-byte stub).

## Decisions Made
- Followed the plan's preference to extend the two existing QtTest files rather than add a new test binary (scope fence: "Do NOT create a new test binary/target").
- The stateChanged-forwarding assertion waits for `editor->modelCount() >= 1` (the authoritative import-completion state), not just the first `stateChanged` signal. This was discovered during Task 5 verification: waiting only for the first signal let the QtConcurrent import worker thread still be running when the BackendContext destructor tore down the object graph, producing a dangling-thread segfault at process exit in `QList<bool>::clear` (deep inside an Eigen triangular-matrix-vector-product on a QThreadPool thread). Waiting for the completion state lets the worker finish first; the crash disappeared and the full suite exits 0. This is the established pattern from `editor_import_model_updates_state` (which waits on `ProjectServiceMock::loadFinished`).
- Kept the SHELL-04 notification guard as a source-grep of anchoring code rather than a visual/render test — Phase 51 verifies the non-overlapping contract, it does not redesign placement (CONTEXT).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] stateChanged-forwarding assertion caused a teardown segfault**
- **Found during:** Task 5 (build + test verification)
- **Issue:** The original Task 1 test body waited only for the first `BackendContext::stateChanged` signal after `editor->loadFile(kStlPath)`. The first stateChanged fires before the QtConcurrent model-import worker thread completes; when the test slot returns, the BackendContext destructor runs while the worker is still executing an Eigen triangular-matrix-vector-product, causing an access violation in `QList<bool>::clear` at global QThreadPool shutdown (process exit code 139).
- **Fix:** Added `QTRY_VERIFY_WITH_TIMEOUT(editor->modelCount() >= 1, 10000);` after the stateChanged wait so the assertion blocks until the import is fully complete (modelCount flips from 0 to >=1 only after the worker finishes). The worker thread is joined before the BackendContext destructor runs.
- **Files modified:** tests/ViewModelSmokeTests.cpp
- **Verification:** Isolated run of the new slot exits 0 (3 passed, 0 failed, no crash); full ViewModelSmokeTests suite exits 0 (66 passed, 0 failed, 1 skipped). Baseline (pre-Phase-51-03) ViewModelSmokeTests also exits 0 with no crash, confirming the crash was introduced by the original test body and is now resolved.
- **Committed in:** `28ec2a1` (folded back into Task 1 via `git commit --fixup` + `git rebase --autosquash`)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** The fix makes the new test teardown-safe without weakening any assertion. No scope creep — the assertion logic is unchanged (still verifies stateChanged forwarding); only the completion-wait was strengthened.

## Issues Encountered

**Build environment: vcvars64.bat PATH parsing failure (pre-existing, unrelated to this plan).**
- **Symptom:** The canonical build command `scripts/auto_verify_with_vcvars.ps1` fails immediately with `此时不应有 \VMware\VMware。` ("`\VMware\VMware` is not expected at this time") because the agent shell's inherited PATH contains space-laden Windows entries (e.g. `C:\Program Files (x86)\VMware\VMware Workstation\`) that break vcvars64.bat's batch parsing, so the MSVC `INCLUDE` env var never propagates. This was already documented in Plan 51-01's and 51-02's SUMMARYs and is NOT caused by this plan.
- **Workaround:** Per the execution-context instruction, verified the build + tests with a sanitized PATH:
  ```
  cmd.exe /c "set PATH=C:\Windows\System32;C:\Windows;C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.357\bin\Hostx64\x64;E:\Qt6.10\bin && vcvars64.bat && ninja <targets>"
  ```
- **Verification outcome (all with the sanitized PATH, no project changes to fix the environment):**
  - `ninja owzx_app_core OWzxSlicer`: `[5/5] Linking CXX executable OWzxSlicer.exe`, 0 FAILED, 0 errors. The RCC recompiled `qml.qrc` cleanly with Toolbar.qml removed (step `[2/5] Automatic RCC for src/qml_gui/qml.qrc` + `[4/5] qrc_qml.cpp.obj`) — no "file does not exist" error.
  - `ninja ViewModelSmokeTests QmlUiAuditTests`: both built with 0 errors; AUTOMOC re-ran for ViewModelSmokeTests (the new slot was picked up by the moc).
  - `ViewModelSmokeTests.exe` full run: **66 passed, 0 failed, 1 skipped, exit 0**. The new `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip` slot PASSES (the 1 skipped is an unrelated pre-existing skip, not this test).
  - `QmlUiAuditTests.exe` full run: **25 passed, 0 failed, 0 skipped, exit 0**. Both new slots PASS: `shellActionsBindToBackendContextGates()` and `notificationSurfacesStayNonOverlapping()`.
- **Conclusion:** All 5 plan tasks are verified. The 3 new test slots compile and pass; the qrc compiles with Toolbar.qml removed; the inventory edit is doc-only. The build-script environment issue is pre-existing and out of scope for Phase 51.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 51 (Shell and Navigation Restoration) is complete: all 3 plans (51-01 C++ gates, 51-02 QML bindings, 51-03 tests + cleanup) are done. The shell action gates are Q_PROPERTY-registered, bound in BBLTopbar.qml, and now locked by automated tests against regression.
- The 4 orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage, ConfigPage) remain on disk and in qml.qrc — their removal is deferred to Phase 57 (Deprecated UI Removal), where the §7 cleanup checklist Toolbar.qml line and the Settings replace rows will be consumed.
- Manual visual UAT against the PREP-TOP screenshot region (verifying Undo/Redo are greyed when the stack is empty, Save disabled mid-slice, etc.) is deferred to Phase 58 (VERIFY-01).
- No blockers. Scope fence held: only tests/ViewModelSmokeTests.cpp, tests/QmlUiAuditTests.cpp, src/qml_gui/qml.qrc, docs/v3.6-ui-inventory.md modified, and src/qml_gui/components/Toolbar.qml deleted.

---
*Phase: 51-shell-and-navigation-restoration*
*Completed: 2026-07-01*
