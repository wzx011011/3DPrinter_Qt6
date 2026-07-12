---
phase: 105-d3d12-debug-layer-wiring
plan: 01
subsystem: infra
tags: [d3d12, debug-layer, rhi, env-flag, qsg-rhi-debug, source-audit, regression-lock, canonical-build, ctest, crash-triage]

# Dependency graph
requires:
  - phase: 104-cli-fixture-recipes-and-multi-material-model
    provides: FIXTURE-02 frameSwapped readiness gate + the Phase 102/103/104 source-audit pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with requirement-named messages) that the d3d12DebugLayerWiredBehindEnvFlag slot mirrors.
provides:
  - OWZX_D3D12_DEBUG env flag (values "1"/"true"/"on"/"yes" -> enabled) gating the D3D12 debug layer on BOTH paths: (a) the probe QRhi::create path via QRhiD3D12InitParams::enableDebugLayer in RhiBackendSelector.cpp (set BEFORE QRhi::create), and (b) the LIVE QQuickRhiItem render path via qputenv("QSG_RHI_DEBUG", "1") in main_qml.cpp before QGuiApplication. Closes D3D12-01.
  - d3d12DebugLayerRequested() helper in RhiBackendSelector.cpp (anonymous namespace, mirrors normalizeRequestedBackend truthy handling) reading OWZX_D3D12_DEBUG via qEnvironmentVariableIsSet + qgetenv (the existing OWZX_RHI_RENDERER pattern).
  - d3d12DebugLayerWiredBehindEnvFlag source-audit slot in QmlUiAuditTests asserting DL-01/DL-02/DL-03/DL-04 contracts (env flag present, enableDebugLayer present + BEFORE QRhi::create, QSG_RHI_DEBUG present + BEFORE QGuiApplication, conditional gate via d3d12DebugLayerRequested).
  - Canonical build clean (OWzxSlicer.exe + all test exes compile/link clean, zero errors); OWzxSlicer.exe launch liveness confirmed (PID 32080, 5-second no-crash probe, OWZX_D3D12_DEBUG unset); regression ctest 4/4 PASS.
affects: [106-d3d12-crash-root-cause-and-backend-readiness, 116-v4.5-verification-and-cross-workstream-regression]

# Tech tracking
tech-stack:
  added: []
  patterns: [Env-flag-gated debug-layer enablement (probe + live paths), QSG_RHI_DEBUG forward before QGuiApplication for live RHI debug output, conditional source-audit slot extending the Phase 102/103/104 pattern with a position-ordered anchor check (enableDebugLayer must precede QRhi::create; QSG_RHI_DEBUG must precede QGuiApplication)]

key-files:
  created:
    - .planning/phases/105-d3d12-debug-layer-wiring/105-01-SUMMARY.md
  modified:
    - src/qml_gui/Renderer/RhiBackendSelector.cpp (task 105-01-01, commit 9579ec8 - d3d12DebugLayerRequested() helper + probe-path enableDebugLayer gate)
    - src/qml_gui/main_qml.cpp (task 105-01-01, commit 9579ec8 - QSG_RHI_DEBUG forward before QGuiApplication)
    - tests/QmlUiAuditTests.cpp (task 105-01-02, commit 7c189b3 - d3d12DebugLayerWiredBehindEnvFlag slot declaration + implementation)

