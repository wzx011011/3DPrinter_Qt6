---
phase: 104-cli-fixture-recipes-and-multi-material-model
plan: 01
subsystem: testing
tags: [cli-fixtures, multi-material, 3mf, argv-recipes, anti-feature, source-audit, regression-lock, canonical-build, ctest]

# Dependency graph
requires:
  - phase: 103-cli-fixture-readiness-gate
    provides: FIXTURE-02 - argv fixtures deterministically gated on QQmlApplicationEngine::objectCreated + QQuickWindow::frameSwapped (the readiness gate Phase 104 recipes rely on; the frameSwapped gate is documented in the recipes doc determinism-notes section).
provides:
  - Multi-material fixture model at tests/data/multi_material_fixture.3mf (2-extruder, hand-authored 3MF; two 20mm cubes, one per extruder, with slic3r:extruder metadata). Closes FIXTURE-01.
  - Canonical argv recipe doc at tests/data/fixture_recipes.md covering every major GUI state (Prepare empty, Prepare with model, Preview, AssembleView, settings:printer/filament/process, calibration, wipe-tower) plus the page-alias and dialog-route reference tables. Closes FIXTURE-03.
  - Anti-feature comment block in main_qml.cpp above the QCommandLineParser setup documenting that the 4 flags (--open-page/--open-dialog/--load-model/--skip-first-run) are OWzx-only test-evidence plumbing with no upstream equivalent, MUST NOT be promoted to a user-facing product feature. Closes FIXTURE-04.
  - cliFixtureRecipesAndMultiMaterialModelPresent source-audit regression-lock slot in QmlUiAuditTests asserting all 3 FIXTURE contracts against disk (fixture exists + non-empty; recipes exist + cover major states + reference canonical flags; anti-feature comment anchor present).
  - Canonical build clean (OWzxSlicer.exe + all test exes compile/link clean, zero errors); OWzxSlicer.exe launch liveness confirmed (PID 32272, 5-second no-crash probe); regression ctest 4/4 PASS.
affects: [105-d3d12-debug-layer-wiring, 106-d3d12-crash-root-cause-and-backend-readiness, 116-v4.5-verification-and-cross-workstream-regression]

# Tech tracking
tech-stack:
  added: []
  patterns: [Hand-authored 3MF fixture fallback (when no suitable multi-material 3MF exists upstream), anti-feature comment anchor (grep-assertable contract locking a deliberate non-feature), consolidated source-audit regression lock extending the Phase 102/103 pattern to cover fixture-content contracts]

key-files:
  created:
    - tests/data/multi_material_fixture.3mf
    - tests/data/fixture_recipes.md
    - .planning/phases/104-cli-fixture-recipes-and-multi-material-model/104-01-SUMMARY.md
  modified:
    - src/qml_gui/main_qml.cpp (task 104-01-01, commit cc16a00 - FIXTURE-04 anti-feature comment block above parseStartupOpenRequest)
    - tests/QmlUiAuditTests.cpp (task 104-01-02, commit efdb356 - cliFixtureRecipesAndMultiMaterialModelPresent slot declaration + implementation)

