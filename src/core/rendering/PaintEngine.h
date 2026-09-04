#pragma once

// ===========================================================================
// Phase 120 (PAINT-01): per-volume TriangleSelector owner + select_patch
// driver + get_facets reader. This is the Qt6 equivalent of upstream
// GLGizmoPainterBase::m_triangle_selectors (GLGizmoPainterBase.hpp:250), but
// pure C++ (no GL / wx / Qt widget dependency) so it is unit-testable.
//
// Upstream reference:
//   third_party/OrcaSlicer/src/libslic3r/TriangleSelector.hpp -- the
//   TriangleSelector class is PURE libslic3r (no GL/wx coupling) and is
//   already compiled into the project (CMakeLists.txt:457-458). It is REUSED
//   byte-for-byte. This class owns the Qt6 lifetime + wiring, NOT a
//   reimplementation of the selector or the cursor math.
//
//   TriangleSelector.hpp:299     ctor (const TriangleMesh&, edge_limit)
//   TriangleSelector.hpp:306-312 select_patch (facet_start, cursor, state,
//                                          trafo_no_translate, splitting)
//   TriangleSelector.hpp:114/123 cursor_factory (Sphere/Circle + HeightRange)
//   TriangleSelector.hpp:333     get_facets (EnforcerBlockerType) -> ITS
//   TriangleSelector.hpp:357/360 serialize / deserialize (TriangleSplittingData)
//   TriangleSelector.hpp:13-38   EnforcerBlockerType enum (None/Enforcer/
//                                Blocker + Extruder1..16 for MMU)
//
// REUSE, NOT REIMPLEMENT (TS-03):
//   This class wraps `Slic3r::TriangleSelector` directly. It does NOT
//   reimplement the adaptive subdivision, the cursor containment tests, or
//   the serialize/deserialize bitstream. The selector is constructed lazily
//   per (objectIndex, volumeIndex) from the Phase 120 TriangleMesh source
//   (ProjectServiceMock::volumeMeshTriangleMesh), cached, and invalidated on
//   model change. The TriangleMesh shared_ptr is held alongside the selector
//   so the reference TriangleSelector stores (TriangleSelector.hpp:477 m_mesh)
//   stays valid for the selector's whole lifetime (TS-01 ownership contract).
//
// THREE STRUCTURAL GAPS THIS CLASS BRIDGES (Phase 120 CONTEXT.md):
//   1. TriangleSelector ctor takes `const TriangleMesh&` -- the mesh source
//      here returns shared_ptr<const TriangleMesh> (aliasing shallow-share,
//      TS-01). The shared_ptr is stashed on the cache entry so the refcount
//      keeps the TriangleMesh alive.
//   2. select_patch needs mesh-local hit -- PaintEngine.paintAt takes the
//      meshLocalHit (Vec3f, from SceneRaycasterHit.meshLocalPosition, TS-02)
//      and feeds it to cursor_factory as the cursor center.
//   3. m_paintData was a flat placeholder -- PaintEngine is the real
//      per-volume owner (replaces the QList<ObjectPaintData> stub).
//
// UNIT-TESTABLE BOUNDARY (TS-08):
//   The cursor-construction + select_patch invocation is extracted as a free
//   helper (applyPaintToSelector) so a unit test can drive it with a
//   synthesized TriangleMesh + a known facet, WITHOUT a Model/renderer. The
//   ViewModelSmokeTests case exercises it directly.
//
// The whole class is #ifdef HAS_LIBSLIC3R -- the TriangleSelector /
// TriangleMesh / indexed_triangle_set types only exist when libslic3r is
// built (same guard as SceneRaycaster / MeasureEngine). A non-lib stub is
// provided so EditorViewModel can compile unconditionally.
// ===========================================================================

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <tuple>
#include <utility>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Point.hpp>  // Vec3f, Vec3d, Transform3d (via Point.hpp -> Eigen)
#include <libslic3r/TriangleSelector.hpp>
#include <libslic3r/TriangleMesh.hpp>  // TriangleMesh, indexed_triangle_set
#endif

