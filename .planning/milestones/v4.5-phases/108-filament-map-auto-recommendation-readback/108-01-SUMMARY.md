---
phase: 108-filament-map-auto-recommendation-readback
plan: 01
subsystem: infra
tags: [filament-map, print-readback, libslic3r, slice-service, viewmodel, qml, ams]

# Dependency graph
requires:
  - phase: 107-filament-map-mode-enum-widening-and-3mf-migration
    provides: 4-value OWzx::FilamentMapMode enum (fmmAutoForFlush/fmmAutoForMatch/fmmManual/fmmDefault) at PartPlate.h:96-101 with numeric values identical to upstream Slic3r::FilamentMapMode.
provides:
  - Post-slice filament-map auto-recommendation readback wired end-to-end from Print::get_filament_maps() (captured by value in the SliceService worker, no Print* escape) through SliceService::filamentMapReady into EditorViewModel Q_PROPERTYs (hasAutoFilamentMap/autoFilamentMapMode/autoFilamentMaps) for Phase 110 UI binding.
  - Success-branch-only emit gate (cancel/error branches do not emit), mirroring the v4.4 WTREAD-02 wipe-tower gate.
  - FMAP-01 valid gate: when the user picked Manual (mode >= fmmManual), the engine computes no auto-map, so hasAutoFilamentMap stays false and no stale map leaks to the UI.
affects: [110-filament-group-popup-ui, 111-filament-map-round-trip]

# Tech tracking
tech-stack:
  added: []
  patterns: [Print-lifetime-bounded capture-by-value readback (FilamentMapResult POD, mirrors WipeTowerGeometry), data-driven Q_PROPERTY NOTIFY gate (filamentMapChanged, mirrors wipeTowerGeometryChanged), success-branch-only emit (same gate as v4.4 WTREAD-02), QMetaObject::invokeMethod signal-emission test pattern with qRegisterMetaType<FilamentMapResult>]

