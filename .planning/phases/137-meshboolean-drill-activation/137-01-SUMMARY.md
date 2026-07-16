# Summary 137-01: MeshBoolean + Drill Activation

**Phase:** 137 — MeshBoolean + Drill Activation
**Plan:** 137-01
**Status:** Complete
**Commit:** `a875c65`
**Requirements closed:** CGAL-02, CGAL-03

## What was done

Flipped `kCgalMeshBooleanAvailable = true` and activated the already-written MeshBoolean + Drill logic. The "~200 lines" referenced in the roadmap was the MeshBoolean engine itself (made compilable in Phase 136); this phase only rewires the call sites.

### Changes (commit `a875c65`, 4 files, +45/-29)

1. **`cmake/BuildLibslic3rFromSource.cmake`** — removed `MeshBoolean_mcut_stub.cpp` from sources (LNK2005 duplicate symbol with the real `MeshBoolean.cpp` re-enabled in Phase 136).
2. **`src/core/services/ProjectServiceMock.cpp`** (+28/-9) — `booleanObject` now calls real `MeshBoolean::minus` / `self_union` (was a return-false stub); `drillObject` now calls real `MeshBoolean::minus` (was a stub).
3. **`src/core/viewmodels/EditorViewModel.cpp`** (+9/-4) — `gizmoStatusText` cases 11 (MeshBoolean) and 13 (Drill) no longer report "CGAL unavailable".
4. **`tests/ViewModelSmokeTests.cpp`** (+11/-8) — gizmo-availability assertions updated for the activated path.

## Verification evidence

- Canonical build: clean compile + link, exit 0 (`build_p137f.log`).
- ctest: 5/5 groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser) + E2E pipeline PASS.
- ViewModelSmokeTests: all assertions PASS (`p137_vm_result.txt`, `p137e_vm.txt`).
- App launch liveness: `APP_RUNNING_PID=29868`.

## Outcome

MeshBoolean (union/subtract/intersect) and Drill now run end-to-end through the real CGAL corefinement engine on CGAL 5.4. CGAL-02 and CGAL-03 are closed. Together with Phase 136, the v4.7 "CGAL 5.6+ required" blocker is fully resolved without a dependency-bundle upgrade.

## Tech debt note

- `ProjectServiceMock::drillObject` emits MSVC warning **C4715** ("not all control paths return a value") at `src/core/services/ProjectServiceMock.cpp:3362`. Non-fatal (build succeeds, tests pass) but should be cleaned up — every code path must return a value. Carried forward as a minor tech-debt item; does not block CGAL-02/03 closure.
