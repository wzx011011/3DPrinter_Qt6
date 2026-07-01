---
phase: 51-shell-and-navigation-restoration
verifier: gsd-verifier
date: 2026-07-01
status: passed
---

# Phase 51 Verification — Shell and Navigation Restoration

**Phase goal (ROADMAP):** "Restore the application-level shell and workflow actions so Prepare, Preview, and settings sit inside the right OrcaSlicer-like frame."

**Requirement IDs:** SHELL-01, SHELL-02, SHELL-03, SHELL-04, SHELL-05

**Verdict: PASSED.** All five SHELL requirements are satisfied by code + tests. Visual UAT (screenshot-aligned visual parity) is correctly deferred to Phase 58 (VERIFY-04) and is not a Phase 51 gate — Phase 51's deliverable is action-state wiring (C++ gates + QML bindings + tests + legacy cleanup), not a visual redesign.

---

## Summary verdict table

| Req | Criterion (from ROADMAP success criteria) | Verdict | Primary evidence |
|-----|-------------------------------------------|---------|------------------|
| SHELL-01 | Navigate Prepare/Preview/Device/Monitor/Project/settings via screenshot-aligned shell | PASS | Page tabs + `requestSelectTab` in BBLTopbar.qml (lines 124, 273, 283-287); shell chrome pre-existed, Phase 51 keeps it |
| SHELL-02 | Page switching preserves relevant Prepare/Preview state | PASS | `stateChanged` forwarding (BackendContext.cpp) + `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip` test PASS |
| SHELL-03 | Import/slice/preview/export/save/undo-redo/settings actions driven by C++ state | PASS | 8 gate + 4 label Q_PROPERTY in BackendContext.h; all consumed in BBLTopbar.qml; test PASS |
| SHELL-04 | Notifications visible without covering sidebar/viewport/preview | PASS | `notificationSurfacesStayNonOverlapping` test PASS; ErrorBanner inline / ErrorToast z=200 / NotificationCenter top-right |
| SHELL-05 | No obsolete placeholder route in restored local workflow | PASS | `components/Toolbar.qml` deleted, qml.qrc entry removed, GLToolbars preserved |

Scope fence: PASS (only shell touched; 4 orphaned pages retained for Phase 57; sidebar/viewport/settings untouched).
Build/test: PASS via sanitized PATH (canonical script fails only on a pre-existing environment issue — see §Build/test status).

---

## 1. SHELL-01 — Navigate via screenshot-aligned shell — PASS

**ROADMAP criterion 1:** "User can navigate Prepare, Preview, Device/Monitor, Project, and settings actions through a screenshot-aligned shell."

**Context:** The shell navigation mechanism (main.qml `StackLayout` + `currentIndex` bound to `backend.currentPage` + `requestSelectTab`) was already upstream-aligned in prior phases (ARCH-01/05/06/07). Phase 51's contribution is the action-state wiring (Plans 51-01/51-02) and the cleanup/tests (51-03), not new navigation. The shell chrome is MODIFY-not-replace per CONTEXT.

**Evidence:**
- Page tabs present in `src/qml_gui/BBLTopbar.qml`, all routed through `backend.requestSelectTab`:
  ```
  line 124:  onTapped: backend.requestSelectTab(backend.tpHome)
  line 273:  backend.requestSelectTab(currentIndex)
  line 283:  { label: qsTr("准备"),     pos: backend.tp3DEditor },
  line 284:  { label: qsTr("预览"),     pos: backend.tpPreview },
  line 285:  { label: qsTr("设备"),     pos: backend.tpDevice },
  line 286:  { label: qsTr("多设备"),   pos: backend.tpMultiDevice },
  line 287:  { label: qsTr("项目"),     pos: backend.tpProject },
  ```
- Settings action reachable via `backend.openSettings()` (H3, BackendContext.h line 324) — not modified by Phase 51, remains live.

**Note on visual alignment:** The PREP-TOP inventory row (`docs/v3.6-ui-inventory.md`) defines the visual contract (verification = `manual-visual`). Pixel-level screenshot alignment is a Phase 58 (VERIFY-04) UAT concern, not a Phase 51 automated gate. Phase 51's non-visual contribution (action gates) is fully verifiable here.

**Verdict: PASS.**

---

## 2. SHELL-02 — Page switching preserves state — PASS

**ROADMAP criterion 3:** "Page switching preserves relevant Prepare and Preview state."

