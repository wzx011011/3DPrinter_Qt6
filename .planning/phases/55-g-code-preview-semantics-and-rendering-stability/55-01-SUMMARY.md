---
phase: 55-g-code-preview-semantics-and-rendering-stability
plan: 01
subsystem: testing
tags: [qttest, gcode-preview, parser, fixture, cmake, vcvars]

# Dependency graph
requires: []
provides:
  - "Committed OrcaSlicer-style G-code fixture (tests/fixtures/orca_sample.gcode) with 12 distinct ;TYPE: roles, 2 layers, travel moves, tool change, and tagged comments (;HEIGHT/WIDTH/FEEDRATE/FAN/TEMP/ACCEL/LAYER)"
  - "PreviewParserTests QtTest target registered in CMakeLists.txt and runnable via ctest -R PreviewParser"
  - "Four-slot RED/GREEN scaffold: fixture-presence (GREEN), roleForType (RED-by-skip), 17-mode viewModes (RED-by-skip), Summary legend sentinel (RED-by-skip)"
  - "Hardened canonical build script (PATH sanitize + Windows Kits UCRT fallback) so vcvars64.bat survives a polluted PATH"
affects: [55-02, 55-04, 55-05]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Single-file cpp-internal Q_OBJECT QtTest requires both QTEST_MAIN(<Class>) and #include \"<Class>.moc\" at the bottom (matches QmlUiAuditTests/ViewModelSmokeTests/PartPlateTests/PrepareSceneDataTests/E2EWorkflowTests)"
    - "RED-by-skip via QSKIP for Plan-02-dependent contract assertions (auto-flips to real assertions when Plan 02 implements the API)"

key-files:
  created:
    - tests/fixtures/orca_sample.gcode
    - tests/PreviewParserTests.cpp
  modified:
    - CMakeLists.txt
    - scripts/auto_verify_with_vcvars.ps1

key-decisions:
  - "Use QSKIP (not QEXPECT_FAIL) for the three Plan-02-dependent test slots. QEXPECT_FAIL only covers the immediately-following assertion, so multi-assertion contract checks (e.g. nine viewModes contains() checks) leaked hard failures. QSKIP exits the test body cleanly until Plan 02 implements the API, then Plan 02 removes the QSKIP to make the assertions real."
  - "PreviewParserTests is linked against owzx_app_core + Qt6 modules (same pattern as ViewModelSmokeTests/PartPlateTests) because it constructs the real PreviewViewModel via SliceService."
  - "Add PreviewParserTests to the canonical build script's target whitelist and add a PreviewParser test-execution block, mirroring the existing PrepareScene/PartPlate/ViewModel/QmlUiAudit blocks — the plan requires the canonical build to compile/link/run the new target."

patterns-established:
  - "Canonical build hardening: sanitize PATH entries containing both spaces and parentheses before invoking vcvars64.bat, and add a Windows Kits UCRT/UM fallback when vcvars's findstr-based SDK detection silently fails. This is the documented fix for C1083 'cannot open <vector>/<stdio.h>' in polluted-PATH environments."

requirements-completed: [GCODE-01, GCODE-02]

# Metrics
duration: ~45min
completed: 2026-07-02
---

# Phase 55 Plan 01: Preview Parser Test Foundation Summary

**OrcaSlicer-style G-code fixture + registered PreviewParserTests target (four RED/GREEN scaffold slots) so Plan 02's 20-role parser, 17-mode viewModes, and Summary legend have a deterministic place to land assertions**

## Performance

- **Duration:** ~45 min (resume of an in-progress plan; Task 1 fixture was already committed by a prior session)
- **Completed:** 2026-07-02
- **Tasks:** 2
- **Files modified:** 4 (1 fixture, 1 new test, CMakeLists, build script)

## Accomplishments
- Committed deterministic OrcaSlicer-style G-code fixture (12 `;TYPE:` roles, 2 layers, travel moves, T0/T1 tool change, all 7 tagged-comment types) — the no-placeholder real-data anchor for GCODE-01 in Plan 04.
- Registered PreviewParserTests QtTest target in CMakeLists.txt, linked against owzx_app_core so Plan 02 can assert against the real PreviewViewModel.
- Four-slot test scaffold landed: fixture-presence is GREEN (guards the fixture from being shrunk); roleForType / 17-mode-viewModes / Summary-legend are RED-by-skip, ready for Plan 02 to flip green.
- Hardened the canonical build script against a polluted PATH (VMware Workstation entry broke vcvars64.bat) so the build is reproducible in this environment.

## Task Commits

Each task was committed atomically:

1. **Task 1: Commit OrcaSlicer-style G-code fixture** — `c4a35cf` (test) — committed by a prior session; verified against all Task 1 acceptance criteria (12 ;TYPE: roles, 2 layers, ≥4 G0 travels, T0/T1 tool change, all 7 tagged comments, ASCII-only, 66 non-comment lines).
2. **Task 2: Register PreviewParserTests target with RED scaffold** — `22ca25c` (test) — PreviewParserTests.cpp (four slots, QTEST_MAIN + .moc), CMakeLists.txt registration, build-script target whitelist + test-execution block, plus the vcvars PATH/UCRT hardening needed to actually verify it.

## Files Created/Modified
- `tests/fixtures/orca_sample.gcode` — OrcaSlicer-style G-code fixture: 12 `;TYPE:` roles (Inner/Outer/Overhang wall, Sparse/Internal solid infill, Top surface, Bridge, Gap infill, Skirt, Support, Support interface, Prime tower), 2 layers, 10 travel moves, T0/T1 tool change, `;HEIGHT/WIDTH/FEEDRATE/FAN/TEMP/ACCEL/LAYER` tags.
- `tests/PreviewParserTests.cpp` — Four-slot QtTest scaffold. `test_fixture_has_expected_role_coverage` (GREEN) asserts the fixture carries the required role strings + both layers. `test_role_string_mapping_covers_upstream_enum` (RED-by-skip) will assert `roleForType("Inner wall")==1` once Plan 02 implements the Q_INVOKABLE mapper. `test_view_modes_match_upstream_seventeen` (RED-by-skip) will assert viewModes().size()==17 plus the 9 required display names once Plan 02 aligns to upstream EViewType. `test_summary_mode_has_no_gradient_legend` (RED-by-skip, compile-guarded `#if 0`) will assert the Summary-mode legend sentinel once Plan 02 defines it.
- `CMakeLists.txt` — `qt_add_executable(PreviewParserTests ...)` + `target_link_libraries(... owzx_app_core Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent Qt6::Network Qt6::Test)` + `target_compile_definitions(... QT_TESTCASE_SOURCEDIR)` + `target_link_options(... /FORCE:MULTIPLE)` + `add_test` + `set_tests_properties(... ENVIRONMENT "PATH=...")`, placed immediately after the PartPlateTests block.
- `scripts/auto_verify_with_vcvars.ps1` — Added PreviewParserTests to the build target whitelist and a `[PreviewParser]` test-execution block; hardened vcvars invocation (see deviations).

## Decisions Made
- **QSKIP over QEXPECT_FAIL for multi-assertion RED slots.** The in-tree draft used QEXPECT_FAIL, which only neutralizes the *next* assertion; the nine `viewModes().contains()` checks after it ran as hard failures (e.g. "Actual Speed" is absent from the current 13-mode list). Rewrote both Plan-02-dependent slots as `if (precondition-not-met) QSKIP(...)`, which exits the test body cleanly and lets Plan 02 simply delete the QSKIP to activate the real assertions. This matches the Summary-legend slot's existing `#if 0 / QSKIP` pattern.
- **QTEST_MAIN + .moc include required.** A single-file cpp-internal Q_OBJECT test will not link without `QTEST_MAIN(PreviewParserTests)` (LNK2001 unresolved `main`) and the AutoMoc-generated `#include "PreviewParserTests.moc"` (AutoMoc error: Q_OBJECT without moc include). Added both, matching every sibling single-file QtTest.
- **Canonical script carries the env hardening.** The vcvars PATH/UCRT fix is committed in the build script (not a local workaround) so the build is reproducible for everyone on this VM and for CI.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Missing critical gap] Canonical build script did not build or run PreviewParserTests**
- **Found during:** Task 2 (register PreviewParserTests target)
- **Issue:** `scripts/auto_verify_with_vcvars.ps1` has an explicit `ninja` target whitelist (lines 100-107) that did not include PreviewParserTests, and there was no test-execution block for it. The plan's Task 2 acceptance criteria explicitly require the canonical build to compile/link the new target and `ctest -R PreviewParser` to run it — both impossible without registering the target in the script.
- **Fix:** Added `Invoke-NinjaTarget 'PreviewParserTests'` to the build section and a `[PreviewParser]` test-execution block mirroring the existing PrepareScene/PartPlate/ViewModel/QmlUiAudit blocks.
- **Files modified:** scripts/auto_verify_with_vcvars.ps1
- **Verification:** build7 `scripts/auto_verify_with_vcvars.ps1` exit 0; `[PreviewParser] PreviewParser tests passed`.
- **Committed in:** `22ca25c` (part of Task 2 commit)