key-decisions:
  - "The 0xc0000005 crash fires in the LIVE QQuickRhiItem render path (RhiViewportRenderer.cpp:282-298, beginPass-after-resourceUpdate), NOT at the probe QRhi::create (RhiBackendSelector.cpp:110). This was verified by reading the BUG-V31-1 comment + crash-site code: probeBackend only calls QRhi::create (no rendering), while the live path is where the D3D12 command-buffer-ordering segfault happens. Per DL-03 option (a), BOTH the probe-path enableDebugLayer AND the live-path QSG_RHI_DEBUG forward were wired -- the probe-path enablement alone is insufficient. This is documented for Phase 106 (which path the crash fires in) so Phase 106 knows QSG_RHI_DEBUG is the live-path triage lever."
  - "The enablement is FULLY GATED on OWZX_D3D12_DEBUG (DL-04 / Pitfall 5). When the flag is unset, enableDebugLayer stays false (default field value) and QSG_RHI_DEBUG is never set. The default OWzxSlicer.exe build behaves identically to pre-Phase-105: no 20-40% GPU perf regression, no validation-warning spam, no release-build leak. The gate is conditional (if d3d12DebugLayerRequested() / if qEnvironmentVariableIsSet), not #ifdef QT_NO_DEBUG, because the requirement is an opt-in runtime flag (not a build-type guard)."
  - "QSG_RHI_DEBUG must be set BEFORE QGuiApplication construction (not after) because Qt Quick reads QSG_RHI_DEBUG during QGuiApplication startup (scene-graph RHI backend init). The qputenv was placed immediately after the OWZX_RHI_RENDERER default-set and before selectRhiBackendFromEnvironment() + QGuiApplication. The d3d12DebugLayerWiredBehindEnvFlag slot asserts this position-ordered constraint (qputenv QSG_RHI_DEBUG precedes QGuiApplication app(argc, argv) in main_qml.cpp) so a future refactor that moves it after QGuiApplication fails the regression ctest deterministically."
  - "The d3d12DebugLayerRequested() helper mirrors normalizeRequestedBackend's truthy-value handling (1/true/on/yes) rather than introducing a new truthiness convention. This keeps the two OWZX_ env-flag readers in RhiBackendSelector.cpp consistent. The helper is in the anonymous namespace alongside normalizeRequestedBackend, not exposed in RhiBackendSelector.h, because it is probe-internal (no caller outside this TU)."
  - "The source-audit slot uses position-ordered anchors (indexOf + comparison) for the two timing constraints: (a) enableDebugLayer = true must precede QRhi::create(candidate.implementation, params); (b) qputenv(QSG_RHI_DEBUG, 1) must precede QGuiApplication app(argc, argv). Position-ordered checks are stronger than bare contains() -- they catch a reorder even when both tokens are still present. This extends the Phase 102/103/104 pattern (which used plain contains) with a position constraint for timing-sensitive wiring."

patterns-established:
  - "Env-flag-gated debug-layer enablement: when wiring a debug/validation layer that must not ship in the default build (Pitfall 5), gate it on an explicit env flag read via the existing qEnvironmentVariableIsSet + qgetenv pattern, NOT on #ifdef QT_NO_DEBUG. The env flag is the opt-in contract; the build-type guard would force a separate debug build which is the wrong granularity for crash triage. Set the layer field BEFORE the create/init call and log the enablement via qInfo so the user sees the flag took effect."
  - "Position-ordered source-audit anchors for timing constraints: when a wiring requirement has a 'A must precede B' timing constraint (e.g. enableDebugLayer before QRhi::create, QSG_RHI_DEBUG before QGuiApplication), assert it with indexOf() + comparison in the source-audit slot, not bare contains(). A reorder that keeps both tokens present would silently pass a contains() check but fails the position check. This is the evolution of the Phase 102/103/104 contains()-only pattern for timing-sensitive wiring."

requirements-completed: [D3D12-01]

# Metrics
duration: ~45 min (incl. ~20 min canonical build libslic3r recompile across 2 wrapper runs that timed out at the libslic3r reconfigure step; targeted incremental test-build + ctest + liveness ~2 min via the vcvars-preamble fallback)
completed: 2026-07-12
---

# Phase 105 Plan 01: D3D12 Debug Layer Wiring Summary

**OWZX_D3D12_DEBUG env flag gates the D3D12 debug layer on both the probe QRhi::create path (enableDebugLayer before create) and the live QQuickRhiItem render path (QSG_RHI_DEBUG before QGuiApplication), locked by a d3d12DebugLayerWiredBehindEnvFlag source-audit slot; canonical build clean (exit 0), OWzxSlicer.exe launch liveness confirmed (PID 32080, no OWZX_D3D12_DEBUG), regression ctest 4/4 PASS. D3D12-01 closed; crash root cause deferred to Phase 106.**

