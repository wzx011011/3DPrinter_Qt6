---
phase: 51-shell-and-navigation-restoration
plan: 01
subsystem: ui
tags: [qml, qt6, q-property, viewmodel, shell, moc]

# Dependency graph
requires:
  - phase: prior EditorViewModel/PreviewViewModel
    provides: source-of-truth gate getters (canRequestSlice, isSlicing, canExportGCode, canUndo, canRedo, loading, hasSliceResult, exportActionHint, slicing)
provides:
  - "BackendContext 8 shell action gate Q_PROPERTY (canImport, canSlice, isSlicing, canExport, canSave, canUndo, canRedo, isBusy)"
  - "BackendContext 4 blocked-reason label Q_PROPERTY (exportActionLabel, exportActionHint, saveActionLabel, saveActionHint)"
  - "BackendContext bulk stateChanged() signal + viewmodel stateChanged forwarding (SHELL-02 mechanism)"
affects: [51-02-qml-shell-binding, 51-03-shell-tests-and-cleanup, 52-sidebar]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "BackendContext shell gate Q_PROPERTY (READ + NOTIFY stateChanged) forwarding to owned viewmodel getters"
    - "Bulk stateChanged() signal forwarding from owned viewmodels to the composition root"

key-files:
  created: []
  modified:
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp

key-decisions:
  - "isSlicing() aggregates editorViewModel_->isSlicing() || previewViewModel_->slicing() (cross-viewmodel slice state)"
  - "isBusy() = isSlicing() || editorViewModel_->loading() (any blocking op in flight)"
  - "canSave gated by !isSlicing() && !isBusy() — mutating the project mid-slice is unsafe (CONTEXT decision)"
  - "exportActionLabel/saveActionLabel are unconditional tr() strings (no dead ternary); blocked reasons live in the *Hint getters"
  - "canUndo/canRedo forward raw undo-stack availability; the page gate (currentPage === tp3DEditor) stays in QML (Plan 51-02)"
  - "stateChanged() is a bulk refresh signal (not per-property); mirrors the established EditorViewModel single-stateChanged pattern"

patterns-established:
  - "BackendContext shell gate: Q_PROPERTY(bool gate READ gate NOTIFY stateChanged) + computed getter forwarding to owned viewmodels, no member vars"
  - "SHELL-02 mechanism: connect owned viewmodel stateChanged -> emit BackendContext stateChanged so shell state stays live across page round-trips without per-page reset"

requirements-completed: [SHELL-02, SHELL-03]

# Metrics
duration: ~50min
completed: 2026-07-01
---

# Phase 51 Plan 01: C++ Shell Action Gate Properties on BackendContext Summary

**8 shell action gate Q_PROPERTY + 4 blocked-reason label Q_PROPERTY on BackendContext, forwarding to EditorViewModel/PreviewViewModel, plus a bulk stateChanged() signal wired from the owned viewmodels so shell gate state stays live across a Prepare↔Preview round-trip.**

## Performance

- **Duration:** ~50 min (build verification dominated — full libslic3r rebuild cycle)
- **Started:** 2026-07-01
- **Completed:** 2026-07-01
- **Tasks:** 5
- **Files modified:** 2

## Accomplishments
- Added the `void stateChanged()` signal to BackendContext (it did NOT exist before — required sub-step so the Q_PROPERTY NOTIFY clauses compile via moc).
- Declared 8 shell action gate Q_PROPERTY (canImport, canSlice, isSlicing, canExport, canSave, canUndo, canRedo, isBusy) and 4 blocked-reason label Q_PROPERTY (exportActionLabel, exportActionHint, saveActionLabel, saveActionHint), all READ + NOTIFY stateChanged.
- Implemented the 12 getters, each forwarding to the underlying EditorViewModel/PreviewViewModel source-of-truth property (e.g. canSlice→canRequestSlice, canUndo→editorViewModel.canUndo). `canSave` and the busy gates are false while isSlicing/isBusy is true.
- Wired EditorViewModel::stateChanged and PreviewViewModel::stateChanged to re-emit BackendContext::stateChanged so the shell gates refresh across a Prepare↔Preview round-trip without per-page state reset (SHELL-02 mechanism). The existing displayProjectTitleChanged connect was preserved (not removed).
- Build verification passed: the C++ compiles and links cleanly with no moc/link errors.

## Task Commits

Each task was committed atomically:

1. **Task 1: Add gate/label Q_PROPERTY + stateChanged signal to BackendContext.h** - `2d53cf3` (feat)
2. **Task 2: Declare 12 accessor methods on BackendContext.h** - `3d080c1` (feat)
3. **Task 3: Implement 12 getters in BackendContext.cpp** - `3a8dfa1` (feat)
4. **Task 4: Forward viewmodel stateChanged through BackendContext::stateChanged** - `0cbcbfc` (feat)

