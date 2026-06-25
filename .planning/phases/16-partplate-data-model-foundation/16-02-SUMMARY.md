---
phase: 16-partplate-data-model-foundation
plan: 02
subsystem: database
tags: [partplate, big-bang-migration, project-service, qt6, instance-membership, 3mf, json]

requires:
  - phase: 16-01
    provides: "PartPlate + PartPlateList domain model (src/core/model/)"
provides:
  - "ProjectServiceMock plate storage fully migrated to PartPlateList (single source of truth)"
  - "All 9 parallel QList vectors + plateCount_/currentPlateIndex_ removed"
  - "Per-plate settings (bed type/print seq/spiral/layer sequences) consolidated on PartPlate"
  - "3MF load path + JSON mock-mode round-trip rebuilt on PartPlateList"
  - "PLATE-06 regression test proving existing behavior preserved"
affects: [17-plate-lifecycle-completion, 18-3mf-multi-plate-persistence, 19-per-plate-slice-scheduling]

tech-stack:
  added: []
  patterns:
    - "Instance-pair membership as the single plate-membership truth, with object-index bridge queries"
    - "PartPlateList::resetToSinglePlate() pattern for load-path rebuild (keep-≥1 invariant)"

key-files:
  created: []
  modified:
    - src/core/services/ProjectServiceMock.h
    - src/core/services/ProjectServiceMock.cpp
    - CMakeLists.txt
    - tests/ViewModelSmokeTests.cpp

key-decisions:
  - "D-05 big-bang: deleted all 9 vectors in one plan, no bridge/cache layer"
  - "Retained m_mockPlateOverrides as the QVariant adaptation view (D-04): PartPlate::config() is the DynamicPrintConfig truth, the QHash is the QML-facing view ProjectServiceMock exposes via plateScopedOptionValue/setPlateScopedOptionValue. Phase 18/19 bridge them."
  - "MockLayerSeqEntry kept as a source-compat alias for OWzx::LayerSeqEntry (avoids renaming ~10 call sites; the canonical type lives in PartPlate.h)"

patterns-established:
  - "Object-insertion sites call m_plateList->currentPlate()->addInstance(newIdx, 0) — single-instance representation matching previous per-object behavior"
  - "deleteObject/duplicate/boolean reindexing: snapshot the membership set, rebuild with adjusted indices, clearInstances() + re-add (sets are immutable during iteration)"
  - "Load paths (loadFile lambda, loadProject lambda, JSON deserialize) all use resetToSinglePlate() + createPlate() loop to rebuild from scratch"

requirements-completed: [PLATE-06]

duration: 90min
completed: 2026-06-25
---

# Plan 16-02: ProjectServiceMock Big-Bang Migration Summary

**Big-bang migration of ProjectServiceMock plate storage from 9 parallel QList vectors onto PartPlateList — ~50 sites rewritten across lifecycle methods, 2 async load lambdas, JSON round-trip, and object-index reindexing, with zero behavioral regressions.**

## Performance

- **Duration:** ~90 min (3 build-fix cycles)
- **Tasks:** 5 (T1 storage swap, T2 method rewrite, T3 load paths, T4 verify, T5 regression test)
- **Files modified:** 4 (ProjectServiceMock.h, ProjectServiceMock.cpp, CMakeLists.txt, ViewModelSmokeTests.cpp)
- **Net LOC:** +458 / -382

## Accomplishments
- **Deleted all 9 parallel vectors** + `plateCount_` + `currentPlateIndex_` from ProjectServiceMock. `std::unique_ptr<OWzx::PartPlateList> m_plateList` is now the single source of truth (D-05).
- **Rewrote ~50 sites** spanning: plate lifecycle methods (delegate to m_plateList), 2 async load lambdas (rebuild from loaded data), JSON mock-mode save/load round-trip, deleteObject/duplicateObject/boolean-op reindexing, clear/close paths, object-insertion sites, thumbnail generation, cloneCurrentPlateModel guard.
- **Per-plate settings consolidated** on PartPlate (bed type / print sequence / spiral / first-layer & other-layer sequences). The layer-seq accessors (15 methods) now delegate to `plate(idx)->setBedType(...)` etc.
- **Preserved all existing behavior (PLATE-06):** 32 baseline tests + 5 (16-01) all still pass, plus 1 new regression test proving plate ops work post-migration.