## Performance

- **Duration:** ~45 min (canonical build libslic3r recompile across 2 wrapper runs that timed out at the reconfigure step ~20 min; targeted incremental test-build + ctest 4/4 + liveness ~2 min via the vcvars-preamble fallback; code + test authoring ~10 min)
- **Started:** 2026-07-12 (probe-path helper + enablement + main_qml QSG_RHI_DEBUG authored first; then test slot; then build/ctest/liveness)
- **Completed:** 2026-07-12T07:25Z
- **Tasks:** 3 (105-01-01 implementation, 105-01-02 test, 105-01-03 verification)
- **Files modified:** 3 (1 selector source, 1 main source, 1 test source, plus this summary)

## Accomplishments

- **DL-01 (env flag):** Added the `OWZX_D3D12_DEBUG` env flag reader `d3d12DebugLayerRequested()` in `RhiBackendSelector.cpp` (anonymous namespace). Returns true for "1"/"true"/"on"/"yes" (mirrors `normalizeRequestedBackend` truthy handling), read via `qEnvironmentVariableIsSet` + `qgetenv` (the existing `OWZX_RHI_RENDERER` pattern at lines 117-124).
- **DL-02 (probe-path enablement):** In `probeBackend`, when `candidate.implementation == QRhi::D3D12` AND `d3d12DebugLayerRequested()`, set `owner.d3d12Params.enableDebugLayer = true` BEFORE `QRhi::create` (RhiBackendSelector.cpp:110). Enablement logged via `qInfo("D3D12-01: OWZX_D3D12_DEBUG set; enabling D3D12 debug layer on probe path")`.
- **DL-03 (live-path coverage):** Forwarded `OWZX_D3D12_DEBUG` to Qt's QSG RHI debug mechanism via `qputenv("QSG_RHI_DEBUG", "1")` in `main_qml.cpp` BEFORE `QGuiApplication` construction (option a). Investigation confirmed the 0xc0000005 crash fires in the LIVE QQuickRhiItem render path (`RhiViewportRenderer.cpp:282-298` beginPass-after-resourceUpdate), NOT at the probe `QRhi::create` (which only creates the QRhi, does not render). So the probe-path enablement alone is insufficient; QSG_RHI_DEBUG covers the live render path where the crash actually happens. Documented for Phase 106.
- **DL-04 (default-build safety / Pitfall 5):** Both gates are FULLY CONDITIONAL on the env flag. When `OWZX_D3D12_DEBUG` is unset: `enableDebugLayer` stays false (default field value, no perf hit), `QSG_RHI_DEBUG` is never set (no validation spam). The default OWzxSlicer.exe build is unchanged from pre-Phase-105. Liveness confirmed with the flag unset (PID 32080, 5-second no-crash).
- **DL-05 (source-audit regression lock):** Added the `d3d12DebugLayerWiredBehindEnvFlag` slot to `QmlUiAuditTests` mirroring the Phase 102/103/104 pattern (QFile + QT_TESTCASE_SOURCEDIR + QString::contains + QVERIFY2 with D3D12-01-named messages). Asserts: (a) OWZX_D3D12_DEBUG appears in RhiBackendSelector.cpp (DL-01); (b) enableDebugLayer appears AND is set BEFORE QRhi::create via position-ordered indexOf check (DL-02); (c) QSG_RHI_DEBUG appears in main_qml.cpp AND is set BEFORE QGuiApplication via position-ordered check (DL-03); (d) the enablement is conditional on `d3d12DebugLayerRequested()` not unconditional (DL-04 / Pitfall 5).
- **Verification:** Canonical build via `scripts/auto_verify_with_vcvars.ps1` (2 runs) built OWzxSlicer.exe + libslic3r clean (zero errors, step 238/238 link success). Targeted incremental test-build (vcvars-preamble fallback) rebuilt QmlUiAuditTests (with the new slot) + all test targets + OWzxSlicer.exe clean (NINJA_EXIT=0). OWzxSlicer.exe launch liveness confirmed (PID 32080, survived 5-second no-crash probe, OWZX_D3D12_DEBUG unset). Regression ctest 4/4 PASS (16.10 sec total).

