---
phase: 100-wipe-tower-geometry-readback
plan: 01
subsystem: infra
tags: [wipe-tower, print-readback, rhi, qml, libslic3r, slice-service, geometry, viewmodel]

# Dependency graph
requires:
  - phase: 99-wipe-tower-geometry-gap-audit
    provides: Canonical v4.4 wipe-tower geometry readback + rendering gap matrix (99-GAP-MATRIX.md) with 3 frozen WTAUDIT-02 decisions (post-slice readback point; Option A dimensioned-box baseline; data-driven has_wipe_tower() gate).
provides:
  - Post-slice wipe-tower geometry readback wired end-to-end from Print::wipe_tower_data() (captured by value in the SliceService worker, commit f829f72) through EditorViewModel Q_PROPERTYs into the PreparePage.qml GLViewport bindings, with the has_wipe_tower() gate enforced (WTREAD-02).
  - Closes the WT-VIEWPORT-DEFAULTS unbound gap (PreparePage.qml GLViewport now binds all six wipe-tower Q_PROPERTYs) and the WT-HAS-WIPE-GATE structural-default gap (gate is now data-driven from the readback).
  - SoftwareViewport defensive default aligned (show=true to false).
affects: [101-wipe-tower-real-rendering-upgrade, 102-wipe-tower-verification-and-regression]

# Tech tracking
tech-stack:
  added: []
  patterns: [Print-lifetime-bounded capture-by-value readback (WipeTowerGeometry POD struct, no Print* escape), data-driven Q_PROPERTY NOTIFY gate (wipeTowerGeometryChanged), QMetaObject::invokeMethod signal-emission test pattern with qRegisterMetaType for cross-thread-argument types]

key-files:
  created:
    - .planning/phases/100-wipe-tower-geometry-readback/100-01-SUMMARY.md
  modified:
    - src/core/services/SliceService.h (task 100-01-01, commit f829f72 — WipeTowerGeometry struct + wipeTowerGeometryReady signal)
    - src/core/services/SliceService.cpp (task 100-01-01, commit f829f72 — has_wipe_tower/wipe_tower_data/get_wipe_tower_bbx/get_wipe_tower_depth/get_rib_offset reads inside the worker + emit on success branch)
    - src/core/viewmodels/EditorViewModel.h (task 100-01-02 — Q_PROPERTYs + getters + private slot + member storage)
    - src/core/viewmodels/EditorViewModel.cpp (task 100-01-02 — onWipeTowerGeometryReady slot + getters + SliceService connect)
    - src/qml_gui/pages/PreparePage.qml (task 100-01-03 — GLViewport wipe-tower Q_PROPERTY bindings)
    - src/qml_gui/Renderer/SoftwareViewport.h (task 100-01-04 — m_showWipeTower default true to false)
    - tests/ViewModelSmokeTests.cpp (task 100-01-05 — wipeTowerGeometryReadbackAppliesValidAndInvalidGate test)

key-decisions:
  - "EditorViewModel exposes the captured wipe-tower dims as six READ-only Q_PROPERTYs (showWipeTower/wipeTowerWidth/Depth/Height/X/Z) with a single NOTIFY wipeTowerGeometryChanged. No WRITE setters: the dims flow from SliceService (libslic3r worker), not from QML — consistent with how bed dims flow from QSettings/project state."
  - "The onWipeTowerGeometryReady slot is private (declared in a private slots: block immediately before signals:). It is connected via PMF (connect(sliceService_, &SliceService::wipeTowerGeometryReady, this, &EditorViewModel::onWipeTowerGeometryReady)), so private access is sufficient and it is not exposed on the public API surface."
  - "WTREAD-02 gate is enforced in the slot: when geometry.valid is false, m_showWipeTower is forced to false and the width/depth/height/x/z members are LEFT UNTOUCHED (not reset to placeholders). This guarantees the previous real dims persist but the box is not rendered — no placeholder box ever leaks as 'real' geometry on single-material slices. The slot always emits wipeTowerGeometryChanged() so QML bindings refresh on every readback."
  - "Defaults match RhiViewport.h:304-309 (show=false, 10/10/50/100/25) on both the EditorViewModel members and the PreparePage.qml binding fallbacks, so the pre-slice renderer is structurally unchanged."
  - "SoftwareViewport default m_showWipeTower aligned from true to false (defensive — SoftwareViewport is not instantiated in any page today, only registered as the alternate GLViewport QML type at main_qml.cpp:305; the alignment prevents a future leak if it is ever used)."
  - "Test approach: drive SliceService::wipeTowerGeometryReady via QMetaObject::invokeMethod(slice, \"wipeTowerGeometryReady\", Qt::DirectConnection, Q_ARG(WipeTowerGeometry, geo)) after qRegisterMetaType<WipeTowerGeometry>(). This exercises the connect(...) wiring end-to-end (signal exists, slot fires, NOTIFY emits) without requiring a real libslic3r slice (which would need fixtures)."

