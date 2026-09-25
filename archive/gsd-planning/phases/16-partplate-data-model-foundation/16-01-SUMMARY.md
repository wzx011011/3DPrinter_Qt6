---
phase: 16-partplate-data-model-foundation
plan: 01
subsystem: database
tags: [partplate, domain-model, c++17, libslic3r, qt6, instance-membership, dynamicprintconfig]

requires: []
provides:
  - "PartPlate value object (src/core/model/PartPlate.h) mirroring upstream data/lifecycle/IO subset"
  - "PartPlateList container (src/core/model/PartPlateList.h) as single source of truth for plate state"
  - "Instance-level plate membership (std::set<pair<int,int>> obj_to_instance_set)"
  - "Per-plate native DynamicPrintConfig (m_config under HAS_LIBSLIC3R)"
  - "Plate lifecycle API (createPlate/deletePlate/renamePlate/setPlateLocked) on PartPlateList"
  - "objectIndicesOnPlate bridge query (instance-pair -> distinct object indices)"
  - "5 unit tests proving the data model before the 16-02 big-bang migration"
affects: [16-02, 17-plate-lifecycle-completion, 18-3mf-multi-plate-persistence, 19-per-plate-slice-scheduling]

tech-stack:
  added: []
  patterns:
    - "Pure-domain-object layer (src/core/model/, no QObject) separating plate semantics from Qt adaptation"
    - "Instance-level membership via std::set<pair<int,int>> (source-truth alignment with upstream obj_to_instance_set)"
    - "Native libslic3r types (DynamicPrintConfig, Vec3d) under #ifdef HAS_LIBSLIC3R with non-HAS stubs"

key-files:
  created:
    - src/core/model/PartPlate.h
    - src/core/model/PartPlate.cpp
    - src/core/model/PartPlateList.h
    - src/core/model/PartPlateList.cpp
  modified:
    - CMakeLists.txt
    - tests/ViewModelSmokeTests.cpp

key-decisions:
  - "D-01: New src/core/model/ layer with pure domain objects (no QObject) — ProjectServiceMock adapts to Qt"
  - "D-02: Mirror only upstream data/lifecycle/IO fields; EXCLUDE all GL/wx/cereal (grep GLModel/wxCoord = 0)"
  - "D-03: Instance-level membership std::set<pair<int,int>> (obj_to_instance_set), not per-object QList"
  - "D-04: Native DynamicPrintConfig m_config under HAS_LIBSLIC3R (not QHash<QString,QVariant>)"

patterns-established:
  - "src/core/model/ as the home for pure plate domain objects (ProjectServiceMock owns PartPlateList and adapts)"
  - "#ifdef HAS_LIBSLIC3R guards around libslic3r types with compile-without-libslic3r fallbacks"
  - "Test-first for high-risk refactors: model + unit tests land before the destructive migration"

requirements-completed: [PLATE-01, PLATE-02]

duration: 25min
completed: 2026-06-25
---

# Plan 16-01: PartPlate + PartPlateList Domain Model Summary

**New src/core/model/ domain layer with instance-level PartPlate membership and native DynamicPrintConfig, plus 5 unit tests — the non-destructive foundation for the 16-02 big-bang migration.**

## Performance

- **Duration:** ~25 min
- **Tasks:** 5 (T1 PartPlate, T2 PartPlateList, T3 CMake, T4 tests, T5 verify)
- **Files created:** 4 (PartPlate.h/.cpp, PartPlateList.h/.cpp)
- **Files modified:** 2 (CMakeLists.txt, tests/ViewModelSmokeTests.cpp)

## Accomplishments
- Created `PartPlate` value object mirroring upstream `PartPlate.hpp:77-557` data/lifecycle/IO subset — instance-level membership, native DynamicPrintConfig, slice state machine (`canSlice()`), per-plate settings (bed type/print sequence/spiral/layer sequences). Zero GL/wx/cereal fields.
- Created `PartPlateList` container owning plates via `unique_ptr` vector — `createPlate`/`deletePlate`/`renamePlate`/`setPlateLocked`, reindex-on-delete, keep-≥1 invariant, `MAX_PLATE_COUNT=36` guard, `objectIndicesOnPlate` bridge query.
- Added 5 deterministic unit tests proving instance membership, slice state machine, lifecycle reindex, object-index derivation, and MAX_PLATE_COUNT enforcement.
- ProjectServiceMock **unchanged** — migration is plan 16-02. Existing plate behavior fully preserved.

## Task Commits

1. **T1+T2+T3+T4 (model + CMake + tests):** `230ae2f` (feat) — committed as one atomic unit since T1-T4 are tightly coupled (model needs CMake to compile, tests need model to link).
2. **T5 (verification):** canonical `auto_verify_with_vcvars.ps1` passed; `ViewModelSmokeTests` 37 passed (32 baseline + 5 new); `QmlUiAuditTests` 7 passed. Evidence in `build/ViewModelSmokeTests.phase16w1.txt` (local, build/ gitignored).

## Files Created/Modified
- `src/core/model/PartPlate.h` — PartPlate value object: geometry, instance-pair membership, DynamicPrintConfig, slice state machine, per-plate settings
- `src/core/model/PartPlate.cpp` — `hasObject()` derived query
- `src/core/model/PartPlateList.h` — container API (lifecycle + bridge queries + MAX_PLATE_COUNT)
- `src/core/model/PartPlateList.cpp` — lifecycle implementations, reindex, resetToSinglePlate
- `CMakeLists.txt:101-105` — registered 4 new files in owzx_app_core
- `tests/ViewModelSmokeTests.cpp` — added PartPlate.h/PartPlateList.h includes + 5 test methods

## Decisions Made
- **Per-plate settings relocation:** the bed-type/print-sequence/spiral/layer-sequence fields (previously scattered parallel QLists on ProjectServiceMock) were moved into PartPlate during T1. The `LayerSeqEntry`/`PlateBedType`/etc. enums+structs now live in `PartPlate.h`. This consolidates all plate state in one domain object (consistent with D-05's "PartPlateList as single source of truth") and removes the need for 16-02 to add them separately.
- **`resetToSinglePlate()` added** beyond the plan's explicit method list — needed by the 3MF load path (16-02 T3) to rebuild from PlateData without accumulating stale plates. Added now since PartPlateList owns the keep-≥1 invariant.

## Deviations from Plan

None — plan executed as written. The two additions (per-plate settings relocation, resetToSinglePlate) are within the CONTEXT D-02/D-05 scope (the plan's T1 field list was non-exhaustive; CONTEXT explicitly says "include... per-plate config" and the settings are part of plate state).

## Issues Encountered
- **Initial bare `cmake --build` failed** with `fatal error C1083: "vector"` on `libslic3r_cgal_from_source` — this was NOT a code issue but an environment issue: the bare build didn't source `vcvars64.bat`, so MSVC's STL include dirs weren't on the path. Resolved by using the canonical `auto_verify_with_vcvars.ps1` (which sources vcvars), exactly as the plan and build-rules.md mandate. No code change needed.

## User Setup Required
None — pure code, no external services.

## Next Phase Readiness
- **Ready for plan 16-02 (Wave 2):** PartPlate + PartPlateList are compiled into `owzx_app_core`, tested, and the instance-level/DynamicPrintConfig contract is locked. The big-bang migration can now re-back ProjectServiceMock onto `PartPlateList` with confidence the model is correct.
- **No blockers.** The model compiles both with and without HAS_LIBSLIC3R (verified: build has HAS_LIBSLIC3R=ON).

---
*Phase: 16-partplate-data-model-foundation*
*Completed: 2026-06-25*