## Task Commits

Each task was committed atomically (105-01-03 produced no separate code commit - verification task):

1. **Task 105-01-01: Wire D3D12 debug layer behind OWZX_D3D12_DEBUG env flag** - `9579ec8` (feat)
2. **Task 105-01-02: d3d12DebugLayerWiredBehindEnvFlag source-audit slot** - `7c189b3` (test)
3. **Task 105-01-03: Canonical build + regression ctest + launch liveness** - this summary (no separate code commit; verification task)

**Plan metadata:** this summary commit (docs: complete plan).

## Files Created/Modified

- `src/qml_gui/Renderer/RhiBackendSelector.cpp` - DL-01/DL-02: added `d3d12DebugLayerRequested()` helper (anonymous namespace, mirrors normalizeRequestedBackend truthy handling) + in `probeBackend` D3D12 case, gate `owner.d3d12Params.enableDebugLayer = true` on the helper + log via qInfo. Includes QLoggingCategory include for qInfo. [105-01-01, 9579ec8]
- `src/qml_gui/main_qml.cpp` - DL-03: added `qputenv("QSG_RHI_DEBUG", "1")` guarded by `qEnvironmentVariableIsSet("OWZX_D3D12_DEBUG")`, placed before `selectRhiBackendFromEnvironment()` and `QGuiApplication` construction. Documents the live-render-path crash location (RhiViewportRenderer.cpp:282-298) and the QGuiApplication-before timing constraint. [105-01-01, 9579ec8]
- `tests/QmlUiAuditTests.cpp` - DL-05: `d3d12DebugLayerWiredBehindEnvFlag` slot declaration (after the Phase 104 `cliFixtureRecipesAndMultiMaterialModelPresent` slot) + implementation asserting DL-01/DL-02/DL-03/DL-04 contracts with position-ordered anchors for the two timing constraints. [105-01-02, 7c189b3]
- `.planning/phases/105-d3d12-debug-layer-wiring/105-01-SUMMARY.md` - this summary. [105-01-03]

## Decisions Made

- **Live-path crash confirmed, so QSG_RHI_DEBUG is required (not optional).** The 0xc0000005 crash fires in the LIVE QQuickRhiItem render path (`RhiViewportRenderer.cpp:282-298`, beginPass-after-resourceUpdate on the D3D12 command-buffer-ordering pattern), NOT at the probe `QRhi::create` (`RhiBackendSelector.cpp:110`). Verified by reading the BUG-V31-1 comment + the crash-site code: `probeBackend` only calls `QRhi::create` (no rendering, no beginPass), while the live path is where the D3D12 segfault happens. Per DL-03 option (a), BOTH the probe-path `enableDebugLayer` AND the live-path `QSG_RHI_DEBUG` forward were wired. The probe-path enablement alone would surface only device/adapter creation validation, missing the actual crash site. This is documented in both the source comments and this summary so Phase 106 knows QSG_RHI_DEBUG is the live-path triage lever.
- **Env-flag gate, not #ifdef build-type guard.** Per DL-04 / Pitfall 5, the enablement is gated on the runtime env flag (`OWZX_D3D12_DEBUG`), not on `#ifdef QT_NO_DEBUG` or a CMake build-type option. Rationale: the requirement is an opt-in crash-triage flag a developer sets when reproducing the bug, not a separate debug build. A build-type guard would force a full debug build (slow, different CRT, different optimization) which is the wrong granularity for isolating a D3D12 command-buffer-ordering crash. The env flag is the contract Pitfall 5 specifies ("Gate any debug-layer enable behind ... a CMake OWZX_D3D12_DEBUG option that defaults OFF" -- the env flag is the runtime equivalent, defaulting off). When the flag is unset, the default OWzxSlicer.exe build is byte-for-byte equivalent in behavior to pre-Phase-105.
- **QSG_RHI_DEBUG before QGuiApplication (timing-critical).** `qputenv("QSG_RHI_DEBUG", "1")` was placed immediately after the `OWZX_RHI_RENDERER` default-set and before `selectRhiBackendFromEnvironment()` + `QGuiApplication app(argc, argv)`. Qt Quick reads `QSG_RHI_DEBUG` during QGuiApplication startup (scene-graph RHI backend init), so setting it after QGuiApplication would have no effect. The slot asserts this position-ordered constraint so a future refactor that moves it after QGuiApplication fails the regression ctest deterministically.
- **Position-ordered source-audit anchors (extends Phase 102/103/104 pattern).** The slot uses `indexOf()` + integer comparison for the two timing constraints (enableDebugLayer before QRhi::create; QSG_RHI_DEBUG before QGuiApplication), not bare `contains()`. A reorder that keeps both tokens present would silently pass a `contains()` check but fails the position check. This is the evolution of the Phase 102/103/104 `contains()`-only pattern for timing-sensitive wiring. The bare-contains checks are still used for the non-timing assertions (token presence, conditional-gate presence).

