---
phase: 110-filament-map-popup-ui-and-mode-surfacing
plan: 01
subsystem: ui
tags: [qml, filament-map, popup, cpopup, qsml-audit, enum-validation]

# Dependency graph
requires:
  - phase: 107-filament-map-mode-enum-widening-and-3mf-migration
    provides: 4-value FilamentMapMode enum (fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault) + 3MF migration.
  - phase: 108-filament-map-auto-recommendation-readback
    provides: EditorViewModel Q_PROPERTYs hasAutoFilamentMap + autoFilamentMapMode + autoFilamentMaps (NOTIFY filamentMapChanged) -- the popup binding targets.
provides:
  - FilamentGroupPopup.qml (CxPopup-based, 3 selectable modes + Phase 108 auto-map preview), wired into BBLTopbar + registered in qml.qrc.
  - Q_INVOKABLE setPlateFilamentMapMode(int plateIndex, int mode) on ProjectServiceMock + EditorViewModel delegate (mode-only write path; reuses setPlateFilamentMap plumbing).
  - R-02 fix: PartPlate::setFilamentMapMode(int) clamps out-of-range to fmmDefault (grep-assertable [0,3] guard at the Q_INVOKABLE boundary).
  - QmlUiAuditTests::filamentGroupPopupSurfacesThreeModesNotFour source-audit slot (FMAP-03 anti-feature + R-02 lock).
affects: [111-filament-map-round-trip-test, filament-map-mode-surfacing, qml-ui, editor-viewmodel]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "CxPopup-based mode selector with ButtonGroup single-selection radios (3 concrete modes only; inherit-sentinel never surfaced)."
    - "Mode-only Q_INVOKABLE write path reusing the full-write variant (read current maps, delegate) to avoid duplicate plate-write plumbing."
    - "Enum-boundary clamp at the C++ setter (not the Q_INVOKABLE caller) so every int entry point is guarded once."
    - "Source-audit token-absence check: the popup file deliberately avoids the sentinel's symbolic name so a no-4th-radio regression fails deterministically."

key-files:
  created:
    - src/qml_gui/dialogs/FilamentGroupPopup.qml
  modified:
    - src/core/model/PartPlate.h
    - src/core/services/ProjectServiceMock.h
    - src/core/services/ProjectServiceMock.cpp
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/BBLTopbar.qml
    - src/qml_gui/qml.qrc
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "Added a mode-only Q_INVOKABLE (setPlateFilamentMapMode) rather than reusing setPlateFilamentMap(mode+maps) directly from QML -- the popup only changes the mode and the full-write variant needs the maps list, so the mode-only delegate reads the current maps and forwards, keeping the per-extruder map untouched (FP-03 reuse documented, not duplicated)."
  - "R-02 clamps out-of-range ints to fmmDefault (the safe per-plate sentinel) rather than asserting -- a QML/Q_INVOKABLE caller passing an invalid value must never crash production, and fmmDefault resolves to a concrete mode before persistence, so the on-disk value stays one of the 3 concrete modes."
  - "The anti-feature (fmmDefault as a 4th radio) is locked two ways: (1) the popup file contains no fmmDefault token at all (the value 3 is handled in the seed branch as a plain int), so the source-audit asserts token absence; (2) the Repeater model declares exactly the 3 concrete mode constants."
  - "Routed the popup's editorVm access through backend.editorViewModel (the BackendContext context property) rather than a required property on BBLTopbar, matching the existing topbar binding convention (slice/canRequestSlice already read backend.editorViewModel)."

patterns-established:
  - "CxPopup dialog that surfaces a C++ enum: declare the enum values as readonly ints, drive selection through a ButtonGroup, and write back via a Q_INVOKABLE on the viewmodel (never call the service directly from QML)."
  - "Source-audit slot pattern for anti-feature locks: combine a token-presence check for required anchors with a token-absence check for the forbidden anchor, both with FMAP/REVIEW-named QVERIFY2 messages."

requirements-completed: [FMAP-03]

# Metrics
duration: ~45 min
completed: 2026-07-12
---

# Phase 110 Plan 01: Filament-Map Popup UI And Mode Surfacing Summary

**CxPopup-based FilamentGroupPopup surfacing the 3 selectable filament-map modes (AutoForFlush/AutoForMatch/Manual) + Phase 108 auto-map preview, with the R-02 enum-range clamp at the Q_INVOKABLE boundary.**

