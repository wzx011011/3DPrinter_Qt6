// ===========================================================================
// Phase 120 (PAINT-01): PaintEngine implementation.
// See PaintEngine.h for the reuse-not-reimplement contract (TS-03) and the
// three structural gaps this bridges (CONTEXT.md).
//
// REUSE: this file includes <libslic3r/TriangleSelector.hpp> and calls the
// upstream API directly (ctor / select_patch / get_facets / serialize /
// deserialize). It does NOT reimplement the selector, the cursor math, or the
// subdivision. A grep for a hand-rolled selector here MUST return zero.
// ===========================================================================

#include "PaintEngine.h"

#ifdef HAS_LIBSLIC3R
// REUSE (TS-07e): include the upstream TriangleSelector.hpp directly so the
// .cpp drives the upstream API (ctor / select_patch / get_facets / serialize /
// deserialize / cursor_factory) byte-for-byte. This file wraps the upstream
// class; it does NOT redefine it.
#include <libslic3r/TriangleSelector.hpp>
#include <utility>

namespace OWzx {
namespace {

// PAINT-GAPFILL-PORT: exposes the PROTECTED TriangleSelector state
// (m_vertices / m_triangles are protected, TriangleSelector.hpp:364+) so the
// gap-fragment leaves can be flattened for the gap-fill tool view. This
// mirrors upstream TriangleSelectorGUI, which subclasses TriangleSelector for
// exactly the same reason (read-only leaf access for rendering,
// GLGizmoPainterBase.cpp:1123-1157). No behavior is overridden.
//
// Destruction note: the cache stores unique_ptr<TriangleSelector> over this
// subclass exactly like upstream GLGizmoPainterBase::m_triangle_selectors
// stores TriangleSelectorGUI/TriangleSelectorPatch instances
// (GLGizmoPainterBase.hpp:250). TriangleSelector adds no data members to the
// derived layout, so the base-subobject address equals the object address.
class PaintSelector final : public Slic3r::TriangleSelector
{
public:
  explicit PaintSelector(const Slic3r::TriangleMesh &mesh)
      : Slic3r::TriangleSelector(mesh)
  {}

  // PAINT-GAPFILL-PORT: gap fragments = leaf triangles whose mesh-space area
  // is below the gap-area threshold (upstream TrianglePatch::is_fragment,
  // GLGizmoPainterBase.cpp:1234-1236: `area < TriangleSelectorPatch::
  // gap_area`). Areas are mm2 (mesh units), matching the upstream slider
  // range GapAreaMin=0 / GapAreaMax=5 / GapAreaStep=0.2
  // (GLGizmoPainterBase.hpp:117-119). A threshold <= 0 disables the view.
  // Output is vertex-compacted like the upstream get_facets result.
  indexed_triangle_set gapFragmentsIts(float gapAreaMm2) const
  {
    indexed_triangle_set out;
    if (gapAreaMm2 <= 0.f)
      return out;
    std::vector<int> vertexMap(m_vertices.size(), -1);
    for (const Triangle &tr : m_triangles) {
      if (tr.is_split() || !tr.valid())
        continue;
      const Slic3r::Vec3f &a = m_vertices[size_t(tr.verts_idxs[0])].v;
      const Slic3r::Vec3f &b = m_vertices[size_t(tr.verts_idxs[1])].v;
      const Slic3r::Vec3f &c = m_vertices[size_t(tr.verts_idxs[2])].v;
      const float area = 0.5f * (b - a).cross(c - a).norm();
      if (area >= gapAreaMm2)
        continue;
      Slic3r::Vec3i32 tri;
      bool ok = true;
      for (int k = 0; k < 3; ++k) {
        const int old = tr.verts_idxs[k];
        if (old < 0 || size_t(old) >= m_vertices.size()) {
          ok = false;
          break;
        }
        if (vertexMap[size_t(old)] < 0) {
          vertexMap[size_t(old)] = int(out.vertices.size());
          out.vertices.push_back(m_vertices[size_t(old)].v);
        }
        tri[k] = vertexMap[size_t(old)];
      }
      if (ok)
        out.indices.push_back(tri);
    }
    return out;
  }
};

} // namespace

// Out-of-line destructor: the cache holds unique_ptr<TriangleSelector> over a
// type that is complete here (TriangleSelector.hpp is included by the header).
// Defining it here (not = default in the header) lets the compiler emit the
// deleter against the complete type in every TU that includes the header
// without HAS_LIBSLIC3R-defining members.
PaintEngine::~PaintEngine() = default;

Slic3r::TriangleSelector *PaintEngine::ensureSelector(
    int objectIndex, int volumeIndex,
    const std::shared_ptr<const Slic3r::TriangleMesh> &meshSharedPtr)
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it != m_cache.end())
    return it->second.selector.get(); // cache hit (TS-03): reuse the selector

  // nullptr mesh => no geometry for this (object, volume); skip caching so a
  // future reload is re-tried.
  if (!meshSharedPtr)
    return nullptr;

  // Build the TriangleSelector over the mesh. The ctor indexes the mesh
  // (TriangleSelector.hpp:299) -- the dominant cost, amortized over the
  // selector's lifetime by the cache (pitfall 7 mitigation).
  //
  // The shared_ptr is stashed on the CacheEntry so the TriangleMesh outlives
  // the selector (TriangleSelector.hpp:477 stores a const TriangleMesh& -- if
  // the TriangleMesh were destroyed first, the reference would dangle). This
  // is the TS-01 ownership contract bridged at the libslic3r->Qt boundary.
  auto entry = std::make_unique<CacheEntry>();
  entry->mesh = meshSharedPtr;
  // PaintSelector (PAINT-GAPFILL-PORT) subclasses the upstream selector
  // read-only to expose the leaf triangles for the gap-fragment reader; no
  // behavior override.
  entry->selector = std::make_unique<PaintSelector>(*meshSharedPtr);
  Slic3r::TriangleSelector *raw = entry->selector.get();
  m_cache.emplace(key, std::move(*entry));
  return raw;
}

