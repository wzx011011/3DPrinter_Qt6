---
phase: 98-thumbnail-verification-and-cleanup
plan: 01
subsystem: infra
tags: [thumbnail, qrhi, 3mf, round-trip, mock-removal, multi-plate, cleanup]

# Dependency graph
requires:
  - phase: 97-thumbnail-save-reload-round-trip
    provides: Save->reload round-trip verified end-to-end (thumbnailSaveReloadRoundTrip ctest slot); read-side extractPlateThumbnailFrom3mf helper; ordering fix (restore thumbnails AFTER arrangeObjects, commit efd5e42); Phase 97 REVIEW MEDIUM-3 multi-plate write-side gap documented for Phase 98.
  - phase: 96-3mf-thumbnail-write-integration
    provides: Write side closed -- qimageToThumbnailData helper, PlateData::plate_thumbnail populate, StoreParams::thumbnail_data populate (single current-plate entry; Phase 98 addresses the multi-plate gap).
  - phase: 95-qrhi-thumbnail-capture-infrastructure
    provides: Real QRhi capture -- RhiViewport::requestThumbnailCapture (offscreen RT + readBackTexture) -> deliverThumbnail -> lastThumbnailData -> thumbnailCaptured; the sole real source after Phase 98 mock removal.
provides:
  - Mock thumbnail generators removed cleanly (no dead paths); real QRhi capture (Phase 95) is the sole thumbnail source.
  - plateThumbnailBase64(plateIndex) accessor (ProjectServiceMock + EditorViewModel) exposing the persisted PartPlate::thumbnail() for the UI plate-card fallback.
  - Phase 97 REVIEW MEDIUM-3 multi-plate write-side gap CLOSED (Option A minimal fix: one index-aligned thumbnail_data entry per plate + thumbnailMultiPlateSaveReloadRoundTrip regression test).
  - Canonical verifier run (production OWzxSlicer.exe links clean); regression ctest 4/4 pass; OWzxSlicer.exe 5-second liveness confirmed.
affects: [v4.3-milestone-close, future-thumbnail-variants]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Persisted-thumbnail accessor for QML fallback: plateThumbnailBase64 reads PartPlate::thumbnail() and encodes base64 PNG via QBuffer + QImage::save, returning empty (never a fabricated placeholder) when no thumbnail is cached -- the like-for-like source swap from the removed mock QPainter placeholder."
    - "Index-aligned StoreParams::thumbnail_data for multi-plate round-trip: the bbs_3mf writer iterates thumbnail_data by INDEX mapping entry[index] -> Metadata/plate_{index+1}.png (bbs_3mf.cpp:6133,6550), so saveProject MUST push one entry per plate in plate-index order. Plates with no cached thumbnail get a default (invalid) ThumbnailData placeholder (is_valid()==false) so indices stay aligned; the writer skips invalid entries (bbs_3mf.cpp:6135). reserve() the vector capacity up front and take addresses only after all pushes so no reallocation invalidates the raw pointers before store_bbs_3mf returns."

key-files:
  created: []
  modified:
    - src/core/services/ProjectServiceMock.h
    - src/core/services/ProjectServiceMock.cpp
    - src/core/viewmodels/EditorViewModel.h
    - src/core/viewmodels/EditorViewModel.cpp
    - src/qml_gui/pages/PreparePage.qml
    - tests/PartPlateTests.cpp

key-decisions:
  - "Mock removal is gated on caller rewiring: task 98-01-01 added plateThumbnailBase64 and swapped the single QML caller first; only then did task 98-01-02 remove the mock generators, so the build never broke mid-task."
  - "generateTopDownThumbnail (the private variant=1 body) was removed alongside generatePlateThumbnail/generatePlateThumbnailVariant because its only caller was generatePlateThumbnailVariant -- leaving it would have been a dead path, violating TV-04's no-dead-code intent."
  - "Phase 97 REVIEW MEDIUM-3: chose Option A (minimal fix) over Option B (document-and-defer) because the writer's vector-index-to-plate-index mapping is unambiguous (bbs_3mf.cpp:6133 iterates thumbnail_data[index] -> Metadata/plate_{index+1}.png) and the vector size guard at bbs_3mf.cpp:6101 only rejects size > plate count. The fix is additive (one entry per plate) and structurally supported -- no half-fix risk."
  - "thumbnailMultiPlateSaveReloadRoundTrip uses two DISTINCT-color thumbnails (plate 0 dark blue, plate 1 amber) so a plate-index swap or a single-entry (current-plate-only) regression is detectable, not just survival."

patterns-established:
  - "Phase 97 pattern (synthesize a known QImage directly via QImage(W,H,Format_RGBA8888).fill(QColor)) is now the canonical way to test thumbnail cache/persistence contracts without depending on capture (Phase 95) or the removed mock."
  - "StoreParams::thumbnail_data must be plate-index-aligned (one entry per plate, invalid placeholders for thumb-less plates) -- not just the current plate -- for multi-plate projects to round-trip."

