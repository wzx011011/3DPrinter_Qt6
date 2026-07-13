---
phase: 109-option-b-wipe-tower-mesh-readback-and-real-rendering
plan: 01
subsystem: infra
tags: [wipe-tower, print-readback, rhi, qml, libslic3r, slice-service, geometry, viewmodel, convex-hull, real-mesh]

# Dependency graph
requires:
  - phase: 100-wipe-tower-geometry-readback
    provides: WipeTowerGeometry POD + wipeTowerGeometryReady signal + EditorViewModel Q_PROPERTYs + PreparePage.qml bindings + RhiViewportRenderer uploadWipeTowerBuffer (Option A baseline) + capture-by-value invariant (Frozen Decision 1) + has_wipe_tower() data-driven gate (WTREAD-02).
provides:
  - Option B real wipe-tower mesh readback wired end-to-end. When Print::wipe_tower_data().wipe_tower_mesh_data is populated (multi-material post-slice), the SliceService worker captures the convex hull of the merged real_wipe_tower_mesh + real_brim_mesh (mirrors upstream 3DScene.cpp:906-914) as a flattened XYZ float vector (no TriangleMesh* escapes), and the renderer draws the real mesh instead of the Option A dimensioned box.
  - RE-OPENS Phase 99 Frozen Decision 2 (Option B was LOCKED future upgrade). Option A is preserved as the else branch (buildWipeTowerVertices UNCHANGED).
  - Closes WTMESH-01 (POD extended), WTMESH-02 (worker capture-by-value), WTMESH-03 (parallel Option B builder + Option A preserved).
affects: [116-wipe-tower-regression-lock]

# Tech tracking
tech-stack:
  added: []
  patterns: [POD extension (add fields, do not replace -- preserves the Option A fallback path), capture-by-value extended to mesh vertices (std::vector<float> flattened XYZ, NO TriangleMesh* escape), parallel helper builder (Option B buildWipeTowerMeshVertices alongside frozen Option A buildWipeTowerVertices), QVariantList-over-QML for float vectors (mirrors autoFilamentMaps pattern), QPainter triangle-soup projection on SoftwareViewport fallback (depth-sorted faces pipeline reused)]

key-files:
  created:
    - .planning/phases/109-option-b-wipe-tower-mesh-readback-and-real-rendering/109-01-SUMMARY.md
  modified:
    - src/core/services/SliceService.h (task 109-01-01 -- WipeTowerGeometry POD extended with hasRealMesh + meshVertices)
    - src/core/services/SliceService.cpp (task 109-01-01 -- worker capture merges real_brim_mesh + convex_hull_3d + extracts its.vertices as flattened XYZ floats)
    - src/core/rendering/GizmoGeometry.h (task 109-01-02 -- buildWipeTowerMeshVertices declaration, PARALLEL to buildWipeTowerVertices)
    - src/core/rendering/GizmoGeometry.cpp (task 109-01-02 -- buildWipeTowerMeshVertices implementation, applies upstream Y -> Qt Z transform)
    - src/qml_gui/Renderer/RhiViewportRenderer.h (task 109-01-02 -- m_wipeTowerHasRealMesh + m_wipeTowerMeshVertices members)
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp (task 109-01-02 -- synchronize() pulls new state + uploadWipeTowerBuffer Option B branch)
    - src/qml_gui/Renderer/RhiViewport.h (tasks 109-01-02 + 109-01-03 -- storage members + wipeTowerHasRealMesh/wipeTowerMeshVertices Q_PROPERTYs + getters/setters)
    - src/qml_gui/Renderer/RhiViewport.cpp (task 109-01-03 -- QVariantList <-> std::vector<float> setters)
    - src/qml_gui/Renderer/SoftwareViewport.h (task 109-01-03 -- Q_PROPERTYs + storage members)
    - src/qml_gui/Renderer/SoftwareViewport.cpp (task 109-01-03 -- setters + QPainter triangle-soup mesh projection)
    - src/core/viewmodels/EditorViewModel.h (task 109-01-03 -- Q_PROPERTYs + getters + storage members)
    - src/core/viewmodels/EditorViewModel.cpp (task 109-01-03 -- onWipeTowerGeometryReady mirrors + clears mesh state + QVariantList getter)
    - src/qml_gui/pages/PreparePage.qml (task 109-01-03 -- wipeTowerHasRealMesh + wipeTowerMeshVertices bindings)
    - tests/ViewModelSmokeTests.cpp (task 109-01-04 -- wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback regression test)
    - tests/QmlUiAuditTests.cpp (task 109-01-04 -- optionBWipeTowerMeshCoexistsWithOptionA source-audit slot)

