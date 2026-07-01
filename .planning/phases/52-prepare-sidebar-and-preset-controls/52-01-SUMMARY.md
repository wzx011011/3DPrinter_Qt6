---
phase: 52-prepare-sidebar-and-preset-controls
plan: 01
subsystem: core
tags: [qt6, qml, viewmodel, moc, slice-invalidation, staleness, signals-slots]

# Dependency graph
requires:
  - phase: 51-shell-and-navigation-restoration
    provides: BackendContext stateChanged signal + viewmodel->BackendContext forwarding pattern reused for the invalidation connect
provides:
  - EditorViewModel Q_PROPERTYs stalePlateIndices (QVariantList) + hasStaleSliceResults (bool), NOTIFY stateChanged, backed by existing m_stalePlateIndices set
  - BackendContext wiring ConfigViewModel::stateChanged -> EditorViewModel::invalidateAllSliceResults() + re-emit editor stateChanged (PREPSB-05 critical gap fix)
  - BackendContext settingsRequested(QString category) signal + Q_INVOKABLE forwardSettingsRequest(QString category) slot (PREPSB-02 interim entry point)
  - EditorViewModel::invalidateAllSliceResults() made public for the composition-root caller
affects: [52-02-sidebar-qml-wiring, 52-03-sidebar-tests, 56-settings-dialog]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Staleness exposed to QML as a Q_PROPERTY NOTIFY stateChanged, kept SEPARATE from canSlice (canSlice = 'something to slice'; staleness = 'result out of date')"
    - "Composition-root invalidation: BackendContext connects ConfigViewModel::stateChanged -> EditorViewModel::invalidateAllSliceResults() so a preset change invalidates ALL plates"

key-files:
  created: []
  modified:
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/BackendContext.h
    - src/qml_gui/BackendContext.cpp

key-decisions:
  - "Reuse the existing invalidateAllSliceResults() (do NOT rename / do NOT duplicate) -- made it public so BackendContext can call it"
  - "Invalidate on ANY ConfigViewModel::stateChanged (conservative default per CONTEXT); invalidating an unchanged result is harmless (forces a safe reslice). The 'reason'-field filtering is deferred."
  - "forwardSettingsRequest interim body is an honest qInfo log + emit settingsRequested -- visible deferred entry point, NOT silent dead UI"

patterns-established:
  - "Staleness Q_PROPERTY surface on EditorViewModel, NOTIFY stateChanged, backed by m_stalePlateIndices"
  - "BackendContext as composition-root invalidation wiring point between ConfigViewModel and EditorViewModel"
  - "Deferred-entry-point pattern: qInfo log 'pending Phase N' + emit signal so a future phase connects the real handler"

requirements-completed: [PREPSB-05, PREPSB-02]

# Metrics
duration: 18 min
completed: 2026-07-01
---

# Phase 52 Plan 01: C++ Foundation -- Slice Invalidation Wiring + Staleness Q_PROPERTYs + Settings Signal Forward Summary

**Staleness Q_PROPERTYs on EditorViewModel plus a CRITICAL preset-change slice-invalidation connect in BackendContext, and an honest deferred settings-request entry point -- all C++, no QML touched**

## Performance

- **Duration:** ~18 min
- **Started:** 2026-07-01T12:20Z
- **Completed:** 2026-07-01T12:38Z
- **Tasks:** 6 (all completed)
- **Files modified:** 4

## Accomplishments
- PREPSB-05: Exposed slice staleness to QML via two new EditorViewModel Q_PROPERTYs (`stalePlateIndices` QVariantList, `hasStaleSliceResults` bool), NOTIFY `stateChanged`, backed by the existing `m_stalePlateIndices` set -- so Preview/Export can show a "stale -- reslice" indicator and gate stale-result export.
- PREPSB-05 CRITICAL gap fix: Wired `ConfigViewModel::stateChanged` -> `EditorViewModel::invalidateAllSliceResults()` + re-emit `editorViewModel_->stateChanged()` in the BackendContext composition root. This closes the silent-correctness bug where changing a filament/printer/process preset did not invalidate a previously-sliced/exported result (a user could export G-code based on the OLD preset). Invalidation marks ALL plates stale (a preset change affects every plate).
- PREPSB-02: Added a `settingsRequested(QString category)` signal + `Q_INVOKABLE forwardSettingsRequest(QString category)` slot on BackendContext. The slot body honestly logs `[Backend] settingsRequested(...) -- settings dialog pending Phase 56` AND emits the signal, so the entry point is visible but explicitly deferred until Phase 56 wires the independent dialog (not silent dead UI).
- Kept staleness SEPARATE from `canSlice` per CONTEXT (canSlice = "is there something to slice"; staleness = "is the existing result out of date"). No conflation.
- Build passes via the sanitized-PATH ninja pattern (owzx_app_core + OWzxSlicer, exit 0, no moc/link errors for the new symbols).

## Task Commits

Each task was committed atomically:

1. **Task 1: staleness Q_PROPERTYs in EditorViewModel.h** - `149b631` (feat)
2. **Task 2: staleness accessors in EditorViewModel.cpp** - `83b1ce0` (feat)
3. **Task 3: settingsRequested signal in BackendContext.h** - `7b30149` (feat)
4. **Task 4: forwardSettingsRequest slot in BackendContext.h** - `20ea979` (feat)
5. **Task 5: invalidation connect + forwardSettingsRequest impl in BackendContext.cpp** - `68dd502` (feat)
6. **Task 6: build-verification fix -- invalidateAllSliceResults public** - `5b4c393` (fix)

