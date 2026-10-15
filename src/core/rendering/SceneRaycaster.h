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
//   ProjectService::volumeMeshIts accessor (the ITS source) and
//   reconstructed ONLY when invalidate() is called (the model change signal).
//   No reconstruction happens on cursor move.
// ===========================================================================

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <QMatrix4x4>
#include <QVector3D>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Point.hpp>  // Vec3d, Transform3d (via Point.hpp -> Eigen)
struct indexed_triangle_set;
#endif

#include "MeshRaycaster.h"

// ===========================================================================
// S4 (audit 2026-10-10 ch.4): SceneTransforms -- the single spelling of the
// pick/render world chain.
//
// Since S3 the renderer draws every volume as
//   world_gl = channel · A · (volMat · v)
// where the mesh blob bakes the volume-local GL vertices A·(volMat·v) and the
// per-batch instance channel (ProjectService::meshBatchInstanceTransforms)
// applies T(plateShift) · A · instMat · A⁻¹. A is the slic3r->GL AXES-ROT
// permutation (GL.x=x, GL.y=z, GL.z=-y; det=+1).
//
// The pick side raycasts the RAW volume ITS (slic3r volume-local, pre-volMat
// -- ProjectService::volumeMeshIts), so its candidate worldTransform must be
// the same chain:
//   worldTransform = channel · A · volMat
// (SceneTransforms::volumeWorldMatrix / volumeWorldTransform below). Before
// S4 every EditorViewModel pick entry re-assembled its own transform from the
// decomposed object accessors -- GL-mapped translation + slic3r-frame
// rotation/scale, no volume matrix, no plate shift -- which is frame-mixed
// and diverges from the rendered frame exactly when the instance is rotated
// (the "90° ghost": stage-2 hit lands on geometry the render never drew).
// Consuming the render channel makes that divergence structurally impossible:
// pick and render walk the same matrices.
//
// Qt-only (usable from the standalone picking suites, which do not link
// libslic3r); the Eigen bridge for the ViewModel lives under HAS_LIBSLIC3R.
// ===========================================================================
namespace OWzx
{
namespace SceneTransforms
{
// The slic3r->GL AXES-ROT permutation A (GL.x=x, GL.y=z, GL.z=-y; det=+1, a
// rotation -- not a mirror). Mirror of the production spelling at
// ProjectService::axesRotSlic3rToGl (ProjectService.cpp:513), which owns the
// render-side compose; the two must stay the same matrix and the S4 suite in
// tests/ObjectPickingTests.cpp pins the chain against the rendered frame.
inline QMatrix4x4 slic3rToGlAxes()
{
  // QMatrix4x4's array constructor is row-major.
  const float values[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
  };
  return QMatrix4x4(values);
}

// The full pick-side chain: raw volume-local mesh vertex -> rendered GL
// world. `batchInstanceChannel` is one entry of the S3 render channel
// (ProjectService::meshBatchInstanceTransforms -- the same matrices the
// renderer consumes); `volumeMatrix` is the volume's own (slic3r-frame)
// transform, identity for the common single-part import where the instance
// carries the whole placement.
inline QMatrix4x4 volumeWorldMatrix(
    const QMatrix4x4 &batchInstanceChannel,
    const QMatrix4x4 &volumeMatrix = QMatrix4x4())
{
  return batchInstanceChannel * slic3rToGlAxes() * volumeMatrix;
}

// Direction back-conversion GL -> slic3r (linear part only). Consumers whose
// math is slic3r-frame (e.g. the flatten rotation, which feeds
// setObjectRotation) convert the world-space hit normal through this. A is
// orthonormal (a +-1 permutation), so the inverse is the transpose.
inline QVector3D glDirectionToSlic3r(const QVector3D &glDirection)
{
  return slic3rToGlAxes().transposed().mapVector(glDirection);
}
} // namespace SceneTransforms

// The Eigen bridge + the raycaster itself only exist with libslic3r; the
// unconditional OWzx namespace opened above closes AFTER this guard so
// SceneTransforms is available to the standalone (non-HAS_LIBSLIC3R) suites.
#ifdef HAS_LIBSLIC3R


namespace
{
// Eigen bridge for the ViewModel pick paths: QMatrix4x4 (Qt, row-major
// accessors) -> Transform3d (Eigen, column-major storage).
inline Slic3r::Transform3d transformFromQMatrix(const QMatrix4x4 &m)
{
  Slic3r::Transform3d t = Slic3r::Transform3d::Identity();
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c)
      t.matrix()(r, c) = double(m(r, c));
  return t;
}
} // namespace