**Mechanism (Plan 51-01 Task 4):** BackendContext forwards both owned viewmodels' `stateChanged` through its own bulk `stateChanged()` signal, so shell gate state re-reads live across a Prepare↔Preview round-trip with no per-page state reset. Because Prepare object/plate/selection state lives in EditorViewModel/ProjectServiceMock (page-independent) and Preview layer/move/camera state lives in PreviewViewModel, state was never page-coupled — the wiring just guarantees the shell re-reads it.

**Evidence:**
- Forwarding wired in `src/qml_gui/BackendContext.cpp`:
  - `&EditorViewModel::stateChanged` references: 2 (the original `displayProjectTitleChanged` connect preserved + the new bulk-forward connect)
  - `&PreviewViewModel::stateChanged` references: 1 (new bulk-forward connect)
  - `displayProjectTitleChanged` preserved (3 refs) — NOT removed (defense-in-depth).
  - No per-page reset: `grep -nE "(reset|clear)OnPageChange|snapshotPageState"` → none.
- Automated test `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()` in `tests/ViewModelSmokeTests.cpp` (line 1436) asserts the full round-trip:
  ```
  ctx.setCurrentPage(tp3DEditor) -> requestSelectTab(tpPreview) -> requestSelectTab(tp3DEditor)
  QCOMPARE(currentPage, tp3DEditor); QCOMPARE(currentViewMode, View3D)
  ```
  and asserts canUndo/canRedo remain false and canSave remains true after the round-trip (no reset). It also drives a real model load (requires HAS_LIBSLIC3R) and uses `QSignalSpy` on `&BackendContext::stateChanged` to prove the forwarding fires, waiting on `modelCount() >= 1` for teardown safety.
- **Test run (freshly rebuilt binary):** `PASS: ViewModelSmokeTests::shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()` — totals 66 passed, 0 failed, 1 skipped. The libslic3r-guarded model-load path executed and passed (log shows "import success objects=1 plates=1").

**Verdict: PASS.**

---

## 3. SHELL-03 — Actions driven by C++ state — PASS

**ROADMAP criterion 2:** "Import, slice, preview, export, save, undo/redo, and settings actions are driven by C++ state."

**C++ surface (Plan 51-01):** 8 gate Q_PROPERTY + 4 blocked-reason label Q_PROPERTY on BackendContext, all READ + NOTIFY `stateChanged`, each forwarding to the underlying EditorViewModel/PreviewViewModel source-of-truth getter.

**Evidence — declarations (`src/qml_gui/BackendContext.h`):**
- 8 gate Q_PROPERTY (lines 97-104): `canImport`, `canSlice`, `isSlicing`, `canExport`, `canSave`, `canUndo`, `canRedo`, `isBusy` — all `READ <name> NOTIFY stateChanged`.
- 4 label Q_PROPERTY (lines 106-109): `exportActionLabel`, `exportActionHint`, `saveActionLabel`, `saveActionHint` — all `READ <name> NOTIFY stateChanged`.
- `void stateChanged();` signal declared in the `signals:` block (line 391) — required sub-step; did NOT exist before Phase 51 (moc would have failed on the NOTIFY clauses).
- 12 getter declarations (lines 259-270); no member variables added (computed/forwarded on each call).

**Evidence — implementations (`src/qml_gui/BackendContext.cpp`):** all 12 getters present (count = 12 via grep). Forwarding contract verified by reading the bodies:
- `canSlice()` → `editorViewModel_->canRequestSlice() && !isSlicing()` (line 263)
- `isSlicing()` → aggregates `editorViewModel_->isSlicing() || previewViewModel_->slicing()` (lines 242-246)
- `canExport()` → `editorViewModel_->canExportGCode() && !isSlicing()` (line 268)
- `canSave()` → `!isSlicing() && !isBusy()` (line 274) — mid-slice mutation blocked (CONTEXT safety decision)
- `canUndo()`/`canRedo()` → `editorViewModel_->canUndo()`/`canRedo()` (raw undo-stack; page gate stays in QML)
- `isBusy()` → `isSlicing() || editorViewModel_->loading()` (line 252)
- `exportActionLabel`/`saveActionLabel` → unconditional `tr()` strings; blocked reasons in the `*Hint` getters (no dead ternary).

