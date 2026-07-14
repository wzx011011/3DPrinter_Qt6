---
phase: 113-scene-and-mesh-raycaster-port
status: APPROVED-WITH-FOLLOWUP
verdict: ship (fix M-01/M-02 before scaled instances enter picking path)
counts: {blockers: 0, highs: 0, mediums: 2, lows: 1, infos: 2}
---

# Phase 113 Code Review — Scene + Mesh Raycaster Port

## Verdict: APPROVED for ship — M-01/M-02 tracked as required follow-up

The port is faithful to upstream, two-stage pick correctly wired, cache works, test deterministic. Two findings relate to direction normalization under non-uniform scaling.

## Findings

| # | Severity | Finding |
|---|----------|---------|
| M-01 | medium | `MeshRaycaster.h:98-100` documents rayDir "does NOT need to be normalized" — INCORRECT. `AABBMesh::query_ray_hit` opens with `assert(is_approx(dir.norm(), 1.))`. Debug build trips; Release produces non-unit t. No runtime bug today (all callers pass unit rays) but the comment misleads future callers. Fix: normalize inside rayCast OR correct the comment. |
| M-02 | medium | `SceneRaycaster.cpp:59-61` — `inv.linear() * rayDirWorld` is NOT unit under non-uniform scale. Handed to AABBMesh which asserts unit. Latent Debug assert under scaled instances. Fix: add `.normalized()` after the inverse-transform. |
| L-1 | low | Normal transform uses `linear()` not inverse-transpose — only exact under uniform scale + rotation. Acceptable for Phase 113 scope (picking hit point unaffected; only normal degrades). Track for Phase 114/115 if measure relies on world normals from scaled instances. |
| I-1 | info | Pure-CPU guarantee verified transitively (AABBMesh + TriangleMesh include closure has zero GL/wxWidgets). |
| I-2 | info | AABBMesh reuse confirmed (not reimplementation) — MeshRaycaster.cpp delegates to query_ray_hit which uses igl AABB tree traversal. No Möller-Trumbore written. |

## Verified

- Two-stage pick (pitfall 7): stage 1 = ObjectPicking AABB; stage 2 = per-triangle ITS on candidates only. Cache + invalidation correct.
- Closest-hit semantics: squared world-space distance reduction (metric-independent).
- Hit result complete: objectIndex + volumeIndex + facetIdx + worldPosition + worldNormal.
- Cross-phase: SceneRaycaster decoupled from ProjectServiceMock via VolumeMeshItsFn DI. ITS shared_ptr flows safely from Phase 112 aliasing contract.
- Test quality: deterministic (its_make_cube), covers hit/miss/closest + cache lifecycle + nullptr-ITS tolerance.

Regression: PartPlateTests 53/53, QmlUiAuditTests 83/83 pass.