// S4: the volume world chain as a Transform3d -- the candidate worldTransform
// for SceneRaycaster::hitTest (see the SceneTransforms header comment). The
// channel·A product is evaluated in float (the channel is float data); the
// volume matrix rides the Eigen product so double precision is kept where it
// exists.
inline Slic3r::Transform3d volumeWorldTransform(
    const QMatrix4x4 &batchInstanceChannel,
    const Slic3r::Transform3d &volumeMatrix = Slic3r::Transform3d::Identity())
{
  return transformFromQMatrix(batchInstanceChannel
                              * SceneTransforms::slic3rToGlAxes())
         * volumeMatrix;
}

// S4 re-review: the Z-UP world frame (upstream/slic3r world) as an Eigen
// transform -- the inverse of the AXES-ROT permutation (GL(X,Y,Z) ->
// slic3r(X,-Z,Y)). Math that compares WORLD Z against a height (the vendored
// TriangleSelector::HeightRange cursor, TriangleSelector.cpp:1154-1176, and
// anything else with upstream Z=up semantics) must consume A⁻¹·chain, NOT
// the GL chain: the GL chain's z column is the desktop depth axis.
inline Slic3r::Transform3d glToSlic3rAxes()
{
  Slic3r::Transform3d t = Slic3r::Transform3d::Identity();
  t.matrix() << 1.0, 0.0, 0.0, 0.0,
                0.0, 0.0, -1.0, 0.0,
                0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 0.0, 1.0;
  return t;
}

// Hit result for the two-stage pick. This is the WORLD-SPACE view the
// Phase 114 (Measuring) + Phase 115 (snap UX) consumers need; MeshRaycasterHit
// (MeshRaycaster.h) is the mesh-local intermediate.
//
// MR-04: object index, volume index, triangle index, world-space hit
// position, world-space normal -- sufficient for the measure + snap
// downstream phases.
//
// TS-02 (Phase 120 / PAINT-01): meshLocalPosition carries the mesh-local hit
// point (the raw MeshRaycasterHit.position, a Vec3f in mesh coordinates).
// TriangleSelector::select_patch needs the mesh-local hit point
// (TriangleSelector.hpp:306-312 -- facet_start + cursor center are mesh-local)
// to drive the brush subdivision. Without this field the paint path would
// have to re-derive the local hit from worldPosition + the inverse transform,
// which loses the float precision of the BVH intersection. Phase 120 fills
// this from the winning MeshRaycasterHit (previously discarded at
// SceneRaycaster.cpp:85-89).
struct SceneRaycasterHit
{
  bool          hit         = false;
  int           objectIndex = -1;          // candidate object that was hit
  int           volumeIndex = -1;          // candidate volume that was hit
  std::size_t   facetIdx    = static_cast<std::size_t>(-1); // ITS triangle index
  Slic3r::Vec3d worldPosition{0.0, 0.0, 0.0}; // world-space intersection
  Slic3r::Vec3d worldNormal{0.0, 0.0, 0.0};   // world-space face normal
  Slic3r::Vec3f meshLocalPosition{0.f, 0.f, 0.f}; // TS-02: mesh-local hit (Vec3f)
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
  // S4 contract: the FULL GL-frame world chain
  //   T(plateShift) · A · instMat · volMat
  // (A = the slic3r->GL AXES-ROT permutation) that maps a RAW volume-local
  // ITS vertex (ProjectService::volumeMeshIts, pre-volMat) to the exact
  // world position the renderer drew. Compose it through
  // OWzx::volumeWorldTransform from the S3 render channel
  // (ProjectService::meshBatchInstanceTransforms) so picking consumes the
  // same matrices the render submission walked -- hand-assembling it from
  // the decomposed object accessors is what produced the frame-mixed
  // "rotated ghost" hits S4 removes. Mesh-local ray =
  // inverse(worldTransform) * world ray (mirror of MeshUtils.cpp:432-434).
  Slic3r::Transform3d worldTransform = Slic3r::Transform3d::Identity();
};

// SceneRaycaster -- caches per-volume MeshRaycaster instances and runs the
// stage-2 per-triangle raycast on candidate volumes only (MR-02/MR-03).
//
// The ITS source is an injected callable (VolumeMeshItsFn) so this class has
// no hard dependency on ProjectService -- it just needs something that
// returns shared_ptr<const indexed_triangle_set> for (object, volume). In
// production this is `projectServiceMock->volumeMeshIts`; in tests it can be
// a lambda over a synthetic ITS.
class SceneRaycaster
{
public:
  // Callable: (objectIndex, volumeIndex) -> shared_ptr<const ITS> (nullptr if
  // invalid / no mesh). Modeled on ProjectService::volumeMeshIts (Phase 112).
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

#endif // HAS_LIBSLIC3R
} // namespace OWzx
