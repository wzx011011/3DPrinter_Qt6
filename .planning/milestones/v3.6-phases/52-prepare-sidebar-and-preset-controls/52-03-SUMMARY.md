---
phase: 52-prepare-sidebar-and-preset-controls
plan: 03
subsystem: testing
tags: [qt6, qml, qttest, qsignalspy, meta-object, source-grep-audit, slice-invalidation, sidebar]

# Dependency graph
requires:
  - phase: 52-prepare-sidebar-and-preset-controls (plan 01)
    provides: EditorViewModel staleness Q_PROPERTYs (hasStaleSliceResults/stalePlateIndices) + BackendContext configVm.stateChanged->invalidateAllSliceResults connect + settingsRequested signal/forwardSettingsRequest slot that this test wave verifies
  - phase: 52-prepare-sidebar-and-preset-controls (plan 02)
    provides: LeftSidebar.qml + FilamentSlot.qml Plan 52-02 bindings (extruderCount, isPresetDirty, presetActionBlocker, forwardSettingsRequest, filterOptionIndices, request*Scope, hidden popup) that the QML source-text audit guards
provides:
  - C++ regression guard sidebarPresetChangeInvalidatesSliceResults (PREPSB-05): verifies the staleness Q_PROPERTYs are registered + the configVm->editor invalidation connect is wired and fires
  - C++ regression guard sidebarSettingsForwardEmitsRequestedSignal (PREPSB-02): verifies forwardSettingsRequest emits settingsRequested with the right category
  - QML source-text audit leftSidebarPresetControlsAreWiredAndHonest (PREPSB-01..04): guards every Plan 52-02 sidebar/popup binding against regression
affects: [56-settings-dialog, 58-uat]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Honest connect-wired guard: assert the QSignalSpy on editor stateChanged fires after driving a configVm change (the deterministic, no-libslic3r path), rather than asserting stale-becomes-true which needs a real slice fixture"
    - "Source-grep audit pattern (mirrors Phase 51-03 shellActionsBind): readSource + contains/count assertions lock QML bindings so a regression deleting a binding is caught at test time"

key-files:
  created: []
  modified:
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "The PREPSB-05 invalidation test asserts the CONNECT FIRES (editor stateChanged spy after a configVm change), not that stale flips true. Driving stale-becomes-true requires a prior real slice result (libslic3r + model fixture); the connect-fires assertion is the deterministic, honest regression guard. This is documented explicitly in the test comments."
  - "Both new ViewModelSmokeTests slots inherit the initTestCase HAS_LIBSLIC3R skip -- the invalidation test needs configVm preset data (loadDefault), the settings test constructs BackendContext standalone but shares the skip gate."
  - "The QML audit uses exact-string contains/count assertions against the Plan 52-02 bindings (extruderCount, Math.max(1,), isPresetDirty>=2, presetActionBlocker(2,, backend.forwardSettingsRequest(\"process\"), filterOptionIndices>=2, requestGlobalScope/ObjectScope/PlateScope, no colorPickerLoader dead toggle, TODO(Phase 56)). Verified each assertion string matches the live QML before committing."

patterns-established:
  - "Composition-root connect-fires guard via QSignalSpy on the downstream viewmodel's stateChanged after driving the upstream viewmodel (no real slice needed)"
  - "Deferred-entry-point test pattern: indexOfSignal + QSignalSpy to verify a forward slot emits its signal exactly once with the right argument"

requirements-completed: [PREPSB-01, PREPSB-02, PREPSB-03, PREPSB-04, PREPSB-05]

# Metrics
duration: 14 min
completed: 2026-07-01
---

# Phase 52 Plan 03: Phase 52 Verification -- Slice Invalidation, Settings Forward, and QML Sidebar Audit Summary

**Two C++ regression guards (PREPSB-05 connect-fires invalidation + PREPSB-02 honest settings forward) plus a QML source-text audit (PREPSB-01..04) that lock every Plan 52-01/52-02 binding against regression -- test files only, all green via the sanitized-PATH build pattern**

## Performance

- **Duration:** ~14 min
- **Started:** 2026-07-01T13:40Z
- **Completed:** 2026-07-01T13:54Z
- **Tasks:** 5 (all completed)
- **Files modified:** 2