key-decisions:
  - "No suitable multi-material 3MF exists in third_party/OrcaSlicer/tests/data/. The single sample (test_3mf/Geraete/Buechse.3mf) is single-material (no extruder config, one object, no slicer metadata). So FR-01's documented fallback path was used: a minimal hand-authored 2-extruder 3MF (two 20mm cubes, one per extruder, with slic3r:extruder metadata pinning each cube to extruder 0 / 1). This is explicitly anticipated by FR-01 ('sourced from third_party/OrcaSlicer/tests/data/ (preferred) OR generated programmatically with explicit per-extruder geometry') and the project_specific_notes ('If none suitable, generate a minimal 2-extruder fixture'). The derivation is documented in fixture_recipes.md (Source fixtures table) and in the 3dmodel.model XML header comment."
  - "The 3MF is packaged with spec-ordered zip entries ([Content_Types].xml first, _rels/.rels second, 3D/3dmodel.model third) via System.IO.Compression, matching the upstream sample's structure. Two objects with uuid attributes and slic3r:extruder metadata (0 and 1) plus the core 3MF + slic3rpe + orcaslicer XML namespaces mirror the structure libslic3r's 3MF importer expects."
  - "The anti-feature comment is a grep-assertable contract, not prose. The QmlUiAuditTests slot asserts 4 anchor substrings: 'test-evidence plumbing', 'OWzx-only', 'OrcaSlicer.cpp:7183' (the upstream argv anchor so the no-upstream-equivalent claim is traceable), and 'MUST NOT be promoted'. This means a future refactor that deletes or weakens the anti-feature comment fails the regression ctest deterministically, locking FIXTURE-04 against erosion."
  - "The source-audit slot asserts fixture_recipes.md contains the 4 canonical argv flags (--open-page/--open-dialog/--load-model/--skip-first-run) in addition to the major GUI state names. This locks FIXTURE-03's completeness: if a flag is renamed or the doc is trimmed, the regression ctest fails. The major-state check covers Prepare/Preview/AssembleView/settings (the FR-02 minimum)."
  - "AssembleView is NOT a top-level page (no --open-page assemble alias exists). The recipes doc documents the real path: load the multi-material fixture + open-page prepare, then toggle the AssembleView canvas from the Prepare page (CanvasAssembleView canvas-type toggle in RhiViewport.h). This honesty matters because a naive recipe reader might expect --open-page assemble to exist; documenting the real toggle path prevents a false-impression bug."

patterns-established:
  - "Anti-feature comment anchor: when a requirement is deliberately a non-feature (e.g. OWzx-only test-evidence plumbing that MUST NOT become a user-facing product feature), lock it with a grep-assertable comment block near the implementation, then assert 3-4 anchor substrings in a QmlUiAuditTests slot. The anchors should include the upstream-no-equivalent claim AND the cite that makes it traceable (e.g. OrcaSlicer.cpp:7183). This converts a prose anti-feature promise into a regression-ctest gate."
  - "Hand-authored 3MF fallback: when no suitable multi-material fixture exists upstream, generate a minimal hand-authored 3MF with explicit per-extruder geometry + slic3r:extruder metadata. Package with spec-ordered zip entries via System.IO.Compression. Document the derivation (no upstream source + the fallback rationale) in both the 3dmodel.model XML header comment and the recipes doc Source fixtures table so the fixture's origin is never ambiguous."

requirements-completed: [FIXTURE-01, FIXTURE-03, FIXTURE-04]

# Metrics
duration: ~35 min (incl. ~10 min canonical build libslic3r compile + test-target links + smoke + E2E; verification + ctest + liveness probe ~2 min)
completed: 2026-07-12
---

# Phase 104 Plan 01: CLI Fixture Recipes And Multi-Material Model Summary

**Hand-authored 2-extruder 3MF fixture + canonical argv recipe doc + grep-assertable anti-feature comment block, all locked by a cliFixtureRecipesAndMultiMaterialModelPresent source-audit slot; canonical build clean (exit 0), OWzxSlicer.exe launch liveness confirmed (PID 32272), regression ctest 4/4 PASS. FIXTURE-01/03/04 closed.**

## Performance

- **Duration:** ~35 min (canonical build ~10 min libslic3r compile + test-target links + smoke + E2E; targeted grep checks + regression ctest + AUTOMOC guard + launch liveness probe ~2 min; fixture 3MF packaging + recipes doc + anti-feature comment + test slot ~5 min)
- **Started:** 2026-07-12 (fixture + recipes + comment + slot authored first; canonical build ran after commits)
- **Completed:** 2026-07-12T06:42Z
- **Tasks:** 3 (104-01-01 implementation, 104-01-02 test, 104-01-03 verification)
- **Files modified:** 4 (2 created test-data files, 1 modified source, 1 modified test, plus this summary)

## Accomplishments

