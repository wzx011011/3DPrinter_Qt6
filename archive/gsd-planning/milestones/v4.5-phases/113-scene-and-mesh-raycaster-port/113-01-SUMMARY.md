---
phase: 113-scene-and-mesh-raycaster-port
plan: 01
subsystem: rendering
tags: [raycaster, mesh, picking, moller-trumbore, aabbmesh, libslic3r, measure]

# Dependency graph
requires:
  - phase: 112-per-volume-its-accessor-and-mesh-cache
    provides: volumeMeshIts(objectIndex, volumeIndex) — the shared_ptr<const indexed_triangle_set> input the raycaster consumes.
provides:
  - Pure-CPU MeshRaycaster (src/core/rendering/MeshRaycaster.h/.cpp) — ray-triangle intersection over a shared_ptr<const indexed_triangle_set>, reusing libslic3r AABBMesh BVH. Cites upstream MeshUtils.hpp:159+ / MeshUtils.cpp:425-466.
  - Thin Qt6 SceneRaycaster wrapper (src/core/rendering/SceneRaycaster.h/.cpp) — per-volume MeshRaycaster cache + stage-2 hitTest with world-space hit result.
  - SceneRaycasterHit struct carrying object/volume/facet/world-position/world-normal (the Phase 114 Measuring + Phase 115 snap UX input).
  - SceneRaycasterCandidate stage-1 seam type — the ObjectPicking -> SceneRaycaster handoff contract.
affects: [114-measuring-engine, 115-snap-ux, rhi-viewport-picking-bridge, editor-viewmodel-measure-state]

# Tech tracking
tech-stack:
  added: []  # reuses libslic3r AABBMesh (no new third-party dep)
  patterns:
    - "Pure-CPU raycaster over libslic3r AABBMesh BVH — no GL/wxWidgets/Qt RHI deps in the header (mirrors the GizmoGeometry/ObjectPicking pure-CPU rendering-helper pattern)"
    - "Two-stage pick (pitfall 7 mitigation): stage 1 ObjectPicking ray-AABB coarse prefilter -> stage 2 SceneRaycaster per-triangle ITS raycast on candidate volumes only"
    - "Per-volume raycaster cache (MR-02): BVH built once in MeshRaycaster ctor, cached by SceneRaycaster keyed on (object,volume), invalidated on model change — never rebuilt per mouse-move"
    - "Injected ITS source (VolumeMeshItsFn callable) so SceneRaycaster has no hard ProjectServiceMock dependency — testable with a synthetic ITS lambda"

key-files:
  created:
    - src/core/rendering/MeshRaycaster.h
    - src/core/rendering/MeshRaycaster.cpp
    - src/core/rendering/SceneRaycaster.h
    - src/core/rendering/SceneRaycaster.cpp
  modified:
    - src/core/rendering/ObjectPicking.h  # documents the stage-1 seam of the two-stage pick
    - CMakeLists.txt                      # registers the 4 new files in owzx_app_core (MR-05)
    - tests/PartPlateTests.cpp            # MR-06 deterministic regression slot
    - tests/QmlUiAuditTests.cpp           # MR-07 source-audit slot

key-decisions:
  - "Reuse libslic3r AABBMesh for the intersection math instead of re-implementing Moller-Trumbore. AABBMesh is pure libslic3r (no GL/wx), builds the BVH once, and answers query_ray_hits() in O(log N). This keeps the Qt6 math byte-for-byte aligned with upstream MeshUtils.hpp:159+ (same epsilon policy, same BVH traversal, same hit_result semantics) and satisfies MR-01 (pure-CPU) without re-deriving the algorithm."
  - "SceneRaycaster is a THIN Qt6 wrapper, not a verbatim port of upstream SceneRaycaster.hpp. The upstream EType enum (Bed/Volume/Gizmo/FallbackGizmo), encode_id/decode_id, and GL debug viz are too coupled to port; per the Phase 113 CONTEXT.md deferred section, this plan ports ONLY the Volume path the picking + measure workstream needs. Bed and Gizmo grabber raycasting remain deferred (Phase 115 snap UX may revisit)."
  - "ITS source injected as a VolumeMeshItsFn std::function (the ProjectServiceMock::volumeMeshIts signature). This decouples SceneRaycaster from ProjectServiceMock so the deterministic unit test can drive it with a synthetic cube ITS lambda — no model load required."
  - "std::map (not unordered_map) for the per-volume cache to avoid needing a std::hash<std::pair<int,int>> specialization. Cache size is bounded by candidate-volume count, so the O(log N) lookup is irrelevant."

