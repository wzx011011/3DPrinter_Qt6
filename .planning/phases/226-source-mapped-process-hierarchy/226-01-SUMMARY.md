---
phase: 226-source-mapped-process-hierarchy
plan: 01
subsystem: ui
tags: [qt6, qml, process-settings, orcaslicer, source-truth]

requires:
  - phase: v5.10
    provides: Existing preset-backed SettingsDialog and ConfigOptionModel routes
provides:
  - Ordered six-page Process hierarchy derived from OrcaSlicer TabPrint
  - Page-qualified 36-group Process projection with fail-closed key membership
  - Process-only QML tabs and ordered group rendering
affects: [227-process-projection-disclosure-filtering, process-settings, preset-ui]

tech-stack:
  added: []
  patterns: [ordered C++ source manifest, Process-only QML projection, protected-hunk temporary index]

key-files:
  created: []
  modified:
    - src/qml_gui/Models/ConfigOptionModel.h
    - src/qml_gui/Models/ConfigOptionModel.cpp
    - src/qml_gui/dialogs/SettingsDialog.qml
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "Use one ordered C++ manifest as the Process page, group, and key authority."
  - "Keep Printer and Material on their existing projection paths."
  - "Defer disclosure, counts, and group actions to Phase 227."

patterns-established:
  - "Process hierarchy: derive lookup and schema keys from one ordered TabPrint manifest."
  - "QML boundary: consume page/group/index invokables without recreating mapping rules."

requirements-completed: [HIER-01, HIER-02]

duration: 1h 20m active recovery
completed: 2026-08-11
---

# Phase 226 Plan 01: Source-Mapped Process Hierarchy Summary

**The Process dialog now consumes the locked OrcaSlicer six-page, 36-group hierarchy in source order with no category or Others fallback.**

## Performance

- **Resumed:** 2026-08-11T17:45:00+08:00
- **Completed:** 2026-08-11T19:00:00+08:00
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Replaced the legacy Process hash/category authority with one ordered C++ manifest transcribed from `Tab.cpp:2005-2376`.
- Exposed Process-only page, group, and ordered-row invokables while preserving Printer and Material behavior.
- Rendered six equal-width Process tabs and only source-ordered non-empty groups through existing `OptionRow` delegates.
- Added independent C++ and QML audit coverage for all pages, 36 page-qualified groups, ordered key membership, and legacy-page rejection.

## Task Commit

The interrupted execution was recovered as one protected phase commit:

1. **Tasks 1-2: C++ manifest and Process QML projection** - `17ca3b1`

## Files Created/Modified

- `src/qml_gui/Models/ConfigOptionModel.h` - Process hierarchy invokable API.
- `src/qml_gui/Models/ConfigOptionModel.cpp` - Ordered source manifest, derived lookup, and fail-closed projection.
- `src/qml_gui/dialogs/SettingsDialog.qml` - Process-only six-tab and ordered static-group presentation.
- `tests/ViewModelSmokeTests.cpp` - Independent full hierarchy and key projection regression test.
- `tests/QmlUiAuditTests.cpp` - Process QML contract and legacy-route rejection audit.

## Decisions Made

- Process order is never derived from `QHash`, labels, categories, or alphabetical sorting.
- Unmapped Process keys remain outside the projection instead of being repaired into `Others`.
- Static headings intentionally replace the decorative `+` control in this phase; real disclosure state belongs to Phase 227.

## Deviations from Plan

### Auto-fixed Issues

**1. Visible Process rows required an explicit delegate height**
- **Found during:** Fixed-size visual inspection.
- **Issue:** Repeater-created `OptionRow` delegates had zero height.
- **Fix:** Bound the Process delegate height to `totalHeight`.
- **Verification:** Canonical build/test/launch gate and focused QML audit passed.
- **Committed in:** `17ca3b1`

**2. Protected-index validation required Windows Git compatibility corrections**
- **Found during:** Phase-only commit gate.
- **Issue:** Local Git lacked `diff --no-index --label`; two PowerShell assertions mishandled indentation and `MatchCollection`; an empty pre-existing staged patch was not a valid `git apply` input.
- **Fix:** Generated symmetric baseline/current patches with `-p2`, retained exact anchor/order checks, and skipped only the zero-byte staged patch.
- **Verification:** Five expected files, four test anchors, user-hunk exclusion, `git diff --check`, and the encoding guard all passed before `commit-tree`.
- **Committed in:** Commit plumbing only; no additional product files changed.

**Total deviations:** 2 auto-fixed blocking validation/presentation issues.
**Impact on plan:** No feature scope expansion; all HIER-01/HIER-02 guarantees remain enforced.

## Verification

- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` completed its single-instance build, automated-test, and launch path before phase commit recovery.
- `build/ViewModelSmokeTests.exe sourceMappedProcessHierarchyMatchesTabPrint` passed.
- `build/QmlUiAuditTests.exe processSettingsConsumesSourceMappedHierarchy` passed.
- Global encoding guard passed against the five-file temporary index.
- Captured pre-existing user hunks in both shared test files remain unstaged in the working tree.

## User Setup Required

None.

## Next Phase Readiness

- Phase 227 can add page-qualified disclosure state, counts, search reveal, and mode refresh on top of stable C++ page/group identity.
- No blocking issues remain for the next phase.

## Self-Check: PASSED

- Commit `17ca3b1` contains exactly the five declared phase files.
- HIER-01 and HIER-02 focused tests pass.
- User-owned dirty hunks remain outside the phase commit.

---
*Phase: 226-source-mapped-process-hierarchy*
*Completed: 2026-08-11*