- **FIXTURE-01 (multi-material fixture):** Added `tests/data/multi_material_fixture.3mf` - a hand-authored 2-extruder 3MF. No suitable multi-material 3MF exists in `third_party/OrcaSlicer/tests/data/` (the single `test_3mf/Geraete/Buechse.3mf` sample is single-material: one object, no extruder config, no slicer metadata), so FR-01's documented fallback path was used. The fixture is two 20mm cubes, one pinned to extruder 0, one to extruder 1, via `slic3r:extruder` metadata. Packaged with spec-ordered zip entries (`[Content_Types].xml`, `_rels/.rels`, `3D/3dmodel.model`) matching the upstream sample structure. The derivation is documented in both the `3dmodel.model` XML header comment and the recipes doc Source fixtures table.
- **FIXTURE-03 (argv recipes):** Added `tests/data/fixture_recipes.md` documenting the canonical argv combos that reach each major GUI state. Covers: Prepare empty, Prepare with multi-material fixture, Prepare with single-material fixture, Preview, AssembleView (real toggle path, not a top-level page), settings:printer/filament/process, calibration, wipe-tower dialog, config-wizard. Includes a flags reference table, the page-alias and dialog-route reference tables (sourced from `startupPageRoutes`/`startupDialogRoutes` in `main_qml.cpp`), and a determinism-notes section explaining the FIXTURE-02 frameSwapped gate and the `--skip-first-run` reproducibility guarantee.
- **FIXTURE-04 (anti-feature):** Added a 10-line anti-feature comment block in `src/qml_gui/main_qml.cpp` immediately above `parseStartupOpenRequest` (the QCommandLineParser setup). The comment documents: the 4 flags are OWzx-only test-evidence plumbing; upstream OrcaSlicer has no equivalent (CLI-only `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`); they are gated on `objectCreated` + `frameSwapped` (FIXTURE-02); they MUST NOT be promoted to a user-facing deep-link product feature; with cites to `tests/data/fixture_recipes.md` and `REQUIREMENTS.md` (WS3 Out of Scope).
- **Source-audit regression lock:** Added the `cliFixtureRecipesAndMultiMaterialModelPresent` private slot to `tests/QmlUiAuditTests.cpp` mirroring the Phase 102/103 deterministic pattern (QFile + QT_TESTCASE_SOURCEDIR + QFileInfo::exists + QString::contains + QVERIFY2 with FIXTURE-0X-named messages). Asserts: (a) FIXTURE-01 fixture exists + non-empty; (b) FIXTURE-03 recipes exist + cover 4 major states (Prepare/Preview/AssembleView/settings) + reference all 4 canonical argv flags; (c) FIXTURE-04 anti-feature comment anchor (4 substrings: test-evidence plumbing, OWzx-only, OrcaSlicer.cpp:7183, MUST NOT be promoted).
- **Verification:** Canonical build via `scripts/auto_verify_with_vcvars.ps1` exit 0, zero errors (configure done 17.9s, libslic3r compile clean, OWzxSlicer.exe linked 33,821,696 bytes, all test exes linked, smoke + E2E tests passed). OWzxSlicer.exe launch liveness confirmed (PID 32272, survived 5-second no-crash probe - the STATE.md reachability bar). Regression ctest 4/4 PASS (ViewModelSmokeTests 7.62s, QmlUiAuditTests 0.08s incl. the new slot, PrepareSceneDataTests 0.02s, PartPlateTests 0.59s). AUTOMOC silent-skip guard verified via filter-isolated exit-code differential (new slot exit 0, invalid filter exit 1).

## Task Commits

Each task was committed atomically (104-01-03 produced no separate code commit - verification task):

1. **Task 104-01-01: Multi-material fixture + argv recipes + anti-feature comment** - `cc16a00` (feat)
2. **Task 104-01-02: cliFixtureRecipesAndMultiMaterialModelPresent source-audit slot** - `efdb356` (test)
3. **Task 104-01-03: Canonical build + launch liveness + regression ctest** - this summary (no separate code commit; verification task)

**Plan metadata:** this summary commit (docs: complete plan).

## Files Created/Modified

