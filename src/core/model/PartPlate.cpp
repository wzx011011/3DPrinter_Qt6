// PartPlate implementation.
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:77-557.
// Most accessors are inline in the header (pure value object); only the
// derived membership query, the legacy filament-map migration predicate and
// the print-volume geometry need a definition file.

#include "PartPlate.h"

#include <cmath>

namespace OWzx {

namespace {
// Upstream BuildVolume::BedEpsilon (GUI/BuildVolume.hpp:90) = 3 * EPSILON
// (libslic3r.h:52, EPSILON = 1e-4). Mirrored locally because BuildVolume.hpp
// is a GUI-layer header the core model must not include.
constexpr double kBedEpsilon = 3.0 * 1e-4;
}  // namespace

bool PartPlate::hasObject(int objIdx) const {
  // An object is on this plate if ANY of its instances is in the membership set.
  // The set is keyed by (objectIndex, instanceIndex) pairs.
  for (const auto& pair : m_obj_to_instance_set) {
    if (pair.first == objIdx) {
      return true;
    }
  }
  return false;
}

#ifdef HAS_LIBSLIC3R

const Slic3r::BoundingBoxf3& PartPlate::boundingBox() const {
  // Lazy rebuild mirrors upstream calc_bounding_boxes (PartPlate.cpp:349-356):
  // the box merges the shape points at z=0. Without a custom shape the plate
  // prints on the default rectangle centered on the plate origin (upstream
  // set_shape translates the centered bed shape by the plate grid position,
  // PartPlate.cpp:2609-2611, so the origin is the plate center).
  if (m_bounding_box_dirty) {
    m_bounding_box = Slic3r::BoundingBoxf3();
    if (!m_shape.empty()) {
      for (const Slic3r::Vec2d& p : m_shape) {
        m_bounding_box.merge(Slic3r::Vec3d(p(0), p(1), 0.0));
      }
    } else {
      const double hw = 0.5 * static_cast<double>(m_width);
      const double hd = 0.5 * static_cast<double>(m_depth);
      m_bounding_box.merge(m_origin + Slic3r::Vec3d(-hw, -hd, 0.0));
      m_bounding_box.merge(m_origin + Slic3r::Vec3d(hw, -hd, 0.0));
      m_bounding_box.merge(m_origin + Slic3r::Vec3d(hw, hd, 0.0));
      m_bounding_box.merge(m_origin + Slic3r::Vec3d(-hw, hd, 0.0));
    }
    m_bounding_box_dirty = false;
  }
  return m_bounding_box;
}

bool PartPlate::contains(const Slic3r::BoundingBoxf3& box) const {
  // Upstream PartPlate::contains(const BoundingBoxf3&) (PartPlate.cpp:2704-
  // 2714): full containment in the print volume; objects may protrude below
  // the bed (min z -> -1e10) up to +1e3, and the footprint is widened by
  // BedEpsilon on X/Y.
  Slic3r::BoundingBoxf3 print_volume(
      Slic3r::Vec3d(boundingBox().min(0), boundingBox().min(1), 0.0),
      Slic3r::Vec3d(boundingBox().max(0), boundingBox().max(1), 1e3));
  print_volume.min(2) = -1e10;
  print_volume.min(0) -= kBedEpsilon;
  print_volume.min(1) -= kBedEpsilon;
  print_volume.max(0) += kBedEpsilon;
  print_volume.max(1) += kBedEpsilon;
  return print_volume.contains(box);
}

bool PartPlate::intersects(const Slic3r::BoundingBoxf3& box) const {
  // Upstream PartPlate::intersects(const BoundingBoxf3&) (PartPlate.cpp:2716-
  // 2725): the same print volume with an overlap test.
  Slic3r::BoundingBoxf3 print_volume(
      Slic3r::Vec3d(boundingBox().min(0), boundingBox().min(1), 0.0),
      Slic3r::Vec3d(boundingBox().max(0), boundingBox().max(1), 1e3));
  print_volume.min(2) = -1e10;
  print_volume.min(0) -= kBedEpsilon;
  print_volume.min(1) -= kBedEpsilon;
  print_volume.max(0) += kBedEpsilon;
  print_volume.max(1) += kBedEpsilon;
  return print_volume.intersects(box);
}

std::pair<int, int> PartPlate::findInstance(
    const Slic3r::Vec3d& point,
    const std::function<Slic3r::BoundingBoxf3(int objectIndex,
                                              int instanceIndex)>& instanceBounds)
    const {
  if (!instanceBounds) return {-1, -1};
  for (const auto& key : m_obj_to_instance_set) {
    const Slic3r::BoundingBoxf3 box = instanceBounds(key.first, key.second);
    if (box.defined && box.contains(point)) return key;
  }
  return {-1, -1};
}

#endif  // HAS_LIBSLIC3R

FilamentMapMode migrateLegacyFilamentMapMode(int legacyRawInt) {
  // Phase 111 (FMAP-04 / Phase 107 REVIEW R-01): the FM-03 legacy raw-int ->
  // widened-enum migration predicate. Factored out of ProjectServiceMock's two
  // read sites (loadFile ~646, loadProject ~5526) so the legacy branch is unit-
  // testable in isolation -- R-01 observed the round-trip test always took the
  // trusted coEnum branch (the write side produces typed values), so the legacy
  // discriminator (the actual headline FMAP-02 fix) had NO runtime coverage.
  //
  // Pre-v4.5 Qt6 files wrote filament_map_mode as a raw int (0=Auto, 1=Manual)
  // via the old 2-value enum. After the Phase 107 widening, raw 1 MUST map to
  // fmmManual (2) -- NOT the new fmmAutoForMatch (1) -- so a pre-v4.5 "Manual"
  // plate stays Manual after reload. See PartPlate.h for the contract.
  if (legacyRawInt == 0)
    return FilamentMapMode::fmmAutoForFlush;
  if (legacyRawInt == 1)
    return FilamentMapMode::fmmManual;
  return FilamentMapMode::fmmDefault;
}

}  // namespace OWzx