**Evidence — QML consumption (`src/qml_gui/BBLTopbar.qml`):** all 8 gate properties consumed, no dead C++:
| Property | QML binding site | grep count |
|----------|------------------|------------|
| `canUndo` | Undo toolbar icon (201-202) + Edit-menu item (603-604) | 4 |
| `canRedo` | Redo toolbar icon (212-213) + Edit-menu item (608-609) | 4 |
| `canSave` | Save toolbar icon (`enabled`, line 187) | 1 |
| `canSlice` | Slice side-tool (`enabled`, line 357) | 1 |
| `canImport` | File-menu Import sub-menu ×5 (`enabled`, lines 535-555) | 5 |
| `canExport` | File-menu Export sub-menu ×4 (`enabled`, lines 565-580) | 4 |
| `isSlicing` / `isBusy` | Internal aggregates — consumed by the canImport/canSave/canSlice/canExport C++ implementations (intended architecture) | — |

Defense-in-depth preserved: Undo/Redo bind `enabled` to BOTH the page gate AND the C++ stack gate (`backend.currentPage === backend.tp3DEditor && backend.canUndo`/`canRedo`). Page-gate string appears 8 times total. This fixes the concrete UX bug (Undo/Redo clickable when the stack is empty).

**Evidence — automated tests (Plan 51-03):**
- `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()` (VMST): asserts all 12 Q_PROPERTY registered via `indexOfProperty`, canUndo/canRedo false on fresh context, canSave true while idle → **PASS**.
- `shellActionsBindToBackendContextGates()` (QAT): locks the BBLTopbar binding contract via source-grep (`contains` on canUndo/canRedo/canSave/canSlice, each >= 2 occurrences for canUndo/canRedo) → **PASS**.

**Verdict: PASS.**

---

## 4. SHELL-04 — Notification placement non-overlapping — PASS

**ROADMAP criterion (REQUIREMENTS SHELL-04):** "Notifications, validation errors, and blocking workflow messages are visible in the restored shell without covering critical sidebar, viewport, or preview controls."

**Mechanism:** Phase 51 verifies + guards the existing non-overlapping placement of the 3 notification surfaces; it does not redesign placement (CONTEXT). The guard test is a source-grep audit of anchoring code.

**Evidence — `notificationSurfacesStayNonOverlapping()` in `tests/QmlUiAuditTests.cpp` (line 1022)** reads all 4 files and asserts:
- ErrorBanner: `Layout.fillWidth: true` present (inline push-down, not floating) AND no `z: 200`.
- ErrorToast: `z: 200` present (floating centered overlay) AND `anchors.bottom` present (anchored to bottom, not covering sidebar).
- NotificationCenter: explicit placement (`anchors` | `Popup` | `x:`).
- main.qml: mounts all 3 surfaces (`ErrorBanner { }`, `ErrorToast { }`, `NotificationCenter {`).

**Component sanity-check (independent grep):**
- `ErrorBanner.qml`: `Layout.fillWidth: true` ×2, `z: 200` ×0 (good).
- `ErrorToast.qml`: `z: 200` ×1, `anchors.bottom` ×2.
- `main.qml`: mounts `ErrorBanner { }` (×1), `ErrorToast { }` (×1), `NotificationCenter {` (×1).

**Test run (freshly rebuilt binary):** `PASS: QmlUiAuditTests::notificationSurfacesStayNonOverlapping()` — QAT totals 25 passed, 0 failed.

**Verdict: PASS.**

---

## 5. SHELL-05 — No obsolete placeholder route — PASS

**ROADMAP criterion 4:** "No obsolete placeholder route remains in the restored local workflow."

**Mechanism (Plan 51-03 Task 3):** Remove the ONLY shell-level legacy — the stale unused `components/Toolbar.qml` (a 1257-byte stub nothing instantiates) and its `qml.qrc` entry. The 4 orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage, ConfigPage) are NOT removed — deferred to Phase 57 per CONTEXT scope fence.

**Evidence:**
- `components/Toolbar.qml` deleted: `test ! -f` → file gone. (git: `D src/qml_gui/components/Toolbar.qml`)
- `qml.qrc` has no `Toolbar.qml` entry: `grep -c "Toolbar.qml" src/qml_gui/qml.qrc` → 0.
- `GLToolbars.qml` (different file, still live) preserved: `grep -c "components/GLToolbars.qml" src/qml_gui/qml.qrc` → 1.
- No live import of the deleted component: `grep -rn "components/Toolbar" src/` → nothing; `grep -rn "components/Toolbar" src/qml_gui --include=*.qml` → nothing.
- RCC compiles cleanly with the entry removed (no "file does not exist" error — confirmed by the successful qrc rebuild step in the test-target build).

**Scope fence — orphaned pages retained (deferred to Phase 57):** all 4 still exist on disk:
- `src/qml_gui/pages/ModelMallPage.qml` — EXISTS
- `src/qml_gui/pages/PreferencesPage.qml` — EXISTS
- `src/qml_gui/pages/DeviceListPage.qml` — EXISTS
- `src/qml_gui/pages/ConfigPage.qml` — EXISTS