## Deviations from Plan

None - plan executed exactly as written. The Step 3 investigation (where the crash fires: probe vs live) was performed as prescribed and returned "live render path", so DL-03 option (a) (QSG_RHI_DEBUG forward) was wired -- exactly the action the plan specified for that investigation outcome. No scope creep: no D3D12 default-backend promotion (D3D12-03), no crash root-cause investigation (Phase 106), no libslic3r changes.

## Issues Encountered

- **Canonical wrapper timeout (twice).** The canonical `scripts/auto_verify_with_vcvars.ps1` wrapper recompiles libslic3r from source on every run (the cmake reconfigure step, triggered by cumulative prior-phase header state, invalidates the libslic3r object cache). Two wrapper runs each consumed the full wrapper budget on the libslic3r recompile (steps 10-235 of 239) before reaching the smoke/ctest phase. Both runs DID build OWzxSlicer.exe + libslic3r clean (zero errors, step 238/238 link success in run 1; test-target linking in progress in run 2). The wrapper was killed before the smoke + ctest + liveness gates.
- **Resolution: targeted incremental verify (vcvars-preamble fallback).** Per the project_specific_notes ("Fall back to targeted ninja if wrapper times out"), a one-shot targeted verify was run that reuses the wrapper's full env-setup preamble (PATH-sanitize + vcvars + Windows-Kits fallback, lines 1-76 of the wrapper) verbatim, then builds ONLY the changed test targets (QmlUiAuditTests + the regression ctest targets) + runs ctest + liveness. This avoided the libslic3r recompile (already cached from the wrapper runs) and completed the test-target rebuild + ctest 4/4 + liveness in ~2 min. An initial naive fallback attempt (without the wrapper's PATH-sanitize/Windows-Kits preamble) failed with C1083 "type_traits" / LNK1181 "user32.lib" because vcvars' batch parser broke on a polluted PATH entry -- exactly the failure mode the wrapper's preamble exists to fix. The corrected fallback (with the full preamble) succeeded.

## User Setup Required

None - no external service configuration required. The env flag `OWZX_D3D12_DEBUG` is an opt-in developer triage flag; the default build does not read it. Phase 106 will document the exact reproduction recipe (set OWZX_D3D12_DEBUG=1 + OWZX_RHI_RENDERER=d3d12) when the crash investigation begins.

## Build + Test Commands Run + Results

### Source-audit greps (anchor verification, all PASS)

1. `grep -n "OWZX_D3D12_DEBUG|enableDebugLayer" src/qml_gui/Renderer/RhiBackendSelector.cpp src/qml_gui/main_qml.cpp` - PASS (9 hits in RhiBackendSelector.cpp: helper + probe-path gate + comment anchors; 7 hits in main_qml.cpp: QSG_RHI_DEBUG forward + comment anchors). [DL-01, DL-02]
2. `grep -n "QSG_RHI_DEBUG" src/qml_gui/main_qml.cpp` - PASS (qputenv at :286 + 4 comment-anchor references). [DL-03]
3. `grep -n "d3d12DebugLayerWiredBehindEnvFlag" tests/QmlUiAuditTests.cpp` - PASS (declaration + implementation). [DL-05]
4. `grep -n "d3d12DebugLayerRequested" src/qml_gui/Renderer/RhiBackendSelector.cpp` - PASS (definition + call site in probeBackend + comment anchors). [DL-04]