**2. [Rule 1 - Environmental blocker] vcvars64.bat broken by polluted PATH (VMware Workstation)**
- **Found during:** Task 2 verification (canonical build would not run)
- **Issue:** The system PATH contained `F:\Program Files (x86)\VMware\VMware Workstation\bin` (plus three other space+paren entries). vcvars64.bat's weak batch quoting aborts on these with `\VMware\VMware was unexpected at this time`, exiting BEFORE exporting INCLUDE/LIB (`ENV_VARS_SET=0, INCLUDE_LEN=0`). Every `cl.exe` then failed with `C1083: cannot open <vector>` on the first C++ STL compile (libslic3r_cgal). This was pre-existing (confirmed by reproducing with my script edits stashed) and blocked the entire canonical build, not just Plan 55-01.
- **Fix:** Before invoking vcvars, sanitize PATH by dropping entries containing both spaces AND parentheses (system PATH untouched). After vcvars, detect whether UCRT is missing from INCLUDE (vcvars's findstr-based Windows-SDK detection silently fails even after the PATH sanitize) and append the discovered Windows Kits `shared/ucrt/um` includes + `ucrt/um` x64 libs + x64 bin path explicitly.
- **Files modified:** scripts/auto_verify_with_vcvars.ps1
- **Verification:** build7 compiled libslic3r_cgal, libslic3r_from_source, OWzxSlicer, and all test targets cleanly; `[vcvars] Patched Windows Kits 10.0.26100.0 paths into INCLUDE/LIB` logged; full canonical verify exit 0.
- **Committed in:** `22ca25c` (part of Task 2 commit)

**3. [Rule 1 - Bug] PreviewParserTests.cpp missing QTEST_MAIN and .moc include**
- **Found during:** Task 2 build (link failure LNK2001 main; AutoMoc error)
- **Issue:** The in-tree test draft declared `Q_OBJECT` but had neither `QTEST_MAIN(PreviewParserTests)` (entry point) nor `#include "PreviewParserTests.moc"` (meta-object). Both are required for a single-file cpp-internal Q_OBJECT QtTest — every sibling test (QmlUiAuditTests, ViewModelSmokeTests, PartPlateTests, PrepareSceneDataTests, E2EWorkflowTests) carries both.
- **Fix:** Added `QTEST_MAIN(PreviewParserTests)` and `#include "PreviewParserTests.moc"` at the bottom.
- **Files modified:** tests/PreviewParserTests.cpp
- **Verification:** PreviewParserTests.exe links and runs.
- **Committed in:** `22ca25c` (part of Task 2 commit)

---

**Total deviations:** 3 auto-fixed (Rule 1 × 3 — two build/gate gaps and one environmental blocker that prevented any verification).
**Impact on plan:** Deviation 1 is a legitimate plan-gap fix (the plan requires canonical-build verification but the script had a hardcoded whitelist). Deviation 2 is an environmental fix outside the plan's scope but required to make *any* canonical build succeed in this VM; it is committed in the build script so the environment is reproducible. Deviation 3 is a required mechanical fix for the test scaffold to link. No scope creep; all three were necessary for Task 2's acceptance criteria to pass.

## Issues Encountered
- **Safe-resume on entry.** A prior session had committed the Task 1 fixture (`c4a35cf`) and left Task 2 source uncommitted in the working tree (PreviewParserTests.cpp + CMakeLists.txt registration). Confirmed via the safe-resume gate that this was a clean continuation (not an ambiguous partial state): verified the in-tree test draft matched the plan's four-slot design, verified the fixture against all Task 1 acceptance criteria, then completed Task 2 (with the three deviations above) rather than reverting.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 55-02 has a deterministic place to land `roleForType(QString)`, the 17-mode `viewModes()` list, and the Summary-mode legend sentinel: remove the three `QSKIP` guards in PreviewParserTests.cpp and make the assertions real.
  - Note for Plan 02 executor: Plan 55-02 Task 2 step 9 says "remove the QEXPECT_FAIL wrappers"; the actual mechanism is now `QSKIP` (see key-decisions). Remove the `QSKIP` blocks instead.
- Plan 55-04 has the fixture path (`tests/fixtures/orca_sample.gcode`) it needs for the GCODE-01 no-placeholder live-slice assertion.
- The canonical build (`scripts/auto_verify_with_vcvars.ps1`) is green again after the vcvars hardening — Plans 55-02 through 55-05 can verify against it.

## Self-Check: PASSED
- Task 1 acceptance: 12 `;TYPE:` blocks, all 12 required role strings present, 2 `;LAYER:`, all 7 tagged-comment types, 10 `G0` travels, T0/T1 tool change, ASCII-only, 66 non-comment lines — all PASS.
- Task 2 acceptance: file exists with `class PreviewParserTests final : public QObject` + `Q_OBJECT`; CMakeLists has `qt_add_executable(PreviewParserTests` + `add_test(NAME PreviewParserTests`; canonical build `scripts/auto_verify_with_vcvars.ps1` exit 0; `ctest -R PreviewParser` Test #6 Passed (fixture presence GREEN, three Plan-02-dependent tests SKIP, target exits 0) — all PASS.
- `git log --oneline --grep="55-01"` returns 2 commits (`c4a35cf`, `22ca25c`).

---
*Phase: 55-g-code-preview-semantics-and-rendering-stability*
*Completed: 2026-07-02*