## Accomplishments
- PREPSB-05 (CRITICAL): Added `sidebarPresetChangeInvalidatesSliceResults` -- a C++ regression guard that asserts the Plan 52-01 staleness Q_PROPERTYs (`hasStaleSliceResults`, `stalePlateIndices`) are registered on EditorViewModel and initially clean, then drives a config change (print-preset selection / scope toggle) through the owned ConfigViewModel and asserts the `configVm.stateChanged -> editor->invalidateAllSliceResults()` connect fires (editor stateChanged spy count >= 1). This is the honest, deterministic guard for the critical preset-change-must-invalidate-prior-slice-result gap fix.
- PREPSB-02: Added `sidebarSettingsForwardEmitsRequestedSignal` -- a C++ test that asserts `BackendContext::settingsRequested(QString)` is registered as a signal and that `forwardSettingsRequest("process")` emits it exactly once with the right category. Verifies the entry point is honest (visible deferred Phase-56 log + emit), not silent dead UI.
- PREPSB-01..04: Added `leftSidebarPresetControlsAreWiredAndHonest` -- a QmlUiAuditTests source-text audit that reads LeftSidebar.qml + FilamentSlot.qml and asserts every Plan 52-02 binding: dynamic `extruderCount` slot count with `Math.max(1,)` guard and no `model: 5`; dead color picker popup hidden (`onClicked: {}`, no `colorPickerLoader.active = !...` toggle) with a `TODO(Phase 56)`; `isPresetDirty` dirty dots on >= 2 rows; `presetActionBlocker(2,...)` read-only gating; `backend.forwardSettingsRequest("process")` Setting button; `filterOptionIndices` on both onAccepted + onTextChanged; the complete Global/Object/Plate scope triad.
- Build + tests pass via the sanitized-PATH ninja pattern (`ninja owzx_app_core OWzxSlicer ViewModelSmokeTests QmlUiAuditTests`, exit 0, 8/8 steps; both test executables exit 0, 100% tests passed, 0 failures).

## Task Commits

Each task was committed atomically:

1. **Task 1: register PREPSB-05 + PREPSB-02 slot declarations** - `56c864f` (test)
2. **Task 2: implement PREPSB-05 slice invalidation regression guard** - `ffe8f4c` (test)
3. **Task 3: implement PREPSB-02 settings forward honest-entry-point test** - `7d4c48f` (test)
4. **Task 4: add QML sidebar preset-controls source-text audit (PREPSB-01..04)** - `c4e84d1` (test)
5. **Task 5: build + run tests via sanitized PATH** - (verification only, no separate commit)

**Plan metadata:** (this SUMMARY commit)

## Files Created/Modified
- `tests/ViewModelSmokeTests.cpp` - Added 2 private slot declarations + 2 implementations: `sidebarPresetChangeInvalidatesSliceResults` (PREPSB-05 connect-fires guard: staleness Q_PROPERTYs registered + configVm->editor connect wired) and `sidebarSettingsForwardEmitsRequestedSignal` (PREPSB-02 honest forward). Adjacent to the Phase 51-03 shell gate test; auto-discovered by QTEST_MAIN + AUTOMOC.
- `tests/QmlUiAuditTests.cpp` - Added 1 private slot declaration + 1 implementation: `leftSidebarPresetControlsAreWiredAndHonest` (PREPSB-01..04 source-text audit). Mirrors the Phase 51-03 shellActionsBind source-grep pattern; links Qt6::Test only, no C++ symbols.

## Decisions Made
- The PREPSB-05 invalidation test asserts the CONNECT FIRES rather than that `hasStaleSliceResults` flips true. Driving the stale-becomes-true path requires a prior real slice result (libslic3r + a model fixture); the connect-fires assertion is the deterministic, no-libslic3r-required guard. This is documented honestly in the test comments and matches the plan's pragmatic intent (the plan's objective block states "the test asserts the connect fires editor stateChanged spy; full stale-true path needs libslic3r fixture, documented honestly").
- Config-change driver: select an alternate print preset if more than one exists (`requestCurrentPrintPreset`), otherwise force a stateChanged via scope toggle (`requestGlobalScope`). Both are legitimate PREPSB-05 config changes that must invalidate.
- Each QML audit assertion string was verified against the live QML (`grep -c`) before committing, so the audit is grounded in the actual Plan 52-02 output, not the plan's quoted strings.