patterns-established:
  - "Pure-CPU raycaster helper: header includes ONLY libslic3r math headers (AABBMesh + TriangleMesh) + <memory>/<optional>; NEVER GL/wx/qrhi. Built under #ifdef HAS_LIBSLIC3R (the ITS / AABBMesh types only exist there)."
  - "Stage-1 -> stage-2 candidate-seam pattern: ObjectPicking produces SceneRaycasterCandidate entries; SceneRaycaster consumes them. Stage-2 tolerates candidates with no mesh (nullptr ITS) without crashing — mirrors the MI-05 null-return contract on volumeMeshIts."
  - "Source-audit QVERIFY2 messages carry the requirement + truth ID prefix (e.g. 'MEASURE-02/MR-04: ...') so a future regression names the exact contract it violates."

requirements-completed: [MEASURE-02]

# Metrics
duration: 52min
completed: 2026-07-12
---

# Phase 113 Plan 01: Scene And Mesh Raycaster Port Summary

**Pure-CPU MeshRaycaster + thin Qt6 SceneRaycaster port (reusing libslic3r AABBMesh BVH) with a two-stage pick and per-volume cache, closing MEASURE-02 for Phase 114/115.**

## Performance

- **Duration:** ~52 min
- **Tasks:** 3 (113-01-01 implementation, 113-01-02 test+audit, 113-01-03 verification)
- **Files modified:** 8 (4 created, 4 modified)

## Accomplishments
- Ported upstream `MeshRaycaster` (MeshUtils.hpp:159+ / MeshUtils.cpp:425-466) as a pure-CPU Qt6 helper that wraps a `shared_ptr<const indexed_triangle_set>` (the Phase 112 `volumeMeshIts` output) over a libslic3r `AABBMesh` BVH and exposes `rayCast()` returning `{hit, position, normal, facetIdx}` in mesh-local coords (MR-01).
- Built a thin Qt6 `SceneRaycaster` wrapper that caches per-volume `MeshRaycaster` instances (MR-02), runs the stage-2 per-triangle raycast on stage-1 (ObjectPicking) candidate volumes only (MR-03 / pitfall 7 fix), and returns the closest world-space hit `{objectIndex, volumeIndex, facetIdx, worldPosition, worldNormal}` (MR-04).
- Documented the two-stage pick at both ends: `ObjectPicking.h` names itself STAGE 1; `SceneRaycaster.h` names itself STAGE 2 and cites pitfall 7 (the upstream GLGizmoMeasure.cpp:600-615 per-mouse-move volume loop that Qt6 must avoid).
- Registered all four new files in `owzx_app_core` in the root CMakeLists.txt (MR-05).
- Added a deterministic regression test (MR-06) and a source-audit slot (MR-07) that lock the port and every MR truth at the source level.

## Task Commits

Each task was committed atomically:

1. **Task 113-01-01: Port MeshRaycaster + SceneRaycaster Qt6 helpers** — `c271314` (feat)
2. **Task 113-01-02: Add raycaster regression + source-audit slot** — `e73632c` (test)
3. **Task 113-01-03: Build + ctest + document in SUMMARY** — this `docs` commit (verification-only task; no production code, recorded in this SUMMARY)

**Plan metadata:** this `docs(113-01)` commit.