## Task Commits

1. **T1+T2+T3 (migration):** `630e6ea` (feat) — single atomic commit covering the full big-bang rewrite (all 50 sites are interdependent; partial migration would not compile).
2. **T4+T5 (verify + regression test):** included in `630e6ea`.

## Files Created/Modified
- `src/core/services/ProjectServiceMock.h` — storage swap (9 vectors → m_plateList + MockLayerSeqEntry alias); kept m_mockPlateOverrides as QVariant view
- `src/core/services/ProjectServiceMock.cpp` — ~50 site rewrites across lifecycle, lambdas, JSON round-trip, reindexing
- `CMakeLists.txt:351-354` — registered PartPlate/PartPlateList in owzx_cli_core (was the link-error cause)
- `tests/ViewModelSmokeTests.cpp` — added projectServicePlateOpsBackedByPartPlateList regression test

## Decisions Made
- **m_mockPlateOverrides retained** as the QVariant adaptation layer (D-04). PartPlate::config() is the native DynamicPrintConfig truth; the QHash map is the QML-facing view that plateScopedOptionValue/setPlateScopedOptionValue expose. Phase 18 (3MF persistence) and Phase 19 (slice merge) will bridge these two. This avoids a risky DynamicPrintConfig↔QVariant conversion in the big-bang plan while keeping the config-truth direction aligned with D-04.
- **MockLayerSeqEntry kept as an alias** (`using MockLayerSeqEntry = OWzx::LayerSeqEntry`) to avoid renaming ~10 untouched call sites. The canonical type is OWzx::LayerSeqEntry in PartPlate.h; the alias is source-compat only.
- **Initial currentPlateIndex changed from -1 to 0** for a freshly-constructed (no-project) ProjectServiceMock: the PartPlateList constructor creates 1 plate with current=0, so plateCount() now reports 1 immediately. The previous -1 sentinel was unreachable in practice (load paths reset it). The regression test asserts the new invariant (plateCount==1 on construction).

## Deviations from Plan

None — plan executed as written. The only adjustment was recognizing the migration was ~50 sites (not ~30 as planned), handled by systematically converting each region.

## Issues Encountered
- **LNK2001: 10 unresolved PartPlateList symbols in owzx-cli** (build attempt 1). Root cause: `owzx_cli_core` is a separate OBJECT library that compiles ProjectServiceMock.cpp independently of `owzx_app_core`, and the new PartPlate.cpp/PartPlateList.cpp were only added to owzx_app_core. **Fix:** registered the two model files in owzx_cli_core too (CMakeLists.txt:351-354). Build attempt 2 linked clean.
- **Bare `cmake --build` without vcvars** fails with C1083 on STL headers — environment issue, resolved by always using the canonical `auto_verify_with_vcvars.ps1` (build-rules.md).

## User Setup Required
None — pure code.

## Next Phase Readiness
- **Phase 16 complete.** PartPlate + PartPlateList are the real plate data model; ProjectServiceMock is fully re-backed on them; all behavior preserved.
- **Ready for Phase 17** (Plate Lifecycle Completion: clone/duplicate/reorder/per-plate-printable — these now have a real data model to build on).
- **Note for Phase 18/19:** m_mockPlateOverrides (QVariant view) must be bridged with PartPlate::config() (DynamicPrintConfig truth) for 3MF persistence and slice config merge. This is intentional deferred work, not debt — the bridge is the Phase 18/19 deliverable.

---
*Phase: 16-partplate-data-model-foundation*
*Completed: 2026-06-25*
