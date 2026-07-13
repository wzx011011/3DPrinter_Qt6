#pragma once

// ===========================================================================
// Phase 113 (MEASURE-02): thin Qt6 SceneRaycaster wrapper.
//
// Upstream reference:
//   third_party/OrcaSlicer/src/slic3r/GUI/SceneRaycaster.hpp -- the upstream
//   scene wrapper holds per-volume MeshRaycaster instances and a transform
//   per item. The FULL upstream class (EType enum: Bed/Volume/Gizmo/
//   FallbackGizmo, encode_id/decode_id, GL debug viz) is too coupled to port
//   verbatim. Per the Phase 113 CONTEXT.md deferred section, this port covers
//   ONLY the Volume path that the picking + measure workstream needs. Bed and
//   Gizmo grabber raycasting remain deferred (Phase 115 snap UX may revisit).
//
// TWO-STAGE PICK (MR-03 / pitfall 7):
//   Stage 1 -- the EXISTING ObjectPicking::pickSourceObject (ObjectPicking.h:14)
//   does a coarse ray->AABB prefilter over the scene-space PrepareSceneData
//   vertices and narrows to a small set of candidate OBJECT indices. Cheap.
//
//   Stage 2 -- THIS class. SceneRaycaster::hitTest takes the stage-1
//   candidate list + the world-space ray, lazily fetches (or reuses from the
//   cache) a MeshRaycaster per candidate volume, transforms the ray into each
//   volume's mesh-local space, runs the per-triangle ITS raycast, and returns
//   the CLOSEST world-space hit across all candidate volumes.
//
//   This is MANDATORY because upstream GLGizmoMeasure.cpp:600-615 loops
//   m_mesh_raycaster_map on EVERY mouse move (the perf bug pitfall 7 calls
//   out). Qt6 bounds the per-frame cost to one BVH traversal PER CANDIDATE
//   VOLUME (typically 1) instead of per-volume-per-frame across the whole
//   scene.
//
// CACHE (MR-02):
//   The per-volume MeshRaycaster instances are cached here, keyed by
//   (objectIndex, volumeIndex). They are populated lazily from the Phase 112
//   ProjectServiceMock::volumeMeshIts accessor (the ITS source) and
//   reconstructed ONLY when invalidate() is called (the model change signal).
//   No reconstruction happens on cursor move.
// ===========================================================================

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Point.hpp>  // Vec3d, Transform3d (via Point.hpp -> Eigen)
struct indexed_triangle_set;
#endif

#include "MeshRaycaster.h"

#ifdef HAS_LIBSLIC3R
namespace OWzx {

// Hit result for the two-stage pick. This is the WORLD-SPACE view the
// Phase 114 (Measuring) + Phase 115 (snap UX) consumers need; MeshRaycasterHit
// (MeshRaycaster.h) is the mesh-local intermediate.
//
// MR-04: object index, volume index, triangle index, world-space hit
// position, world-space normal -- sufficient for the measure + snap
// downstream phases.
struct SceneRaycasterHit
{
  bool          hit         = false;
  int           objectIndex = -1;          // candidate object that was hit
  int           volumeIndex = -1;          // candidate volume that was hit
  std::size_t   facetIdx    = static_cast<std::size_t>(-1); // ITS triangle index
  Slic3r::Vec3d worldPosition{0.0, 0.0, 0.0}; // world-space intersection
  Slic3r::Vec3d worldNormal{0.0, 0.0, 0.0};   // world-space face normal
};

// A single candidate volume produced by stage 1 (the coarse AABB pick).
// SceneRaycaster does NOT perform stage 1 itself -- the caller
// (RhiViewportRenderer / EditorViewModel picking bridge) runs
// ObjectPicking::pickSourceObject and hands the survivors in as candidates.
// This keeps the coarse stage in its existing pure-CPU helper and avoids the
// upstream coupling of fusing both stages into one class.
struct SceneRaycasterCandidate
{
  int               objectIndex  = -1;
  int               volumeIndex  = -1;
  // World transform of the volume (ModelVolume::get_matrix() /
  // instance matrix). Mesh-local ray = inverse(worldTransform) * world ray
  // (mirror of MeshUtils.cpp:432-434).
  Slic3r::Transform3d worldTransform = Slic3r::Transform3d::Identity();
};

// SceneRaycaster -- caches per-volume MeshRaycaster instances and runs the
// stage-2 per-triangle raycast on candidate volumes only (MR-02/MR-03).
//
// The ITS source is an injected callable (VolumeMeshItsFn) so this class has
// no hard dependency on ProjectServiceMock -- it just needs something that
// returns shared_ptr<const indexed_triangle_set> for (object, volume). In
// production this is `projectServiceMock->volumeMeshIts`; in tests it can be
// a lambda over a synthetic ITS.
class SceneRaycaster
{
public:
  // Callable: (objectIndex, volumeIndex) -> shared_ptr<const ITS> (nullptr if
  // invalid / no mesh). Modeled on ProjectServiceMock::volumeMeshIts (Phase 112).
  using VolumeMeshItsFn =
      std::function<std::shared_ptr<const indexed_triangle_set>(int, int)>;

  explicit SceneRaycaster(VolumeMeshItsFn itsSource)
      : m_itsSource(std::move(itsSource))
  {}

  // Stage-2 hit test (MR-03). Takes the stage-1 candidate list + a world-space
  // ray. Returns the CLOSEST hit across all candidate volumes, or
  // hit==false if every candidate missed.
  //
  // The ray does NOT loop the whole scene -- only the candidate volumes that
  // stage 1 (ObjectPicking::pickSourceObject) pre-narrowed. This is the
  // pitfall-7 fix.
  SceneRaycasterHit hitTest(const Slic3r::Vec3d &rayOriginWorld,
                            const Slic3r::Vec3d &rayDirWorld,
                            const std::vector<SceneRaycasterCandidate> &candidates);

  // Drop every cached MeshRaycaster. Call on model change (load / cut /
  // boolean / simplify / drill / orient / arrange / delete-volume). The next
  // hitTest() lazily rebuilds only the candidate volumes it touches.
  void invalidate() { m_cache.clear(); }

  // Drop the cache entry for a single volume (e.g. one volume's mesh changed
  // but the rest of the scene is intact). Granular invalidation keeps the
  // cost proportional to the change.
  void invalidateVolume(int objectIndex, int volumeIndex)
  {
    m_cache.erase(volumeKey(objectIndex, volumeIndex));
  }

  // Number of currently-cached raycasters (diagnostic / test hook). Bounded
  // by the candidate-volume count actually seen since the last invalidate().
  std::size_t cachedRaycasterCount() const { return m_cache.size(); }

private:
  using Key = std::pair<int, int>;

  static Key volumeKey(int objectIndex, int volumeIndex)
  {
    return std::make_pair(objectIndex, volumeIndex);
  }

  // Lazily fetch-or-build the MeshRaycaster for one volume. Returns nullptr
  // if the ITS source has no mesh for that (object, volume) pair.
  std::shared_ptr<MeshRaycaster> raycasterFor(int objectIndex, int volumeIndex);

  VolumeMeshItsFn m_itsSource;
  // std::map (not unordered_map) so we don't need a std::hash<std::pair>
  // specialization. Cache size is bounded by the candidate-volume count, so
  // the O(log N) lookup is irrelevant here.
  std::map<Key, std::shared_ptr<MeshRaycaster>> m_cache;
};

} // namespace OWzx
#endif // HAS_LIBSLIC3R