## Deviations from Plan

None - plan executed exactly as written. All 5 tasks met their acceptance criteria on the first implementation; the build (Task 5) passed on the first attempt with no moc/link errors for the new symbols. The one adjustment was running ctest with separate `-R ViewModelSmokeTests` and `-R QmlUiAuditTests` calls instead of a single `-R 'ViewModelSmokeTests|QmlUiAuditTests'` (the pipe was mis-parsed by cmd.exe quoting); this is a test-runner invocation detail, not a plan deviation, and both calls reported 100% passed.

## Issues Encountered
- **vcvars64.bat environment issue (pre-existing, out of scope):** The canonical build command `scripts/auto_verify_with_vcvars.ps1` fails because the system PATH contains space-laden `C:\Program Files (x86)\VMware\VMware Workstation\` entries that break vcvars64.bat's batch parsing. This is an ENVIRONMENTAL issue carried from Phase 51 (documented in 51-03-SUMMARY.md), NOT caused by this plan's test changes, and is out of scope to fix. Build + test verification used the established sanitized-PATH ninja + ctest pattern instead, which succeeded.
- The only build diagnostic was a pre-existing C4267 warning (size_t->int conversion) inside libslic3r's `third_party/.../ExtrusionEntity.hpp` (a header pulled in transitively), unrelated to this plan's test code. No new warnings or errors from the new test slots.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Phase 52 (Prepare Sidebar and Preset Controls) is now fully verified across all three plans: 52-01 (C++ foundation), 52-02 (QML wiring), 52-03 (this verification wave). All five PREPSB requirements (PREPSB-01..05) have automated regression guards.
- The new tests are green and will catch any future regression that: (a) removes the configVm->editor invalidation connect, (b) makes the settings forward silent (no emit), or (c) deletes/breaks any of the 7 audited sidebar bindings.
- Deferred to Phase 56: the full stale-becomes-true slice-invalidates test (needs a libslic3r + model fixture), the independent settings dialog, full ParamsPanel option rendering, and full filament color metadata. Deferred to Phase 58: manual visual UAT against the PREP-SIDEBAR screenshot.
- No blockers.

## Self-Check: PASSED
All plan-level `<verification>` commands re-run green:
1. New C++ test slots registered + implemented: `grep -c "sidebarPresetChangeInvalidatesSliceResults|sidebarSettingsForwardEmitsRequestedSignal"` = 4 (2 decls + 2 defs) -- PASS
2. PREPSB-05 invalidation test asserts the connect: body contains `indexOfProperty("hasStaleSliceResults")` + `indexOfProperty("stalePlateIndices")` + drives a config change (`requestCurrentPrintPreset`/`requestGlobalScope`) + asserts `editorSpy.count()` -- PASS
3. PREPSB-02 settings test asserts the signal: body contains `indexOfSignal("settingsRequested(QString)")` + `forwardSettingsRequest` + `spy.count() == 1` -- PASS
4. QML audit slot registered + implemented: `grep -c "leftSidebarPresetControlsAreWiredAndHonest"` = 2 (decl + def) -- PASS
5. The audit asserts all PREPSB aspects: body contains `extruderCount`, `colorPickerLoader.active = !colorPickerLoader.active` (negated), `TODO(Phase 56)`, `isPresetDirty`, `presetActionBlocker(2,`, `forwardSettingsRequest`, `filterOptionIndices`, `requestGlobalScope`, `requestObjectScope`, `requestPlateScope` -- PASS
6. Build + tests pass via sanitized PATH: `ninja owzx_app_core OWzxSlicer ViewModelSmokeTests QmlUiAuditTests` exit 0 (8/8 steps); ViewModelSmokeTests + QmlUiAuditTests both exit 0 (100% passed, 0 failed) -- PASS
7. Scope fence: `git diff --name-only 56c864f^..HEAD` shows ONLY tests/ViewModelSmokeTests.cpp + tests/QmlUiAuditTests.cpp (no source/qml/CMake/inventory edits by this plan) -- PASS

---
*Phase: 52-prepare-sidebar-and-preset-controls*
*Plan: 03*
*Completed: 2026-07-01*