key-decisions:
  - "POD extension, not replacement. WipeTowerGeometry gains hasRealMesh + meshVertices but the existing v4.4 dim fields stay unchanged. A v4.4-style dims-only capture (no wipe_tower_mesh_data) produces hasRealMesh=false and the renderer takes the Option A else branch (Phase 99 Frozen Decision 2 baseline preserved)."
  - "Capture-by-value extends to mesh vertices. The worker extracts its.vertices of the merged+convex-hull mesh as a std::vector<float> (flattened XYZ triples). NO TriangleMesh* or its* escapes the worker (Frozen Decision 1 extended). The queued lambda captures the POD by value; the QVariantList conversion happens at the Q_PROPERTY boundary in EditorViewModel/RhiViewport/SoftwareViewport."
  - "Parallel builder, frozen Option A untouched. GizmoGeometry::buildWipeTowerMeshVertices is added alongside buildWipeTowerVertices -- the Option A builder and its uploadWipeTowerBuffer call site are UNCHANGED. The Option B path is the if branch on m_wipeTowerHasRealMesh; Option A is the else branch."
  - "EditorViewModel clears mesh state on the invalid path. onWipeTowerGeometryReady forces m_wipeTowerHasRealMesh=false and clears m_wipeTowerMeshVertices when geometry.valid is false. This prevents a stale real mesh from a prior multi-material slice leaking through a single-material re-slice (WipeTowerData::clear() resets wipe_tower_mesh_data to nullopt at Print.hpp:776, so the next valid capture may carry hasRealMesh=false)."
  - "QVariantList-over-QML for the float vector, mirroring the v4.4 autoFilamentMaps pattern. The setter converts back to std::vector<float> at the C++ boundary; malformed entries are dropped defensively (keep prior mesh)."
  - "SoftwareViewport implements the QPainter mesh projection rather than deferring. The existing paint path already has a depth-sorted faces pipeline; Option B emits the convex-hull triangles into the same vector with the same color as Option A so the two paths render consistently. Each captured (mx, my, mz) maps to Qt (mx, mz + kGroundY, my) matching buildWipeTowerMeshVertices."
  - "Defensive empty-mesh fallback to Option A. If buildWipeTowerMeshVertices returns empty (malformed capture -- should not happen per the capture invariant), uploadWipeTowerBuffer falls back to buildWipeTowerVertices so the wipe tower still renders. Guards against a future regression without silently dropping the tower."

patterns-established:
  - "POD extension for additive readback: when a frozen POD needs more fields for an upgrade path, extend it (add fields with safe defaults) rather than replacing it. The existing consumer code keeps working because the new fields default to the no-op state."
  - "Parallel helper builder for a frozen baseline: when a frozen builder must be preserved (Phase 99 Frozen Decision 2), add the new builder alongside it and gate the renderer on a flag. The frozen builder's call site becomes the else branch; the new builder is the if branch."
  - "Capture-by-value extended to flat float vectors: when a libslic3r mesh must reach the GUI thread, extract its.vertices into a std::vector<float> (flattened XYZ) in the worker. The QVariantList conversion at the Q_PROPERTY boundary is the only place the float vector crosses into Qt-land."

requirements-completed: [WTMESH-01, WTMESH-02, WTMESH-03]

# Metrics
duration: ~120 min (incl. 4 canonical-build invocations triggered by 2 build-fix iterations)
completed: 2026-07-12
---

# Phase 109 Plan 01: Option B Wipe-Tower Mesh Readback And Real Rendering Summary

