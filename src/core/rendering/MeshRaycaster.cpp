// ===========================================================================
// Phase 113 (MEASURE-02): pure-CPU MeshRaycaster port implementation.
// See MeshRaycaster.h for the upstream anchor + MR-01/MR-02 contracts.
// ===========================================================================

#include "MeshRaycaster.h"

#ifdef HAS_LIBSLIC3R
#include <cassert>

namespace OWzx {

MeshRaycaster::MeshRaycaster(std::shared_ptr<const indexed_triangle_set> its,
                             bool calculate_epsilon)
    : m_its(std::move(its))
    // AABBMesh builds the BVH once here (AABBMesh.cpp:14-31 init()). The
    // calculate_epsilon flag mirrors upstream MeshUtils.hpp:166
    // (`m_emesh(*m_mesh, true)`): true => derive the ray-triangle epsilon
    // from the average edge length; false => default 1e-6.
    , m_emesh(*m_its, calculate_epsilon)
    // its_face_normals (TriangleMesh.hpp:328) mirrors upstream
    // MeshUtils.hpp:167 `m_normals(its_face_normals(m_mesh->its))`.
    , m_normals(Slic3r::its_face_normals(*m_its))
{
  // The Phase 112 accessor guarantees non-empty ITS (vertices+indices both
  // populated), and the shared_ptr is non-null. Assert the invariant here so
  // a misuse fails loudly instead of building an empty BVH.
  assert(m_its && "MeshRaycaster requires a non-null ITS");
  assert(!m_its->indices.empty() && "MeshRaycaster requires a non-empty triangle set");
}

MeshRaycasterHit MeshRaycaster::rayCast(const Slic3r::Vec3d &rayOrigin,
                                        const Slic3r::Vec3d &rayDir) const
{
  // Mirror of upstream unproject_on_mesh / closest_hit
  // (MeshUtils.cpp:436 `m_emesh.query_ray_hits(point, direction)` +
  // :532 closest_hit which takes the nearest hit). We use the single-hit
  // query_ray_hit() (AABBMesh.hpp:118) which already returns the closest --
  // it is the libslic3r primitive closest_hit builds on.
  const Slic3r::AABBMesh::hit_result result = m_emesh.query_ray_hit(rayOrigin, rayDir);

  MeshRaycasterHit out;
  if (!result.is_hit())
    return out; // hit == false, no intersection

  out.hit      = true;
  // position()/normal() are in the same mesh-local space as the ray the
  // caller passed in (the caller is responsible for the world->mesh inverse
  // transform -- mirror of MeshUtils.cpp:432-434).
  out.position = result.position().cast<float>();
  out.normal   = result.normal().cast<float>();
  out.facetIdx = static_cast<std::size_t>(result.face());
  return out;
}

Slic3r::Vec3f MeshRaycaster::triangleNormal(std::size_t facetIdx) const
{
  // Mirror of upstream MeshRaycaster::get_triangle_normal (MeshUtils.cpp:412).
  if (facetIdx >= m_normals.size())
    return Slic3r::Vec3f::Zero();
  return m_normals[facetIdx];
}

} // namespace OWzx
#endif // HAS_LIBSLIC3R
