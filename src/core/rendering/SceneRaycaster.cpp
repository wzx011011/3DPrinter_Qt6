// ===========================================================================
// Phase 113 (MEASURE-02): thin Qt6 SceneRaycaster implementation.
// See SceneRaycaster.h for the two-stage pick (MR-03) + cache (MR-02)
// contracts and the upstream SceneRaycaster.hpp reference.
// ===========================================================================

#include "SceneRaycaster.h"

#ifdef HAS_LIBSLIC3R
#include <limits>

namespace OWzx {

namespace {
// Limit FP used to skip degenerate/zero-scale transforms and to early-out of
// the closest-hit reduction when no candidate has hit yet.
constexpr double kInfty = std::numeric_limits<double>::infinity();
} // namespace

std::shared_ptr<MeshRaycaster>
SceneRaycaster::raycasterFor(int objectIndex, int volumeIndex)
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it != m_cache.end())
    return it->second; // cache hit (MR-02): reuse the BVH built on first touch

  // Lazily fetch the ITS from the Phase 112 accessor. nullptr => no mesh for
  // this (object, volume); skip caching so a future reload is re-tried.
  std::shared_ptr<const indexed_triangle_set> its = m_itsSource(objectIndex, volumeIndex);
  if (!its)
    return nullptr;

  // Build the MeshRaycaster (BVH construction happens here -- the dominant
  // cost). Cache the result so subsequent hitTest() calls on the same
  // volume are pure traversals.
  std::shared_ptr<MeshRaycaster> rc = std::make_shared<MeshRaycaster>(its);
  m_cache.emplace(key, rc);
  return rc;
}

SceneRaycasterHit SceneRaycaster::hitTest(
    const Slic3r::Vec3d &rayOriginWorld,
    const Slic3r::Vec3d &rayDirWorld,
    const std::vector<SceneRaycasterCandidate> &candidates)
{
  SceneRaycasterHit best; // hit==false by default
  double bestDistSq = kInfty;

  for (const SceneRaycasterCandidate &cand : candidates) {
    std::shared_ptr<MeshRaycaster> rc = raycasterFor(cand.objectIndex, cand.volumeIndex);
    if (!rc)
      continue; // stage-1 produced a candidate with no mesh (deleted volume, etc.)

    // World -> mesh-local ray transform. Mirrors upstream MeshUtils.cpp:432-434
    //   Transform3d inv = trafo.inverse();
    //   point     = inv * point;
    //   direction = inv.linear() * direction;
    const Slic3r::Transform3d inv = cand.worldTransform.inverse();
    const Slic3r::Vec3d meshOrigin = inv * rayOriginWorld;
    // Phase 113 REVIEW M-02: inv.linear() * unitDir is NOT unit under
    // non-uniform scale. Normalize so AABBMesh's Debug assert
    // (is_approx(dir.norm(), 1.)) never trips on scaled instances.
    const Slic3r::Vec3d meshDir = (inv.linear() * rayDirWorld).normalized();

    const MeshRaycasterHit localHit = rc->rayCast(meshOrigin, meshDir);
    if (!localHit.hit)
      continue;

    // Lift the mesh-local result back into world space for the consumer.
    const Slic3r::Vec3d worldPos = cand.worldTransform * localHit.position.cast<double>();
    // Normal: the inverse-transpose of the linear part would be strictly
    // correct under non-uniform scaling; upstream uses trafo.linear() *
    // normal in closest_hit callers, so we match that. Phase 114/115 deal
    // with uniform-scale + rotation only (model instance transforms), where
    // linear() is exact.
    const Slic3r::Vec3d worldNrm =
        (cand.worldTransform.linear() * localHit.normal.cast<double>());

    // Closest-hit reduction across candidate volumes (MR-04). Comparing
    // squared distance from the world ray origin keeps this metric-free.
    const double distSq = (worldPos - rayOriginWorld).squaredNorm();
    if (distSq < bestDistSq) {
      bestDistSq = distSq;
      best.hit = true;
      best.objectIndex = cand.objectIndex;
      best.volumeIndex = cand.volumeIndex;
      best.facetIdx = localHit.facetIdx;
      best.worldPosition = worldPos;
      // Normalize so the consumer gets a unit normal regardless of the
      // transform's scale. Degenerate (zero-length) normals are left as-is.
      const double nrmLen = worldNrm.norm();
      best.worldNormal = (nrmLen > 1e-12) ? (worldNrm / nrmLen) : worldNrm;
      // TS-02 (Phase 120): preserve the mesh-local hit point (Vec3f, raw BVH
      // intersection precision). Previously discarded here. TriangleSelector::
      // select_patch needs this as the cursor center (TriangleSelector.hpp:
      // 306-312) so the paint brush subdivides around the exact hit, not a
      // world->local round-trip approximation.
      best.meshLocalPosition = localHit.position;
    }
  }

  return best;
}

} // namespace OWzx
#endif // HAS_LIBSLIC3R