Slic3r::TriangleSelector *PaintEngine::ensureSelector(int objectIndex,
                                                      int volumeIndex)
{
  // Lazily fetch the TriangleMesh from the Phase 120 accessor. nullptr => no
  // mesh for this (object, volume); ensureSelector returns nullptr.
  std::shared_ptr<const Slic3r::TriangleMesh> mesh =
      m_meshSource ? m_meshSource(objectIndex, volumeIndex) : nullptr;
  return ensureSelector(objectIndex, volumeIndex, mesh);
}

bool PaintEngine::paintAt(int objectIndex, int volumeIndex, int facetIdx,
                          const Slic3r::Vec3f &meshLocalHit,
                          float brushRadius, PaintCursorType cursor,
                          Slic3r::EnforcerBlockerType state,
                          const Slic3r::Transform3d &trafo,
                          const Slic3r::Vec3f &cameraPosMeshLocal,
                          float highlightByAngleDeg)
{
  Slic3r::TriangleSelector *selector = ensureSelector(objectIndex, volumeIndex);
  if (!selector)
    return false; // mesh source had no mesh for the pair

  // Upstream GLGizmoPainterBase supplies the camera source separately from the
  // surface hit so Circle cursor clipping follows the actual view direction.
  applyPaintToSelector(*selector, facetIdx, meshLocalHit, brushRadius, cursor,
                        state, trafo, cameraPosMeshLocal, highlightByAngleDeg);
  return true;
}

bool PaintEngine::smartFillAt(int objectIndex, int volumeIndex, int facetIdx,
                              const Slic3r::Vec3f &meshLocalHit,
                              float seedFillAngle, float highlightByAngleDeg,
                              Slic3r::EnforcerBlockerType state,
                              const Slic3r::Transform3d &trafo)
{
  Slic3r::TriangleSelector *selector = ensureSelector(objectIndex, volumeIndex);
  if (!selector)
    return false; // mesh source had no mesh for the pair

  applySmartFillToSelector(*selector, facetIdx, meshLocalHit, seedFillAngle,
                           highlightByAngleDeg, state, trafo);
  return true;
}

std::shared_ptr<indexed_triangle_set>
PaintEngine::getFacets(int objectIndex, int volumeIndex,
                       Slic3r::EnforcerBlockerType state)
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it == m_cache.end() || !it->second.selector)
    return nullptr;

  // Wrap the upstream get_facets (TriangleSelector.hpp:333) in a shared_ptr so
  // the caller (Phase 121 overlay renderer) can hold it without copying.
  // get_facets returns an ITS by value (move-constructed here).
  auto its = std::make_shared<indexed_triangle_set>(
      it->second.selector->get_facets(state));
  return its;
}