namespace OWzx {

// Paint kind: which painter gizmo a selector belongs to. The canonical enum
// for the whole paint pipeline (ProjectServiceMock.h aliases ::PaintKind to
// this). It bridges the TriangleSelector into one of the three ModelVolume
// FacetsAnnotation members (upstream update_model_object:
// GLGizmoFdmSupports.cpp:577, GLGizmoSeam.cpp:315,
// GLGizmoMmuSegmentation.cpp:700). Upstream each painter gizmo owns its OWN
// TriangleSelector (GLGizmoPainterBase.hpp:247 m_triangle_selectors is
// per-gizmo state), so selectors must never be shared across kinds.
// Declared outside the HAS_LIBSLIC3R guard: it is a plain enum with no
// libslic3r dependency, and the service-layer API signatures reference it
// unconditionally.
enum class PaintKind : int
{
  Support = 0,
  Seam = 1,
  Mmu = 2
};

} // namespace OWzx

#ifdef HAS_LIBSLIC3R
namespace OWzx {

// Brush cursor shape. Mirrors TriangleSelector::CursorType
// (TriangleSelector.hpp:52-59). Phase 120 wires Sphere + Circle (the two
// SinglePointCursor shapes). HeightRange / Capsule are Phase 121+ (brush UI).
enum class PaintCursorType : int
{
  Circle = 0,      // TriangleSelector::CursorType::CIRCLE
  Sphere = 1,      // TriangleSelector::CursorType::SPHERE
  HeightRange = 2  // TriangleSelector::CursorType::HEIGHT_RANGE (MMU tool)
};

// PaintEngine -- per-volume TriangleSelector owner + select_patch driver.
//
// The TriangleMesh source is an injected callable (VolumeMeshFn) so this class
// has no hard dependency on ProjectServiceMock -- it just needs something that
// returns shared_ptr<const TriangleMesh> for (object, volume). In production
// this is ProjectServiceMock::volumeMeshTriangleMesh (Phase 120 TS-01). The
// callable is the SAME mesh the ModelVolume owns (aliasing shared_ptr), so the
// TriangleMesh outlives every selector that references it.
//
// CACHE (TS-03): one TriangleSelector per (objectIndex, volumeIndex, kind),
// built lazily on the first paintAt() call and reconstructed ONLY when
// invalidate()/invalidateVolume()/clearObject() is called (the model change
// signal). No reconstruction happens on cursor move (pitfall 7 -- the
// TriangleSelector ctor indexes the mesh; rebuilding per-frame melts the
// framerate). The kind dimension mirrors upstream: GLGizmoFdmSupports,
// GLGizmoSeam, and GLGizmoMmuSegmentation each own separate selectors
// (GLGizmoPainterBase.hpp:247), so Support state never leaks into Seam or
// MMU annotations.
class PaintEngine
{
public:
  // Callable: (objectIndex, volumeIndex) -> shared_ptr<const TriangleMesh>
  // (nullptr if invalid / no mesh). The Phase 120 accessor
  // ProjectServiceMock::volumeMeshTriangleMesh returns exactly this type.
  using VolumeMeshFn =
      std::function<std::shared_ptr<const Slic3r::TriangleMesh>(int, int)>;

  explicit PaintEngine(VolumeMeshFn meshSource)
      : m_meshSource(std::move(meshSource))
  {}

  // Cannot be defaulted in the header: the cache holds unique_ptr to an
  // incomplete-in-other-TUs type when this header is included without
  // HAS_LIBSLIC3R. Defined out-of-line in the .cpp where the selector type
  // is complete.
  ~PaintEngine();

  // Lazily fetch-or-build the TriangleSelector for one volume + paint kind.
  // Returns a raw pointer (non-owning) to the cached selector, or nullptr if
  // the mesh source has no mesh for that (object, volume) pair. The selector
  // stays owned by this engine; callers MUST NOT delete it.
  Slic3r::TriangleSelector *ensureSelector(
      int objectIndex, int volumeIndex, PaintKind kind,
      const std::shared_ptr<const Slic3r::TriangleMesh> &meshSharedPtr);

