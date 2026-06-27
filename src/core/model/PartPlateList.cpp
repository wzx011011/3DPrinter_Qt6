// PartPlateList implementation.
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:559-937 (PartPlateList),
// PartPlate.cpp:4407 (create_plate), :4554 (delete_plate).

#include "PartPlateList.h"

#include <QSet>  // for distinct-object collapse in objectIndicesOnPlate

namespace OWzx {

// Source truth: PartPlate.cpp:55. File-private; not exported.
namespace {
constexpr double LOGICAL_PART_PLATE_GAP = 1.0 / 5.0;
}  // namespace

PartPlateList::PartPlateList() {
  // Upstream starts with a single default plate; Qt6 preserves the same invariant
  // (a project always has >= 1 plate). createPlate handles index assignment.
  createPlate();
  m_current_plate = 0;
}

PartPlate* PartPlateList::plate(int index) {
  if (index < 0 || index >= plateCount()) return nullptr;
  return m_plate_list[index].get();
}

const PartPlate* PartPlateList::plate(int index) const {
  if (index < 0 || index >= plateCount()) return nullptr;
  return m_plate_list[index].get();
}

void PartPlateList::setCurrentPlateIndex(int index) {
  if (index < 0 || index >= plateCount()) return;
  m_current_plate = index;
}

PartPlate* PartPlateList::createPlate() {
  // Upstream create_plate guards against exceeding MAX_PLATE_COUNT.
  if (plateCount() >= kMaxPlateCount) return nullptr;
  auto p = std::make_unique<PartPlate>(plateCount());
  PartPlate* raw = p.get();
  m_plate_list.push_back(std::move(p));
  // Keep grid geometry consistent with the new count (Phase 29 D-29-4).
  updatePlateCols();
  updatePlateOrigins();
  return raw;
}

bool PartPlateList::deletePlate(int index) {
  if (plateCount() <= 1) return false;  // upstream invariant: keep >= 1 plate
  if (index < 0 || index >= plateCount()) return false;
  m_plate_list.erase(m_plate_list.begin() + index);
  reindex();
  // Keep current plate index valid after deletion.
  if (m_current_plate >= plateCount()) {
    m_current_plate = plateCount() - 1;
  }
  // Keep grid geometry consistent with the new count (Phase 29 D-29-4).
  updatePlateCols();
  updatePlateOrigins();
  return true;
}

bool PartPlateList::renamePlate(int index, const std::string& name) {
  PartPlate* p = plate(index);
  if (!p) return false;
  p->setName(name);
  return true;
}

void PartPlateList::setPlateLocked(int index, bool locked) {
  PartPlate* p = plate(index);
  if (p) p->setLocked(locked);
}

bool PartPlateList::movePlate(int oldIndex, int newIndex) {
  // Geometry recompute via updatePlateCols() + updatePlateOrigins()
  // (Phase 29 D-29-14, closes Phase 17 D-07 deferral).
  if (oldIndex < 0 || oldIndex >= plateCount()) return false;
  if (newIndex < 0 || newIndex >= plateCount()) return false;
  if (oldIndex == newIndex) return false;

  // Extract the plate, shift the in-between elements, reinsert at newIndex.
  std::unique_ptr<PartPlate> moved = std::move(m_plate_list[oldIndex]);
  if (oldIndex < newIndex) {
    for (int i = oldIndex; i < newIndex; ++i)
      m_plate_list[i] = std::move(m_plate_list[i + 1]);
  } else {
    for (int i = oldIndex; i > newIndex; --i)
      m_plate_list[i] = std::move(m_plate_list[i - 1]);
  }
  m_plate_list[newIndex] = std::move(moved);
  reindex();
  // Recompute grid geometry for the new plate order (Phase 29 D-29-14).
  updatePlateCols();
  updatePlateOrigins();

  // Track the current plate through the shift.
  if (m_current_plate == oldIndex) {
    m_current_plate = newIndex;
  } else if (oldIndex < newIndex) {
    // shift was [old+1..new] -> [old..new-1]; current in (old, new] decrements
    if (m_current_plate > oldIndex && m_current_plate <= newIndex)
      --m_current_plate;
  } else {  // oldIndex > newIndex
    // shift was [new..old-1] -> [new+1..old]; current in [new, old) increments
    if (m_current_plate >= newIndex && m_current_plate < oldIndex)
      ++m_current_plate;
  }
  return true;
}

void PartPlateList::setPlatePrintable(int index, bool printable) {
  PartPlate* p = plate(index);
  if (p) p->setPrintable(printable);
}

int PartPlateList::plateIndexForObject(int objIdx) const {
  for (int i = 0; i < plateCount(); ++i) {
    if (m_plate_list[i]->hasObject(objIdx)) return i;
  }
  return -1;
}

QList<int> PartPlateList::objectIndicesOnPlate(int plateIndex) const {
  QList<int> result;
  const PartPlate* p = plate(plateIndex);
  if (!p) return result;
  // Collapse instance pairs to distinct object indices, preserving first-seen order.
  QSet<int> seen;
  for (const auto& pair : p->objToInstanceSet()) {
    if (!seen.contains(pair.first)) {
      seen.insert(pair.first);
      result.append(pair.first);
    }
  }
  return result;
}

void PartPlateList::resetToSinglePlate() {
  // Keep the invariant: always >= 1 plate. Used by the 3MF load path.
  if (m_plate_list.empty()) {
    createPlate();
    m_current_plate = 0;
    return;
  }
  // Erase all but the first, then clear its state for a fresh load.
  m_plate_list.erase(m_plate_list.begin() + 1, m_plate_list.end());
  PartPlate* first = m_plate_list[0].get();
  first->clearInstances();
  first->setName({});
  first->setPlateIndex(0);
  first->setLocked(false);
  m_current_plate = 0;
}

void PartPlateList::reindex() {
  for (int i = 0; i < plateCount(); ++i) {
    m_plate_list[i]->setPlateIndex(i);
  }
}

// ── Plate-grid geometry (v3.2 Phase 29) ────────────────────────────────────
// Mirrors upstream PartPlate.cpp:4836-4870 (stride + update_plate_cols),
// :3905-3964 (compute_shape_position + compute_origin),
// :4872-4892 (update_all_plates_pos_and_size core loop),
// :5365-5376 (compute_plate_index).

void PartPlateList::setPlateSize(int width, int depth, int height) {
  m_plate_width = width;
  m_plate_depth = depth;
  m_plate_height = height;
  updatePlateOrigins();
}

double PartPlateList::plateStrideX() const {
  return m_plate_width * (1.0 + LOGICAL_PART_PLATE_GAP);  // PartPlate.cpp:4841
}

double PartPlateList::plateStrideY() const {
  return m_plate_depth * (1.0 + LOGICAL_PART_PLATE_GAP);  // PartPlate.cpp:4849
}

#ifdef HAS_LIBSLIC3R
Slic3r::Vec2d PartPlateList::computeShapePosition(int index, int cols) const {
  int row = index / cols;
  int col = index % cols;
  Slic3r::Vec2d pos;
  pos.x() = col * plateStrideX();   // +X to the right
  pos.y() = -row * plateStrideY();  // NEGATIVE Y downward (PartPlate.cpp:3960-3961)
  return pos;
}

Slic3r::Vec3d PartPlateList::computeOrigin(int index, int cols) const {
  Slic3r::Vec2d pos = computeShapePosition(index, cols);
  return Slic3r::Vec3d(pos.x(), pos.y(), 0.0);
}
#endif

int PartPlateList::computePlateIndex(double translationX_mm, double translationY_mm) const {
  // Decodes the negative-Y-row encoding of computeShapePosition (PartPlate.cpp:5365-5376).
  //
  // Source truth (PartPlate.cpp:5369-5370) operates on the ArrangePolygon
  // translation space, which is already shifted by one stride relative to the
  // raw world offset (the row decode `(stride_y - t_y)/stride_y` reflects that
  // shifted space). Qt6 reads the raw world offset from ModelInstance::get_offset()
  // directly, so the correct decode is the unshifted inverse of computeShapePosition:
  //   col_value = world_x / stride_x        (computeShapePosition used  col*stride_x)
  //   row_value = -world_y / stride_y       (computeShapePosition used -row*stride_y)
  // This keeps plate 0 (world origin) decoding to row 0 / col 0.
  const double strideX = plateStrideX();
  const double strideY = plateStrideY();
  // Defensive: if plate size was never set (stride 0), everything is on plate 0.
  if (strideX <= 0.0 || strideY <= 0.0) return 0;
  float col_value = static_cast<float>(translationX_mm / strideX);
  float row_value = static_cast<float>(-translationY_mm / strideY);
  int row = static_cast<int>(std::round(row_value));
  int col = static_cast<int>(std::round(col_value));
  return row * m_plate_cols + col;
}

void PartPlateList::updatePlateCols() {
  // PartPlate.cpp:4863-4870 (BOOST_LOG dropped).
  m_plate_count = static_cast<int>(m_plate_list.size());
  m_plate_cols = compute_colum_count(m_plate_count);
}

void PartPlateList::updatePlateOrigins() {
  // Mirrors update_all_plates_pos_and_size core loop (PartPlate.cpp:4872-4892);
  // wipe-tower and unprintable branches stripped (out of Phase 29 scope).
#ifdef HAS_LIBSLIC3R
  for (int i = 0; i < plateCount(); ++i) {
    PartPlate* p = plate(i);
    if (p) p->setOrigin(computeOrigin(i, m_plate_cols));
  }
#else
  // Non-HAS_LIBSLIC3R fallback: write via the 3-double setOrigin overload.
  for (int i = 0; i < plateCount(); ++i) {
    PartPlate* p = plate(i);
    if (!p) continue;
    const int row = (m_plate_cols > 0) ? i / m_plate_cols : 0;
    const int col = (m_plate_cols > 0) ? i % m_plate_cols : 0;
    p->setOrigin(col * plateStrideX(), -row * plateStrideY(), 0.0);
  }
#endif
}

void PartPlateList::rebuildPlatesAfterArrangement(bool exceptLocked, bool recyclePlates) {
#ifdef HAS_LIBSLIC3R
  // Mirrors upstream rebuild_plates_after_arrangement (PartPlate.cpp:6096-6139),
  // but decodes plate index from each instance's world translation via
  // computePlateIndex instead of bbox intersection (D-29-5/D-29-7), because
  // ModelArrange.cpp:98 resets bed_idx to 0 so the per-bed split must be
  // reconstructed from the arranged translations.
  if (!m_model) return;

  // 1. CLEAR non-locked memberships (preserve locked when exceptLocked=true).
  //    Mirrors upstream clear(false,false,except_locked,...) at PartPlate.cpp:5261.
  for (int i = 0; i < plateCount(); ++i) {
    PartPlate* p = plate(i);
    if (!p) continue;
    if (!(exceptLocked && p->isLocked())) p->clearInstances();
  }

  // 2. RE-DISTRIBUTE via computePlateIndex on each instance's world offset.
  //    Upstream sorts by arrange_order before reload (PartPlate.cpp:6103); Qt6
  //    decodes plate index from each instance's translation directly, so
  //    ordering is irrelevant (D-29-5 divergence).
  for (int objIdx = 0; objIdx < int(m_model->objects.size()); ++objIdx) {
    const auto& obj = m_model->objects[size_t(objIdx)];
    if (!obj) continue;
    for (int instIdx = 0; instIdx < int(obj->instances.size()); ++instIdx) {
      const auto& inst = obj->instances[size_t(instIdx)];
      if (!inst) continue;
      const Slic3r::Vec3d offset = inst->get_offset();  // world translation in mm
      int plateIdx = computePlateIndex(offset.x(), offset.y());
      if (plateIdx < 0) plateIdx = 0;  // clamp negative (objects left/above grid)
      // Create plates up to plateIdx if needed (up to kMaxPlateCount).
      while (plateCount() <= plateIdx && plateCount() < kMaxPlateCount) {
        createPlate();  // createPlate keeps cols/origins consistent
      }
      if (plateIdx < plateCount()) plate(plateIdx)->addInstance(objIdx, instIdx);
    }
  }

  // 3. RECYCLE trailing empty non-locked plates (recyclePlates=true).
  //    Mirrors PartPlate.cpp:6111-6127 EXACTLY: reverse iteration, stops at
  //    i==1 (plate 0 NEVER deleted), `break` on first non-empty/non-locked so
  //    only a CONTIGUOUS trailing run is recycled. Locked plates never deleted.
  if (recyclePlates) {
    for (int i = plateCount() - 1; i > 0; --i) {
      PartPlate* p = plate(i);
      if (!p) continue;
      if (p->empty()) {
        deletePlate(i);  // deletePlate guards keep>=1; Plan 01 wired reindex+update
      } else if (p->isLocked()) {
        continue;
      } else {
        break;  // contiguous trailing run only
      }
    }
  }

  // 4. Refresh origins for all surviving plates' new grid positions.
  updatePlateCols();
  updatePlateOrigins();
#else
  (void)exceptLocked;
  (void)recyclePlates;
#endif
}

}  // namespace OWzx