bool PaintEngine::hasFacets(int objectIndex, int volumeIndex,
                            Slic3r::EnforcerBlockerType state) const
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it == m_cache.end() || !it->second.selector)
    return false;
  return it->second.selector->has_facets(state);
}

// PAINT-GAPFILL-PORT: fragment leaves below the gap-area threshold. Reads the
// cached selector without rebuilding unrelated volumes; ensureSelector lazily
// builds for the requested pair only (same contract as getFacets' neighbors).
std::shared_ptr<indexed_triangle_set>
PaintEngine::getGapFragments(int objectIndex, int volumeIndex, float gapAreaMm2)
{
  Slic3r::TriangleSelector *selector = ensureSelector(objectIndex, volumeIndex);
  if (!selector)
    return nullptr;
  auto *paintSelector = static_cast<PaintSelector *>(selector);
  return std::make_shared<indexed_triangle_set>(
      paintSelector->gapFragmentsIts(gapAreaMm2));
}

const Slic3r::TriangleSelector *PaintEngine::cachedSelectorForVolume(
    int objectIndex, int volumeIndex) const
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it == m_cache.end() || !it->second.selector)
    return nullptr;
  return it->second.selector.get();
}

void PaintEngine::clearObject(int objectIndex)
{
  // Erase every (objectIndex, *) entry. std::map::erase with iterator-based
  // loop avoids the iterator-invalidation hazard of a range erase on a key
  // prefix (std::map has no prefix-erase API).
  for (auto it = m_cache.begin(); it != m_cache.end();) {
    if (it->first.first == objectIndex)
      it = m_cache.erase(it);
    else
      ++it;
  }
}

std::optional<Slic3r::TriangleSelector::TriangleSplittingData>
PaintEngine::serialize(int objectIndex, int volumeIndex) const
{
  const Key key = volumeKey(objectIndex, volumeIndex);
  auto it = m_cache.find(key);
  if (it == m_cache.end() || !it->second.selector)
    return std::nullopt;
  return it->second.selector->serialize();
}

bool PaintEngine::deserialize(
    int objectIndex, int volumeIndex,
    const Slic3r::TriangleSelector::TriangleSplittingData &data)
{
  Slic3r::TriangleSelector *selector = ensureSelector(objectIndex, volumeIndex);
  if (!selector)
    return false;
  // deserialize assumes the correct mesh is loaded (TriangleSelector.hpp:359).
  // The selector was built from the live mesh above, so this holds.
  selector->deserialize(data);
  return true;
}

// applyPaintToSelector -- pure helper (TS-08). Unit-testable without a Model.
//
// Builds a SinglePointCursor::Sphere or ::Circle via cursor_factory
// (TriangleSelector.hpp:114) and drives select_patch. The cursor is in
// mesh-local coords (center + camera source + trafo). select_patch subdivides
// triangles inside the cursor (triangle_splitting=true, the upstream painting
// default) and stamps them with `state`.
void applyPaintToSelector(Slic3r::TriangleSelector &selector,
                          int facetIdx,
                          const Slic3r::Vec3f &meshLocalHit,
                          float brushRadius, PaintCursorType cursor,
                          Slic3r::EnforcerBlockerType state,
                          const Slic3r::Transform3d &trafo,
                          const Slic3r::Vec3f &cameraPosMeshLocal,
                          float highlightByAngleDeg)
{
  // Map the Qt enum back to the upstream CursorType (TriangleSelector.hpp:52).
  const Slic3r::TriangleSelector::CursorType cursorType =
      (cursor == PaintCursorType::Sphere)
          ? Slic3r::TriangleSelector::CursorType::SPHERE
          : Slic3r::TriangleSelector::CursorType::CIRCLE;

  // Default ClippingPlane (inactive -- offset == FLT_MAX,
  // TriangleSelector.hpp:65). Phase 121+ may thread a real clipping plane
  // (cross-section-aware painting).
  const Slic3r::TriangleSelector::ClippingPlane clippingPlane;

  // cursor_factory (TriangleSelector.hpp:114) builds the SinglePointCursor
  // (Sphere or Circle) from the mesh-local center + camera source + radius +
  // trafo. The unique_ptr<Cursor> is moved into select_patch.
  auto cursorPtr = Slic3r::TriangleSelector::SinglePointCursor::cursor_factory(
      meshLocalHit, cameraPosMeshLocal, brushRadius, cursorType, trafo,
      clippingPlane);

  // select_patch (TriangleSelector.hpp:306-312): facet_start = the hit facet,
  // cursor = the brush, new_state = the paint state, trafo_no_translate =
  // mesh->world without translation, triangle_splitting = true (subdivide
  // inside the brush for a clean paint edge). highlight_by_angle_deg is the
  // Phase 240 (GIZ-02) overhang filter (upstream threads
  // m_paint_on_overhangs_only ? m_highlight_by_angle_threshold_deg : 0.f,
  // GLGizmoPainterBase.cpp:805).
  selector.select_patch(/*facet_start=*/facetIdx, std::move(cursorPtr),
                        /*new_state=*/state,
                        /*trafo_no_translate=*/trafo,
                        /*triangle_splitting=*/true,
                        /*highlight_by_angle_deg=*/highlightByAngleDeg);
}

