---
phase: 112-per-volume-its-accessor-and-mesh-cache
plan: 01
subsystem: api
tags: [libslic3r, indexed-triangle-set, shared-ptr, ownership-contract, measure-engine, raycaster, mesh-cache]

# Dependency graph
requires: []
provides:
  - "ProjectServiceMock::volumeMeshIts(int objectIndex, int volumeIndex) const -> std::shared_ptr<const indexed_triangle_set> (the per-volume ITS PUBLIC API)"
  - "ITS ownership contract (shallow-share via shared_ptr aliasing constructor over ModelVolume::mesh_ptr()) documented at the declaration"
  - "Defensive nullptr return for all invalid index/mesh cases (MI-05)"
  - "SurfaceFeature boundary flag (raw void* volume / vector<int>* plane_indices scrubbing deferred to Phase 113/114)"
affects: [113-sceneraycaster-port, 114-measuring-instantiation, AssembleViewDataPool ModelObjectsClipper]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "shared_ptr aliasing constructor for zero-copy shallow-share of a libslic3r value member across the libslic3r->Qt boundary"
    - "Source-truth ownership contract documented as named sections at the declaration (OWNERSHIP CONTRACT / SHALLOW-SHARE DECISION / CACHE DECISION / DEFENSIVE NULL RETURN / SURFACE FEATURE BOUNDARY) so a source-audit slot can lock each one"

key-files:
  created: []
  modified:
    - "src/core/services/ProjectServiceMock.h (accessor declaration + full ownership contract)"
    - "src/core/services/ProjectServiceMock.cpp (accessor implementation)"
    - "tests/ViewModelSmokeTests.cpp (regression slot perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices)"
    - "tests/QmlUiAuditTests.cpp (source-audit slot perVolumeItsAccessorPresent)"

key-decisions:
  - "Shallow-share via the std::shared_ptr aliasing constructor -- NOT a copy. The ModelVolume already owns its TriangleMesh via std::shared_ptr<const TriangleMesh> m_mesh (Model.hpp:1040, accessor mesh_ptr() at Model.hpp:856), and `its` is a public value member of TriangleMesh (TriangleMesh.hpp:158). The returned shared_ptr aliases m_mesh's refcount onto &its, so the TriangleMesh (and its ITS) stays alive as long as ANY caller holds the result -- safe even if the ModelVolume is removed mid-use."
  - "No separate mesh cache on ProjectServiceMock (MI-04). The ModelVolume's m_mesh IS the cache: it is the in-memory source of truth, it is already reference-counted, and set_mesh/reset_mesh atomically swap it. A parallel (objectIndex, volumeIndex) cache would duplicate the libslic3r ownership model, require its own invalidation wiring on every model mutation, and risk diverging from the live mesh."
  - "indexed_triangle_set forward-declared at GLOBAL scope in ProjectServiceMock.h (not Slic3r::). Upstream forward-declares it before `namespace Slic3r` at Measure.hpp:9 and uses it as a public value member of TriangleMesh at TriangleMesh.hpp:158. The plan prose wrote Slic3r::indexed_triangle_set; the source-truth type is global, so the implementation and the audit lock the global-scope form."
  - "New comments use ASCII-only dividers (==/--) instead of the Unicode box-drawing (--) the rest of the file uses. Respects the build rule 'All new or modified source comments must be English and ASCII-only' without churning existing lines out of scope."

patterns-established:
  - "Per-volume ITS accessor pattern: volumeMeshIts(objIdx, volIdx) returns shared_ptr<const indexed_triangle_set> via the aliasing constructor. Phase 113 (SceneRaycaster) and Phase 114 (Measure::Measuring) consume this directly; both construct their per-volume acceleration structures from the returned ITS, and the shared_ptr lifetime keeps the ITS valid for the structure's lifetime."
  - "Named-section ownership contract at the declaration, each section locked by a QVERIFY2 in the source-audit slot. Future phases that add similar cross-boundary accessors (e.g. a per-volume config accessor) should mirror this so the lifetime documentation is regression-proof."

requirements-completed: [MEASURE-01]

# Metrics
duration: ~35min (build dominated; code change was ~125 lines accessor + ~160 lines tests)
completed: 2026-07-12
---