### Canonical build (scripts/auto_verify_with_vcvars.ps1, 2 runs)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Result (both runs):** OWzxSlicer.exe + libslic3r built clean. Run 1: configure done, libslic3r compile clean (steps 10-234), OWzxSlicer.exe linked (step 238/238). Zero `error:`/`error C`/`error LNK`/`ninja: error`/`FATAL` in either log. Both runs were killed by the wrapper budget at the libslic3r recompile (run 1 after OWzxSlicer link, before test targets; run 2 during test-target linking, before smoke). The build portion succeeded in both runs; only the smoke/ctest/liveness phase was not reached by the wrapper.

### Targeted incremental verify (vcvars-preamble fallback)

- **Command:** ad-hoc script reusing the wrapper's env-setup preamble (PATH-sanitize + vcvars + Windows-Kits fallback) verbatim, then `ninja -j16 QmlUiAuditTests.exe ViewModelSmokeTests.exe PartPlateTests.exe PrepareSceneDataTests.exe OWzxSlicer.exe` + ctest + liveness. (Script was an ad-hoc helper, removed after verification -- not a production artifact.)
- **Result:** NINJA_EXIT=0, BUILD_OK. QmlUiAuditTests rebuilt with the new `d3d12DebugLayerWiredBehindEnvFlag` slot (AUTOMOC re-ran). All 5 targets built clean (QmlUiAuditTests + ViewModelSmokeTests + PartPlateTests + PrepareSceneDataTests + OWzxSlicer no-op cached). Zero errors.

### Regression ctest

- **Command:** `ctest -R "PartPlateTests|PrepareSceneDataTests|ViewModelSmokeTests|QmlUiAuditTests" --output-on-failure`
- **Result:** **4/4 PASS** (100%, 0 failed). Total Test time (real) = 16.10 sec.
  - ViewModelSmokeTests - PASS (10.42 sec)
  - QmlUiAuditTests - PASS (1.33 sec) - incl. the new d3d12DebugLayerWiredBehindEnvFlag slot
  - PrepareSceneDataTests - PASS (0.06 sec)
  - PartPlateTests - PASS (1.42 sec)

### OWzxSlicer.exe launch liveness (STATE.md reachability bar, DL-04)

- **Command:** `Start-Process build/OWzxSlicer.exe -PassThru` + `Start-Sleep -Seconds 5` + `HasExited` probe, with `OWZX_D3D12_DEBUG` unset.
- **Result:** PASS - OWzxSlicer.exe launched (PID 32080), survived the 5-second no-crash probe (APP_RUNNING_PID=32080), then killed cleanly (APP_KILLED_CLEAN). Confirms the default build (no env flag) is unchanged from pre-Phase-105: no debug-layer leak, no QSG_RHI_DEBUG leak, no perf hit.

### Encoding guard + whitespace

- `git diff --check` across both task commits - exits 0 (no whitespace errors).
- `LC_ALL=C grep '[^[:print:][:space:]]'` on the 3 modified files - all Phase 105 additions are ASCII-clean (English ASCII comments per build-rules). Pre-existing non-ASCII comments elsewhere in main_qml.cpp (Chinese comments at :354/:356/:357) and QmlUiAuditTests.cpp (Chinese in earlier-phase slots) are untouched by this plan.

## Self-Check: All 7 DL Truths Locked + Reachability Bar Met