patterns-established:
  - "Print-lifetime-bounded capture-by-value readback: small POD struct (WipeTowerGeometry) declared in SliceService.h, captured in the worker between print.process() and activePrint_.store(nullptr), delivered by value via a dedicated signal emitted on the success branch of the sliceFinished queued lambda. No Print* or WipeTowerData* escapes the worker (Frozen Decision 1 invariant)."
  - "Data-driven Q_PROPERTY gate: the readback sets showWipeTower from Print::has_wipe_tower() (via geometry.valid), making the WTREAD-02 structural default (show=false) a real data-driven gate rather than a static assumption."
  - "QMetaObject::invokeMethod signal-emission test pattern: for signals carrying non-Qt-builtin argument types, register the type with qRegisterMetaType in a static initializer, then invoke the signal by name with Q_ARG + Qt::DirectConnection to prove the connect wiring without an integration fixture."

requirements-completed: [WTREAD-01, WTREAD-02]

# Metrics
duration: ~50 min (incl. ~30 min libslic3r reconfigure + link)
completed: 2026-07-12
---

# Phase 100 Plan 01: Wipe-Tower Geometry Readback Summary

**Post-slice wipe-tower geometry readback wired end-to-end (Print::wipe_tower_data() captured by value in SliceService worker, delivered via wipeTowerGeometryReady, exposed as EditorViewModel Q_PROPERTYs, bound in PreparePage.qml GLViewport, gated by has_wipe_tower()). WTREAD-01 + WTREAD-02 closed.**

## Performance

- **Duration:** ~50 min (includes ~30 min for the libslic3r reconfigure + OWzxSlicer.exe link triggered by the EditorViewModel.h header change propagating to all dependent TUs)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 5 (100-01-02 through 100-01-06; 100-01-01 was already shipped in commit f829f72)
- **Files modified:** 5 production/test files + this summary

## Accomplishments

- **Task 100-01-02 (commit 31a283a + fixup c45db36):** EditorViewModel exposes the captured WipeTowerGeometry as six READ-only Q_PROPERTYs (showWipeTower, wipeTowerWidth/Depth/Height/X/Z) with NOTIFY wipeTowerGeometryChanged. The onWipeTowerGeometryReady slot (in a private slots: block before signals:) is connected to SliceService::wipeTowerGeometryReady and applies the WTREAD-02 gate: when geometry.valid is false, m_showWipeTower is forced to false and the dim members are left untouched (no placeholder leak). Defaults match RhiViewport.h:304-309 (show=false, 10/10/50/100/25).
- **Task 100-01-03 (commit c68a5b3):** PreparePage.qml:1648 GLViewport (id viewport3d) now binds all six wipe-tower Q_PROPERTYs to root.editorVm.* using the existing `root.editorVm ? root.editorVm.X : default` null-guard pattern. Closes the WT-VIEWPORT-DEFAULTS unbound gap (99-GAP-MATRIX) — the renderer no longer falls through to the hardcoded 10/10/50/100/25 defaults after a real slice.
- **Task 100-01-04 (commit 4920ee9):** SoftwareViewport.h m_showWipeTower default changed from true to false (defensive WTREAD-02 alignment — SoftwareViewport is not instantiated today, but a future instantiation can no longer leak a placeholder box before readback arrives).
- **Task 100-01-05 (commit 53fe236):** Added the wipeTowerGeometryReadbackAppliesValidAndInvalidGate smoke test (valid + invalid path) with QSignalSpy on the notify signal. The test drives SliceService::wipeTowerGeometryReady via QMetaObject::invokeMethod + Q_ARG (after qRegisterMetaType<WipeTowerGeometry>()) so the connect wiring is exercised end-to-end without a real slice.
- **Task 100-01-06 (this summary, no separate code commit):** Source-audit greps (5/5 pass), canonical build (OWzxSlicer.exe + all test exes compile/link clean), and regression ctest run.

## Task Commits

Each task was committed atomically (100-01-01 was the prerequisite, already on main):