# Phase 112: Per-Volume ITS Accessor And Mesh Cache Summary

**Per-volume indexed_triangle_set accessor on ProjectServiceMock with a documented shallow-share ownership contract, unblocking Phase 113 (SceneRaycaster), Phase 114 (Measure::Measuring), and the AssembleViewDataPool ModelObjectsClipper.**

## Performance

- **Duration:** ~35 min (canonical build + full ctest; the code change itself was small)
- **Started:** 2026-07-12
- **Completed:** 2026-07-12
- **Tasks:** 3 (2 atomic commits + verification)
- **Files modified:** 4

## Accomplishments
- Closed MEASURE-01: `ProjectServiceMock::volumeMeshIts(int objectIndex, int volumeIndex) const` returns `std::shared_ptr<const indexed_triangle_set>` -- the PUBLIC API the downstream phases consume.
- Established the ITS ownership contract (pitfall 6 / MI-02..MI-06) at the declaration: shallow-share via the shared_ptr aliasing constructor, no separate cache, defensive nullptr return, SurfaceFeature raw-pointer boundary flagged for Phase 113/114.
- Regression + source-audit coverage: ViewModelSmokeTests proves the valid-path (non-null ITS with vertex/triangle counts) and the null-path (invalid indices); QmlUiAuditTests locks the accessor signature + every ownership-contract section header.

## Task Commits

Each task was committed atomically:

1. **Task 112-01-01: per-volume ITS accessor + ownership contract** - `dac49bc` (feat)
2. **Task 112-01-02: regression test + source-audit slot** - `c1d185a` (test)
3. **Task 112-01-03: build + ctest verification** - documented in this SUMMARY (no code commit; verification-only task per plan)

## Files Created/Modified
- `src/core/services/ProjectServiceMock.h` - forward-declares global-scope `indexed_triangle_set`; declares `volumeMeshIts` with the full ownership contract (OWNERSHIP CONTRACT / SHALLOW-SHARE DECISION / CACHE DECISION / DEFENSIVE NULL RETURN / SURFACE FEATURE BOUNDARY sections).
- `src/core/services/ProjectServiceMock.cpp` - implements `volumeMeshIts`: index validation, grabs `ModelVolume::mesh_ptr()`, empty-mesh guard, returns via the aliasing constructor `std::shared_ptr<const indexed_triangle_set>(meshPtr, &its)`.
- `tests/ViewModelSmokeTests.cpp` - `perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices`: loads hotend.stl, asserts non-null ITS for (0,0) with non-empty vertices/indices and triangle-count consistency, asserts nullptr for negative + out-of-range object/volume indices.
- `tests/QmlUiAuditTests.cpp` - `perVolumeItsAccessorPresent`: source-audit lock for the accessor signature + every ownership-contract section header + the aliasing-constructor implementation + the regression slot existence.

## Decisions Made

### MI-03 shallow-share vs copy
Investigated whether `vol->mesh().its` is safe to share by reference for the ModelVolume's lifetime. Findings:
- `ModelVolume` owns the TriangleMesh via `std::shared_ptr<const TriangleMesh> m_mesh` (Model.hpp:1040). The accessor `mesh_ptr()` (Model.hpp:856) returns this shared_ptr directly.
- `set_mesh` / `reset_mesh` (Model.hpp:857-863) always construct a NEW shared_ptr (`make_shared`); they never mutate the existing TriangleMesh in place. A caller holding a prior ITS therefore never observes a detached or torn-down ITS -- worst case is a STALE-but-valid ITS, which is exactly what `Measure::Measuring` needs (it indexes the mesh in its ctor and treats it as immutable).
- Qt implicit-share detach does NOT apply: TriangleMesh / indexed_triangle_set are plain libslic3r types (no QSharedData), and the `shared_ptr<const ...>` constness prevents writes from the Qt side.

Decision: shallow-share via the aliasing constructor. Zero copy, the returned shared_ptr shares m_mesh's refcount, the TriangleMesh (and its `its` member) stays alive as long as ANY caller holds the result. This closes the use-after-free hazard from pitfall 6 (detach-on-copy + rebuild-timing lifetime bugs).

