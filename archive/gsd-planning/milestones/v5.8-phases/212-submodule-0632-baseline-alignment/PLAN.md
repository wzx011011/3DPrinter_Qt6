# Phase 212 Plan: Submodule 0632bae8 Baseline Alignment

**Requirement:** BASE-01..04
**Goal:** Repair the build break from the v5.7→main merge (submodule pinned
to 4cb3b9ce while origin/main code targets 0632bae8) by creating a
0632bae8-based submodule branch carrying the OWzx-local compat deltas, and
adapting ProjectServiceMock.cpp to 0632's API surface.
**Status:** Complete · **Commit:** `b19087b`

## Root Cause

The merge commit `36f951d` pinned `third_party/OrcaSlicer` back to `4cb3b9ce`
(AGENTS.md-locked OWzx compat) while origin/main's ProjectServiceMock.cpp had
already migrated to `0632bae8`'s newer APIs (`ModelVolume::split` 1-arg,
`ModelObject::split` 1-arg, no `auto_drop`). Result: 2 compile errors
(C2660), build broken.

## Submodule Branch `owzx-cgal54-on-0632` (pushed to github-fork)

Base `0632bae8` + 5 OWzx-compat commits:

1. `fdcb3178` CGAL 5.4 hole refinement — port segment() `#if 0`→`#if 1` flip
   from 4cb3b9ce; drop the standalone CGAL `repair()` (0632 uses admesh).
2. `0f09d671` mcut `frontend.h` missing `<chrono>` — MSVC 14.50.35717 dropped
   the transitive include via `<future>`.
3. `2946a626` TriangleMeshDeal (loop subdivision) ported from 4cb3b9ce.
4. `a43a41b5` DRC.hpp + stub DRC.cpp (draco unavailable; store/load → false).
5. `bdbc7ecb` TriangleMeshDeal.cpp igl::loop overload adaptation
   (Map<const RowMatrixX3f> → copy into RowMatrixX3f; 0632's loop requires
   PlainObjectBase inputs).

## Main-Repo Adaptation (ProjectServiceMock.cpp)

- `object->volumes[i]->split(1, false)` → `split(1)` (0632 is 1-arg).
- `ModelInstance::auto_drop` removed: dropObjectToBed drops unconditionally
  (0632 ensure_on_bed has no per-instance toggle); objectAutoDrop returns
  true (effective behaviour); setObjectAutoDrop is a no-op accept.
- `Utils.hpp` include retained; `TriangleMeshDeal.hpp`/`Format/DRC.hpp`
  includes resolve via the ported submodule files.

## Verification

- `cmake --build build --target owzx_app_core` clean (no error C).
- Full canonical build green: OWzxSlicer.exe + all test exes link.
- `multiPlateFullStateRoundTrip` PASS (the prior "failure" was a stale 07-23
  binary; source already contained syncPlatePrintSequenceConfig).
- `E2EWorkflowTests::test_slice_produces_gcode_file` PASS (cereal StaticObject
  no longer blocks).
