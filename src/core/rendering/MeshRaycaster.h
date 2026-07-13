#pragma once

// ===========================================================================
// Phase 113 (MEASURE-02): pure-CPU MeshRaycaster port.
//
// Upstream anchor:
//   third_party/OrcaSlicer/src/slic3r/GUI/MeshUtils.hpp:159+ (class
//   MeshRaycaster) + MeshUtils.cpp:425-466 (unproject_on_mesh) and :532-565
//   (closest_hit). The upstream math is delegated to libslic3r's AABBMesh
//   (libslic3r/AABBMesh.{hpp,cpp}), which builds a BVH over the ITS once and
//   answers query_ray_hits() in O(log N). That class is PURE libslic3r (no
//   GL / wxWidgets dependency), so this port reuses it directly instead of
//   re-implementing Moller-Trumbore. This satisfies MR-01 (pure-CPU, no
//   GL/wxWidgets deps) and keeps the intersection math byte-for-byte aligned
//   with upstream (same epsilon policy, same BVH traversal, same hit_result
//   semantics).
//
// Pure-CPU guarantee (MR-01): this header pulls in ONLY libslic3r math headers
// (AABBMesh + TriangleMesh) plus <memory>/<optional>. It must NOT include any
// GL / wxWidgets / Qt RHI header. It is built under #ifdef HAS_LIBSLIC3R (the
// ITS input only exists there -- Phase 112 ProjectServiceMock::volumeMeshIts
// returns shared_ptr<const indexed_triangle_set>).
//
// Cache contract (MR-02): MeshRaycaster constructs the AABBMesh (BVH) ONCE in
// its ctor and reuses it for every rayCast() call. Callers (SceneRaycaster)
// hold a per-volume MeshRaycaster instance and DO NOT reconstruct it per
// mouse-move. This is the pitfall-7 mitigation -- the BVH build is amortized
// over the volume's lifetime, not the cursor-move rate.
// ===========================================================================

#include <memory>
#include <optional>
#include <cstddef>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/AABBMesh.hpp>
#include <libslic3r/TriangleMesh.hpp>  // indexed_triangle_set, its_face_normals
#endif

#ifdef HAS_LIBSLIC3R
namespace OWzx {

// Result of a single ray-triangle intersection against the wrapped ITS.
// Mirrors the subset of AABBMesh::hit_result (libslic3r/AABBMesh.hpp:65-100)
// that the Qt6 picking/measure path needs. World-space conversion is the
// caller's responsibility (MeshRaycaster works in MESH-LOCAL space; the
// SceneRaycaster applies the per-volume transform to lift the result into
// world space -- see SceneRaycaster::HitResult).
struct MeshRaycasterHit
{
  bool   hit       = false;          // false => ray missed every triangle
  Slic3r::Vec3f position{0.f, 0.f, 0.f};   // intersection, MESH-LOCAL coords
  Slic3r::Vec3f normal{0.f, 0.f, 0.f};     // face normal, MESH-LOCAL coords
  std::size_t facetIdx = static_cast<std::size_t>(-1); // index into ITS indices
};

// MeshRaycaster -- Qt6 pure-CPU port of upstream MeshUtils.hpp:159+.
//
// Wraps a shared_ptr<const indexed_triangle_set> (the Phase 112
// ProjectServiceMock::volumeMeshIts output) and exposes a rayCast() query in
// MESH-LOCAL coordinates. The caller is responsible for transforming the ray
// from world to mesh-local before calling rayCast() (mirror of upstream
// unproject_on_mesh lines 432-434: `Transform3d inv = trafo.inverse();
// point = inv*point; direction = inv.linear()*direction;`).
//
// MR-01 pure-CPU: no GL / wxWidgets / Qt RHI deps. Only libslic3r AABBMesh
// (BVH) + the indexed_triangle_set.
//
// MR-02 cache: the AABBMesh BVH is built once in the ctor (the dominant cost)
// and reused for every rayCast(). SceneRaycaster caches MeshRaycaster
// instances per (object, volume) and invalidates on model change; a per-
// mouse-mouse reconstruction would rebuild the BVH every frame (pitfall 7).
class MeshRaycaster
{
public:
  // Constructs a raycaster over the given ITS. The ITS is held by shared_ptr
  // (shallow-share -- the Phase 112 accessor returns an aliasing shared_ptr
  // that keeps the TriangleMesh alive, see ProjectServiceMock.h ownership
  // contract). The AABBMesh BVH + face normals are precomputed here so
  // rayCast() is a pure traversal.
  //
  // calculate_epsilon=true mirrors upstream MeshUtils.hpp:166
  // (`m_emesh(*m_mesh, true)`), deriving the triangle-ray intersection
  // epsilon from the average edge length. False falls back to AABBMesh's
  // default epsilon (sane for "reasonable" meshes -- AABBMesh.cpp:13).
  explicit MeshRaycaster(std::shared_ptr<const indexed_triangle_set> its,
                         bool calculate_epsilon = true);

  // Returns the ITS this raycaster was built over (for inspection / cache
  // identity checks). Never null for a valid MeshRaycaster (ctor asserts).
  const std::shared_ptr<const indexed_triangle_set> &its() const { return m_its; }

  // Cast a ray in MESH-LOCAL coordinates against the cached BVH. Returns the
  // CLOSEST intersection (mirrors upstream closest_hit, MeshUtils.cpp:532+,
  // which delegates to AABBMesh::query_ray_hit and picks the nearest). When
  // the ray misses every triangle, MeshRaycasterHit::hit == false.
  //
  // rayDir does NOT need to be normalized -- AABBMesh normalizes internally
  // (its hit_result.position() = source + dir * t uses the same dir scale).
  // Callers that need a metric distance should normalize before calling.
  MeshRaycasterHit rayCast(const Slic3r::Vec3d &rayOrigin,
                           const Slic3r::Vec3d &rayDir) const;

  // Precomputed face normal at the given facet index. Mirrors upstream
  // MeshRaycaster::get_triangle_normal (MeshUtils.cpp:412-415) -- the normals
  // are computed once in the ctor via its_face_normals (same as upstream).
  Slic3r::Vec3f triangleNormal(std::size_t facetIdx) const;

private:
  std::shared_ptr<const indexed_triangle_set> m_its;
  Slic3r::AABBMesh                 m_emesh;
  std::vector<Slic3r::Vec3f>       m_normals; // its_face_normals() output (TriangleMesh.hpp:328)
};

} // namespace OWzx
#endif // HAS_LIBSLIC3R
