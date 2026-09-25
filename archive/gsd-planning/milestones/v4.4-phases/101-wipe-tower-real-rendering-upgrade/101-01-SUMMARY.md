---
phase: 101-wipe-tower-real-rendering-upgrade
plan: 01
subsystem: ui
tags: [wipe-tower, rhi, software-viewport, qpainter, rendering, regression-test, option-a-baseline]

# Dependency graph
requires:
  - phase: 100-wipe-tower-geometry-readback
    provides: WipeTowerGeometry POD captured by value in SliceService worker; wipeTowerGeometryReady signal; EditorViewModel 6 Q_PROPERTYs (showWipeTower, wipeTowerWidth/Depth/Height/X/Z) with NOTIFY wipeTowerGeometryChanged; PreparePage.qml:1648 GLViewport binds all 6 to editorVm; SoftwareViewport m_showWipeTower default false (defensive WTREAD-02); Phase 100 REVIEW W1 fix (corner-to-center conversion, commit b12d0e5).
  - phase: 99-wipe-tower-geometry-gap-audit
    provides: Frozen Decision 2 (WT-RENDER-UPGRADE) - Option A dimensioned box LOCKED as v4.4 baseline; Option B real mesh documented as future upgrade. WT-PLACEHOLDER-BOX + WT-RENDERER-BUFFER + WT-RENDER-UPGRADE regions mapped to Phase 101.
provides:
  - Option A dimensioned-box wipe-tower rendering LOCKED as the v4.4 baseline via a documentation comment in GizmoGeometry::buildWipeTowerVertices citing the upstream source-truth anchors (3DScene.cpp:840-885 load_wipe_tower_preview + make_cube at :855).
  - Option B (real wipe-tower mesh via wipe_tower_mesh_data->real_wipe_tower_mesh + real_brim_mesh + convex_hull_3d) documented as a LOCKED future upgrade requiring WTAUDIT-02 re-opening + an ITS vertex format extension in GizmoGeometry + RhiViewportRenderer.
  - SoftwareViewport rendering gap CLOSED: the software fallback path now renders the real-dims wipe-tower box via a QPainter paint in paintScene (gated on m_showWipeTower, mirrors the RHI box geometry + color), so the fallback path does not lag the RHI path.
  - Regression test wipeTowerRealDimsReachRendererPipeline locking the contract that the real sliced dims reach the render pipeline (PreparePage.qml GLViewport binds all 6 wipe-tower Q_PROPERTYs to editorVm).
  - RHI render pipeline (RhiViewportRenderer uploadWipeTowerBuffer / renderWipeTower / synchronize) confirmed UNCHANGED - it was already correct end-to-end after Phase 100.
affects: [102-wipe-tower-verification-and-regression]

