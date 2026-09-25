---
phase: 112-per-volume-its-accessor-and-mesh-cache
status: APPROVED
verdict: ship
counts: {blockers: 0, highs: 0, mediums: 0, lows: 1, infos: 2}
---

# Phase 112 Code Review — Per-Volume ITS Accessor

## Verdict: APPROVED (ship)

The aliasing-contract lifetime argument is sound and verified against libslic3r source (Model.hpp:856 mesh_ptr() returns shared_ptr by value; TriangleMesh::its is public value member; set_mesh/reset_mesh always make_shared fresh — never mutate in place; so a caller holding a prior ITS aliasing pointer never observes a torn-down ITS).

## Findings

| # | Severity | Finding |
|---|----------|---------|
| L-1 | low | Header omits `Slic3r::` qualifier that MI-01 names — actually CORRECT (indexed_triangle_set is at global scope). The plan's MI-01 wording is what's wrong, not the code. |
| I-1 | info | MI-07/MI-08 (runtime test + source-audit) verified via ctest (QmlUiAuditTests 83/83 + ViewModelSmokeTests). The source-audit slot is high quality (15 QVERIFY2 assertions). |
| I-2 | info | No separate mesh cache (MI-04) is the right call — ModelVolume::m_mesh IS the cache (shared_ptr, atomically swapped on set_mesh/reset_mesh). |

## Verified

- **Aliasing shared_ptr lifetime:** `shared_ptr<const indexed_triangle_set>(meshPtr, &its)` shares meshPtr's refcount. TriangleMesh stays alive until last holder releases. set_mesh/reset_mesh always make_shared fresh (never mutate in place) — "stale-but-valid" is the worst case, exactly what Measure::Measuring needs.
- **Defensive null return:** 6 failure cases covered (no model, bad indices, null object/volume, null mesh_ptr, empty mesh). No crash.
- **Pitfall 6:** ITS lifetime closed by aliasing shared_ptr. SurfaceFeature raw-pointer boundary documented (deferred to Phase 113/114).

Regression: PartPlateTests 53/53, QmlUiAuditTests 83/83 pass.