## Files Created/Modified
- `src/core/rendering/MeshRaycaster.h` — pure-CPU raycaster class; wraps shared_ptr<const indexed_triangle_set> over libslic3r AABBMesh BVH; cites upstream MeshUtils.hpp:159+.
- `src/core/rendering/MeshRaycaster.cpp` — ctor builds BVH + face normals once (its_face_normals); rayCast delegates to AABBMesh::query_ray_hit (closest hit).
- `src/core/rendering/SceneRaycaster.h` — thin Qt6 wrapper; SceneRaycasterHit/SceneRaycasterCandidate structs; VolumeMeshItsFn callable; cache + invalidate API.
- `src/core/rendering/SceneRaycaster.cpp` — lazy per-volume raycaster fetch (cache hit reuses BVH); hitTest transforms world->mesh, raycasts each candidate, returns closest world hit.
- `src/core/rendering/ObjectPicking.h` — documents the two-stage pick (MR-03) at the stage-1 seam.
- `CMakeLists.txt` — registers the 4 new files in owzx_app_core (MR-05).
- `tests/PartPlateTests.cpp` — MR-06 deterministic regression slot meshAndSceneRaycasterHitMissAndClosestPick.
- `tests/QmlUiAuditTests.cpp` — MR-07 source-audit slot meshAndSceneRaycasterPorted.

## Decisions Made
- Reuse libslic3r AABBMesh (not re-implement Moller-Trumbore) — see key-decisions frontmatter.
- Thin wrapper, not a verbatim upstream port — only the Volume path is ported (Bed/Gizmo deferred).
- ITS source injected as a callable — testable with a synthetic cube ITS, no hard ProjectServiceMock dependency.
- std::map cache (not unordered_map) — avoids needing std::hash<std::pair> specialization.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] indexed_triangle_set is at GLOBAL scope, not Slic3r::**
- **Found during:** Task 113-01-01 (initial compile of owzx_app_core)
- **Issue:** The first draft of MeshRaycaster.h/SceneRaycaster.h used `Slic3r::indexed_triangle_set`. MSVC rejected with C2039 ("indexed_triangle_set": not a member of "Slic3r"). ProjectServiceMock.h:16-21 already documents that indexed_triangle_set lives at GLOBAL scope in libslic3r (forward-declared before `namespace Slic3r` at Measure.hpp:9).
- **Fix:** Replaced all `Slic3r::indexed_triangle_set` occurrences with global-scope `indexed_triangle_set` across all four new files (5 occurrences).
- **Files modified:** src/core/rendering/MeshRaycaster.h, .cpp, SceneRaycaster.h, .cpp
- **Verification:** ninja owzx_app_core clean (exit 0).
- **Committed in:** c271314 (part of Task 1 commit).

**2. [Rule 1 - Bug] std::unordered_map<std::pair<int,int>, ...> has no default hash**
- **Found during:** Task 113-01-01 (initial compile of owzx_app_core)
- **Issue:** The SceneRaycaster cache was declared `std::unordered_map<std::pair<int,int>, std::shared_ptr<MeshRaycaster>>`. std::hash has no specialization for std::pair, so MSVC errored in <xhash> (C2064/C2056).
- **Fix:** Switched the cache to `std::map<Key, ...>` (Key = std::pair<int,int>), which only needs operator< (available for pair). Cache size is bounded by candidate-volume count so the O(log N) lookup is irrelevant. Documented the choice inline.
- **Files modified:** src/core/rendering/SceneRaycaster.h
- **Verification:** ninja owzx_app_core clean (exit 0).
- **Committed in:** c271314 (part of Task 1 commit).

**3. [Rule 1 - Bug] Stray #ifdef/#endif imbalance after inserting the PartPlateTests slot**
- **Found during:** Task 113-01-02 ( inserting the regression slot body)
- **Issue:** The Edit that inserted the new method consumed the closing `#endif  // HAS_LIBSLIC3R` of the preceding filamentMapSaveReloadRoundTrip block, leaving the 852-block unclosed.
- **Fix:** Restored the closing `#endif` for the 852 block before opening the new 961-block. Re-verified ifdef/endif balance (6 opens, 6 closes).
- **Files modified:** tests/PartPlateTests.cpp
- **Verification:** ninja PartPlateTests clean; ctest PartPlateTests Passed.
- **Committed in:** e73632c (part of Task 2 commit).

---