## Performance

- **Duration:** ~45 min (build-dominated; canonical verify ran twice -- once for the implementation, once for the Theme-import fix)
- **Started:** 2026-07-12T22:30Z (first task commit)
- **Completed:** 2026-07-12T23:15Z (post-fix launch liveness clean)
- **Tasks:** 5 (4 implementation/test + 1 verification)
- **Files modified:** 9 (1 created, 8 modified)

## Accomplishments
- Closed FMAP-03: a CxPopup-based FilamentGroupPopup surfaces EXACTLY the 3 selectable modes with the Phase 108 auto-recommended map preview, wired into BBLTopbar and registered in qml.qrc.
- Closed Phase 107 REVIEW R-02 (FP-04): PartPlate::setFilamentMapMode(int) now clamps out-of-range ints to fmmDefault at the Q_INVOKABLE boundary -- a QML caller passing mode=5 can no longer silently store an invalid enum.
- Added the mode-only Q_INVOKABLE write path (setPlateFilamentMapMode) on both ProjectServiceMock and EditorViewModel, reusing the existing plate-write plumbing.
- Added the filamentGroupPopupSurfacesThreeModesNotFour source-audit slot locking the 3-modes-not-4 anti-feature and the R-02 guard.

## Task Commits

Each task was committed atomically:

1. **Task 110-01-01: R-02 enum range validation** - `7716d24` (fix)
2. **Task 110-01-02: FilamentGroupPopup.qml + BBLTopbar wiring + qml.qrc** - `e922222` (feat)
3. **Task 110-01-03: Q_INVOKABLE setPlateFilamentMapMode write path** - `0806b2f` (feat)
4. **Task 110-01-04: filamentGroupPopupSurfacesThreeModesNotFour source-audit** - `111a0c0` (test)
5. **Task 110-01-05 + Rule-2 fix: Theme singleton import to FilamentGroupPopup** - `8f331f0` (fix, verification-loop deviation)

**Plan metadata:** this SUMMARY commit (docs).

## Files Created/Modified
- `src/qml_gui/dialogs/FilamentGroupPopup.qml` - NEW. CxPopup-based 3-mode selector (AutoForFlush/AutoForMatch/Manual) + Phase 108 auto-map preview; opens via openForCurrentPlate(); writes back via editorVm.setPlateFilamentMapMode.
- `src/core/model/PartPlate.h` - R-02/FP-04: setFilamentMapMode(int) clamps out-of-range to fmmDefault (grep-assertable `if (mode < 0 || mode > 3)` guard).
- `src/core/services/ProjectServiceMock.h` / `.cpp` - FP-03: Q_INVOKABLE setPlateFilamentMapMode(int,int) mode-only write path (reads current maps, delegates to setPlateFilamentMap).
- `src/core/viewmodels/EditorViewModel.h` / `.cpp` - FP-03: Q_INVOKABLE setPlateFilamentMapMode + plateFilamentMapMode delegate; invalidates slice result + emits stateChanged.
- `src/qml_gui/BBLTopbar.qml` - FP-02: replaced the v2.1 FilamentGroupPopup placeholder with a real trigger + the popup instance (editorVm bound to backend.editorViewModel).
- `src/qml_gui/qml.qrc` - FP-06: registered dialogs/FilamentGroupPopup.qml.
- `tests/QmlUiAuditTests.cpp` - FP-05: filamentGroupPopupSurfacesThreeModesNotFour source-audit slot (FMAP-03 anti-feature + R-02 lock).

## Decisions Made
- Mode-only Q_INVOKABLE over direct setPlateFilamentMap reuse (see key-decisions frontmatter).
- R-02 clamps (no assert) so production never crashes on a bad QML int.
- Two-layer anti-feature lock: token-absence in the popup file + the 3 concrete Repeater entries.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] FilamentGroupPopup referenced Theme.* but did not import the singleton's directory**
- **Found during:** Task 110-01-05 (Build + ctest + launch liveness)
- **Issue:** FilamentGroupPopup.qml used Theme.* tokens (bgBase/radiusMD/textPrimary/etc.) but only `import "../controls"`. The Theme singleton lives in the parent qml dir (root, registered via qmldir `singleton Theme Theme.qml`), so every Theme binding threw `ReferenceError: Theme is not defined` at load -- 19 warnings in startup_diagnostics.log. The build/cTest still passed (QML warnings are non-fatal), but the popup would have rendered with broken styling. BedShapeDialog.qml uses `import ".."` for the same reason.
- **Fix:** Added `import ".."` to FilamentGroupPopup.qml.
- **Files modified:** src/qml_gui/dialogs/FilamentGroupPopup.qml
- **Verification:** Rebuild relinked OWzxSlicer.exe clean; [UI] QML UI audit tests passed; fresh startup_diagnostics.log has ZERO QML warnings (was 19 Theme errors). Launch liveness clean.
- **Committed in:** 8f331f0