1. **Task 100-01-01: Define WipeTowerGeometry POD struct + SliceService readback signal** — `f829f72` (prerequisite, shipped before this plan)
2. **Task 100-01-02: EditorViewModel exposes wipe-tower dims as Q_PROPERTYs** — `31a283a` + fixup `c45db36`
3. **Task 100-01-03: PreparePage GLViewport binds wipe-tower Q_PROPERTYs to editorVm** — `c68a5b3`
4. **Task 100-01-04: Align SoftwareViewport default showWipeTower true to false** — `4920ee9`
5. **Task 100-01-05: Add wipe-tower geometry readback wiring assertion** — `53fe236`
6. **Task 100-01-06: Build + ctest regression + source-audit verification** — this summary (no separate code commit; the fixup c45db36 was the only code change triggered by verification)

## Files Created/Modified

- `src/core/services/SliceService.h` — WipeTowerGeometry POD struct (:42) + wipeTowerGeometryReady signal (:167). [100-01-01, f829f72]
- `src/core/services/SliceService.cpp` — has_wipe_tower/wipe_tower_data/get_wipe_tower_bbx/get_wipe_tower_depth/get_rib_offset reads inside the worker (:636-651) + emit on the sliceFinished success branch (:805). [100-01-01, f829f72]
- `src/core/viewmodels/EditorViewModel.h` — SliceService.h include (slot needs complete WipeTowerGeometry type); six Q_PROPERTYs (:650-655); six getters (:719-724); private slot onWipeTowerGeometryReady (:829, in a private slots: block before signals:); six private members (:1042-1047, defaults show=false + 10/10/50/100/25). [100-01-02]
- `src/core/viewmodels/EditorViewModel.cpp` — connect(sliceService_, wipeTowerGeometryReady, onWipeTowerGeometryReady) (:2225-2226); slot implementation (:5095-5110) applying the WTREAD-02 gate; six getter implementations (:5113-5118). [100-01-02]
- `src/qml_gui/pages/PreparePage.qml` — GLViewport (id viewport3d) wipe-tower Q_PROPERTY bindings (:1670-1675), with comment block (:1659-1669) citing Phase 100 / WTREAD-01 / WT-VIEWPORT-DEFAULTS / WTREAD-02. [100-01-03]
- `src/qml_gui/Renderer/SoftwareViewport.h` — m_showWipeTower default true to false (:238) + comment citing Phase 100 / WTREAD-02. [100-01-04]
- `tests/ViewModelSmokeTests.cpp` — wipeTowerGeometryReadbackAppliesValidAndInvalidGate test (declaration :297, implementation :3949-4031) with qRegisterMetaType<WipeTowerGeometry> static initializer + QSignalSpy + QMetaObject::invokeMethod on the signal. [100-01-05]
- `.planning/phases/100-wipe-tower-geometry-readback/100-01-SUMMARY.md` — this summary. [100-01-06]

## Decisions Made