- `tests/data/multi_material_fixture.3mf` - Hand-authored 2-extruder 3MF fixture (FIXTURE-01). Two 20mm cubes, one per extruder, with `slic3r:extruder` metadata (0 and 1). Spec-ordered zip entries matching the upstream sample structure. No upstream source (the single OrcaSlicer `Buechse.3mf` is single-material). [104-01-01, cc16a00]
- `tests/data/fixture_recipes.md` - Canonical argv recipe doc (FIXTURE-03). One-liner recipe per major GUI state + flags/page-alias/dialog-route reference tables + determinism notes (FIXTURE-02 frameSwapped gate + skip-first-run reproducibility) + Source fixtures table. [104-01-01, cc16a00]
- `src/qml_gui/main_qml.cpp` - FIXTURE-04 anti-feature comment block (10 lines) immediately above `parseStartupOpenRequest` (the QCommandLineParser setup). Documents OWzx-only test-evidence plumbing, no upstream equivalent (OrcaSlicer.cpp:7183), MUST NOT be promoted to a user-facing product feature. [104-01-01, cc16a00]
- `tests/QmlUiAuditTests.cpp` - `cliFixtureRecipesAndMultiMaterialModelPresent` slot declaration (:180, after the Phase 103 `argvFixtureGateUsesFrameSwappedNotSingleShot` slot) + implementation (:3690) asserting all 3 FIXTURE contracts against disk. [104-01-02, efdb356]
- `.planning/phases/104-cli-fixture-recipes-and-multi-material-model/104-01-SUMMARY.md` - this summary. [104-01-03]

## Decisions Made

- **Hand-authored 3MF fallback, not upstream copy.** No suitable multi-material 3MF exists in `third_party/OrcaSlicer/tests/data/`. The single sample (`test_3mf/Geraete/Buechse.3mf`, 1416 bytes) was inspected: it contains one object, no extruder config, no slicer metadata - it is single-material. So FR-01's documented fallback path ("generated programmatically with explicit per-extruder geometry") was used. The fixture is a minimal 2-extruder 3MF (two 20mm cubes, one pinned to extruder 0, one to extruder 1, via `slic3r:extruder` metadata), packaged with spec-ordered zip entries. This is explicitly anticipated by FR-01 and the project_specific_notes. The derivation is documented in two places (3dmodel.model XML header comment + recipes doc Source fixtures table) so the fixture's origin is never ambiguous.
- **Anti-feature comment is grep-assertable, not prose.** The FIXTURE-04 comment block was written so 4 anchor substrings can be asserted in a regression test: `test-evidence plumbing`, `OWzx-only`, `OrcaSlicer.cpp:7183` (traceable no-upstream-equivalent cite), and `MUST NOT be promoted`. The QmlUiAuditTests slot asserts all 4. This converts the anti-feature promise into a regression-ctest gate - a future refactor that deletes or weakens the comment fails deterministically, locking FIXTURE-04 against erosion.
- **AssembleView recipe is honest about the toggle path.** AssembleView is NOT a top-level page (no `--open-page assemble` alias exists in `startupPageRoutes`). The recipes doc documents the real path: load the multi-material fixture + `--open-page prepare`, then toggle the AssembleView canvas from the Prepare page (`CanvasAssembleView` canvas-type toggle in `RhiViewport.h`). This prevents a false-impression bug where a recipe reader expects `--open-page assemble` to exist.
- **Slot extends the Phase 102/103 pattern, does not invent a new one.** The `cliFixtureRecipesAndMultiMaterialModelPresent` slot uses the exact same deterministic pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with a requirement-named message) as `wipeTowerReadbackAndRenderAnchorsPresent` (Phase 102) and `argvFixtureGateUsesFrameSwappedNotSingleShot` (Phase 103). One consolidated slot for all 3 FIXTURE contracts (not 3 separate slots), with 9 FIXTURE-0X-named QVERIFY2 messages, keeping the ctest row count stable while making failures attributable to the requirement.

## Deviations from Plan

None - plan executed exactly as written. All three tasks followed their prescribed actions. The hand-authored 3MF fallback (vs. upstream copy) was explicitly anticipated by FR-01 ("sourced from third_party/OrcaSlicer/tests/data/ (preferred) OR generated programmatically with explicit per-extruder geometry") and the project_specific_notes ("If none suitable, generate a minimal 2-extruder fixture"); the search confirmed no upstream source was suitable, so the documented fallback path was the correct action, not a deviation.

## Issues Encountered