  // Convenience overload: pulls the mesh from the injected source then calls
  // the overload above. Used by paintAt + the ViewModel.
  Slic3r::TriangleSelector *ensureSelector(int objectIndex, int volumeIndex,
                                           PaintKind kind);

  // Drive select_patch on the (lazily-built) selector for (object, volume,
  // kind).
  //
  // facetIdx:    the ITS triangle index the ray hit (SceneRaycasterHit::
  //              facetIdx) -- select_patch's facet_start.
  // meshLocalHit: the mesh-local hit point (SceneRaycasterHit::
  //              meshLocalPosition, TS-02) -- the cursor center.
  // cursor:      brush shape (Circle/Sphere) + radius.
  // state:       EnforcerBlockerType (None/Enforcer/Blocker/ExtruderN).
  // trafo:       mesh->world transform WITHOUT translation (select_patch's
  //              trafo_no_translate, TriangleSelector.hpp:309). The caller
  //              strips translation upstream.
  // highlightByAngleDeg: Phase 240 (GIZ-02) overhang filter. Upstream
  //              GLGizmoPainterBase threads
  //              m_paint_on_overhangs_only ? m_highlight_by_angle_threshold_deg : 0.f
  //              into every select_patch call (GLGizmoPainterBase.cpp:800-805);
  //              a non-zero value restricts painting to facets whose slope
  //              exceeds the threshold.
  //
  // Returns true if the selector exists (or was built) AND select_patch ran.
  // Returns false if the mesh source had no mesh for the (object, volume).
  //
  // clippingPlane (P2.3-1): mesh-local cross-section plane limiting painting
  // to facets on the visible side (upstream threads
  // get_clipping_plane_in_volume_coordinates() into every select_patch,
  // GLGizmoPainterBase.cpp:724-728). Default-constructed = inactive
  // (offset FLT_MAX, TriangleSelector.hpp:63) so existing callers keep the
  // unclipped behavior.
  bool paintAt(int objectIndex, int volumeIndex, PaintKind kind, int facetIdx,
               const Slic3r::Vec3f &meshLocalHit,
               float brushRadius, PaintCursorType cursor,
               Slic3r::EnforcerBlockerType state,
               const Slic3r::Transform3d &trafo,
               const Slic3r::Vec3f &cameraPosMeshLocal,
               float highlightByAngleDeg = 0.f,
               const Slic3r::TriangleSelector::ClippingPlane &clippingPlane =
                   Slic3r::TriangleSelector::ClippingPlane());

  // Phase 240 (GIZ-02): smart (seed) fill. Mirrors the upstream SMART_FILL
  // tool click path (GLGizmoPainterBase.cpp:773-781): seed_fill_select_
  // triangles grows a selection from the hit facet bounded by
  // seedFillAngle (the maximal angle between two facets painted by the same
  // color) + the optional overhang filter, then seed_fill_apply_on_
  // triangles commits the selected region with `state`.
  //
  // seedFillAngle:      region-growth angle threshold in degrees
  //                     (upstream m_smart_fill_angle).
  // highlightByAngleDeg: overhang filter angle in degrees, 0 = off
  //                     (upstream m_paint_on_overhangs_only ?
  //                     m_highlight_by_angle_threshold_deg : 0.f).
  bool smartFillAt(int objectIndex, int volumeIndex, PaintKind kind,
                   int facetIdx,
                   const Slic3r::Vec3f &meshLocalHit,
                   float seedFillAngle, float highlightByAngleDeg,
                   Slic3r::EnforcerBlockerType state,
                   const Slic3r::Transform3d &trafo,
                   const Slic3r::TriangleSelector::ClippingPlane &clippingPlane =
                       Slic3r::TriangleSelector::ClippingPlane());

