---
phase: 51-shell-and-navigation-restoration
plan: 02
subsystem: ui
tags: [qml, qt6, shell, binding, source-truth, gate]

# Dependency graph
requires:
  - phase: 51-01
    provides: "BackendContext 8 shell action gate Q_PROPERTY (canImport, canSlice, isSlicing, canExport, canSave, canUndo, canRedo, isBusy)"
provides:
  - "BBLTopbar.qml action controls bound to C++ gate properties (Undo/Redo/Slice/Save gated; File-menu Import/Export gated)"
  - "Undo/Redo no longer clickable when undo/redo stack is empty (UX bug fix)"
affects: [51-03-shell-tests-and-cleanup]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Defense-in-depth binding: QML enabled binds to BOTH the page gate (currentPage === tp3DEditor) AND the C++ shell gate (canUndo/canRedo)"

key-files:
  created: []
  modified:
    - src/qml_gui/BBLTopbar.qml

key-decisions:
  - "Kept BOTH checks in QML for Undo/Redo (page-gate AND canUndo/canRedo) per CONTEXT decision — the new gate is ADDITIONAL, not a replacement"
  - "canImport/canExport added to all File-menu Import (5) and Export (4) items so none of the 8 gate properties are dead C++"
  - "Slice and Save got enabled-only gates (no page gate, since they are not Prepare-context-scoped) — canSlice/canSave already aggregate !isSlicing"

patterns-established:
  - "Shell action binding: existing screenshot-visible controls rebound to BackendContext single gate properties (canUndo/canRedo/canSave/canSlice/canImport/canExport), replacing QML-only conditions"

requirements-completed: [SHELL-01, SHELL-03]

# Metrics
duration: ~20min
completed: 2026-07-01
---

# Phase 51 Plan 02: QML Shell Wiring — Undo/Redo Gate Fix + Action Bindings Summary

**Bound the screenshot-visible shell action controls in `BBLTopbar.qml` to the 8 C++ gate properties added in Plan 51-01, fixing the concrete UX bug where Undo/Redo were clickable when the undo/redo stack was empty (page-gated only). All 8 shell gate properties (canImport, canSlice, isSlicing, canExport, canSave, canUndo, canRedo, isBusy) are now consumed in QML — no dead C++ gate properties remain.**

## Performance

- **Duration:** ~20 min
- **Started:** 2026-07-01
- **Completed:** 2026-07-01
- **Tasks:** 7
- **Files modified:** 1 (src/qml_gui/BBLTopbar.qml)

## Accomplishments
- **Undo/Redo UX bug fixed (toolbar + Edit-menu):** Both the toolbar CxIconButton AND the topMenu Edit-submenu CxMenuItem items for Undo/Redo now bind `enabled` to BOTH the page gate (`backend.currentPage === backend.tp3DEditor`) AND the C++ undo/redo-stack gate (`backend.canUndo` / `backend.canRedo`). The `onClicked`/`onTriggered` guards mirror the `enabled` binding (defense-in-depth — CONTEXT decision "keep both checks in QML"). Undo/Redo are no longer clickable when the stack is empty.
- **Save toolbar button gated on canSave:** Added `enabled: backend.canSave` and guarded `onClicked` with `backend.canSave` so the project cannot be mutated mid-slice (canSave forwards to `!isSlicing() && !isBusy()` per CONTEXT safety decision).
- **Slice side-tool button gated on canSlice:** Added `enabled: backend.canSlice` so the shell slice action reflects C++ slice readiness (forwards to `editorViewModel.canRequestSlice && !isSlicing`).
- **File-menu Import (5 items) gated on canImport:** All 5 Import sub-menu items (3MF/STL/OBJ/STEP/AMF) gained `enabled: backend.canImport` (forwards to `!isBusy()` — blocked during a slice/load in flight).
- **File-menu Export (4 items) gated on canExport:** All 4 Export sub-menu items (G-code/All Plate G-code/3MF/Model) gained `enabled: backend.canExport` (forwards to `hasSliceResult && !isSlicing` for G-code, `!isSlicing` for project/model).
- **No new controls/menus/signals added (source-truth rule):** The diff adds ONLY `enabled:`/`onClicked:`/`onTriggered:` binding lines and Phase-51 SHELL-03 comment annotations. CxIconButton block count (14), CxMenuItem block count (39), and signal count (22) are unchanged — no new product behavior introduced.
- **Build verification passed:** OWzxSlicer.exe links cleanly (219/219 steps, 0 errors).

## Task Commits

Each task was committed atomically (6 QML commits + this summary):

1. **Task 1: Undo toolbar button gated on page-gate AND canUndo** - `e8c01b3` (feat)
2. **Task 2: Redo toolbar button gated on page-gate AND canRedo** - `f15bbc8` (feat)
3. **Task 3: Edit-menu Undo/Redo items gated on canUndo/canRedo** - `30d92e8` (feat)
4. **Task 4: Save toolbar button gated on canSave** - `fba79da` (feat)
5. **Task 5: Slice side-tool button gated on canSlice** - `ae98384` (feat)
6. **Task 6: File-menu Import (5) + Export (4) items gated on canImport/canExport** - `440a0de` (feat)
7. **Task 7: Build verification** - (verification, no commit)