**Total deviations:** 3 auto-fixed (3 Rule-1 bugs — all compile/blocker issues caught by the targeted build before commit).
**Impact on plan:** All three were mechanical compile fixes necessary for the code to build. No scope creep; the design (which math to port, the cache, the two-stage pick) is exactly as planned.

## Issues Encountered
- **Targeted-build env setup:** the canonical `scripts/auto_verify_with_vcvars.ps1` is a ~30-min full rebuild. The objective permits a targeted-ninja fallback (vcvars + Windows-Kits env preamble + ninja target). The first fallback attempt failed because my hand-rolled PowerShell preamble dropped the canonical script's two critical pieces: (1) PATH sanitization (drops entries with spaces+parens that break vcvars64.bat batch parsing — the source of the `此时不应有 \VMware\VMware` glitch) and (2) the Windows-Kits INCLUDE/LIB fallback (vcvars's findstr-based SDK detection fails silently without sanitization, leaving <stdio.h>/<cmath> unfindable). Reusing the canonical env preamble verbatim fixed both. No production code affected — this was a build-harness issue only.

## User Setup Required
None — no external service configuration required. The raycaster reuses the existing libslic3r dependency (already built into owzx_app_core under HAS_LIBSLIC3R).

## Verification (MR-08)

- **Configure:** `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..` (reconfigure to pick up the new CMakeLists sources) — exit 0.
- **Build (targeted fallback):** `ninja -j16 owzx_app_core` — exit 0 (compiles MeshRaycaster + SceneRaycaster + the ObjectPicking.h doc edit into the production library). This is the targeted-ninja fallback the objective permits (the canonical `scripts/auto_verify_with_vcvars.ps1` ~30-min full rebuild was not run; the production link of OWzxSlicer.exe was therefore not exercised by this plan — only the owzx_app_core library target that owns the new sources. The full app link will be exercised by the next canonical verify).
- **Test build:** `ninja -j16 PartPlateTests QmlUiAuditTests` — exit 0 (216/216, PartPlateTests.exe linked).
- **ctest:** `ctest -C Release -R "PartPlateTests|QmlUiAuditTests" --output-on-failure` — **100% tests passed, 0 tests failed out of 2** (QmlUiAuditTests 0.63s, PartPlateTests 4.57s). Both new slots (meshAndSceneRaycasterPorted, meshAndSceneRaycasterHitMissAndClosestPick) execute and pass.
- **MR-09:** `git diff --check` exit 0 (clean); encoding guard clean (all new lines ASCII, no BOMs).

Note on build command: per `.codex/rules/build-rules.md`, the canonical full verification command is `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`. This plan used the targeted-ninja fallback (owzx_app_core + the two test targets) rather than the full rebuild; the full app smoke + E2E + the broader ctest suite will be re-run by the next canonical verify. The new sources compile and link into owzx_app_core and both affected test exes build and pass.

## Next Phase Readiness
- **Phase 114 (Measuring engine):** unblocked. `SceneRaycasterHit` carries exactly what `Measure::Measuring` consumes — object index, volume index, triangle index, world-space hit position, world-space normal. The ITS source injection point (`VolumeMeshItsFn`) is the same accessor Phase 112 exposed.
- **Phase 115 (snap UX):** unblocked. The world-space hit + normal drive feature snapping; the stage-1/stage-2 split bounds the per-frame cost (one BVH traversal per candidate volume, not per-volume-per-frame across the scene).
- **RHI picking bridge:** the `SceneRaycasterCandidate` seam type is the handoff contract — `RhiViewportRenderer` / `EditorViewModel` runs `ObjectPicking::pickSourceObject` (stage 1) and wraps survivors as candidates for `SceneRaycaster::hitTest` (stage 2).
- **Deferred (intentional):** Bed raycasting, Gizmo grabber raycasting, and the upstream `EType` enum + `encode_id`/`decode_id` ID-packing scheme. These are not needed for the picking + measure workstream and would pull in coupling that the CONTEXT.md deferred section explicitly excluded.

---
*Phase: 113-scene-and-mesh-raycaster-port*
*Completed: 2026-07-12*