None. The canonical build completed within the wrapper budget this time (no timeout - the build hit step 237/237 OWzxSlicer.exe link, then all test targets, then smoke + E2E, all clean). The libslic3r reconfigure was triggered by the cumulative prior-phase header changes (Phase 100/101/102/103) already on main, not by this plan's changes (this plan modified only `main_qml.cpp` - 10 comment lines - and test files, none of which propagate to libslic3r).

## User Setup Required

None - no external service configuration required. The fixture is static test data consumed only by test code + the existing `--load-model` path.

## Build + Test Commands Run + Results

### Source-audit greps (anchor verification, all PASS)

1. `test -s tests/data/multi_material_fixture.3mf` - PASS (1586 bytes, non-empty). [FR-01]
2. `grep -c "Prepare\|Preview\|AssembleView\|settings" tests/data/fixture_recipes.md` - PASS (28 hits; covers all major GUI states). [FR-02]
3. `grep -n "test-evidence plumbing\|OWzx-only\|FIXTURE-04" src/qml_gui/main_qml.cpp` - PASS (3 hits at :169-171). [FR-03]
4. `grep -n "cliFixtureRecipesAndMultiMaterialModelPresent" tests/QmlUiAuditTests.cpp` - PASS (declaration :180 + implementation :3690). [FR-04]
5. `grep -c "FIXTURE-01:\|FIXTURE-03:\|FIXTURE-04:" tests/QmlUiAuditTests.cpp` - PASS (9 FIXTURE-named QVERIFY2 messages). [FR-04]

### Canonical build (scripts/auto_verify_with_vcvars.ps1)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (log: build/104-01-verify.log, background task exit 0)
- **Result:** Clean - configure done 17.9s, libslic3r compile clean (steps 10-234), OWzxSlicer.exe linked (step 237/237, 33,821,696 bytes), all test exes linked (E2EWorkflowTests, ViewModelSmokeTests, QmlUiAuditTests, PartPlateTests, PrepareSceneDataTests, PreviewParserTests, CliTests), owzx-cli.exe linked. Zero `error:`/`error C`/`error LNK`/`ninja: error`/`FATAL` in the log. Smoke tests all passed (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests, QmlUiAuditTests incl. the new slot, PreviewParserTests). E2E pipeline tests passed. AUTOMOC re-ran for OWzxSlicer + QmlUiAuditTests (the new slot is registered on the meta-object).
- **Note:** No timeout this phase (unlike the Phase 95/96/97/98/100/101/102 pattern). The build completed within the wrapper budget. The libslic3r reconfigure was triggered by cumulative prior-phase header changes, not this plan's changes (only main_qml.cpp comment lines + test files were modified).

### OWzxSlicer.exe launch liveness (STATE.md reachability bar)

- **Command:** `Start-Process build/OWzxSlicer.exe -PassThru` + `Start-Sleep -Seconds 5` + `HasExited` probe.
- **Result:** PASS - OWzxSlicer.exe launched (PID 32272), survived the 5-second no-crash probe, then killed cleanly. Per STATE.md, runtime VISUAL evidence (Windows capture API screenshot) is blocked; reachability via process-liveness + canonical verifier + regression ctest is the documented evidence standard.

### Regression ctest

- **Command:** `ctest --test-dir build -R "PartPlateTests|PrepareSceneDataTests|ViewModelSmokeTests|QmlUiAuditTests" --output-on-failure`
- **Result:** **4/4 PASS** (100%, 0 failed). Total Test time (real) = 8.33 sec.
  - ViewModelSmokeTests - PASS (7.62 sec)
  - QmlUiAuditTests - PASS (0.08 sec) - incl. the new cliFixtureRecipesAndMultiMaterialModelPresent slot
  - PrepareSceneDataTests - PASS (0.02 sec)
  - PartPlateTests - PASS (0.59 sec)

### AUTOMOC silent-skip guard (new slot registration proof)

- **Command:** `./QmlUiAuditTests.exe cliFixtureRecipesAndMultiMaterialModelPresent` vs `./QmlUiAuditTests.exe nonexistentSlotXYZ123`
- **Result:** PASS - the new slot filter returns exit 0 (matched a registered test AND passed); the invalid filter returns exit 1 (no match). The exit-code differential proves the slot is registered on the meta-object (AUTOMOC ran) AND passes, guarding against the silent-skip caveat documented at the top of QmlUiAuditTests.cpp.