key-files:
  created:
    - .planning/phases/108-filament-map-auto-recommendation-readback/108-01-SUMMARY.md
  modified:
    - src/core/services/SliceService.h (task 108-01-01 -- FilamentMapResult POD + #include core/model/PartPlate.h + filamentMapReady signal)
    - src/core/services/SliceService.cpp (task 108-01-01 -- worker capture + get_filament_map_mode/get_filament_maps reads + fmmManual valid gate + success-branch emit)
    - src/core/viewmodels/EditorViewModel.h (task 108-01-02 -- 3 Q_PROPERTYs + getters + private slot + NOTIFY signal + member storage)
    - src/core/viewmodels/EditorViewModel.cpp (task 108-01-02 -- connect + onFilamentMapReady slot + getters)
    - tests/ViewModelSmokeTests.cpp (task 108-01-03 -- filamentMapAutoRecommendationReadbackWired test)
    - tests/QmlUiAuditTests.cpp (task 108-01-03 -- filamentMapAutoRecommendationReadbackPresent source-audit)

key-decisions:
  - "FilamentMapResult.mode uses the Phase 107 OWzx::FilamentMapMode enum (not the upstream Slic3r::FilamentMapMode). The two have identical numeric values (0/1/2/3), so a static_cast<int> round-trips losslessly at the capture boundary (SliceService.cpp:684-686). Keeping the POD on the OWzx enum keeps SliceService.h buildable in mock mode and on every consumer without dragging libslic3r headers into the type definition."
  - "valid is set true ONLY when the upstream auto-recommendation actually ran, i.e. when the resolved mode was < fmmManual (Print.cpp:2485 condition). When the user picked Manual, the engine's Print.cpp:2484-2491 branch does not fire and get_filament_maps() returns the (uncomputed) config-stored value; the worker does not surface that as an auto recommendation. This mirrors the WTREAD-02 has_wipe_tower() gate (a single source-of-truth gate on the captured struct)."
  - "EditorViewModel exposes the readback as three READ-only Q_PROPERTYs with NOTIFY filamentMapChanged (no WRITE setters): the map flows one-way from SliceService (libslic3r worker) to QML, exactly like the six wipeTower* Q_PROPERTYs. The slot applies the valid gate: when result.valid is false, m_hasAutoFilamentMap is forced to false and the maps/mode members are left untouched (no stale map leaks as a fresh recommendation) -- mirrors the v4.4 decision to leave the wipe-tower dims untouched on the invalid path."
  - "SliceService.h now #includes core/model/PartPlate.h so the FilamentMapResult.mode field has the complete enum type on every consumer of the header (EditorViewModel, tests). This is a heavier include than WipeTowerGeometry (which is self-contained), but it is the clean way to reuse the Phase 107 enum without redeclaring it or leaking the upstream Slic3r:: enum into the Qt6 layer."
  - "Test drives SliceService::filamentMapReady via QMetaObject::invokeMethod + Q_ARG (after qRegisterMetaType<FilamentMapResult>()) so the connect(...) wiring is exercised end-to-end without a real libslic3r multi-material slice (which would need fixtures). Same pattern as the v4.4 wipe-tower readback test."

patterns-established:
  - "Print-lifetime-bounded capture-by-value readback for a second readback payload (FilamentMapResult). The v4.4 WipeTowerGeometry pattern (Phase 100) is now a reusable template: declare a POD in SliceService.h, capture by value in the worker between print.process() and activePrint_.store(nullptr), add the value to the GUI-thread queued lambda capture, emit a dedicated signal on the success branch of sliceFinished, expose via READ-only Q_PROPERTYs on EditorViewModel, gate the UI on the captured valid flag."
  - "Success-branch-only emit gate is now applied consistently across both readbacks (wipe-tower + filament-map). The cancel/error branches above the emit site return early, so neither signal fires on a failed/cancelled slice. This is the v4.4 WTREAD-02 gate generalized."

requirements-completed: [FMAP-01]

# Metrics
duration: ~25 min (incl. ~20 min libslic3r_from_source rebuild triggered by the SliceService.h header change)
completed: 2026-07-12
---

# Phase 108 Plan 01: Filament-Map Auto Recommendation Readback Summary

**Post-slice filament-map auto-recommendation readback wired end-to-end (Print::get_filament_maps() captured by value in the SliceService worker, delivered via filamentMapReady, exposed as EditorViewModel Q_PROPERTYs for Phase 110 UI binding, valid-gated so Manual-mode slices surface no stale map). FMAP-01 closed.**

## Performance

- **Duration:** ~25 min (includes ~20 min for the libslic3r_from_source rebuild + OWzxSlicer.exe link triggered by the SliceService.h header change propagating to dependent TUs)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 4 (108-01-01 through 108-01-04)
- **Files modified:** 6 production/test files + this summary

## Accomplishments

- **Task 108-01-01 (commit 7be9c18):** FilamentMapResult POD declared in SliceService.h (immediately after WipeTowerGeometry) with three fields: `bool valid` (the auto-recommendation-ran gate), `OWzx::FilamentMapMode mode` (Phase 107 enum), `std::vector<int> maps` (Print::get_filament_maps result). SliceService.h now #includes core/model/PartPlate.h for the enum type. filamentMapReady signal declared alongside wipeTowerGeometryReady. The worker captures the result by value (capturedFilamentMap{}) in the same Print-lifetime window as WipeTowerGeometry (between print.process() and activePrint_.store(nullptr)): reads print.get_filament_map_mode() + print.get_filament_maps(), sets valid=true ONLY when mode < fmmManual (the upstream Print.cpp:2485 auto-recommendation condition). Emit on the success branch of the sliceFinished queued lambda only (cancel/error branches return early), same gate as wipeTowerGeometryReady (v4.4 WTREAD-02).
- **Task 108-01-02 (commit e1cd5c7):** EditorViewModel exposes the readback as three READ-only Q_PROPERTYs NOTIFY filamentMapChanged: hasAutoFilamentMap (bool, the valid gate mirroring WTREAD-02 showWipeTower), autoFilamentMapMode (int, resolved OWzx::FilamentMapMode), autoFilamentMaps (QVariantList, 1-based per-extruder group ids). The onFilamentMapReady private slot (in the same private slots block as onWipeTowerGeometryReady) is connected to SliceService::filamentMapReady and applies the valid gate: when result.valid is false (user picked Manual), m_hasAutoFilamentMap forced to false and maps/mode members left untouched (no stale map leaks to the Phase 110 UI). Defaults keep the pre-slice UI inert.
- **Task 108-01-03 (commit ac9749b):** Added the filamentMapAutoRecommendationReadbackWired smoke test (valid + invalid/manual path) with QSignalSpy on filamentMapChanged. The test drives SliceService::filamentMapReady via QMetaObject::invokeMethod + Q_ARG (after qRegisterMetaType<FilamentMapResult>()) so the connect wiring is exercised end-to-end without a real libslic3r slice. Also added the filamentMapAutoRecommendationReadbackPresent source-audit slot in QmlUiAuditTests asserting the POD + signal + worker capture site (get_filament_maps/get_filament_map_mode/fmmManual gate) + success-branch emit in SliceService and the slot + 3 Q_PROPERTYs + NOTIFY + connect wiring in EditorViewModel. FMAP-01/FR-named QVERIFY2 messages.
- **Task 108-01-04 (this summary, no separate code commit):** Canonical build (scripts/auto_verify_with_vcvars.ps1) ran green; all 5 regression test suites passed; both new slots verified in isolation.

## Task Commits

Each task was committed atomically:

1. **Task 108-01-01: FilamentMapResult POD + filamentMapReady signal + worker capture + emit** -- `7be9c18` (feat)
2. **Task 108-01-02: EditorViewModel onFilamentMapReady slot + 3 Q_PROPERTYs + NOTIFY** -- `e1cd5c7` (feat)
3. **Task 108-01-03: filament-map readback wiring test + source-audit slot** -- `ac9749b` (test)
4. **Task 108-01-04: build + ctest + SUMMARY** -- this summary (docs, no separate code commit)

## Files Created/Modified

- `src/core/services/SliceService.h` -- FilamentMapResult POD (after WipeTowerGeometry) + #include core/model/PartPlate.h + #include <vector> + filamentMapReady signal (next to wipeTowerGeometryReady). [108-01-01, 7be9c18]
- `src/core/services/SliceService.cpp` -- capturedFilamentMap{} declared next to capturedGeometry; worker capture block reading print.get_filament_map_mode()/print.get_filament_maps() with the mode < fmmManual valid gate; capturedFilamentMap added to the GUI-thread queued lambda capture; emit filamentMapReady(capturedFilamentMap) on the success branch (after wipeTowerGeometryReady). [108-01-01, 7be9c18]
- `src/core/viewmodels/EditorViewModel.h` -- 3 Q_PROPERTYs (hasAutoFilamentMap/autoFilamentMapMode/autoFilamentMaps NOTIFY filamentMapChanged); 3 getters; onFilamentMapReady private slot (in the same private slots block as onWipeTowerGeometryReady); filamentMapChanged signal; 3 private members (defaults keep pre-slice UI inert: hasAuto=false, mode=fmmDefault, maps={}). [108-01-02, e1cd5c7]
- `src/core/viewmodels/EditorViewModel.cpp` -- connect(sliceService_, filamentMapReady, onFilamentMapReady) next to the wipe-tower connect; onFilamentMapReady slot applying the valid gate (valid path overwrites mode + builds a QVariantList from maps; invalid path forces hasAuto=false and leaves maps/mode untouched); 3 getter implementations. [108-01-02, e1cd5c7]
- `tests/ViewModelSmokeTests.cpp` -- filamentMapAutoRecommendationReadbackWired test (declaration + implementation) with qRegisterMetaType<FilamentMapResult> static initializer + QSignalSpy + QMetaObject::invokeMethod on the signal; valid path (mode=fmmAutoForFlush, maps={1,2,3}) + invalid/manual path (valid=false, maps/mode persist). [108-01-03, ac9749b]
- `tests/QmlUiAuditTests.cpp` -- filamentMapAutoRecommendationReadbackPresent source-audit slot (declaration + implementation) asserting the end-to-end wiring with FMAP-01/FR-named QVERIFY2 messages. [108-01-03, ac9749b]

## Decisions Made

- **OWzx enum on the POD (not the upstream Slic3r enum).** The FilamentMapResult.mode field uses OWzx::FilamentMapMode (the Phase 107 enum) rather than Slic3r::FilamentMapMode. The two have identical numeric values (0/1/2/3), so the worker static_casts losslessly at the capture boundary (SliceService.cpp:684-686). This keeps SliceService.h buildable in mock mode (HAS_LIBSLIC3R off) and avoids dragging libslic3r headers into the type definition of the POD.
- **valid gate mirrors WTREAD-02 has_wipe_tower().** Just as WipeTowerGeometry.valid is the single gate for whether to render the wipe-tower, FilamentMapResult.valid is the single gate for whether to surface an auto recommendation. valid is true ONLY when the upstream auto-recommendation ran (mode < fmmManual per Print.cpp:2485); when the user picked Manual, the engine computes no auto-map and valid stays false. The slot forces m_hasAutoFilamentMap=false on the invalid path and leaves the maps/mode members untouched -- mirrors the v4.4 decision to leave the wipe-tower dims untouched so no stale value leaks as fresh.
- **Read-only Q_PROPERTYs (one-way libslic3r -> GUI flow).** No WRITE setters: the map flows from SliceService (libslic3r worker), not from QML -- consistent with the six wipeTower* Q_PROPERTYs and how bed dims flow from QSettings/project state. Phase 110 will add the FilamentGroupPopup UI that binds these for display; Phase 111 will add the full round-trip (FMAP-04).
- **Test invokes the signal by name, not a friend-call on the slot.** QMetaObject::invokeMethod(&slice, "filamentMapReady", Qt::DirectConnection, Q_ARG(FilamentMapResult, ...)) proves the connect(...) wiring end-to-end (signal exists on the meta-object, slot reachable, NOTIFY fires) without a real libslic3r multi-material slice (which would need fixtures). Required qRegisterMetaType<FilamentMapResult>("FilamentMapResult") so Q_ARG can wrap the argument.

## Deviations from Plan

None -- plan executed exactly as written. The four tasks landed in four atomic commits (3 feat/test + 1 docs), no build-fix commits needed.

## Issues Encountered

None. The SliceService.h header change (adding #include core/model/PartPlate.h + the FilamentMapResult POD) triggered the expected libslic3r_from_source + OWzxSlicer.exe rebuild (~20 min), but the canonical build completed within the wrapper budget this run (unlike the v4.4 Phase 100 run, which timed out mid-link and required a targeted ninja fallback). All test exes compiled, linked, and ran green on the first canonical-build invocation.

## Build + Test Commands Run + Results

### Source-audit greps (plan verification, PASS)

1. `grep -rn "FilamentMapResult|filamentMapReady|onFilamentMapReady|get_filament_maps" src/` -- PASS (POD SliceService.h:84; signal SliceService.h:210; worker capture SliceService.cpp:688 + emit :855; slot EditorViewModel.h:855 + connect EditorViewModel.cpp:2232-2233).
2. Capture-by-value invariant: `capturedFilamentMap` is the only filament-map value captured into the GUI-thread queued lambda (SliceService.cpp:756). No `Print*` or libslic3r reference type is captured. PASS.

### Canonical build (scripts/auto_verify_with_vcvars.ps1)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1 > build/108-01-verify.log 2>&1`
- **Result:** PASS -- OWzxSlicer.exe compiled and linked clean (237/237 ninja targets), all test exes compiled and linked clean. No compile/link errors. The build completed within the wrapper budget (no targeted ninja fallback needed).
- **Log:** `build/108-01-verify.log`

### Regression ctest (run by the canonical verifier)

- **Result:** ALL FIVE PASSED (exit 0):
  - PrepareSceneDataTests.exe -- PASS ([PrepareScene] Prepare scene data tests passed)
  - PartPlateTests.exe -- PASS ([PartPlate] PartPlate geometry + arrangement tests passed)
  - ViewModelSmokeTests.exe -- PASS ([ViewModel] ViewModel smoke tests passed) -- includes the new filamentMapAutoRecommendationReadbackWired slot
  - QmlUiAuditTests.exe -- PASS ([UI] QML UI audit tests passed) -- includes the new filamentMapAutoRecommendationReadbackPresent slot
  - PreviewParserTests.exe -- PASS ([PreviewParser] PreviewParser tests passed)

### New slots verified in isolation (post-build, deterministic re-run)

- `build/ViewModelSmokeTests.exe filamentMapAutoRecommendationReadbackWired` -- PASS (exit 0)
- `build/QmlUiAuditTests.exe filamentMapAutoRecommendationReadbackPresent` -- PASS (exit 0)

### Production app smoke

- OWzxSlicer.exe launched under the verifier (APP_RUNNING_PID=15236), confirming the production link is clean and the app starts.

### Encoding guard

- `py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py` run against all 6 modified production + test files (SliceService.h/.cpp, EditorViewModel.h/.cpp, ViewModelSmokeTests.cpp, QmlUiAuditTests.cpp) -- all PASS (ASCII-only, English comments).

### `git diff --check`

- Exits 0 on all commits (no whitespace errors).

## Self-Check: Capture-By-Value Invariant + Success-Branch Gate

- **No Print* escapes the worker (Frozen Decision 1).** The FilamentMapResult struct (SliceService.h:84) is captured BY VALUE into the worker-local `capturedFilamentMap` (SliceService.cpp:431), populated from `print.get_filament_map_mode()` / `print.get_filament_maps()` between `print.process()` (:590) and `activePrint_.store(nullptr)` (the store call after the capture block), and delivered by value into the sliceFinished queued lambda capture list (:756) before the GUI-thread emit (:855). No `Print*` is captured into the lambda or stored on SliceService/EditorViewModel. Confirmed by grep: the only value captured for the filament-map path is `capturedFilamentMap` (a pure value type). The `activePrint_` atomic lifetime is preserved unchanged.
- **Success-branch-only emit (v4.4 WTREAD-02 gate generalized).** The emit at SliceService.cpp:855 is reached only on the success branch of the sliceFinished queued lambda. The cancel branch (:735-750) and the error branch (:752-767) both `return` early before reaching the emit site. So filamentMapReady fires ONLY when print.process() succeeded -- same gate as wipeTowerGeometryReady (emitted 2 lines above). The source-audit slot (QmlUiAuditTests.cpp:filamentMapAutoRecommendationReadbackPresent) locks this anchor with `emit receiver->filamentMapReady(capturedFilamentMap)`.
- **valid is the single gate (mirrors WTREAD-02 has_wipe_tower()).** The worker sets `capturedFilamentMap.valid = true` ONLY when `mapModeInt < fmmManual` (SliceService.cpp:687), which is the upstream Print.cpp:2485 condition that gates whether the auto-recommendation branch fired. When the user picked Manual, the engine computed no auto-map, so valid stays false and the slot (EditorViewModel.cpp:5137) forces m_hasAutoFilamentMap=false without overwriting the maps/mode members. The test asserts this explicitly (after the invalid readback, autoFilamentMapMode stays fmmAutoForFlush and autoFilamentMaps stays {1,2,3} from the prior valid readback -- not reset to placeholders).

## Next Phase Readiness

- **Phase 110 (FMAP-03 FilamentGroupPopup UI):** the auto recommendation now reaches the EditorViewModel Q_PROPERTY layer end-to-end. Phase 110 can bind hasAutoFilamentMap/autoFilamentMapMode/autoFilamentMaps in the popup for display, and surface the recommended map for the user to accept. No rediscovery needed -- the readback contract is locked.
- **Phase 111 (FMAP-04 full round-trip):** the capture-by-value readback half of the round-trip is done; Phase 111 adds the user-accept path that writes the accepted map back through the per-plate config (the WRITE direction this phase deliberately does not add).
- No blockers.

## Self-Check: PASSED

- All source-audit greps return the expected hits.
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier.
- All five regression test suites pass (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests, QmlUiAuditTests, PreviewParserTests).
- Both new slots pass in isolation (filamentMapAutoRecommendationReadbackWired, filamentMapAutoRecommendationReadbackPresent).
- `git diff --check` exits 0; encoding guard exits 0 for all 6 modified files.
- Capture-by-value invariant preserved (no Print* escapes the worker -- Frozen Decision 1).
- Success-branch-only emit gate enforced (cancel/error branches do not emit filamentMapReady).
- valid gate enforced data-driven (FMAP-01): Manual-mode slices surface no auto recommendation.

---
*Phase: 108-filament-map-auto-recommendation-readback*
*Plan: 01*
*Completed: 2026-07-12*