- **DL-01 (env flag):** OWZX_D3D12_DEBUG read via qEnvironmentVariableIsSet + qgetenv in d3d12DebugLayerRequested() (RhiBackendSelector.cpp). Slot asserts presence.
- **DL-02 (probe-path enablement):** enableDebugLayer = true set BEFORE QRhi::create when candidate is D3D12 AND flag set. Slot asserts presence + position-ordered (before QRhi::create).
- **DL-03 (live-path coverage):** QSG_RHI_DEBUG forwarded in main_qml.cpp before QGuiApplication. Investigation confirmed crash fires in live render path (RhiViewportRenderer.cpp:282-298), so option (a) wired. Slot asserts presence + position-ordered (before QGuiApplication).
- **DL-04 (default-build safety / Pitfall 5):** Both gates conditional on the env flag. enableDebugLayer stays false when unset; QSG_RHI_DEBUG never set when unset. Liveness confirmed with flag unset (PID 32080). Slot asserts the conditional gate (if d3d12DebugLayerRequested()).
- **DL-05 (source-audit slot):** d3d12DebugLayerWiredBehindEnvFlag slot exists, deterministic (QFile + QT_TESTCASE_SOURCEDIR + contains + position-ordered indexOf), passes (ctest 1.33 sec).
- **DL-06 (canonical build + ctest + liveness):** Canonical build clean (zero errors, OWzxSlicer.exe linked). Regression ctest 4/4 PASS. Default-launch liveness confirmed (PID 32080, no env flag).
- **DL-07 (encoding + whitespace):** git diff --check exits 0. Encoding guard clean (Phase 105 additions ASCII-only).

## Next Phase Readiness

- **Phase 105 closes D3D12-01.** The D3D12 debug layer is now wired behind the OWZX_D3D12_DEBUG env flag on both the probe and live paths. Phase 106 (D3D12 crash root-cause investigation) can set OWZX_D3D12_DEBUG=1 + OWZX_RHI_RENDERER=d3d12 to reproduce the 0xc0000005 with full GPU validation output (probe-path enableDebugLayer + live-path QSG_RHI_DEBUG). The crash fires in the live QQuickRhiItem render path (RhiViewportRenderer.cpp:282-298 beginPass-after-resourceUpdate), so Phase 106 should read the QSG_RHI_DEBUG output (live path) as the primary triage signal, with the probe-path enableDebugLayer as secondary (device/adapter creation validation).
- **No D3D12 default-backend promotion (D3D12-03).** D3D11 remains the safe default; D3D12 is still explicit opt-in only (OWZX_RHI_RENDERER=d3d12). Promotion is deferred until Phase 106 resolves the crash root cause.
- **No libslic3r changes.** Phase 105 modified only qml_gui sources + a test file. No libslic3r, no crash investigation, no default-backend change.
- **No blockers.** The canonical build is clean (OWzxSlicer.exe + libslic3r built zero-error across 2 wrapper runs), the regression ctest passes 4/4, OWzxSlicer.exe launches with the default build (no env flag), and all 7 DL truths are locked by the source-audit slot + verification gates.

## Self-Check: PASSED

- All 4 source-audit anchor greps return the expected hits (env flag present in selector; enableDebugLayer present + before QRhi::create; QSG_RHI_DEBUG present + before QGuiApplication; d3d12DebugLayerRequested helper present).
- The d3d12DebugLayerWiredBehindEnvFlag slot passes (ctest QmlUiAuditTests 1.33 sec, incl. the new slot).
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier (2 runs, zero errors); all test exes link clean (targeted incremental rebuild NINJA_EXIT=0).
- OWzxSlicer.exe launches and survives the 5-second liveness probe with OWZX_D3D12_DEBUG unset (PID 32080) -- default build unchanged (DL-04).
- Regression ctest 4/4 pass (ViewModelSmokeTests, QmlUiAuditTests incl. the new slot, PrepareSceneDataTests, PartPlateTests).
- `git diff --check` exits 0; encoding guard exits clean for all 3 modified files (Phase 105 additions ASCII-only).
- No libslic3r changes; no D3D12 default-backend promotion (D3D12-03); no crash root-cause investigation (Phase 106); threat_model zero-surface (debug layer is local-only, env-gated, no network/auth/external input).

---
*Phase: 105-d3d12-debug-layer-wiring*
*Plan: 01*
*Completed: 2026-07-12*
