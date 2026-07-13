---
phase: 103-cli-fixture-readiness-gate
plan: 01
subsystem: testing
tags: [cli-fixtures, argv-gate, frameSwapped, source-audit, regression-lock, canonical-build, launch-liveness, ctest]

# Dependency graph
requires:
  - phase: 102-wipe-tower-verification-and-regression
    provides: The deterministic source-audit regression-lock pattern (wipeTowerReadbackAndRenderAnchorsPresent in QmlUiAuditTests — QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with a region-named message) mirrored here for FIXTURE-02. Also the AUTOMOC silent-skip guard pattern (filter-isolated exit-code differential).
  - phase: 93-assembleview-restoration (v4.2)
    provides: The argv plumbing being gated — parseStartupOpenRequest (main_qml.cpp:170-205) + applyStartupOpenRequests + the 4 QCommandLineOption definitions (--load-model, --open-page, --open-dialog, --skip-first-run) + the call site at main_qml.cpp:349. Phase 103 changes ONLY the gate mechanism, not the plumbing.
provides:
  - FIXTURE-02 closed: the argv startup-fixture gate waits for the first QQuickWindow::frameSwapped (scene graph rendered at least one frame) before applying open-page / load-model / open-dialog, replacing the previous QTimer::singleShot(0) zero-delay timer trick that fired on the next event-loop iteration before a frame was guaranteed. This is the canonical workaround for the recurring Windows-capture-API runtime-evidence blocker (Pitfall 4 readiness gate) — external screenshot capture is now deterministic.
  - applyStartupOpenRequests is now a direct apply function (the singleShot wrapper removed; the open-page / load-model / open-dialog routing logic byte-for-byte preserved). The gate wiring moved to the call site in main() where the QQmlApplicationEngine is in scope.
  - One-shot Qt::QueuedConnection to frameSwapped that disconnects + deletes itself in the handler — fires exactly once on the first frame (preserving the old singleShot "exactly once" contract).
  - Defensive fallback: if the root object is not a QQuickWindow (shouldn't happen for main.qml), apply immediately with a startup-log warning (degraded mode, never hang).
  - New regression-lock test argvFixtureGateUsesFrameSwappedNotSingleShot in QmlUiAuditTests (mirrors Phase 102 pattern; 5 FIXTURE-02-named QVERIFY2 assertions: frameSwapped present, singleShot(0 absent, applyStartupOpenRequests present, QObject::disconnect present, degraded-mode fallback present).
  - Updated the existing guiStartupDeepLinkArgumentsAreExtensible slot: the obsolete 'QTimer::singleShot(0, &backend' token (which locked the removed gate) is replaced with '&QQuickWindow::frameSwapped' so the deep-link plumbing audit locks the new gate contract. The other 16 plumbing tokens are unchanged.
  - Canonical build clean (production code OWzxSlicer.exe links clean, step 237/237); OWzxSlicer.exe launch liveness confirmed (PID 31520, survived 5-second no-crash probe); regression ctest 4/4 PASS; AUTOMOC silent-skip guard passes (new slot exit 0 vs invalid filter exit 1).
affects: [104-cli-fixture-recipes-and-multi-material-model, 106-d3d12-crash-root-cause-and-backend-readiness]

# Tech tracking
tech-stack:
  added: []
  patterns: [QQuickWindow::frameSwapped one-shot readiness gate (Qt::QueuedConnection + disconnect-in-handler for exactly-once semantics; the deterministic-screenshot workaround for the Windows-capture-API blocker), call-site gate wiring (gate lives where the QQmlApplicationEngine is in scope; apply function stays pure)]

key-files:
  created:
    - .planning/phases/103-cli-fixture-readiness-gate/103-01-SUMMARY.md
  modified:
    - src/qml_gui/main_qml.cpp (task 103-01-01, commit eab5270 - singleShot(0) gate removed; frameSwapped one-shot gate added at the call site; applyStartupOpenRequests simplified to a direct apply function with apply logic byte-for-byte preserved; unused <QTimer> include removed)
    - tests/QmlUiAuditTests.cpp (task 103-01-02, commit 01cf2a0 - argvFixtureGateUsesFrameSwappedNotSingleShot slot added + guiStartupDeepLinkArgumentsAreExtensible obsolete singleShot token updated to frameSwapped)

key-decisions:
  - "Call-site gate wiring (FR-05 option B), not a signature change to applyStartupOpenRequests. The gate lives at main() where the QQmlApplicationEngine is in scope (needed to fetch the root QQuickWindow and connect frameSwapped); applyStartupOpenRequests becomes a pure 'apply now' function. This is cleaner than threading the engine into the apply function and keeps the apply logic testable in isolation."
  - "The apply logic (open-page routing via startupPageRoutes + backend.requestSelectTab; load-model via backend.topbarImportModel; open-dialog routing via startupDialogRoutes) is byte-for-byte preserved — only the scheduling wrapper changed. FR-10 honored. The StartupOpenRequest struct + parseStartupOpenRequest + the 4 QCommandLineOption definitions are unchanged."
  - "One-shot connection via QMetaObject::Connection stored on the heap and disconnected + deleted in the handler (Qt::QueuedConnection). This preserves the old singleShot 'exactly once' contract: the apply fires exactly once on the first frameSwapped, not on every subsequent frame. A naive connect without disconnect would re-apply on every frame."
  - "Defensive fallback when qobject_cast<QQuickWindow*>(rootObjects().value(0)) fails: apply immediately with a startup-log warning. This never happens for main.qml (the root is always an ApplicationWindow/QQuickWindow) but prevents the fixture plumbing from hanging forever if it ever did. FR-03 honored."
  - "Rule 3 (blocking) auto-fix in task 103-01-02: the existing guiStartupDeepLinkArgumentsAreExtensible slot at QmlUiAuditTests.cpp:274 asserted 'QTimer::singleShot(0, &backend' WAS present. That assertion locked the old gate contract being removed in 103-01-01, so the regression suite would fail after the gate change. Updated the single token to '&QQuickWindow::frameSwapped' so the deep-link plumbing audit locks the new gate contract. The other 16 plumbing tokens (QCommandLineParser, the 4 QCommandLineOption defs, the struct/route plumbing, topbarImportModel, startupSkipFirstRun) are unchanged — only the obsolete gate-signature token was updated."
  - "Comment wording avoids the literal substring 'singleShot(0' so the regression test's !qml.contains('singleShot(0') assertion passes. The comments use 'zero-delay timer trick' instead. The test targets the gate signature specifically (the literal singleShot(0 form), not the word 'singleShot' generally, because singleShot may legitimately appear elsewhere."
  - "Canonical build timeout recurred (Phase 95-102 pattern): the libslic3r reconfigure + compile consumed the wrapper budget; the background canonical build was killed after OWzxSlicer.exe linked clean (step 237/237) + E2EWorkflowTests.exe linked, during ViewModelSmokeTests.cpp compile. Production code linked clean before the timeout. A targeted ninja build (vcvars env + Windows Kits fallback + ninja -j16) completed the 4 regression test targets clean. The targeted build required replicating the canonical script's full vcvars setup (sanitize PATH + Windows Kits LIB/INCLUDE fallback) — a naive vcvars invocation silently failed Windows SDK detection and the link failed with LNK1181 'cannot open user32.lib' until the fallback was applied."
  - "OWzxSlicer.exe launch liveness (PID 31520, 5-second no-crash) is the STATE.md reachability bar. Windows capture API screenshot is blocked (deferred item); process-liveness + canonical verifier + regression ctest is the documented evidence standard, NOT a screenshot. The frameSwapped gate is the canonical workaround bar per STATE.md deferred items — verified by source audit + regression test + launch liveness, not by a screenshot."

patterns-established:
  - "QQuickWindow::frameSwapped one-shot readiness gate: when a fixture/screenshot path needs 'the GUI has rendered at least one frame' semantics, connect to frameSwapped with a Qt::QueuedConnection that disconnects + deletes itself in the handler. This is deterministic (signal-gate, not QTest::qWait polling) and fires exactly once. The pattern replaces the fragile singleShot(0) trick that fired before the first frame was guaranteed. The D3D12 crash-repro fixture (Phase 106) depends on this same gate."
  - "Comment-wording-aware source-audit tests: when a regression test asserts !source.contains('someToken'), ensure comments in the source do not contain that literal substring. Use descriptive alternatives (e.g. 'zero-delay timer trick' instead of 'singleShot(0)') so the comment documents the change without tripping the negative assertion. The positive assertion (frameSwapped present) is robust to comments because the token is unique to the gate."

requirements-completed: [FIXTURE-02]

# Metrics
duration: ~30 min (incl. ~15 min canonical build that timed out the wrapper after OWzxSlicer.exe linked clean; targeted rebuild of 4 test targets ~2 min; ctest + AUTOMOC guard + launch probe ~1 min)
completed: 2026-07-12
---

# Phase 103 Plan 01: CLI Fixture Readiness Gate Summary

**Replaced the singleShot(0) argv-fixture gate with a one-shot QQuickWindow::frameSwapped gate so external screenshot capture is deterministic (scene graph rendered at least one frame before open-page/load-model/open-dialog apply) — closing FIXTURE-02, the recurring Windows-capture-API runtime-evidence blocker.**

## Performance

- **Duration:** ~30 min (canonical build libslic3r reconfigure + compile consumed the wrapper budget after OWzxSlicer.exe linked clean; targeted rebuild of the 4 regression test targets completed in ~2 min; ctest + AUTOMOC guard + launch probe ~1 min)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 3 (103-01-01 implementation, 103-01-02 test, 103-01-03 verification)
- **Files modified:** 2 (src/qml_gui/main_qml.cpp, tests/QmlUiAuditTests.cpp)

## Accomplishments

- **Task 103-01-01 (commit eab5270):** Replaced the `QTimer::singleShot(0, &backend, ...)` startup-fixture gate at main_qml.cpp with a deterministic two-stage wait. (a) The QML object tree is built (guaranteed by the existing `rootObjects().isEmpty()` check, since `engine->load()` is synchronous). (b) The first `QQuickWindow::frameSwapped` signal fires (the scene graph has rendered at least one frame). `applyStartupOpenRequests` is simplified to a direct apply function (the singleShot wrapper + the empty-request early-return are removed; the open-page / load-model / open-dialog routing logic is byte-for-byte preserved). The gate wiring moved to the call site in main() where the engine is in scope: it obtains the root `QQuickWindow*` via `qobject_cast` (the same pattern already used at the OWZX_FRAMELESS branch at main_qml.cpp:387) and connects a one-shot `Qt::QueuedConnection` to `frameSwapped` that disconnects + deletes itself in the handler, so the apply fires exactly once. Defensive fallback: if the root object is not a QQuickWindow, apply immediately with a startup-log warning (degraded mode, never hang). The unused `<QTimer>` include is removed. The StartupOpenRequest struct + parseStartupOpenRequest + the 4 QCommandLineOption definitions are unchanged (FR-10).
- **Task 103-01-02 (commit 01cf2a0):** Added the `argvFixtureGateUsesFrameSwappedNotSingleShot` private slot to QmlUiAuditTests, mirroring the Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent` pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with a FIXTURE-02-named message). The slot locks 5 gate-contract assertions: (1) frameSwapped present; (2) singleShot(0 absent; (3) applyStartupOpenRequests present (apply routing intact); (4) QObject::disconnect present (one-shot connection; fires exactly once); (5) degraded-mode fallback present (root not QQuickWindow). Rule 3 blocking auto-fix: the existing `guiStartupDeepLinkArgumentsAreExtensible` slot asserted `QTimer::singleShot(0, &backend` WAS present — that assertion locked the old gate contract being removed, so it was updated to `&QQuickWindow::frameSwapped` to lock the new contract. The other 16 deep-link plumbing tokens are unchanged.
- **Task 103-01-03 (no separate commit — verification task):** Canonical build run via scripts/auto_verify_with_vcvars.ps1. Production code (OWzxSlicer.exe) linked clean at step 237/237 (13:58, 33,821,696 bytes); E2EWorkflowTests.exe linked clean; the wrapper killed the background build during ViewModelSmokeTests.cpp compile (the documented Phase 95-102 libslic3r-reconfigure timeout pattern). A targeted ninja build (vcvars env + Windows Kits fallback + ninja -j16) completed the 4 regression test targets clean (NINJA_EXIT=0). OWzxSlicer.exe launch liveness confirmed (PID 31520, survived 5-second no-crash probe — the STATE.md reachability bar, since the Windows capture API screenshot is blocked). Regression ctest 4/4 PASS (ViewModelSmokeTests, QmlUiAuditTests incl. the new slot, PrepareSceneDataTests, PartPlateTests; 15.21 sec total). AUTOMOC silent-skip guard passed: new slot filter returns exit 0 (matched + passed) vs invalid filter exit 1 (no match) — proving the slot is registered AND passes.

## Task Commits

Each task was committed atomically (103-01-03 produced no separate code commit — verification task):

1. **Task 103-01-01: Replace singleShot(0) gate with frameSwapped one-shot gate** - `eab5270` (fix)
2. **Task 103-01-02: argvFixtureGateUsesFrameSwappedNotSingleShot slot + deep-link audit update** - `01cf2a0` (test)
3. **Task 103-01-03: Canonical build + ctest + launch liveness** - this summary (no separate code commit; verification task)

**Plan metadata:** this summary commit (docs: complete plan).

## Files Created/Modified

- `src/qml_gui/main_qml.cpp` - The singleShot(0) gate at line 213 is removed; applyStartupOpenRequests (lines 213-257) is simplified to a direct apply function with the open-page / load-model / open-dialog routing logic byte-for-byte preserved (only the scheduling wrapper changed). The gate wiring moved to the call site at main() (lines 349-382): a one-shot Qt::QueuedConnection to QQuickWindow::frameSwapped on the root window, disconnecting + deleting itself in the handler so the apply fires exactly once on the first frame. Defensive fallback applies immediately with a startup-log warning if the root is not a QQuickWindow. The unused <QTimer> include is removed. [103-01-01, eab5270]
- `tests/QmlUiAuditTests.cpp` - argvFixtureGateUsesFrameSwappedNotSingleShot slot declaration (:170, in the private slots section after wipeTowerReadbackAndRenderAnchorsPresent) + implementation (:3634-3686) with 5 FIXTURE-02-named QVERIFY2 assertions. The existing guiStartupDeepLinkArgumentsAreExtensible slot's obsolete 'QTimer::singleShot(0, &backend' token (:274) is updated to '&QQuickWindow::frameSwapped' (Rule 3 auto-fix: the old token locked the removed gate contract). [103-01-02, 01cf2a0]
- `.planning/phases/103-cli-fixture-readiness-gate/103-01-SUMMARY.md` - this summary. [103-01-03]

## Decisions Made

- **Call-site gate wiring (FR-05 option B).** The gate lives at main() where the QQmlApplicationEngine is in scope (needed to fetch the root QQuickWindow and connect frameSwapped); applyStartupOpenRequests becomes a pure 'apply now' function. Cleaner than threading the engine into the apply function and keeps the apply logic testable in isolation.
- **Apply logic byte-for-byte preserved (FR-10).** The open-page routing (startupPageRoutes + backend.requestSelectTab), load-model (backend.topbarImportModel), and open-dialog routing (startupDialogRoutes) are unchanged. Only the scheduling wrapper changed. The StartupOpenRequest struct + parseStartupOpenRequest + the 4 QCommandLineOption definitions are unchanged.
- **One-shot connection via heap QMetaObject::Connection.** The connection is stored on the heap and disconnected + deleted in the handler (Qt::QueuedConnection), preserving the old singleShot 'exactly once' contract. A naive connect without disconnect would re-apply on every frame.
- **Defensive degraded-mode fallback (FR-03).** If qobject_cast<QQuickWindow*>(rootObjects().value(0)) fails, apply immediately with a startup-log warning. Never happens for main.qml but prevents the fixture plumbing from hanging forever if it ever did.
- **Comment wording avoids 'singleShot(0' literal.** Comments use 'zero-delay timer trick' so the regression test's `!qml.contains("singleShot(0")` assertion passes while still documenting the change.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Existing deep-link test locked the removed singleShot gate contract**
- **Found during:** Task 103-01-02 (argvFixtureGateUsesFrameSwappedNotSingleShot slot)
- **Issue:** The existing `guiStartupDeepLinkArgumentsAreExtensible` slot at QmlUiAuditTests.cpp:274 asserted `QTimer::singleShot(0, &backend` WAS present. This assertion locked the old gate contract that task 103-01-01 removed, so the regression suite would fail after the gate change (the test greps main_qml.cpp for the token and QVERIFY2's if missing).
- **Fix:** Updated the single obsolete token to `&QQuickWindow::frameSwapped` so the deep-link plumbing audit now locks the new gate contract. The other 16 deep-link plumbing tokens (QCommandLineParser, the 4 QCommandLineOption defs, struct StartupPageRoute/StartupDialogRoute, parser.values calls, backend.topbarImportModel, startupSkipFirstRun, applyStartupOpenRequests) are unchanged — only the obsolete gate-signature token was updated.
- **Files modified:** tests/QmlUiAuditTests.cpp
- **Verification:** The updated guiStartupDeepLinkArgumentsAreExtensible slot passes (ctest 4/4 PASS; the new token is present in main_qml.cpp at the gate connect line).
- **Committed in:** 01cf2a0 (task 103-01-02 commit)

**2. [Rule 3 - Blocking] Targeted rebuild required full vcvars setup (Windows Kits fallback)**
- **Found during:** Task 103-01-03 (verification — targeted rebuild of test targets)
- **Issue:** The canonical build timed out the wrapper after OWzxSlicer.exe linked clean (documented Phase 95-102 pattern). The first targeted rebuild attempt (`cmake --build build --target ...` via a naive vcvars wrapper) failed at the ViewModelSmokeTests.exe link with `LNK1181: cannot open input file 'user32.lib'` because vcvars's Windows SDK detection silently failed on a polluted PATH — the LIB env var was missing the `um\x64` path.
- **Fix:** Replicated the canonical script's full vcvars setup (sanitize PATH to drop entries with spaces+parens that break vcvars batch parsing + the Windows Kits fallback that appends the installed Windows Kits Include/Lib paths when vcvars's SDK detection fails). A scratch helper (build/targeted_verify_103.ps1, gitignored) applies the same setup then runs `ninja -j16 QmlUiAuditTests PrepareSceneDataTests PartPlateTests ViewModelSmokeTests`. All 4 targets linked clean (NINJA_EXIT=0).
- **Files modified:** none committed (scratch helper is gitignored under build/)
- **Verification:** All 4 test exes relinked clean; regression ctest 4/4 PASS.
- **Committed in:** n/a (verification-only; scratch file is gitignored)

---

**Total deviations:** 2 auto-fixed (2 blocking)
**Impact on plan:** Both auto-fixes necessary for the regression suite to pass after the gate change. No scope creep. The gate change (FR-01..10) is exactly as planned; the deviations are (1) updating an existing test that locked the old contract, and (2) a build-environment workaround to complete the targeted rebuild after the canonical timeout.

## Issues Encountered

- **Background canonical-build timeout during test-target compilation (documented pattern).** The libslic3r reconfigure + compile consumed the wrapper budget; the background build was killed after OWzxSlicer.exe linked clean (step 237/237) + E2EWorkflowTests.exe linked, during ViewModelSmokeTests.cpp compile. This is the documented Phase 95/96/97/98/100/101/102 pattern. A targeted ninja build (vcvars + Windows Kits fallback + ninja -j16) completed the 4 regression test targets clean. Production code (OWzxSlicer.exe) linked clean in the canonical build before the timeout. No truncated/partial exes left on disk (the build links each exe atomically).
- **vcvars Windows SDK detection failure on polluted PATH.** The first targeted rebuild attempt failed with LNK1181 (user32.lib not found) because vcvars's findstr-based SDK detection silently failed and the UCRT/UM lib paths were not added. Fixed by replicating the canonical script's Windows Kits fallback (sanitize PATH + append installed Windows Kits Include/Lib paths). This is the same environmental hazard the canonical script documents at lines 7-74.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- **Phase 103 is complete.** FIXTURE-02 is closed: the argv fixture gate is deterministic (frameSwapped), the regression lock is in place, the canonical build is clean, OWzxSlicer.exe launches, and the regression ctest passes 4/4.
- **Phase 104 (CLI Fixture Recipes And Multi-Material Model — FIXTURE-01/03/04) is unblocked.** The deterministic gate is the prerequisite for the fixture recipes (FIXTURE-03) that drive screenshot capture of the major GUI states. The recipes can now rely on the frameSwapped gate for deterministic first-frame screenshots.
- **Phase 106 (D3D12 Crash Root-Cause — Pitfall 4/5 cross-workstream dependency) is unblocked.** The D3D12 crash-repro fixture uses this same readiness gate contract (Pitfall 4: 'the fixture phase must ship first or the two phases must coordinate on the readiness contract'). The gate guarantees the renderer attempts the first render pass before any crash-fixture assertion.
- Runtime VISUAL evidence (Windows capture API screenshot) remains blocked per STATE.md; the gate is the canonical workaround bar, verified by source audit + regression test + launch liveness. No blockers.

## Self-Check: All Must-Haves Met + Verification Bar Cleared

- **FR-01 (singleShot(0 removed):** `grep -n "singleShot(0" src/qml_gui/main_qml.cpp` returns exit 1 (zero matches). PASS.
- **FR-02 (objectCreated + frameSwapped two-stage wait):** The rootObjects().isEmpty() check at main_qml.cpp:343 (stage a) precedes the frameSwapped gate at :369 (stage b). frameSwapped present (4 grep hits). PASS.
- **FR-03 (qobject_cast root + defensive fallback):** `qobject_cast<QQuickWindow *>(engine->rootObjects().value(0))` at :364; the else branch at :377-381 applies immediately with the 'FIXTURE-02: root object is not a QQuickWindow' startup-log warning. PASS.
- **FR-04 (one-shot connection, exactly once):** `QObject::connect(... Qt::QueuedConnection)` at :368 stores a heap QMetaObject::Connection; the handler at :370-374 disconnects + deletes it. PASS.
- **FR-05 (call-site gate wiring):** The gate lives at the call site (main_qml.cpp:349-382); applyStartupOpenRequests signature unchanged (no engine param). PASS.
- **FR-06 (canonical build + ctest):** OWzxSlicer.exe links clean (step 237/237); regression ctest 4/4 PASS (15.21 sec). PASS.
- **FR-07 (source-audit test):** argvFixtureGateUsesFrameSwappedNotSingleShot slot exists (decl :170, impl :3634); asserts frameSwapped present + singleShot(0 absent + applyStartupOpenRequests present (5 FIXTURE-02-named QVERIFY2). Slot PASSES (ctest + AUTOMOC guard exit 0). PASS.
- **FR-08 (encoding + whitespace):** `git diff --check` exits 0 for both files; encoding guard exits 0 for both files. All comments English and ASCII-only. PASS.
- **FR-09 (no new product behavior):** The 4 argv flags remain OWzx-only test-evidence plumbing. No user-facing deep-link feature. The comments documenting this are preserved. PASS.
- **FR-10 (argv plumbing contract preserved):** parseStartupOpenRequest + the 4 QCommandLineOption definitions + StartupOpenRequest struct fields + the apply routing logic (open-page via startupPageRoutes + requestSelectTab; load-model via topbarImportModel; open-dialog via startupDialogRoutes) are unchanged. Only the gate scheduling changed. PASS.
- **Launch liveness (FIXTURE-02 reachability bar):** OWzxSlicer.exe launched (PID 31520), survived 5-second no-crash probe. PASS.

## Self-Check: PASSED

- The singleShot(0) gate is removed; the frameSwapped one-shot gate is in place (source audit: 4 frameSwapped hits, 0 singleShot(0 hits).
- applyStartupOpenRequests still exists with the apply routing logic byte-for-byte preserved (3 grep hits).
- The new argvFixtureGateUsesFrameSwappedNotSingleShot slot is registered (AUTOMOC guard: exit 0 vs invalid exit 1) AND passes.
- The existing guiStartupDeepLinkArgumentsAreExtensible slot is updated (obsolete singleShot token → frameSwapped token) and passes.
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier (step 237/237); all 4 regression test exes link clean via the targeted rebuild.
- OWzxSlicer.exe launches and survives the 5-second liveness probe (PID 31520).
- Regression ctest 4/4 pass (ViewModelSmokeTests, QmlUiAuditTests incl. the new slot, PrepareSceneDataTests, PartPlateTests).
- `git diff --check` exits 0; encoding guard exits 0 for both modified files.
- No new product behavior; the 4 argv flags remain OWzx-only test-evidence plumbing. No libslic3r changes. No LAN/device/cloud/network scope.

---
*Phase: 103-cli-fixture-readiness-gate*
*Plan: 01*
*Completed: 2026-07-12*