**Plan metadata:** (this SUMMARY commit)

## Files Created/Modified
- `src/core/viewmodels/EditorViewModel.h` - Added `stalePlateIndices`/`hasStaleSliceResults` Q_PROPERTYs (NOTIFY stateChanged) + accessor declarations; made `invalidateAllSliceResults()` public for the BackendContext caller.
- `src/core/viewmodels/EditorViewModel.cpp` - Implemented `stalePlateIndices()` (builds QVariantList from `m_stalePlateIndices`) and `hasStaleSliceResults()` (returns `!m_stalePlateIndices.isEmpty()`), adjacent to `extruderCount()`.
- `src/qml_gui/BackendContext.h` - Added `settingsRequested(QString category)` signal (signals block) and `Q_INVOKABLE forwardSettingsRequest(QString category)` slot (public).
- `src/qml_gui/BackendContext.cpp` - Added the `ConfigViewModel::stateChanged` -> `invalidateAllSliceResults()` + re-emit connect in the constructor (after the pendingAction connects); implemented `forwardSettingsRequest()` with an honest qInfo log + emit.

## Decisions Made
- Reused the existing `invalidateAllSliceResults()` rather than adding a new preset-specific invalidator (CONTEXT's Claude's Discretion). Did not rename it (plan constraint).
- Used the conservative "invalidate on ANY ConfigViewModel::stateChanged" default accepted by CONTEXT. No reason-field filtering / no-op-re-selection diffing was added (deferred; invalidating an unchanged result is harmless).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Made EditorViewModel::invalidateAllSliceResults() public**
- **Found during:** Task 6 (build verification)
- **Issue:** Plan Task 5 instructs BackendContext to call `EditorViewModel::invalidateAllSliceResults()` on `ConfigViewModel::stateChanged`, but the method was declared `private` (EditorViewModel.h, former line 705). The first build failed with MSVC `error C2248: "EditorViewModel::invalidateAllSliceResults": cannot access private member`.
- **Fix:** Moved the existing method declaration from the `private:` section to the `public:` section (next to `refreshAfterLoad()`, another BackendContext-facing helper). No rename, no duplicate -- honors the plan's "do NOT rename / reuse the existing method" constraint. BackendContext is the composition root that owns both viewmodels, so it is the legitimate caller.
- **Files modified:** src/core/viewmodels/EditorViewModel.h
- **Verification:** Sanitized-PATH `ninja owzx_app_core OWzxSlicer` now exits 0 (11/11 steps, no moc/link errors). BackendContext.cpp.obj compiles cleanly.
- **Committed in:** `5b4c393` (fix)

---

**Total deviations:** 1 auto-fixed (1 blocking / Rule 3)
**Impact on plan:** Necessary for the plan's own instructions to compile. No scope creep -- the change only widens access of the exact method the plan told BackendContext to call; behavior is unchanged.

## Issues Encountered
- **vcvars64.bat environment issue (pre-existing, out of scope):** The canonical build command `scripts/auto_verify_with_vcvars.ps1` fails because the system PATH contains space-laden `C:\Program Files (x86)\VMware\VMware Workstation\` entries that break vcvars64.bat's batch parsing ("此时不应有 \VMware\VMware"). This is an ENVIRONMENTAL issue carried from Phase 51 (documented in 51-03-SUMMARY.md), NOT caused by this plan's code, and is out of scope to fix. Build verification used the established sanitized-PATH ninja pattern instead, which succeeds.
- No other issues. All 6 tasks' acceptance criteria pass on first implementation except the private-access build error above (fixed, then build passed).

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The C++ foundation for Phase 52's sidebar work is complete: Plan 52-02 (QML) can now bind LeftSidebar.qml to the new `stalePlateIndices` / `hasStaleSliceResults` Q_PROPERTYs, call `forwardSettingsRequest("process")` from the "Setting" button, and rely on preset changes auto-invalidating slice results.
- Plan 52-03 (tests) can assert the preset-change -> staleness behavior (change a preset -> `stalePlateIndices` updates -> `hasStaleSliceResults` true).
- No blockers. The deferred items (real settings dialog, reason-field invalidation filtering, full option rendering) are explicitly Phase 56 / later scope.

## Self-Check: PASSED
All plan-level `<verification>` commands (1-8) re-run green:
1. staleness Q_PROPERTY surface: 2 Q_PROPERTYs with NOTIFY stateChanged -- PASS
2. accessors implemented + read `m_stalePlateIndices` / `.isEmpty()` -- PASS
3. settings signal + forward slot (signal decl, slot impl, `emit settingsRequested`) -- PASS
4. invalidation connect present (`&ConfigViewModel::stateChanged, editorViewModel_`, `invalidateAllSliceResults()`, `emit editorViewModel_->stateChanged()`) -- PASS
5. existing wiring preserved (`handleConfigPendingActionApplied`, `displayProjectTitleChanged`) -- PASS
6. no ConfigViewModel changes -- PASS
7. build passes via sanitized PATH (exit 0, no moc/link errors) -- PASS
8. scope fence: no `.qml` / qrc / test / inventory files modified -- PASS

---
*Phase: 52-prepare-sidebar-and-preset-controls*
*Completed: 2026-07-01*