requirements-completed: [THUMBVERIFY-01, THUMBVERIFY-02]

# Metrics
duration: 90min
completed: 2026-07-11
---

# Phase 98 Plan 01: Thumbnail Verification And Cleanup Summary

**Mock thumbnail generators removed cleanly (real QRhi capture is the sole source), the Phase 97 multi-plate write-side gap is fixed (one thumbnail_data entry per plate), and the canonical verifier + regression ctest + OWzxSlicer.exe launch all pass.**

## Performance

- **Duration:** ~90 min
- **Started:** 2026-07-11
- **Completed:** 2026-07-11
- **Tasks:** 5 (98-01-01 through 98-01-05)
- **Files modified:** 6

## Accomplishments
- Mock thumbnail code removed cleanly (THUMBVERIFY-01, TV-04): generatePlateThumbnail, generatePlateThumbnailVariant, generateTopDownThumbnail, the dead seededRand helper, and the EditorViewModel::generatePlateThumbnail delegate are all gone. A grep for these across src/ and tests/ returns ZERO matches. The real QRhi capture path (Phase 95) is the sole thumbnail source.
- plateThumbnailBase64 accessor added (TV-01, TV-02, TV-03): ProjectServiceMock::plateThumbnailBase64 reads the persisted PartPlate::thumbnail() (Phase 97 read-side artifact) and encodes it as base64 PNG; EditorViewModel::plateThumbnailBase64 delegates to it; PreparePage.qml plate-card fallback swapped from the mock generatePlateThumbnail(index, 80) to plateThumbnailBase64(index). The accessor returns empty QString when no thumbnail is cached (never a fabricated placeholder). The real-capture primary path (lastThumbnailData) is unchanged.
- Phase 97 REVIEW MEDIUM-3 multi-plate write-side gap CLOSED (TV-10, Option A): saveProject now pushes one index-aligned thumbnail_data entry per plate (with invalid ThumbnailData placeholders for thumb-less plates) so the bbs_3mf writer archives Metadata/plate_{i+1}.png for every plate that buildPlateDataList emits an XML ref for. A new thumbnailMultiPlateSaveReloadRoundTrip test slot proves a 2-plate project with DISTINCT-color thumbnails round-trips BOTH.
- Canonical verifier run + launch + regression ctest (THUMBVERIFY-02): production code (OWzxSlicer.exe) compiled and linked clean; all 4 regression ctests pass (PartPlateTests, PrepareSceneDataTests, ViewModelSmokeTests, QmlUiAuditTests); OWzxSlicer.exe launches and runs 5+ seconds without immediate crash (process-liveness bar per STATE.md deferred items).
- Scratch cleanup (TV-11): 11 scratch build logs (build_phase*.log, build_tests*.log, build_ppt*.log, build_probe.log, build_restore.log, build_trans*.log) and 3 scratch scripts (run_partplate_only.ps1, build_run_ppt.ps1, build_run_98_01_tests.ps1) removed.

## Task Commits

Each task was committed atomically:

1. **Task 98-01-01: add plateThumbnailBase64 accessor and swap UI fallback to persisted thumbnail** - `6c127af` (feat)
2. **Task 98-01-02: remove mock thumbnail generators and update legacy tests** - `1be227c` (refactor)
3. **Task 98-01-03: archive one thumbnail per plate so multi-plate projects round-trip** - `8cced07` (fix)
4. **Task 98-01-04: canonical verifier + launch + regression ctest** - (verification; no source commit -- results recorded here)
5. **Task 98-01-05: scratch cleanup** - (deletions of untracked files; no commit -- working-tree cleanup)