  // P2.3-5 (MMU height-range tool): paint every facet whose world Z falls in
  // [hitWorldZ, hitWorldZ + height]. Mirrors the upstream HEIGHT_RANGE brush
  // (GLGizmoPainterBase.cpp:726-733): SinglePointCursor::cursor_factory with
  // the world-space bottom Z builds a HeightRange cursor; select_patch's
  // hr_cursor branch then scans ALL mesh facets for the range (facetIdx is
  // carried for contract symmetry but not used by the hr branch).
  bool paintHeightRangeAt(int objectIndex, int volumeIndex, PaintKind kind,
                          float hitWorldZ, float height,
                          Slic3r::EnforcerBlockerType state,
                          const Slic3r::Transform3d &trafo,
                          const Slic3r::Vec3f &cameraPosMeshLocal,
                          const Slic3r::TriangleSelector::ClippingPlane &clippingPlane =
                              Slic3r::TriangleSelector::ClippingPlane());

  // Read the facets currently marked with `state` as an indexed_triangle_set.
  // Wraps TriangleSelector::get_facets (TriangleSelector.hpp:333). Returns a
  // shared_ptr so the caller can hand it to the overlay renderer (Phase 121)
  // without copying. Returns nullptr if no selector exists for the triple.
  std::shared_ptr<indexed_triangle_set>
  getFacets(int objectIndex, int volumeIndex, PaintKind kind,
            Slic3r::EnforcerBlockerType state);

  // P2.3-5: STAGED seed-fill region readback (see getSeedSelection above for
  // the upstream reference) — declared with the reader cluster below.
  std::shared_ptr<indexed_triangle_set>
  getSeedSelection(int objectIndex, int volumeIndex, PaintKind kind);

  // Is anything staged in the seed-fill region for this triple? False when no
  // selector exists.
  bool hasSeedSelection(int objectIndex, int volumeIndex, PaintKind kind) const;

  // P2.3-5 (gap-area tool): facets whose area is below gapAreaMm2 (upstream
  // TrianglePatch::is_fragment, area < TriangleSelectorPatch::gap_area,
  // GLGizmoPainterBase.cpp:1234-1236; gap area range 0..5 mm2, step 0.2).
  // Returns the compact ITS of fragment leaves, or nullptr when no selector
  // exists for the triple.
  std::shared_ptr<indexed_triangle_set>
  getGapFragments(int objectIndex, int volumeIndex, PaintKind kind,
                  float gapAreaMm2);

  // Does any facet carry `state`? Wraps TriangleSelector::has_facets
  // (TriangleSelector.hpp:329). Returns false if no selector exists.
  bool hasFacets(int objectIndex, int volumeIndex, PaintKind kind,
                 Slic3r::EnforcerBlockerType state) const;

  // Drop every cached selector. Call on model change (load / cut / boolean /
  // simplify / drill / orient / arrange / delete-object). The next paintAt()
  // lazily rebuilds only the volumes it touches.
  void invalidate() { m_cache.clear(); }

  // Drop every cached selector for one object (all its volumes + kinds). Used
  // on gizmo-exit cleanup (clearPaintOnObject) so a re-enter rebuilds fresh.
  void clearObject(int objectIndex);

  // Drop the cache entry for a single volume + kind (granular invalidation).
  void invalidateVolume(int objectIndex, int volumeIndex, PaintKind kind)
  {
    m_cache.erase(volumeKey(objectIndex, volumeIndex, kind));
  }

  // Serialize the per-volume paint state (TriangleSelector.hpp:357). Returns
  // nullopt if no selector exists for the triple. The wiring to 3MF save/load
  // is Phase 122/123; this just exposes the upstream API.
  std::optional<Slic3r::TriangleSelector::TriangleSplittingData>
  serialize(int objectIndex, int volumeIndex, PaintKind kind) const;

  // Deserialize into the (lazily-built) selector for (object, volume, kind).
  // Wraps TriangleSelector::deserialize (TriangleSelector.hpp:360). Returns
  // false if the mesh source had no mesh for the triple (cannot build
  // selector).
  bool deserialize(int objectIndex, int volumeIndex, PaintKind kind,
                   const Slic3r::TriangleSelector::TriangleSplittingData &data);

  // Number of currently-cached selectors (diagnostic / test hook).
  std::size_t cachedSelectorCount() const { return m_cache.size(); }