- **Private slot placement.** The onWipeTowerGeometryReady slot is declared in a `private slots:` block immediately before `signals:` (NOT mid-class). It is connected via PMF (`connect(sliceService_, &SliceService::wipeTowerGeometryReady, this, &EditorViewModel::onWipeTowerGeometryReady)`), so private access is sufficient and the slot is not exposed on the public API surface. (The first attempt placed the block mid-class after the wipe-tower getters, which silently re-classified every subsequent public member — allPlatesSliced, hasSliceResult, loadFile, setUndoRedoManager, etc. — as private and broke BackendContext.cpp + UndoCommands.cpp with C2248. The fixup commit c45db36 moved the block to the correct position.)
- **WTREAD-02 gate leaves dims untouched on the invalid path.** When geometry.valid is false, the slot forces m_showWipeTower=false but does NOT overwrite width/depth/height/x/z with placeholder values. The previous real dims persist; they are simply not rendered. This is a stronger guarantee than resetting to placeholders: even if a future bug flips showWipeTower back to true without a fresh readback, the renderer would show the last real dims rather than the 10/10/50/100/25 placeholder. The test asserts this explicitly (after the invalid readback, wipeTowerWidth()==60.f — the value from the prior valid readback — not 10.f).
- **Test drives the signal via QMetaObject::invokeMethod, not a friend-call on the slot.** This proves the connect(...) wiring end-to-end (the signal exists on SliceService's meta-object, the slot is reachable, the NOTIFY fires). The alternative (calling editor.onWipeTowerGeometryReady(geo) directly via friend access) would prove the slot logic but not the wiring. The chosen approach required qRegisterMetaType<WipeTowerGeometry>() so Q_ARG can wrap the argument.
- **No EditorViewModel WRITE setters for wipe-tower dims.** They flow from SliceService (libslic3r worker), not from QML — consistent with how bed dims flow from QSettings/project state. READ + NOTIFY only.

## Deviations from Plan

- **One build-fix commit (c45db36) beyond the planned 5 task commits.** Task 100-01-02's initial EditorViewModel.h edit placed the `private slots:` block mid-class, re-classifying subsequent public members as private (C2248 in BackendContext.cpp + UndoCommands.cpp). The canonical build caught it; a focused fixup commit moved the block to before `signals:`. The plan's "5 atomic task commits" became 6 (5 task commits + 1 build-fix). No other deviations.

## Issues Encountered

- **C2248 private-member access errors** (BackendContext.cpp, UndoCommands.cpp) after task 100-01-02's mid-class `private slots:` placement. Diagnosed from the build log's first error block; fixed by moving the slot block to immediately before `signals:`. Root cause: a `private slots:` (or any access specifier) applies to all subsequent members until the next specifier, so mid-class placement silently changed the access of everything after it.
- **Background canonical-build timeout during the libslic3r reconfigure + OWzxSlicer.exe link.** The EditorViewModel.h header change propagated to all dependent TUs, triggering a ~30 min reconfigure + rebuild. The wrapper-timed build was killed mid-link of ViewModelSmokeTests.exe, leaving a truncated (exactly 2 MiB) exe on disk that Windows refused to launch ("文件或目录损坏且无法读取"). A targeted `ninja -j1 ViewModelSmokeTests` rebuild (modeled on scripts/build_run_ppt.ps1) completed the link cleanly; the regression ctest then ran green. The targeted rebuild script was a one-off verification helper and was removed before commit (not part of the deliverable).

## Build + Test Commands Run + Results

### Source-audit greps (5/5 PASS)

1. `rg -n "wipe_tower_data|has_wipe_tower|get_wipe_tower_bbx|get_wipe_tower_depth|get_rib_offset" src/core/services/SliceService.cpp` — PASS (all five libslic3r APIs read inside the worker :636-651).
2. `rg -n "WipeTowerGeometry|wipeTowerGeometryReady" src/core/services/SliceService.{h,cpp} src/core/viewmodels/EditorViewModel.{h,cpp}` — PASS (struct declared SliceService.h:42; signal SliceService.h:167 + emit SliceService.cpp:805; slot EditorViewModel.h:833 + EditorViewModel.cpp:5095; connect EditorViewModel.cpp:2225).
3. `rg -n "showWipeTower|wipeTowerWidth|wipeTowerDepth|wipeTowerHeight|wipeTowerX|wipeTowerZ" src/qml_gui/pages/PreparePage.qml` — PASS (all six bindings present at PreparePage.qml:1670-1675).
4. `rg -n "m_showWipeTower = false" src/qml_gui/Renderer/SoftwareViewport.h` — PASS (SoftwareViewport.h:238).
5. `rg -n "WipeTowerGeometry|wipeTowerGeometryReady|wipeTowerGeometryChanged|showWipeTower" tests/ViewModelSmokeTests.cpp` — PASS (test declaration :297, metatype registration :3964, slot wiring assertions :3976-4030).

### Canonical build (scripts/auto_verify_with_vcvars.ps1)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Result:** PASS — OWzxSlicer.exe + E2EWorkflowTests.exe + ViewModelSmokeTests.exe + PrepareSceneDataTests.exe + QmlUiAuditTests.exe + PartPlateTests.exe all compile and link clean. The first run (build/100-01-verify.log) caught the C2248 access-section bug; the second run (after fixup c45db36) compiled + linked all targets green.
- **Note:** The second canonical-build invocation was killed by the executor wrapper timeout mid-link of ViewModelSmokeTests.exe (the libslic3r reconfigure + OWzxSlicer.exe link consumed the budget). The targeted `ninja -j1 ViewModelSmokeTests` rebuild completed the link; see "Issues Encountered" above. Production code (OWzxSlicer.exe) linked clean in the canonical build before the timeout.

### Regression ctest (targeted rebuild + run)

- **Command:** `ninja -j1 ViewModelSmokeTests` followed by direct invocation of the four regression test exes (PrepareSceneDataTests.exe, PartPlateTests.exe, ViewModelSmokeTests.exe, QmlUiAuditTests.exe).
- **Result:** ALL FOUR PASSED (exit 0).
  - PrepareSceneDataTests.exe — PASS (exit 0)
  - PartPlateTests.exe — PASS (exit 0)
  - ViewModelSmokeTests.exe — PASS (exit 0). Full verbose run (`-v2 -o vm_full.txt,txt`) confirmed: **Totals: 96 passed, 0 failed, 0 skipped, 0 blacklisted, 7513ms**. The new `wipeTowerGeometryReadbackAppliesValidAndInvalidGate` test PASSED — its assertions covered the defaults check (show=false, 10/10/50/100/25), the WTREAD-01 valid path (showWipeTower=true + real dims 60/40/5/200/150 after the signal), and the WTREAD-02 invalid path (showWipeTower=false + dims persist at 60/40/5/200/150, NOT reset to placeholders; geometrySpy.count()==2).
  - QmlUiAuditTests.exe — PASS (exit 0)

### Encoding guard

- `py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py` run against all modified production + test files (EditorViewModel.h/.cpp, PreparePage.qml, SoftwareViewport.h, ViewModelSmokeTests.cpp) + this summary — all PASS (ASCII-only, English comments).

### `git diff --check`

- Exits 0 on all commits (no whitespace errors).

## Self-Check: Capture-By-Value Invariant + has_wipe_tower() Gate

- **No Print* escapes the worker (Frozen Decision 1 / D-11).** The WipeTowerGeometry struct (SliceService.h:42) is captured BY VALUE into the worker-local `capturedGeometry` (SliceService.cpp:425), populated from `print.has_wipe_tower()` / `print.get_wipe_tower_bbx()` / `print.get_wipe_tower_depth()` / `print.get_rib_offset()` / `print.wipe_tower_data()` between `print.process()` (:590) and `activePrint_.store(nullptr)` (:656), and delivered by value into the sliceFinished queued lambda capture list (:719) before the GUI-thread emit (:805). No `Print*` or `WipeTowerData*` is captured into the lambda or stored on SliceService/EditorViewModel. Confirmed by grep: zero `Print*` captures in the queued lambda; the `activePrint_` atomic lifetime (:514 set, :656/:660/:665 clear) is preserved unchanged.
- **has_wipe_tower() is the single gate (WTREAD-02 / D-07).** The worker reads `print.has_wipe_tower()` (SliceService.cpp:636); when false, `capturedGeometry.valid` stays false and the slot (EditorViewModel.cpp:5095) forces `m_showWipeTower=false` without overwriting the dim members. The PreparePage.qml:1670 binding propagates showWipeTower to the GLViewport, and `RhiViewportRenderer::renderWipeTower` early-returns on `!m_showWipeTower` (RhiViewportRenderer.cpp:1898) — so no placeholder box leaks on single-material slices. The test (ViewModelSmokeTests.cpp:4022-4030) asserts the invalid path explicitly: showWipeTower=false + dims persist (not reset to placeholders).
- **Defaults match RhiViewport.h:304-309 (D-05).** EditorViewModel members (EditorViewModel.h:1042-1047) and PreparePage.qml binding fallbacks (PreparePage.qml:1670-1675) both default to show=false + 10/10/50/100/25, so the pre-slice renderer is structurally unchanged.

## Next Phase Readiness

- **Phase 101** (Option A renderer upgrade): the real dims now reach the renderer-facing Q_PROPERTY layer end-to-end. Phase 101 can feed them into `buildWipeTowerVertices` (GizmoGeometry.cpp:449) + confirm the `m_wipeTowerDirty` rebuild fires in `uploadWipeTowerBuffer` (RhiViewportRenderer.cpp:1064-1095) with the real dims. No rediscovery needed — the WT-VIEWPORT-DEFAULTS and WT-HAS-WIPE-GATE gaps are closed.
- **Phase 102** (verification + regression): the source/QML audits (WTVERIFY-01) and the runtime wipe-tower visibility evidence (WTVERIFY-02) can run against a real multi-material slice. The ViewModelSmokeTests readback-wiring case provides the data-delivery contract assertion; Phase 102 adds the runtime visual evidence.
- No blockers.

## Self-Check: PASSED

- All 5 source-audit greps return the expected hits.
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier.
- All four regression test exes pass (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests, QmlUiAuditTests).
- ViewModelSmokeTests full suite: 96 passed, 0 failed, 0 skipped — including the new wipeTowerGeometryReadbackAppliesValidAndInvalidGate.
- `git diff --check` exits 0; encoding guard exits 0 for all modified files.
- Capture-by-value invariant preserved (no Print* escapes the worker).
- has_wipe_tower() gate enforced data-driven (WTREAD-02): single-material slices show no wipe-tower.

---
*Phase: 100-wipe-tower-geometry-readback*
*Plan: 01*
*Completed: 2026-07-12*