// applySmartFillToSelector -- pure helper (Phase 240 GIZ-02). Unit-testable
// without a Model. Mirrors the upstream SMART_FILL click sequence
// (GLGizmoPainterBase.cpp:773-781): seed_fill_apply_on_triangles commits the
// region staged by the previous click, then seed_fill_select_triangles stages
// the current angle-bounded region (force_reselection=true supports re-click).
void applySmartFillToSelector(Slic3r::TriangleSelector &selector,
                              int facetIdx,
                              const Slic3r::Vec3f &meshLocalHit,
                              float seedFillAngle, float highlightByAngleDeg,
                              Slic3r::EnforcerBlockerType state,
                              const Slic3r::Transform3d &trafo)
{
  const Slic3r::TriangleSelector::ClippingPlane clippingPlane;
  // Upstream applies the region selected by the previous click before
  // selecting the region under the current click. The first click therefore
  // stages a region, while the second click paints the first and stages the
  // second (GLGizmoPainterBase.cpp SMART_FILL LeftDown ordering).
  selector.seed_fill_apply_on_triangles(/*new_state=*/state);
  selector.seed_fill_select_triangles(/*hit=*/meshLocalHit,
                                      /*facet_start=*/facetIdx,
                                      /*trafo_no_translate=*/trafo,
                                      /*clp=*/clippingPlane,
                                      /*seed_fill_angle=*/seedFillAngle,
                                      /*highlight_by_angle_deg=*/highlightByAngleDeg,
                                      /*force_reselection=*/true);
}

// buildPaintClippingPlane -- pure helper (PAINT-ALT-WHEEL-CLIP). Mirrors the
// upstream chain ObjectClipper::set_position_by_ratio
// (GLGizmosCommon.cpp:346-367 -- normal = -camera forward, world offset =
// normal.dot(center) + boundingRadius - ratio * 2 * boundingRadius) followed
// by get_clipping_plane_in_volume_coordinates (GLGizmoPainterBase.cpp:
// 1101-1117 -- the plane point transforms via the inverse, the normal via
// the transpose linear part, the volume-local offset is their dot). Returns
// the inactive plane (offset FLT_MAX) when the ratio is off so callers can
// branch on is_active().
Slic3r::TriangleSelector::ClippingPlane buildPaintClippingPlane(
    double ratio, const Slic3r::Vec3d &cameraForwardWorld,
    const Slic3r::Vec3d &objectCenterWorld, double boundingRadius,
    const Slic3r::Transform3d &worldTransform)
{
  if (ratio <= 0.0 || boundingRadius <= 0.0 ||
      cameraForwardWorld.norm() < 1e-12)
    return {};
  const Slic3r::Vec3d normal = (-cameraForwardWorld).normalized();
  const double dist = normal.dot(objectCenterWorld);
  const double offset = dist + boundingRadius - ratio * 2.0 * boundingRadius;
  const Slic3r::Vec3d pointOnPlane = normal * offset;
  const Slic3r::Vec3d pointTransformed = worldTransform.inverse() * pointOnPlane;
  const Slic3r::Transform3d trafoNormal(worldTransform.linear().transpose());
  const Slic3r::Vec3d normalTransformed = trafoNormal * normal;
  return Slic3r::TriangleSelector::ClippingPlane(
      {float(normalTransformed.x()), float(normalTransformed.y()),
       float(normalTransformed.z()),
       float(pointTransformed.dot(normalTransformed))});
}

} // namespace OWzx
#endif // HAS_LIBSLIC3R