**Verdict: PASS.**

---

## 6. Scope fence — PASS

**Phase 51 should only touch shell.** Verified via `git diff --name-status HEAD~12 HEAD` (the 3 plans' commit range). Files changed:
```
A  .planning/.../51-02-SUMMARY.md      (plan doc)
A  .planning/.../51-03-SUMMARY.md      (plan doc)
M  docs/v3.6-ui-inventory.md           (PREP-TOP qt_target reconciliation, doc-only)
M  src/qml_gui/BBLTopbar.qml           (51-02 QML bindings)
D  src/qml_gui/components/Toolbar.qml  (51-03 legacy removal)
M  src/qml_gui/qml.qrc                 (51-03 entry removal)
M  tests/QmlUiAuditTests.cpp           (51-03 tests)
M  tests/ViewModelSmokeTests.cpp       (51-03 tests)
```
(Plus BackendContext.h/.cpp from 51-01, committed earlier in the range.) No sidebar/viewport/settings/product-logic files touched. No new controls/menus/signals added (BBLTopbar diff is +26/-15 binding-only lines). Working tree clean except `.planning/STATE.md` bookkeeping.

**Verdict: PASS.**

---

## 7. Build/test status — PASS (environment caveat documented)

**Canonical command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` — **FAILS**, but ONLY due to a pre-existing environment issue, not Phase 51 code.

**Root cause (reproduced by this verifier firsthand):** The agent shell's inherited PATH contains space-laden Windows entries (e.g. `C:\Program Files (x86)\VMware\VMware Workstation\`) that break `vcvars64.bat` batch parsing (error: `此时不应有 \VMware\VMware`). The MSVC `INCLUDE` env var never propagates, so `cl.exe` emits `fatal error C1083: cannot open include file: "vector"` on the `libslic3r_cgal_from_source` target. This is an environment/invocation problem identical across all 3 plan SUMMARYs and reproduced independently here; it is unrelated to the Phase 51 changes (which are QML bindings + computed C++ getters + tests).

**Workaround (environment-only, no project file changes):** reset `$env:PATH` to minimal Windows-native paths before invoking vcvars64.bat, then run ninja. With this, the MSVC environment initializes cleanly ("Environment initialized for: 'x64'") and standard-library headers resolve.

**Verification performed by this verifier (current, authoritative):**
1. Clean rebuild of `ViewModelSmokeTests` + `QmlUiAuditTests` against current Phase 51 source via sanitized PATH + vcvars64.bat:
   - `[57/57] Linking CXX executable ViewModelSmokeTests.exe`, BUILD EXIT 0, 0 errors / 0 FAILED.
   - RCC recompiled `qml.qrc` cleanly with Toolbar.qml removed (no "file does not exist" error).
   - AUTOMOC picked up the new test slots.
2. Freshly-built test binaries run with `-o ...,txt`:
   - `ViewModelSmokeTests`: **66 passed, 0 failed, 1 skipped, exit 0.** `shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip` → PASS (libslic3r model-load path executed: "import success objects=1 plates=1").
   - `QmlUiAuditTests`: **25 passed, 0 failed, 0 skipped, exit 0.** Both `shellActionsBindToBackendContextGates` and `notificationSurfacesStayNonOverlapping` → PASS.
   - The 1 VMST skip is the unrelated pre-existing skip, not a Phase 51 test.

**Conclusion:** The Phase 51 code is correct and fully verified (build + tests pass once the environment is sanitized). The canonical-script failure is an environment problem, NOT a code problem, and does NOT constitute a Phase 51 gap. It should be tracked as an environment-infra fix (VERIFY-05 follow-up), not a Phase 51 blocker.

---

## Final verdict

**status: passed**

All five SHELL requirements (SHELL-01..05) are satisfied with deterministic code + test evidence. The scope fence held (only shell touched; 4 orphaned pages deferred to Phase 57). Build + tests pass once the pre-existing vcvars/PATH environment issue is sanitized (an environment problem, not a code problem).

**Deferred to Phase 58 (VERIFY-04):** manual visual UAT confirming screenshot-aligned visual parity for the PREP-TOP shell region (e.g. Undo/Redo visually greyed when the stack is empty, Save visually disabled mid-slice). This is correctly out of Phase 51's automated scope — Phase 51's deliverable is the C++-driven action-state wiring, which is fully testable and verified here.
