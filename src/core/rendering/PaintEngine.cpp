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
  entry->selector = std::make_unique<Slic3r::TriangleSelector>(*meshSharedPtr);
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
                          const Slic3r::Transform3d &trafo)
{
  Slic3r::TriangleSelector *selector = ensureSelector(objectIndex, volumeIndex);
  if (!selector)
    return false; // mesh source had no mesh for the pair

  // Default camera source = the hit point itself (Sphere/Circle containment
  // does not depend on a real camera position for the TS-08 unit-test path).
  // Phase 121 (brush UI) will thread the real camera position through here.
  applyPaintToSelector(*selector, facetIdx, meshLocalHit, brushRadius, cursor,
                        state, trafo, /*cameraPosMeshLocal=*/meshLocalHit);
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
                          const Slic3r::Vec3f &cameraPosMeshLocal)
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
  // inside the brush for a clean paint edge).
  selector.select_patch(/*facet_start=*/facetIdx, std::move(cursorPtr),
                        /*new_state=*/state,
                        /*trafo_no_translate=*/trafo,
                        /*triangle_splitting=*/true);
}

} // namespace OWzx
#endif // HAS_LIBSLIC3R