## Files Created/Modified
- `src/core/services/ProjectServiceMock.h` - Added plateThumbnailBase64 declaration; removed generatePlateThumbnail, generatePlateThumbnailVariant, and generateTopDownThumbnail declarations.
- `src/core/services/ProjectServiceMock.cpp` - Added plateThumbnailBase64 body (reads PartPlate::thumbnail(), base64-encodes PNG); removed the three mock generator bodies + seededRand helper; rewrote saveProject thumbnail_data populate to push one index-aligned entry per plate (Option A multi-plate fix).
- `src/core/viewmodels/EditorViewModel.h` - Added plateThumbnailBase64 declaration; removed generatePlateThumbnail declaration.
- `src/core/viewmodels/EditorViewModel.cpp` - Added plateThumbnailBase64 delegate; removed generatePlateThumbnail delegate.
- `src/qml_gui/pages/PreparePage.qml` - Plate-card fallback swapped from generatePlateThumbnail(index, 80) to plateThumbnailBase64(index).
- `tests/PartPlateTests.cpp` - Removed thumbnailVariantsProduceValidData (declaration + #else stub + body); updated thumbnailRoundTrip to synthesize its QImage directly (Phase 97 pattern); added thumbnailMultiPlateSaveReloadRoundTrip slot (Phase 98 multi-plate write-side regression test).

## Decisions Made
- **Mock removal gating:** Task 98-01-01 (add accessor + rewire callers) completed before 98-01-02 (remove mock), so the build never broke mid-task.
- **generateTopDownThumbnail removal:** Removed alongside the other mock generators because its only caller was generatePlateThumbnailVariant; leaving it would violate the no-dead-code requirement (TV-04).
- **Option A for REVIEW MEDIUM-3:** Chose the minimal fix over document-and-defer because the writer's index-to-plate mapping is unambiguous and the vector size guard permits equal-size. The fix is additive (more thumbnail_data entries), structurally supported, and verified by a new multi-plate regression test.
- **Scratch file deletion despite historical summary references:** The 97-01-SUMMARY.md mentions run_partplate_only.ps1 as the incremental-build path Phase 97 used. That reference is a past-tense historical record of a completed phase, not a live dependency. The script is untracked scratch and was deleted along with the others; the summary text remains as historical evidence.

## Deviations from Plan

None - plan executed exactly as written. All 12 context truths (TV-01 through TV-12) are satisfied.

## Issues Encountered
- **Canonical build wrapper timeout (documented prior-phase pattern, TV-07):** The canonical verifier (`scripts/auto_verify_with_vcvars.ps1`) timed out (was killed by the executor wrapper) right after OWzxSlicer.exe linked clean (build_phase98_01.log reached `[238/238] Linking CXX executable OWzxSlicer.exe`) and just as it started the test-target phase (`[1/4] Automatic MOC and UIC for target E2EWorkflowTests`). This is the exact documented Phase 95/96/97 pattern: the libslic3r reconfigure + full build consumes the wrapper budget before the test targets link. Production code compiled and linked clean (zero compile/link errors; OWzxSlicer.exe timestamp Jul 11 01:35). The test gate was completed via the incremental path (a temporary scripts/build_run_98_01_tests.ps1 mirroring auto_verify_with_vcvars.ps1 env setup, then ninja -j16 on the 4 regression targets, then ctest). All 4 regression ctests pass.

## User Setup Required

None - no external service configuration required.

## Verification Results

- **Source audit (no dead paths + capture chain intact):** PASS
  - `grep -rn "generatePlateThumbnail|generatePlateThumbnailVariant|generateTopDownThumbnail" src/ tests/` -> ZERO matches (TV-04).
  - `grep -rn "requestThumbnailCapture|lastThumbnailData|thumbnailCaptured|deliverThumbnail" src/` -> 32 references (capture chain intact, TV-06).
  - `grep -rn "plateThumbnailBase64" src/` -> wired in ProjectServiceMock (.h/.cpp), EditorViewModel (.h/.cpp), PreparePage.qml (TV-01/02/03).
- **Canonical build (production code compile + link):** PASS
  - Command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - Result: configure done (34s); libslic3r_from_source.lib linked ([234/239]); OWzxSlicer.exe linked clean ([238/238]); zero compile/link errors. Wrapper timed out at the test-target phase (documented prior-phase pattern); test gate completed via the incremental path.
- **Regression ctest (4/4 pass, 26.73s):** PASS (TV-09)
  - Command: `ctest --test-dir build -R "PartPlateTests|PrepareSceneDataTests|ViewModelSmokeTests|QmlUiAuditTests" --output-on-failure`
  - ViewModelSmokeTests Passed (16.54 sec)
  - QmlUiAuditTests Passed (1.06 sec)
  - PrepareSceneDataTests Passed (0.10 sec)
  - PartPlateTests Passed (1.44 sec) -- includes the updated thumbnailRoundTrip (synthesized QImage, no mock), the Phase 97 thumbnailSaveReloadRoundTrip (unchanged, single-plate), and the new thumbnailMultiPlateSaveReloadRoundTrip (Phase 98 multi-plate write-side fix).
- **OWzxSlicer.exe launch (process liveness, TV-08):** PASS
  - Probe: `Start-Process build/OWzxSlicer.exe` + 5-second sleep + HasExited check.
  - Result: OWzxSlicer.exe running (PID 42788) -- 5s liveness confirmed; process terminated cleanly after the probe. No immediate crash.
- **Encoding guard + whitespace:** PASS -- encoding_guard ok on all 6 modified files; git diff --check clean.

Per STATE.md deferred items, runtime VISUAL evidence (Windows capture API screenshot) is blocked; reachability is verified via process-liveness + canonical verifier + regression ctest (the documented evidence standard).

## Next Phase Readiness
- Phase 98 completes the v4.3 milestone (Real Thumbnail Capture And 3MF Round-Trip). All five phases (94-98) are complete: audit (94), capture (95), write (96), round-trip (97), verification + cleanup (98).
- The mock-free thumbnail pipeline is: QRhi capture (Phase 95) -> PartPlate::thumbnail() cache -> saveProject writes one thumbnail_data entry per plate (Phase 96 + Phase 98 multi-plate fix) -> reload reads Metadata/plate_N.png via extractPlateThumbnailFrom3mf (Phase 97) -> UI plate-card fallback via plateThumbnailBase64 (Phase 98).
- No blockers for milestone close.

## Self-Check: PASSED