**Plan metadata:** this summary commit (docs)

## Files Created/Modified
- `src/qml_gui/BackendContext.h` - 8 gate Q_PROPERTY + 4 label Q_PROPERTY + `void stateChanged()` signal + 12 getter declarations
- `src/qml_gui/BackendContext.cpp` - 12 getter implementations (forwarding to viewmodels) + 2 stateChanged forwarding connects in the constructor

## Decisions Made
- Followed the authoritative (revised) forwarding contract supplied in the execution prompt for the few getters where the on-disk plan and the revised contract differed (isSlicing aggregates both viewmodels; exportActionHint suppresses once hasSliceResult; saveActionHint returns tr("busy") while slicing/busy). The on-disk acceptance-criteria greps pass with either variant; the revised contract is the more correct one (no dead ternary for exportActionLabel, cross-viewmodel slice aggregation).
- Used the established single `stateChanged()` bulk-refresh signal pattern (matching EditorViewModel) rather than per-property signals.

## Deviations from Plan

None - plan executed exactly as written. The only nuance is the forwarding contract for 3 getters followed the revised execution-context contract (documented above under Decisions Made), which is consistent with all on-disk acceptance criteria.

## Issues Encountered

**Build environment: vcvars64.bat PATH parsing failure (pre-existing, unrelated to this plan).**
- **Symptom:** The canonical build command `scripts/auto_verify_with_vcvars.ps1` failed with `fatal error C1083: cannot open include file: "vector"` in the `libslic3r_cgal_from_source` target, because vcvars64.bat aborts when the inherited PATH contains space-laden Windows entries (e.g. `Program Files (x86)\VMware\VMware Workstation`) that break batch parsing (error: `此时不应有 \VMware\VMware`).
- **Root cause:** The agent shell's PATH (translated from a POSIX-style PATH) breaks `cmd /c vcvars64.bat & set` in `auto_verify_with_vcvars.ps1` (lines 7-15), so the `INCLUDE` env var never propagates and cl.exe cannot find the C++ standard library headers. The `.obj` files for the CGAL target already existed from a successful build at 01:12 the same day, confirming vcvars works in a clean Windows shell — only this agent shell's mangled PATH triggers it.
- **Workaround:** Ran the canonical script via a thin PowerShell wrapper that resets `$env:PATH` to minimal Windows-native paths (`System32;Windows;Wbem;<PowerShell dir>`) before invoking `auto_verify_with_vcvars.ps1`, so vcvars64.bat initializes cleanly. This is an environment/invocation fix only — no project files were changed.
- **Verification outcome (all with the PATH workaround, no project changes):**
  - `owzx_app_core` target (contains BackendContext) compiled standalone: **EXIT 0** (mocs_compilation + BackendContext.cpp + ProjectServiceMock.cpp).
  - Full target build (OWzxSlicer.exe + ViewModelSmokeTests + PrepareSceneDataTests + QmlUiAuditTests + PartPlateTests + owzx-cli): **NINJA_EXIT=0**, 0 FAILED, 0 error C, 0 fatal error, 0 error LNK, 0 moc errors.
  - Canonical `auto_verify_with_vcvars.ps1` run: `[236/236] Linking CXX executable OWzxSlicer.exe` and `[3/3] Linking CXX executable E2EWorkflowTests.exe` succeeded with 0 errors; the process was killed by the harness background-timeout during the test-target build phase (not a build failure).
- **Conclusion:** The C++ changes compile and link cleanly with no moc/link errors for the 12 new symbols. The build failure is a pre-existing environment issue, not caused by this plan's code.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 51-02 (Wave 2 QML): can now bind BBLTopbar.qml / main.qml action enablement/loading/labels to the single BackendContext gate properties (canImport/canSlice/isSlicing/canExport/canSave/canUndo/canRedo/isBusy + export/save label/hint), replacing the current QML-only conditions. The Undo/Redo page-gate (currentPage === tp3DEditor) combines with backend.canUndo/canRedo in QML.
- Plan 51-03 (Wave 3 tests/cleanup): can extend ViewModelSmokeTests/ShellStateTests to assert the gate properties emit correctly and that state survives a Prepare→Preview→Prepare round-trip (the SHELL-02 forwarding added in Task 4 is the mechanism).
- No blockers. Only BackendContext.h and BackendContext.cpp were modified (scope fence held).

---
*Phase: 51-shell-and-navigation-restoration*
*Completed: 2026-07-01*