### MI-04 mesh cache
No separate mesh cache added. The ModelVolume's m_mesh is the cache. A parallel (objectIndex, volumeIndex) cache would (a) duplicate the libslic3r ownership model, (b) require its own invalidation wiring on every model mutation (load/cut/boolean/simplify/drill/orient/arrange), and (c) risk diverging from the live mesh. The accessor reads live m_mesh every call (O(1) pointer dereference). If a future phase proves hot-path callers need a stable snapshot, they hold the returned shared_ptr for the duration of their use -- that is the cache, and it is caller-owned.

### MI-06 SurfaceFeature boundary (flagged, not implemented)
The accessor returns the ITS ONLY. `SurfaceFeature` (Measure.hpp:27-99) carries raw `void* volume` (Measure.hpp:95) and `std::vector<int>* plane_indices` (Measure.hpp:96) that point at libslic3r-owned memory and MUST NOT escape into Qt. Phase 113 (raycaster) and Phase 114 (Measuring) enforce the scrubbing/repointing at the boundary; this phase flags it in the contract so the downstream phases cannot miss it.

### Global-scope indexed_triangle_set
The plan prose (MI-01) wrote `std::shared_ptr<const Slic3r::indexed_triangle_set>`. Source truth (Measure.hpp:9 forward-declares `struct indexed_triangle_set;` BEFORE `namespace Slic3r` at line 11) is that the type is at GLOBAL scope. The implementation and the source-audit slot lock the global-scope form to match upstream truth.

## Deviations from Plan

None that change scope. Two minor clarifications:
- The plan wrote `Slic3r::indexed_triangle_set`; the source-truth type is global-scope `::indexed_triangle_set`. Implemented the source-truth form (documented above).
- The plan's MI-04 allowed "cache optional but recommended if construction is non-trivial." Construction is trivial (the ITS is already in memory via libslic3r), so no cache was added. Decision documented in the header contract.

No auto-fixes; no scope creep.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required. This is a pure C++ accessor + tests; the libslic3r integration is unchanged (no new dependencies, no config changes).

## Next Phase Readiness
- Phase 113 (SceneRaycaster port): can construct one `SceneRaycasterItem` per volume from `volumeMeshIts(objIdx, volIdx)` (SceneRaycaster.hpp:71-72 `m_volumes` is the consumer). The shared_ptr lifetime keeps the raycaster's ITS valid.
- Phase 114 (Measure::Measuring instantiation): can construct `Measure::Measuring(const indexed_triangle_set&)` (Measure.hpp:122) from `*volumeMeshIts(...)`, holding the shared_ptr for the Measuring object's lifetime.
- AssembleViewDataPool ModelObjectsClipper: can register the `AssembleViewDataID::ModelObjectsClipper = 1 << 4` slot (AssembleViewDataPool.h:44) consuming the same accessor.
- Pitfall 6 ITS lifetime hazard: CLOSED. The shared_ptr aliasing constructor is the documented contract; the regression + source-audit slots lock it.

Blockers: none. The SurfaceFeature raw-pointer scrubbing (MI-06) is a Phase 113/114 responsibility, flagged but not blocking this phase.

## Verification

Canonical build + regression ctest run:
- **Command:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- **Result:** exit code 0.
- **OWzxSlicer.exe:** linked clean (`[237/237] Linking CXX executable OWzxSlicer.exe`).
- **Test suites (all passed):**
  - PrepareSceneDataTests: passed
  - PartPlateTests: passed
  - ViewModelSmokeTests: passed (includes the new `perVolumeItsAccessorReturnsValidMeshAndNullForInvalidIndices` slot)
  - QmlUiAuditTests: passed (includes the new `perVolumeItsAccessorPresent` source-audit slot)
  - PreviewParserTests: passed
  - E2EWorkflowTests: passed
- **Source audit:** `rg -n "volumeMeshIts|indexed_triangle_set" src/core/services/` confirms the accessor in both header and cpp.
- **Encoding guard:** `git diff` new lines are ASCII-only (LC_ALL=C grep for non-printables returns no matches in the changed regions). `git diff --check` clean.

ViewModelSmokeTests.exe and QmlUiAuditTests.exe were both BUILT and EXECUTED (not just built) -- the verify script runs each exe directly and the per-suite "passed" markers are in the log.

---
*Phase: 112-per-volume-its-accessor-and-mesh-cache*
*Completed: 2026-07-12*