# Tech tracking
tech-stack:
  added: []
  patterns: [QPainter depth-sorted face injection into a QQuickPaintedItem paint path (wipe-tower box faces appended into the existing painter's-algorithm faces vector), source-audit fallback regression test pattern (read a .qml file and assert binding strings present when the RHI widget cannot be constructed headless)]

key-files:
  created:
    - .planning/phases/101-wipe-tower-real-rendering-upgrade/101-01-SUMMARY.md
  modified:
    - src/core/rendering/GizmoGeometry.cpp (task 101-01-01, commit 50236d8 - Option A baseline + Option B deferral comment above buildWipeTowerVertices; function body unchanged)
    - src/qml_gui/Renderer/SoftwareViewport.cpp (task 101-01-02, commit 7fadc8a - QPainter wipe-tower box in paintScene consuming the 6 Q_PROPERTYs)
    - tests/ViewModelSmokeTests.cpp (task 101-01-03, commit 0634950 - wipeTowerRealDimsReachRendererPipeline regression test)

key-decisions:
  - "The RHI render pipeline (RhiViewportRenderer::uploadWipeTowerBuffer at .cpp:1064-1095, renderWipeTower at :1894-1908, synchronize dim-pull at :171-189) was ALREADY correct end-to-end after Phase 100 - it feeds real dims to buildWipeTowerVertices and rebuilds on m_wipeTowerDirty. Phase 101 confirms this with a regression test + a documentation comment; it does NOT rewrite the pipeline. git diff b12d0e5..HEAD on RhiViewportRenderer.cpp is empty."
  - "Option A (dimensioned box) is LOCKED as the v4.4 baseline per Phase 99 Frozen Decision 2. The comment in buildWipeTowerVertices cites the upstream box-preview source-truth (3DScene.cpp:840-885 load_wipe_tower_preview + make_cube at :855). Option B (real mesh via wipe_tower_mesh_data->real_wipe_tower_mesh + real_brim_mesh + convex_hull_3d, mirroring 3DScene.cpp:887-925 load_real_wipe_tower_preview) is documented as a LOCKED future upgrade requiring an ITS vertex format extension + WTAUDIT-02 re-opening. Zero Option B executable code landed in src/ (WTR-05)."
  - "The SoftwareViewport rendering gap was closed with the preferred path (a) - a minimal QPainter wipe-tower box in paintScene - rather than the deferral path (b). The box is gated on m_showWipeTower (WTREAD-02), uses the real dims (center convention post-W1-fix), mirrors the RHI box geometry (x-hw..x+hw, z-hd..z+hd, kGroundY..kGroundY+height, Y-up) and color {0.35, 0.60, 0.85, 0.50}, and emits its 5 visible faces (top + 4 sides) into the existing depth-sorted faces vector so they render via the same painter's algorithm as model meshes (no new transform)."
  - "The regression test uses the PreparePage.qml source-audit fallback, NOT RhiViewport construction, because RhiViewport is a QQuickRhiItem requiring a QRhi context that cannot be constructed in the headless test harness. The test reads PreparePage.qml and asserts the GLViewport instance (id viewport3d, ~:1648) binds all 6 wipe-tower Q_PROPERTYs to root.editorVm plus the WTREAD-02 null-editorVm default (show=false). Combined with the Phase 100 readback test (which proves the EditorViewModel Q_PROPERTYs receive real dims from SliceService), this locks the end-to-end dim-reach contract."

patterns-established:
  - "QPainter depth-sorted face injection: when adding a box primitive to a QQuickPaintedItem paint path that already depth-sorts triangle faces, append the new primitive's faces (as quads) into the existing faces vector with the same depth metric (rotated-point Z average) so the unified painter's algorithm handles inter-penetration with existing meshes. Avoids inventing a separate draw pass or transform."
  - "Source-audit fallback regression test: when the binding target widget (e.g. a QQuickRhiItem needing a QRhi context) cannot be constructed in a headless test harness, lock the QML binding contract by reading the .qml source and asserting the exact binding strings are present. This catches accidental unbinds without requiring a live render context."

requirements-completed: [WTRENDER-01, WTRENDER-02]

# Metrics
duration: ~35 min (incl. ~25 min canonical-build libslic3r reconfigure + OWzxSlicer.exe link that timed out the wrapper; targeted rebuild + ctest completed in ~3 min)
completed: 2026-07-12
---

# Phase 101 Plan 01: Wipe-Tower Real Rendering Upgrade Summary

**Option A dimensioned-box wipe-tower rendering LOCKED as the v4.4 baseline (comment in buildWipeTowerVertices); SoftwareViewport rendering gap closed with a QPainter box mirroring the RHI path; regression test locks the dim-reach contract; RHI pipeline confirmed unchanged. WTRENDER-01 + WTRENDER-02 closed.**

## Performance

- **Duration:** ~35 min (canonical build libslic3r reconfigure + OWzxSlicer.exe link consumed the wrapper budget at step 237/239; targeted ninja rebuild of the 5 regression targets + ctest completed the verification in ~3 min)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 4 (101-01-01 through 101-01-04)
- **Files modified:** 3 source/test files + this summary

## Accomplishments

- **Task 101-01-01 (commit 50236d8):** Added a 36-line documentation comment block above `GizmoGeometry::buildWipeTowerVertices` (GizmoGeometry.cpp:449) locking Option A as the v4.4 baseline per Phase 99 Frozen Decision 2. The comment cites the upstream Option A source-truth (3DScene.cpp:840-885 load_wipe_tower_preview + make_cube at :855) and documents Option B (3DScene.cpp:887-925 load_real_wipe_tower_preview + convex_hull_3d at :914 from wipe_tower_mesh_data->real_wipe_tower_mesh + real_brim_mesh at Print.hpp:745-746/:766) as a LOCKED future upgrade requiring an ITS vertex format extension + WTAUDIT-02 re-opening. The Phase 100 REVIEW W1 input contract (x/z arrive as box center) is also documented. The function body is byte-for-byte unchanged (comment-only).
- **Task 101-01-02 (commit 7fadc8a):** Closed the SoftwareViewport rendering gap (WT-SOFTWARE-VIEWPORT from 99-GAP-MATRIX) by adding a QPainter wipe-tower box to `SoftwareViewport::paintScene`. The box is gated on `m_showWipeTower` (WTREAD-02 - no placeholder leak on single-material slices), uses the real dims from the 6 Q_PROPERTYs (center convention), mirrors the RHI box geometry (x-hw..x+hw, z-hd..z+hd, kGroundY -0.04f..kGroundY+height, Y-up) and color {0.35, 0.60, 0.85, 0.50} (GizmoGeometry.cpp:462-463), and emits its 5 visible faces (top + 4 sides; bottom is coplanar with the bed grid and hidden) into the existing depth-sorted faces vector so they render via the unified painter's algorithm. The Q_PROPERTY declarations, setters, and defaults are unchanged (Phase 100 left them correct: show=false, zero dims).
- **Task 101-01-03 (commit 0634950):** Added the `wipeTowerRealDimsReachRendererPipeline` regression test to tests/ViewModelSmokeTests.cpp. The test uses the PreparePage.qml source-audit fallback (RhiViewport is a QQuickRhiItem that cannot be constructed headless): it reads PreparePage.qml and asserts the GLViewport instance (id viewport3d, ~:1648) binds all 6 wipe-tower Q_PROPERTYs (showWipeTower, wipeTowerWidth/Depth/Height/X/Z) to root.editorVm, plus the WTREAD-02 null-editorVm default (show=false). Combined with the Phase 100 readback test, this locks the end-to-end dim-reach contract.
- **Task 101-01-04 (this summary, no separate code commit):** Source-audit greps (4/4 pass), canonical build (OWzxSlicer.exe linked clean before wrapper timeout), targeted ninja rebuild of 5 regression targets, regression ctest (4/4 pass), ViewModelSmokeTests full verbose run (97 passed, 0 failed).

## Task Commits

Each task was committed atomically:

1. **Task 101-01-01: Lock Option A baseline + document Option B as deferred** - `50236d8` (docs)
2. **Task 101-01-02: Close SoftwareViewport rendering gap with QPainter box** - `7fadc8a` (feat)
3. **Task 101-01-03: Add wipeTowerRealDimsReachRendererPipeline regression test** - `0634950` (test)
4. **Task 101-01-04: Build + ctest regression + source-audit verification** - this summary (no separate code commit)

**Plan metadata:** this summary commit (docs: complete plan).

## Files Created/Modified

- `src/core/rendering/GizmoGeometry.cpp` - Option A baseline + Option B deferral comment block above buildWipeTowerVertices (:451-486); function body unchanged. [101-01-01, 50236d8]
- `src/qml_gui/Renderer/SoftwareViewport.cpp` - QPainter wipe-tower box in paintScene (:447-498), consuming the 6 Q_PROPERTYs, gated on m_showWipeTower, mirroring the RHI box geometry + color. [101-01-02, 7fadc8a]
- `tests/ViewModelSmokeTests.cpp` - wipeTowerRealDimsReachRendererPipeline test (declaration :307, implementation :4043-4104) using the PreparePage.qml source-audit fallback. [101-01-03, 0634950]
- `.planning/phases/101-wipe-tower-real-rendering-upgrade/101-01-SUMMARY.md` - this summary. [101-01-04]

## Decisions Made

- **RHI pipeline left unchanged.** The RhiViewportRenderer pipeline (uploadWipeTowerBuffer at .cpp:1064-1095, renderWipeTower at :1894-1908, synchronize dim-pull at :171-189) was already correct end-to-end after Phase 100 - it feeds real dims to buildWipeTowerVertices and rebuilds on m_wipeTowerDirty. Phase 101 confirms this with a regression test + documentation comment, not a rewrite. `git diff b12d0e5..HEAD -- src/qml_gui/Renderer/RhiViewportRenderer.cpp` is empty.
- **SoftwareViewport preferred path (a) taken, not deferral (b).** The plan offered two acceptable resolutions for the SoftwareViewport gap: (a) add a minimal QPainter wipe-tower box, or (b) document the gap as accepted for v4.4. Path (a) was judged small (~52 lines) and low-risk because the existing paintScene already had a depth-sorted faces vector the box faces could join. This makes the software fallback consistent with the RHI path rather than leaving it to lag.
- **Source-audit fallback for the regression test.** RhiViewport is a QQuickRhiItem (RhiViewport.h:19) requiring a QRhi context that cannot be constructed in the headless test harness. The test instead locks the QML binding contract by reading PreparePage.qml and asserting the 6 wipe-tower binding strings are present. This is the strongest feasible assertion given the constraint.
- **Option B documented in a comment only.** Per WTR-05, zero Option B executable code (`wipe_tower_mesh_data`, `real_wipe_tower_mesh`, `real_brim_mesh`, `convex_hull_3d`) lands in src/ - the field names appear ONLY in the documentation comment in GizmoGeometry.cpp:477-483. Option B requires re-opening WTAUDIT-02.

## Deviations from Plan

None - plan executed exactly as written. All four tasks followed their prescribed actions; the SoftwareViewport task took the plan's preferred path (a) rather than the deferral path (b); the test task took the plan's documented source-audit fallback (step 5) rather than RhiViewport construction (step 4).

## Issues Encountered

- **Background canonical-build timeout during the OWzxSlicer.exe link.** The libslic3r reconfigure + compile consumed the budget; the wrapper killed the build at step 237/239 (MOC/UIC for OWzxSlicer, just after libslic3r_from_source.lib linked and OWzxSlicer.exe linked clean). This is the documented prior-phase pattern (Phase 100 hit the same boundary). A targeted `ninja` rebuild of owzx_app_core + the 4 regression test targets (via a one-off helper script modeled on scripts/build_run_ppt.ps1) completed the links cleanly; the regression ctest then ran green. The targeted rebuild helper was a one-off verification artifact and was removed before the SUMMARY commit (not part of the deliverable). Production code (OWzxSlicer.exe) linked clean in the canonical build before the timeout.

## Build + Test Commands Run + Results

### Source-audit greps (4/4 PASS)

1. `rg -n "Option A|Option B|load_wipe_tower_preview|WTAUDIT-02" src/core/rendering/GizmoGeometry.cpp` - PASS (Option A baseline + Option B deferral documented at :451-485).
2. `git diff b12d0e5..HEAD -- src/qml_gui/Renderer/RhiViewportRenderer.cpp` - PASS (empty diff - RHI pipeline unchanged).
3. `rg -n "m_showWipeTower|m_wipeTowerWidth|m_wipeTowerDepth|m_wipeTowerHeight|m_wipeTowerX|m_wipeTowerZ" src/qml_gui/Renderer/SoftwareViewport.cpp` - PASS (paint now consumes all 6 Q_PROPERTYs at :455-465).
4. `rg -n "wipe_tower_mesh_data|real_wipe_tower_mesh|real_brim_mesh|convex_hull_3d" src/` - PASS (matches ONLY in the GizmoGeometry.cpp comment at :477-483; zero executable Option B code - WTR-05).

### Canonical build (scripts/auto_verify_with_vcvars.ps1)

- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (log: build/101-01-verify.log, removed after verification)
- **Result:** Production code linked clean - OWzxSlicer.exe rebuilt (timestamp 01:51, 33,820,672 bytes). Zero `error:`/`error C`/`error LNK`/`ninja: error` in the log. The wrapper killed the build at step 237/239 (MOC/UIC for OWzxSlicer) after the libslic3r reconfigure + compile consumed the budget; OWzxSlicer.exe had already linked clean. The test-exe links (ViewModelSmokeTests etc.) had not yet run when the wrapper timed out - the targeted rebuild below completed them.
- **Note:** This matches the documented Phase 100 timeout pattern. The canonical build's libslic3r reconfigure was triggered by the Phase 100 header changes (already on main), not by Phase 101 (which touched only .cpp files + a test).

### Targeted ninja rebuild (5 regression targets)

- **Command:** one-off helper script (vcvars env + `ninja -j16 <target>` for owzx_app_core, ViewModelSmokeTests, PrepareSceneDataTests, PartPlateTests, QmlUiAuditTests). Removed before commit.
- **Result:** All 5 targets built OK (exit 0). ViewModelSmokeTests.exe linked clean (proves the new test + the SoftwareViewport QPainter box + the GizmoGeometry comment all compile/link into owzx_app_core).

### Regression ctest

- **Command:** `ctest --test-dir build -R "PartPlateTests|PrepareSceneDataTests|ViewModelSmokeTests|QmlUiAuditTests" --output-on-failure`
- **Result:** **4/4 PASS** (100%, 0 failed). Total Test time (real) = 16.14 sec.
  - ViewModelSmokeTests - PASS (13.33 sec)
  - QmlUiAuditTests - PASS (0.35 sec)
  - PrepareSceneDataTests - PASS (0.04 sec)
  - PartPlateTests - PASS (1.07 sec)

### ViewModelSmokeTests full verbose run

- **Command:** `./ViewModelSmokeTests.exe -v2`
- **Result:** **Totals: 97 passed, 0 failed, 0 skipped, 0 blacklisted, 7714ms.** Up from Phase 100's 96 - the new `wipeTowerRealDimsReachRendererPipeline` test is the +1. The test ran all 8 QVERIFY assertions (file exists, file opens, 6 binding strings present, WTREAD-02 null-editorVm default present) successfully.

### Encoding guard + whitespace

- `py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py` on all 3 modified source files (GizmoGeometry.cpp, SoftwareViewport.cpp, ViewModelSmokeTests.cpp) - PASS (ASCII-only, English comments).
- `git diff --check b12d0e5..HEAD` - exits 0 (no whitespace errors).

## Self-Check: Option A Baseline Locked + Option B Deferred + RHI Unchanged

- **Option A baseline documented (WTR-01).** The comment block at GizmoGeometry.cpp:451-486 cites the upstream Option A source-truth (3DScene.cpp:840-885 load_wipe_tower_preview + make_cube at :855) and locks Option A as the v4.4 baseline per Phase 99 Frozen Decision 2. The function body is unchanged (comment-only; git diff confirms only the comment block was added).
- **RHI pipeline unchanged (WTR-02).** `git diff b12d0e5..HEAD -- src/qml_gui/Renderer/RhiViewportRenderer.cpp` is empty. uploadWipeTowerBuffer, renderWipeTower, and synchronize were already correct end-to-end after Phase 100; Phase 101 does not modify them.
- **SoftwareViewport gap closed (WTR-03).** paintScene now consumes all 6 wipe-tower Q_PROPERTYs at SoftwareViewport.cpp:455-465, gated on m_showWipeTower, mirroring the RHI box geometry + color. The box renders on the software fallback path so it does not lag the RHI path.
- **No Option B code leaked (WTR-05).** `rg -n "wipe_tower_mesh_data|real_wipe_tower_mesh|real_brim_mesh|convex_hull_3d" src/` returns matches ONLY in the GizmoGeometry.cpp documentation comment (:477-483). Zero executable Option B code.
- **No new product behavior (WTR-08).** Phase 101's additions are: (1) documentation locking Option A + deferring Option B; (2) the SoftwareViewport QPainter box (consistency with RHI path - the box renders the same real dims the RHI path already renders); (3) a regression test. No change to libslic3r, the Phase 100 readback contract, the RhiViewportRenderer pipeline, or the Phase 100 REVIEW W1 fix.

## Next Phase Readiness

- **Phase 102** (verification + regression): the source/QML audits (WTVERIFY-01) can confirm the placeholder 10/10/50/100/25 defaults are no longer steady-state when a real slice exists, and that the SoftwareViewport fallback path now renders the real-dims box alongside the RHI path. The runtime wipe-tower visibility evidence (WTVERIFY-02) can run against a real multi-material slice on both render paths. The new `wipeTowerRealDimsReachRendererPipeline` test + the Phase 100 readback test provide the data-delivery contract assertion; Phase 102 adds the runtime visual evidence.
- No blockers.

## Self-Check: PASSED

- All 4 source-audit greps return the expected hits (Option A documented; RHI unchanged; SoftwareViewport gap closed; zero Option B code).
- Production code (OWzxSlicer.exe) compiles + links clean via the canonical verifier.
- All 5 targeted rebuild targets (owzx_app_core + 4 regression test exes) link clean.
- Regression ctest 4/4 pass (ViewModelSmokeTests incl. the new test, QmlUiAuditTests, PrepareSceneDataTests, PartPlateTests).
- ViewModelSmokeTests full suite: 97 passed, 0 failed - including the new wipeTowerRealDimsReachRendererPipeline.
- `git diff --check` exits 0; encoding guard exits 0 for all 3 modified files.
- Option A baseline locked; Option B documented as deferred; RHI pipeline unchanged; no new product behavior.

---
*Phase: 101-wipe-tower-real-rendering-upgrade*
*Plan: 01*
*Completed: 2026-07-12*