### Encoding guard + whitespace

- `git diff --check main~2..HEAD` - exits 0 (no whitespace errors across both task commits).
- `py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py src/qml_gui/main_qml.cpp tests/QmlUiAuditTests.cpp tests/data/fixture_recipes.md` - PASS (all 3 files OK, ASCII-only English comments).

## Self-Check: All 3 FIXTURE Contracts Locked + Reachability Bar Met

- **FIXTURE-01 (multi-material fixture):** `tests/data/multi_material_fixture.3mf` exists (1586 bytes, non-empty). Hand-authored 2-extruder 3MF (no upstream source suitable). Two cubes, one per extruder, with `slic3r:extruder` metadata. The cliFixtureRecipesAndMultiMaterialModelPresent slot asserts existence + non-empty (QFileInfo::exists + size > 0).
- **FIXTURE-03 (argv recipes):** `tests/data/fixture_recipes.md` exists, covers all major GUI states (Prepare/Preview/AssembleView/settings), references all 4 canonical argv flags. The slot asserts existence + 4 major-state names + 4 canonical flags.
- **FIXTURE-04 (anti-feature):** `main_qml.cpp` contains the anti-feature comment block above `parseStartupOpenRequest`. The slot asserts 4 anchor substrings (test-evidence plumbing, OWzx-only, OrcaSlicer.cpp:7183, MUST NOT be promoted).
- **Canonical build + reachability:** Canonical build exit 0, zero errors. OWzxSlicer.exe launches (PID 32272, 5-second no-crash). Regression ctest 4/4 PASS. AUTOMOC guard confirms the new slot is registered + passes.
- **Encoding + whitespace:** `git diff --check` exits 0; encoding guard exits 0 for all 3 modified files. All comments English and ASCII-only.
- **No libslic3r changes:** Phase 104 modified only `main_qml.cpp` (10 comment lines) + test files. No libslic3r, no Phase 103 gate change, no new argv flags (the 4 existing flags are the full set), no user-facing deep-link product feature (anti-feature honored).

## Next Phase Readiness

- **Phase 104 closes FIXTURE-01/03/04.** With Phase 103 (FIXTURE-02) + Phase 104 (FIXTURE-01/03/04) complete, all 4 WS3 CLI-fixture requirements are done. The argv fixture plumbing is now: deterministically gated (Phase 103 frameSwapped gate), content-complete (Phase 104 multi-material fixture + recipes), and anti-feature-locked (Phase 104 comment block).
- **Downstream consumers ready:** The multi-material fixture + argv recipes are now available for the WS4 D3D12 crash-repro work (Phase 106) and the v4.5 cross-workstream verification (Phase 116). The recipes doc gives those phases a deterministic way to reach each major GUI state for capture/repro without simulated clicks.
- **No blockers.** The canonical build is clean, the regression ctest passes, OWzxSlicer.exe launches, and all 3 FIXTURE contracts are locked by the source-audit slot.

## Self-Check: PASSED

- All 5 source-audit anchor greps return the expected hits (fixture exists + non-empty; recipes cover major states; anti-feature comment present; slot declared + implemented; 9 FIXTURE-named messages present).
- The cliFixtureRecipesAndMultiMaterialModelPresent slot is registered (filter-isolated exit 0 vs invalid-filter exit 1) AND passes.
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier (exit 0, zero errors); all test exes link clean.
- OWzxSlicer.exe launches and survives the 5-second liveness probe (PID 32272).
- Regression ctest 4/4 pass (ViewModelSmokeTests, QmlUiAuditTests incl. the new slot, PrepareSceneDataTests, PartPlateTests).
- `git diff --check` exits 0; encoding guard exits 0 for all 3 modified files.
- No libslic3r changes; no Phase 103 gate change; no new argv flags; anti-feature honored.

---
*Phase: 104-cli-fixture-recipes-and-multi-material-model*
*Plan: 01*
*Completed: 2026-07-12*
