---
phase: 114-measure-engine-instantiation-and-feature-readouts
status: NEEDS_CHANGES → FIXED
requirement: MEASURE-03
counts: {blockers: 0, highs: 1, mediums: 1, lows: 2}
---

# Phase 114 Code Review — Measure Engine Instantiation

## Verdict: NEEDS_CHANGES → FIXED (H1 applied)

H1: double world-transform in measureFeatures corrupts A→B readout for non-identity-transformed objects. Fix: delete the two extra translate calls (points already world-space from get_feature). Pitfall-6 scrubbing is exemplary — ZERO back-pointer escape verified.

## H1 (HIGH — FIXED): Double world-transform in measureFeatures

`get_feature` (upstream Measure.cpp:583-594) already translates its output SurfaceFeature to world space via `f.translate(world_tran)`. But `measureFeatures` then calls `sfA.translate(worldTransform)` AGAIN — feeding `T*T*p_local` instead of `T*p_local` to `get_measurement`. For any rotated/scaled object, the A→B distance/angle readout is wrong. The regression test masked it by using `Transform3d::Identity()`.

Fix: delete the two `sfA.translate()` / `sfB.translate()` calls at MeasureEngine.cpp:239-240. The QtFeature points are already world-space (comment at line 234-238 is self-contradicting).

## Other Findings

| # | Severity | Finding |
|---|----------|---------|
| M1 | medium | computeMeasureReadoutFromHit re-resolves the "from" feature with onlySelectPlane from the SECOND hit. Benign in current single-caller path (always false); fragile contract. |
| L1 | low | MeasureReadout struct duplicates QtMeasurement shape (angleDeg vs angleRad). Add cross-reference comment. |
| L2 | low | measureFeatures early-returns invalid when either feature invalid — caller doesn't check. Correct behavior; could assert. |

## Verified Strong

- **Pitfall-6 scrubbing:** scrubSurfaceFeature reads ONLY value accessors (get_point/edge/circle/plane). buildLocalSurfaceFeature reconstructs via ctors that leave back-pointers null. ZERO `void* volume` / `plane_indices` in source. static_assert on QtFeature POD field types. Exemplary.
- **INSTANTIATE not reimplement:** make_shared<Measuring>(*its) verbatim. get_feature verbatim. get_measurement verbatim. No math re-derived.
- **Per-volume cache:** lazy, granular invalidate, skips caching nullptr.
- **AABB augment:** AssemblyMeasureGeometry untouched; relationship documented.

Regression: PartPlateTests 53/53, QmlUiAuditTests 83/83 pass.