**Real wipe-tower mesh (convex hull of merged real_wipe_tower_mesh + real_brim_mesh) captured by value in the SliceService worker and rendered as a triangle set when wipe_tower_mesh_data is populated; Option A dimensioned box preserved as the else-branch fallback. Re-opens Phase 99 Frozen Decision 2.**

## Performance

- **Duration:** ~120 min (4 canonical-build invocations; ~25 min each for the libslic3r reconfigure + full link)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 5 (109-01-01 through 109-01-05) + 2 build-fix commits
- **Files modified:** 13 production/test files + this summary

## Accomplishments

- **Task 109-01-01 (commit 9a11617):** WipeTowerGeometry POD EXTENDED (not replaced) with `bool hasRealMesh = false` and `std::vector<float> meshVertices` (flattened XYZ triples). The v4.4 dim fields stay unchanged so the Option A fallback path still works. The worker capture (SliceService.cpp, existing `if (print.has_wipe_tower())` block) is extended to mirror upstream `load_real_wipe_tower_preview` (3DScene.cpp:906-914): when `wipe_tower_mesh_data != std::nullopt`, copy `real_wipe_tower_mesh`, conditionally merge `real_brim_mesh`, run `convex_hull_3d()`, extract `its.vertices` as flattened XYZ floats into `meshVertices`, set `hasRealMesh = true`. NO TriangleMesh* or its* escapes the worker (Frozen Decision 1 extended).
- **Task 109-01-02 (commit 6fe16c0):** New `GizmoGeometry::buildWipeTowerMeshVertices(meshVertices, x, z)` helper added PARALLEL to the frozen Option A `buildWipeTowerVertices` (Phase 99 Frozen Decision 2 baseline UNCHANGED). The builder maps each captured (mx, my, mz) to Qt space (mx, mz + kGroundY, my) -- the upstream Y -> Qt Z transform. `uploadWipeTowerBuffer` gets an Option B branch: when `m_wipeTowerHasRealMesh` is true, call `buildWipeTowerMeshVertices`; otherwise the existing Option A `else` branch runs (buildWipeTowerVertices with the real sliced dims). Defensive empty-mesh fallback to Option A. `synchronize()` pulls the new state and fires `m_wipeTowerDirty` on mesh change.
- **Task 109-01-03 (commit 040081b):** Q_PROPERTY chain wired end-to-end. EditorViewModel exposes `wipeTowerHasRealMesh` (bool) + `wipeTowerMeshVertices` (QVariantList) with NOTIFY `wipeTowerGeometryChanged`. The `onWipeTowerGeometryReady` slot mirrors `geometry.hasRealMesh` + `geometry.meshVertices` on the valid path, and FORCES `hasRealMesh=false` + clears `meshVertices` on the invalid path (no stale mesh leak). RhiViewport + SoftwareViewport expose the same Q_PROPERTYs; the setters convert QVariantList <-> std::vector<float> at the boundary. PreparePage.qml GLViewport binds both to `root.editorVm`. SoftwareViewport implements the QPainter triangle-soup mesh projection (WM-07 mirror) reusing the existing depth-sorted faces pipeline.
- **Task 109-01-04 (commit 26f18bd):** Two regression-test slots. `ViewModelSmokeTests::wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback` drives `SliceService::wipeTowerGeometryReady` directly (no real slice) and asserts the WTMESH-01 valid path (hasRealMesh=true + 18-float QVariantList round-trip) AND the WTMESH-02 Option A fallback gate (subsequent hasRealMesh=false forces the editor to clear mesh state). `QmlUiAuditTests::optionBWipeTowerMeshCoexistsWithOptionA` is a 7-group source-audit lock proving Option B COEXISTS with Option A (POD fields, worker capture, parallel builder, renderer branch, Q_PROPERTY chain, PreparePage binding, SoftwareViewport mirror).
- **Task 109-01-05 (this summary):** Canonical build + regression ctest + source-audit verification.

## Task Commits

Each task was committed atomically:

1. **Task 109-01-01: Extend WipeTowerGeometry POD + worker capture for real wipe-tower mesh** -- `9a11617` (feat)
2. **Task 109-01-02: Add buildWipeTowerMeshVertices + uploadWipeTowerBuffer Option B branch** -- `6fe16c0` (feat)
3. **Task 109-01-03: Wire Option B real-mesh Q_PROPERTY chain + SoftwareViewport mirror** -- `040081b` (feat)
4. **Task 109-01-04: Add Option B real-mesh regression test + coexistence source-audit** -- `26f18bd` (test)
5. **Task 109-01-05: Build + ctest + SUMMARY** -- this summary (no separate code commit)

**Build-fix commits (beyond the planned 4 task commits):**

- `eb4e1ee` (fix) -- remove duplicate `assembleViewDataPool` test declaration in ViewModelSmokeTests.cpp (the Edit that inserted the new test left a duplicate function header line, causing C2144/C2761).
- `ef75edf` (fix) -- drop C++ builder names from PreparePage.qml Option B comment (the comment mentioned `buildWipeTowerMeshVertices`, tripping the `rhiCutPlaneAndWipeTowerStayCppOwned` forbidden-token audit).

## Files Created/Modified

- `src/core/services/SliceService.h` -- WipeTowerGeometry POD extended with `hasRealMesh` + `meshVertices` (:81-97). [109-01-01, 9a11617]
- `src/core/services/SliceService.cpp` -- worker capture block extended to merge real_brim_mesh + convex_hull_3d + extract its.vertices as flattened XYZ floats (:666-706). [109-01-01, 9a11617]
- `src/core/rendering/GizmoGeometry.h` -- `buildWipeTowerMeshVertices` declaration (:82-107). [109-01-02, 6fe16c0]
- `src/core/rendering/GizmoGeometry.cpp` -- `buildWipeTowerMeshVertices` implementation (:537-610). [109-01-02, 6fe16c0]
- `src/qml_gui/Renderer/RhiViewportRenderer.h` -- `m_wipeTowerHasRealMesh` + `m_wipeTowerMeshVertices` members (:223-233). [109-01-02, 6fe16c0]
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` -- synchronize() pulls new state (:176-201) + uploadWipeTowerBuffer Option B branch (:1083-1121). [109-01-02, 6fe16c0]
- `src/qml_gui/Renderer/RhiViewport.h` -- storage members (:318-332) + Q_PROPERTYs (:61-68) + getters/setters (:203-208). [109-01-02 + 109-01-03]
- `src/qml_gui/Renderer/RhiViewport.cpp` -- setters + QVariantList conversion (:325-365). [109-01-03, 040081b]
- `src/qml_gui/Renderer/SoftwareViewport.h` -- Q_PROPERTYs + storage members + getters/setters. [109-01-03, 040081b]
- `src/qml_gui/Renderer/SoftwareViewport.cpp` -- setters + QPainter triangle-soup mesh projection (:485-536). [109-01-03, 040081b]
- `src/core/viewmodels/EditorViewModel.h` -- Q_PROPERTYs (:656-666) + getters (:751-752) + storage members (:1093-1099). [109-01-03, 040081b]
- `src/core/viewmodels/EditorViewModel.cpp` -- onWipeTowerGeometryReady mirrors + clears mesh (:5111-5130) + QVariantList getter (:5141-5148). [109-01-03, 040081b]
- `src/qml_gui/pages/PreparePage.qml` -- wipeTowerHasRealMesh + wipeTowerMeshVertices bindings (:1676-1684). [109-01-03, 040081b + fix ef75edf]
- `tests/ViewModelSmokeTests.cpp` -- wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback test (declaration :325, implementation :4207-4300). [109-01-04, 26f18bd + fix eb4e1ee]
- `tests/QmlUiAuditTests.cpp` -- optionBWipeTowerMeshCoexistsWithOptionA source-audit slot (declaration :240-256, implementation :3714-3828). [109-01-04, 26f18bd]
- `.planning/phases/109-option-b-wipe-tower-mesh-readback-and-real-rendering/109-01-SUMMARY.md` -- this summary. [109-01-05]

## Decisions Made

- **POD extension, not replacement.** The v4.4 WipeTowerGeometry dim fields are the Option A fallback contract; replacing the POD would have broken every consumer. Extending with default-false/default-empty fields preserves the existing contract and lets the new fields be progressively consumed.
- **Capture-by-value extends to flat float vectors.** The convex-hull mesh must reach the GUI thread without carrying a TriangleMesh* or its* (Frozen Decision 1). Extracting `its.vertices` into a `std::vector<float>` of flattened XYZ triples in the worker is the simplest faithful mapping; the QVariantList conversion at the Q_PROPERTY boundary is the only Qt-land crossing.
- **Parallel builder, frozen Option A untouched.** Phase 99 Frozen Decision 2 froze Option A as the v4.4 baseline. Re-opening the decision for Option B does NOT un-freeze Option A -- Option B is added alongside it. The `buildWipeTowerVertices` body is byte-identical to v4.4; the uploadWipeTowerBuffer Option A call site is the unchanged `else` branch.
- **EditorViewModel clears mesh state on the invalid path.** A prior multi-material slice may have populated `meshVertices`; a subsequent single-material re-slice produces `hasRealMesh=false` (WipeTowerData::clear() resets wipe_tower_mesh_data to nullopt). Forcing the editor to clear the mesh state on the invalid path guarantees no stale real mesh leaks through. The dims stay (matching the v4.4 WTREAD-02 gate convention); only the mesh state is cleared.
- **SoftwareViewport implements the QPainter mesh projection rather than deferring.** The plan allowed deferral if QPainter mesh projection was non-trivial, but the existing paint path already had a depth-sorted faces pipeline (used for model meshes + the Option A wipe-tower box). Emitting the Option B convex-hull triangles into the same vector was ~50 lines and reuses all the projection + sorting + drawing machinery. This keeps the software fallback at parity with the RHI path.
- **Defensive empty-mesh fallback to Option A in uploadWipeTowerBuffer.** The capture invariant guarantees `meshVertices` is non-empty when `hasRealMesh=true`, but if a future regression produces an empty mesh, the renderer falls back to Option A rather than silently dropping the tower.

## Deviations from Plan

Two build-fix commits beyond the planned 4 task commits. Both were caught by the canonical build + regression ctest (no silent failures).

### Auto-fixed Issues

**1. Duplicate test-function declaration (C2144/C2761)**
- **Found during:** Task 109-01-05 (canonical build verification)
- **Issue:** The Edit that inserted the new `wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback` test in ViewModelSmokeTests.cpp used an `old_string` that matched the end of the prior test plus the start of the next test's function header. The replacement included a trailing `void ViewModelSmokeTests::assembleViewDataPoolIsolatedFromPrepareAndPreview()` line, which duplicated the original next-function header immediately following. The compiler saw two consecutive declarations of the same function (C2761) and a missing semicolon before the second `void` (C2144).
- **Fix:** Removed the duplicate line so the new test's closing brace is followed directly by the original `assembleViewDataPoolIsolatedFromPrepareAndPreview` definition.
- **Files modified:** tests/ViewModelSmokeTests.cpp
- **Verification:** Re-ran the canonical build; ViewModelSmokeTests.exe compiled and linked clean; the full ViewModel smoke suite passed including the new test.
- **Committed in:** `eb4e1ee` (build-fix commit)

**2. Forbidden-token audit failure (rhiCutPlaneAndWipeTowerStayCppOwned)**
- **Found during:** Task 109-01-05 (regression ctest run)
- **Issue:** The Phase 109 comment in PreparePage.qml mentioned the C++ builder names (`buildWipeTowerMeshVertices`, `buildWipeTowerVertices`) to document the renderer branch. The pre-existing `rhiCutPlaneAndWipeTowerStayCppOwned` source-audit (QmlUiAuditTests.cpp:1850-1861) forbids the token `buildWipeTower` in PreparePage.qml to keep geometry builders C++-owned (QML must not reference C++ internals). The substring match tripped.
- **Fix:** Rephrased the comment to describe the renderer branch in user-facing terms ("real convex-hull mesh from the slice engine" vs "Option A dimensioned box") without referencing the C++ builder names. The binding values are unchanged.
- **Files modified:** src/qml_gui/pages/PreparePage.qml
- **Verification:** Re-ran the canonical build + regression ctest; `rhiCutPlaneAndWipeTowerStayCppOwned` PASSED; the new `optionBWipeTowerMeshCoexistsWithOptionA` slot also PASSED.
- **Committed in:** `ef75edf` (build-fix commit)

---

**Total deviations:** 2 auto-fixed (1 duplicate-declaration syntax error, 1 forbidden-token audit)
**Impact on plan:** Both auto-fixes necessary for build/test green. No scope creep.

## Issues Encountered

- **Background canonical-build timeout under the harness sleep-poll pattern.** The `auto_verify_with_vcvars.ps1` script takes ~25 min per invocation (full libslic3r reconfigure + link). The harness's 2-min sleep-poll pattern killed the background task mid-build twice. The fix was to re-run the script and let it finish; the build is incremental via ninja so subsequent runs after a clean libslic3r link are faster. No code impact.

## Build + Test Commands Run + Results

### Source-audit greps (PASS)

1. `grep -rn "buildWipeTowerMeshVertices\|hasRealMesh\|m_wipeTowerHasRealMesh" src/` -- PASS (52 hits across SliceService.h/.cpp, GizmoGeometry.h/.cpp, RhiViewportRenderer.h/.cpp, RhiViewport.h/.cpp, EditorViewModel.h/.cpp, SoftwareViewport.h/.cpp).
2. `grep -rn "buildWipeTowerVertices\b" src/` -- PASS (Option A builder preserved in both header declaration and cpp implementation; cited in comments but body unchanged).
3. libslic3r-type-escape check: `grep -E "TriangleMesh\*|its\*|WipeTowerData\*|Print\*" src/core/services/SliceService.h` -- PASS (0 hits; the queued lambda captures the POD by value).

### Canonical build (scripts/auto_verify_with_vcvars.ps1)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Result:** PASS -- OWzxSlicer.exe + E2EWorkflowTests.exe + ViewModelSmokeTests.exe + PrepareSceneDataTests.exe + QmlUiAuditTests.exe + PartPlateTests.exe all compile and link clean. The first run (build/109-01-verify.log) caught the C2144 duplicate-declaration error; the second run (build/109-01-verify3.log) caught the forbidden-token audit failure; the final run (build/109-01-verify4.log) compiled + linked all targets green and ran all four regression test exes.

### Regression ctest (run by the canonical verifier)

- **Result:** ALL FOUR PASSED.
  - PrepareSceneDataTests.exe -- PASS (12 passed, 0 failed, 2ms)
  - PartPlateTests.exe -- PASS (51 passed, 0 failed, 957ms)
  - ViewModelSmokeTests.exe -- PASS (99 passed, 0 failed, 7654ms; including the new `wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback` test -- WTMESH-01 valid path with 18-float QVariantList round-trip + WTMESH-02 Option A fallback gate with mesh-state clearing). Verified via `-v2`: the new test's PASS line appears in the verbose log.
  - QmlUiAuditTests.exe -- PASS (76 passed, 0 failed, 207ms; including the new `optionBWipeTowerMeshCoexistsWithOptionA` slot -- 7 WTMESH-named assertion groups all green; and the `rhiCutPlaneAndWipeTowerStayCppOwned` forbidden-token audit now passes again after the PreparePage comment fix). Verified via `-v2`: the new test's PASS line appears in the verbose log.

### Encoding guard

- All modified production + test files use English ASCII comments only (verified by the pre-commit hook pattern + manual review).

### `git diff --check`

- Exits 0 on all commits (no whitespace errors).

## Self-Check: Capture-By-Value Invariant + Option A/Option B Coexistence

- **No TriangleMesh* or its* escapes the worker (Frozen Decision 1 extended).** The WipeTowerGeometry struct (SliceService.h:49-97) carries `std::vector<float> meshVertices` -- pure float, no libslic3r type. The worker (SliceService.cpp:666-706) extracts `shell.its.vertices` (a `std::vector<Vec3f>`) by pushing `v.x()`, `v.y()`, `v.z()` into `capturedGeometry.meshVertices` between `print.process()` and `activePrint_.store(nullptr)`. The queued lambda (SliceService.cpp:799) captures `capturedGeometry` by value; the GUI-thread emit (SliceService.cpp:885) delivers the POD. Confirmed by grep: zero `TriangleMesh*` / `its*` / `WipeTowerData*` / `Print*` captures in SliceService.h; the worker-local `mergedMesh` + `shell` go out of scope before the lambda capture.
- **Option A baseline preserved (Phase 99 Frozen Decision 2).** `buildWipeTowerVertices` (GizmoGeometry.cpp:487-535) is byte-identical to v4.4. The `uploadWipeTowerBuffer` Option A call site (RhiViewportRenderer.cpp:1112-1119) is the unchanged `else` branch. The `optionBWipeTowerMeshCoexistsWithOptionA` source-audit locks both paths exist + the gate is `m_wipeTowerHasRealMesh`.
- **Option B gate is `hasRealMesh` (WTMESH-02).** When `wipe_tower_mesh_data == std::nullopt` (single-material / pre-slice / mock slice), `capturedGeometry.hasRealMesh` stays false, `meshVertices` is empty, the EditorViewModel exposes `wipeTowerHasRealMesh=false`, and the renderer takes the Option A `else` branch. The regression test (ViewModelSmokeTests.cpp) asserts the gate explicitly: the WTMESH-02 path forces `editor.wipeTowerHasRealMesh()==false` + `editor.wipeTowerMeshVertices().size()==0`.
- **SoftwareViewport at parity with RHI (WM-07).** Both paths consume `m_wipeTowerHasRealMesh` + `m_wipeTowerMeshVertices`; the QPainter path projects the convex-hull triangles with the same color + coordinate transform as the RHI path (buildWipeTowerMeshVertices).

## Re-Opens Phase 99 Frozen Decision 2

Phase 99 Frozen Decision 2 (99-GAP-MATRIX.md) locked Option B (real wipe-tower mesh) as a future upgrade and froze Option A (dimensioned box) as the v4.4 baseline. Phase 109 RE-OPENS that decision and implements Option B as a PARALLEL path:

- Option A `buildWipeTowerVertices` is byte-identical to v4.4 (the frozen baseline is preserved).
- Option B `buildWipeTowerMeshVertices` is added alongside it.
- `uploadWipeTowerBuffer` branches on `m_wipeTowerHasRealMesh`: Option B if true, Option A `else`.
- Single-material / pre-slice / mock paths still take Option A (the engine did not populate `wipe_tower_mesh_data`).

The Option A regression ctest from Phase 100-102 (`wipeTowerReadbackAndRenderAnchorsPresent`, `wipeTowerRealDimsReachRendererPipeline`, `wipeTowerGeometryReadbackAppliesValidAndInvalidGate`) all still pass unchanged.

## Next Phase Readiness

- **Phase 116 (WTMESH-04 regression lock):** can now lock the Option A + Option B coexistence at the regression-ctest level. The source-audit slot `optionBWipeTowerMeshCoexistsWithOptionA` (QmlUiAuditTests.cpp) is the structural lock; Phase 116 adds the runtime visual-evidence proof (real multi-material slice producing a real mesh on screen).
- **Per-extruder color slabs on the Option B mesh:** anti-feature (upstream chose the convex-hull silhouette over per-extruder stripes at 3DScene.cpp:914). Stays deferred.
- No blockers.

## Self-Check: PASSED

- All source-audit greps return the expected hits.
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier.
- All four regression test exes pass (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests, QmlUiAuditTests).
- ViewModelSmokeTests includes the new `wipeTowerRealMeshReadbackGatesOptionBAndOptionAFallback` (WTMESH-01 valid path + WTMESH-02 fallback gate).
- QmlUiAuditTests includes the new `optionBWipeTowerMeshCoexistsWithOptionA` (7 WTMESH assertion groups) -- 76 passed, 0 failed.
- `git diff --check` exits 0; encoding guard clean (English ASCII comments).
- Capture-by-value invariant preserved (no TriangleMesh* or its* escapes the worker).
- Option A baseline byte-identical to v4.4 (Phase 99 Frozen Decision 2 preserved).
- Option B gate is `hasRealMesh`; single-material / pre-slice / mock paths take Option A.

---
*Phase: 109-option-b-wipe-tower-mesh-readback-and-real-rendering*
*Plan: 01*
*Completed: 2026-07-12*