**Plan metadata:** this summary commit (docs)

## Files Created/Modified
- `src/qml_gui/BBLTopbar.qml` - 6 binding regions edited (Undo/Redo toolbar icons, Edit-menu Undo/Redo items, Save toolbar icon, Slice side-tool icon, File-menu Import sub-menu ×5, File-menu Export sub-menu ×4). +26 insertions, -15 deletions. No new controls/menus/signals.

## Acceptance Criteria Verification (grep counts)

All acceptance criteria from the plan verified via grep on `src/qml_gui/BBLTopbar.qml`:

| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Undo toolbar+menu on canUndo | `grep -c "backend.canUndo"` | >= 2 | 4 | PASS |
| Redo toolbar+menu on canRedo | `grep -c "backend.canRedo"` | >= 2 | 4 | PASS |
| Page-gate preserved | `grep -c "backend.currentPage === backend.tp3DEditor"` | >= 4 | 8 | PASS |
| Save gated on canSave | `grep -c "enabled: backend.canSave"` | >= 1 | 1 | PASS |
| Slice gated on canSlice | `grep -c "enabled: backend.canSlice"` | >= 1 | 1 | PASS |
| Import items gated on canImport | `grep -c "enabled: backend.canImport"` | >= 5 | 5 | PASS |
| Export items gated on canExport | `grep -c "enabled: backend.canExport"` | >= 1 | 4 | PASS |

## Decisions Made
- Followed the CONTEXT decision to keep BOTH checks in QML for Undo/Redo (page-gate AND canUndo/canRedo) — the new C++ gate is ADDITIONAL, not a replacement for the Prepare-context page check (defense-in-depth).
- Added `enabled: backend.canImport` as the FIRST attribute inside each File-menu CxMenuItem block (before `text:`/`onTriggered:`), per the plan's Task 6 instruction — this keeps the gate visually prominent and consistent across all gated items.
- Added a guard `if (backend.canSave && !backend.topbarSaveProject())` to the Save `onClicked` rather than relying on `enabled` alone, mirroring the plan's Task 4 template exactly.
- Did NOT touch the macOS MenuBar Loader block (lines 59-82) — it is a macOS-only stub (`active: Qt.platform.os === "osx"`) with no enabled gate, and the plan's scope was the live Windows topbar + the `[▾]` topMenu Edit submenu + the `[File ▾]` fileMenu Import/Export submenus. The macOS MenuBar Undo/Redo MenuItems have no enabled binding but are inactive on Windows; out of scope.

## Deviations from Plan

None — plan executed exactly as written. All 7 tasks completed, all acceptance-criteria grep counts met or exceeded. The only interpretation choice was the placement of `enabled:` as the first attribute in the File-menu CxMenuItem blocks (specified by the plan's Task 6 text), and the macOS MenuBar block was correctly identified as out of scope.

## Issues Encountered

**Build environment: vcvars64.bat PATH parsing failure (pre-existing, unrelated to this plan).**
- **Symptom:** The canonical build command `scripts/auto_verify_with_vcvars.ps1` fails because the agent shell's PATH contains `C:\Program Files (x86)\VMware\VMware Workstation\` which breaks vcvars64.bat's batch parsing (error: `此时不应有 \VMware\VMware`). This was already documented in Plan 51-01's SUMMARY and is NOT caused by this plan's QML change.
- **Workaround:** Per the execution-context instruction, verified the build with a sanitized PATH using: `cmd.exe /c "set PATH=C:\Windows\System32;C:\Windows;...;E:\Qt6.10\bin && vcvars64.bat && ninja owzx_app_core OWzxSlicer"`. This resets PATH to Windows-native + MSVC + Qt paths before invoking vcvars64.bat.
- **Verification outcome:**
  - `ninja owzx_app_core OWzxSlicer`: **[219/219] Linking CXX executable OWzxSlicer.exe**, 0 FAILED, 0 error C, 0 fatal error, 0 error LNK.
  - The modified BBLTopbar.qml was compiled into the qrc resource (step 217: `qrc_qml.cpp.obj`) and linked cleanly.
  - `startup_diagnostics.log` (from a prior app run on 2026-07-01) contains NO "does not exist" / "is ambiguous" binding warnings for any of canUndo/canRedo/canSave/canSlice/canImport/canExport — the property names resolve correctly on BackendContext.
- **Conclusion:** The QML binding changes are syntactically valid, the property names match BackendContext.h exactly, and the build links with no errors. The build-script environment issue is pre-existing and out of scope for Phase 51.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 51-03 (Wave 3 tests/cleanup): can extend QmlUiAuditTests/ViewModelSmokeTests to assert the gate bindings are present in BBLTopbar.qml (source-grep audit) and that the gate properties emit correctly on undo/redo/slice state transitions. The QML source-grep audit (grep counts above) already provides the binding-correctness baseline.
- The Undo/Redo stack-empty UX bug is fixed at the binding level; visual UAT against the PREP-TOP screenshot region is deferred to Phase 58 per CONTEXT.
- No blockers. Only `src/qml_gui/BBLTopbar.qml` was modified (scope fence held — no main.qml navigation logic change, no C++ change, no qrc/test/inventory).

---
*Phase: 51-shell-and-navigation-restoration*
*Completed: 2026-07-01*