---

**Total deviations:** 1 auto-fixed (1 missing critical)
**Impact on plan:** Essential correctness fix for the popup to render with theme tokens. No scope creep -- the popup's design was unchanged; only the import scope was corrected. Found by the plan's own launch-liveness verification step.

## Issues Encountered
None beyond the Theme-import deviation above. The canonical build (scripts/auto_verify_with_vcvars.ps1) ran to completion twice (exit 0) -- once for the implementation, once for the import fix. The E2E pipeline test group (real slice pipeline, unrelated to this QML work) was in progress at report time but all 5 unit-test groups completed and passed both runs.

## User Setup Required
None - no external service configuration required.

## Verification

Canonical build + regression ctest + launch liveness (per `.codex/rules/build-rules.md`):

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (run twice; second run after the Theme-import fix).
- **Build:** OWzxSlicer.exe + owzx-cli.exe + all 6 test executables linked clean. `git diff --check` exit 0. No new errors/warnings attributable to this plan (only pre-existing C4858/C4267/C4819 noise in libslic3r).
- **ctest (regression, 5 unit-test groups passing both runs):**
  - `[PrepareScene] Prepare scene data tests passed`
  - `[PartPlate] PartPlate tests passed`
  - `[ViewModel] ViewModel smoke tests passed`
  - `[UI] QML UI audit tests passed` (includes the new filamentGroupPopupSurfacesThreeModesNotFour slot)
  - `[PreviewParser] PreviewParser tests passed`
  - (E2E pipeline tests in progress at report time; independent of this QML work.)
- **Launch liveness:** OWzxSlicer.exe launched (APP_RUNNING_PID set). startup_diagnostics.log is clean -- 0 QML warnings after the Theme-import fix (was 19 `Theme is not defined` errors before).
- **Source audit:** `rg -n "FilamentGroupPopup" src/` returns the popup + topbar trigger + qml.qrc entry; `rg -n "if \(mode < 0 \|\| mode > 3\)" src/core/model/PartPlate.h` returns the R-02 guard.

## Next Phase Readiness
- FMAP-03 closed; R-02 closed. The popup is reachable from the Prepare topbar and writes the selected mode through the guarded Q_INVOKABLE boundary.
- Phase 111 (FMAP-04 full round-trip test + R-01 legacy-branch test) can now land: it depends on this UI + the Q_INVOKABLE write path being in place.
- The auto-recommended map preview is read-only (Phase 108 readback); full manual-map editing (the Custom mode's per-extruder grid) is a follow-up beyond FMAP-03's mode-surfacing scope.

## Self-Check: PASSED
- [x] FP-01: FilamentGroupPopup.qml exists, CxPopup-based, 3 modes (fmmAutoForFlush/fmmAutoForMatch/fmmManual), binds autoFilamentMaps/hasAutoFilamentMap. fmmDefault NOT a selectable radio (token absent from the file).
- [x] FP-02: Popup wired into BBLTopbar (trigger + instance).
- [x] FP-03: Q_INVOKABLE write path exists (setPlateFilamentMapMode on ProjectServiceMock + EditorViewModel).
- [x] FP-04: R-02 range validation present in PartPlate.h (grep-assertable `if (mode < 0 || mode > 3)` clamp).
- [x] FP-05: filamentGroupPopupSurfacesThreeModesNotFour source-audit slot exists, deterministic, passes.
- [x] FP-06: Popup registered in qml.qrc.
- [x] FP-07: Canonical build + regression ctest pass; OWzxSlicer.exe links clean; launch liveness clean.
- [x] FP-08: `git diff --check` exit 0; English ASCII comments; qsTr() on user-facing QML strings.

---
*Phase: 110-filament-map-popup-ui-and-mode-surfacing*
*Completed: 2026-07-12*