  // Read-only access to the cached selector for one volume + kind, WITHOUT
  // building. Returns nullptr if no selector has been built for the
  // (object, volume, kind) triple (e.g. nothing painted yet). Used by the
  // Phase 122/123 write-back that feeds ModelVolume::supported_facets /
  // seam_facets / mmu_segmentation_facets (FacetsAnnotation::set takes a
  // const TriangleSelector&). The pointer stays owned by this engine; callers
  // MUST NOT delete it.
  const Slic3r::TriangleSelector *cachedSelectorForVolume(
      int objectIndex, int volumeIndex, PaintKind kind) const;

private:
  // (objectIndex, volumeIndex, kind) -- one selector per triple. std::tuple
  // has a lexicographic operator< so it works as a std::map key.
  using Key = std::tuple<int, int, PaintKind>;

  static Key volumeKey(int objectIndex, int volumeIndex, PaintKind kind)
  {
    return std::make_tuple(objectIndex, volumeIndex, kind);
  }

  // One cache entry bundles the TriangleSelector with the shared_ptr that
  // keeps its TriangleMesh reference alive. The shared_ptr MUST be held for
  // the selector's whole lifetime (TriangleSelector.hpp:477 stores a const
  // TriangleMesh& -- if the TriangleMesh is destroyed first, the reference
  // dangles).
  struct CacheEntry
  {
    std::shared_ptr<const Slic3r::TriangleMesh> mesh;  // keeps mesh alive
    std::unique_ptr<Slic3r::TriangleSelector>   selector;
  };

  VolumeMeshFn m_meshSource;
  // std::map (not unordered_map) so we don't need a std::hash<std::pair>
  // specialization. Cache size is bounded by the touched-volume count.
  std::map<Key, CacheEntry> m_cache;
};

// applyPaintToSelector -- pure helper extracted for unit testing (TS-08).
//
// Builds a TriangleSelector::Cursor via cursor_factory (Sphere/Circle,
// TriangleSelector.hpp:114) from the brush params + trafo, then drives
// select_patch on the given selector. This is the entire "given a hit + facet
// + brush params -> the selector state change" step, isolated from the
// Model/renderer so a unit test can call it with a synthesized mesh.
//
// trafo is the mesh->world transform WITHOUT translation (select_patch's
// trafo_no_translate). The cursor center (meshLocalHit) + camera source are
// in mesh-local coords. triangle_splitting=true mirrors upstream painting
// (TriangleSelector.hpp:310-311 -- the brush subdivides triangles inside the
// cursor so the paint edge follows the brush outline).
void applyPaintToSelector(Slic3r::TriangleSelector &selector,
                          int facetIdx,
                          const Slic3r::Vec3f &meshLocalHit,
                          float brushRadius, PaintCursorType cursor,
                          Slic3r::EnforcerBlockerType state,
                          const Slic3r::Transform3d &trafo,
                          const Slic3r::Vec3f &cameraPosMeshLocal,
                          float highlightByAngleDeg = 0.f,
                          const Slic3r::TriangleSelector::ClippingPlane &clippingPlane =
                              Slic3r::TriangleSelector::ClippingPlane());

// Phase 240 (GIZ-02): pure smart-fill helper extracted for unit testing
// (same TS-08 pattern as applyPaintToSelector). Drives the upstream
// seed_fill_select_triangles + seed_fill_apply_on_triangles pair exactly as
// PaintEngine::smartFillAt does in production. No Model / renderer needed.
void applySmartFillToSelector(Slic3r::TriangleSelector &selector,
                              int facetIdx,
                              const Slic3r::Vec3f &meshLocalHit,
                              float seedFillAngle, float highlightByAngleDeg,
                              Slic3r::EnforcerBlockerType state,
                              const Slic3r::Transform3d &trafo,
                              const Slic3r::TriangleSelector::ClippingPlane &clippingPlane =
                                  Slic3r::TriangleSelector::ClippingPlane());

} // namespace OWzx
#endif // HAS_LIBSLIC3R
