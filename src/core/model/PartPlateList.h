#pragma once

// PartPlateList — Qt6-native plate container (single source of truth for plate state).
//
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:559-937 (class PartPlateList).
// PURE DOMAIN object (no QObject) per v3.0 Phase 16 decision D-01/D-05.
//
// EXCLUDED (deferred per CONTEXT D-02 / Phase 16 scope):
//   - m_print_list / m_gcode_result_list (per-plate Print + GCode result maps → Phase 19)
//   - GLTexture members, wxGetApp, std::mutex (Qt single-threaded GUI for plate mgmt)
//   - Bed geometry (m_plate_width/depth/height, m_shape, m_extruder_areas → list-level config, future)
//
// This container owns the PartPlate objects and is the single source of truth that
// ProjectServiceMock re-backs its plate Q_PROPERTY/Q_INVOKABLE API onto (D-05 big-bang).

#include <cmath>
#include <memory>
#include <vector>

#include <QList>  // for objectIndicesOnPlate bridge query (matches downstream Qt6 API shape)

#include "PartPlate.h"

namespace OWzx {

/// Maximum number of plates, mirroring upstream MAX_PLATE_COUNT (PartPlate.hpp:36).
inline constexpr int kMaxPlateCount = 36;

/// Compute the grid column count for a given plate count.
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:38-50.
// cols = ceil(sqrt(count)) implemented via float comparison (NOT integer ceil).
// Kept snake_case to mirror upstream exactly (it is NOT a member).
inline int compute_colum_count(int count) {
  float value = sqrt((float)count);
  float round_value = round(value);
  int cols;
  if (value > round_value)
    cols = round_value + 1;
  else
    cols = round_value;
  return cols;
}

// Geometry mirrors upstream PartPlate.cpp:3905-3964,4836-4870,5365-5376 (PartPlate.hpp:38-50).
class PartPlateList {
 public:
  PartPlateList();
  ~PartPlateList() = default;

  // Non-copyable (owns unique_ptrs); the owner (ProjectServiceMock) holds it by value/unique_ptr.
  PartPlateList(const PartPlateList&) = delete;
  PartPlateList& operator=(const PartPlateList&) = delete;

  // ── Counts & access ────────────────────────────────────────────────────
  int plateCount() const { return static_cast<int>(m_plate_list.size()); }

  PartPlate* plate(int index);
  const PartPlate* plate(int index) const;

  PartPlate* currentPlate() { return plate(m_current_plate); }
  const PartPlate* currentPlate() const { return plate(m_current_plate); }

  int currentPlateIndex() const { return m_current_plate; }
  void setCurrentPlateIndex(int index);

  // ── Plate-grid geometry (v3.0 Phase 16 deferred, v3.2 Phase 29 lands it) ──
  // Mirrors upstream PartPlate.cpp:3905-3964,4836-4870,5365-5376.
  int plateCols() const { return m_plate_cols; }
  int plateWidth() const { return m_plate_width; }
  int plateDepth() const { return m_plate_depth; }
  double plateStrideX() const;  // size * (1 + LOGICAL_PART_PLATE_GAP)
  double plateStrideY() const;

#ifdef HAS_LIBSLIC3R
  /// 2D shape position for plate index in a grid of `cols` columns.
  /// y goes NEGATIVE for rows below the first (PartPlate.cpp:3952-3964).
  Slic3r::Vec2d computeShapePosition(int index, int cols) const;
  /// 3D world origin for plate index in a grid of `cols` columns (z=0).
  Slic3r::Vec3d computeOrigin(int index, int cols) const;
#endif

  /// Decode the plate index from a world-space (mm) translation.
  /// Pure-double API so PartPlateList stays free of ArrangePolygon (libslic3r).
  /// SIGN-FLIP decode: row_value = (stride_y - translation_y) / stride_y
  /// (PartPlate.cpp:5365-5376).
  int computePlateIndex(double translationX_mm, double translationY_mm) const;

  /// Sets plate width/depth/height (mm) and refreshes origins. Test seam.
  void setPlateSize(int width, int depth, int height);

  // ── Lifecycle (re-backing targets for ProjectServiceMock, PLATE-06) ────
  /// Creates a new plate with an auto-incremented index. Returns nullptr if
  /// kMaxPlateCount would be exceeded (mirrors upstream create_plate guard).
  PartPlate* createPlate();

  /// Removes the plate at index; reindexes the survivors' m_plate_index;
  /// refuses if it would leave zero plates (mirrors upstream "keep >= 1").
  /// Returns false if index is invalid or it's the last plate.
  bool deletePlate(int index);

  bool renamePlate(int index, const std::string& name);

  void setPlateLocked(int index, bool locked);

  // ── v3.0 Phase 17: lifecycle completion ────────────────────────────────
  /// Reorders plate oldIndex to newIndex (pure metadata shift + reindex).
  /// Adjusts m_current_plate if it was in the shifted range. Returns false on
  /// invalid indices or old==new. (D-07; upstream move_plate_to_index sans geometry.)
  bool movePlate(int oldIndex, int newIndex);

  /// Sets the per-plate printable flag (D-08). No-op if index invalid.
  void setPlatePrintable(int index, bool printable);

  // ── Derived queries (bridge instance-level truth to per-object API) ────
  /// Returns the index of the first plate whose membership set contains objIdx,
  /// or -1 if no plate holds the object.
  int plateIndexForObject(int objIdx) const;

  /// Distinct object indices on a plate (collapses instance pairs to object indices).
  /// Bridges the instance-level truth to the existing per-object API surface that
  /// ProjectServiceMock exposes (plateObjectIndices).
  QList<int> objectIndicesOnPlate(int plateIndex) const;

  // ── Reset (used by 3MF load path before rebuilding from PlateData) ─────
  /// Removes all plates except the first (keeps >= 1 invariant). Resets current to 0.
  void resetToSinglePlate();

 private:
  /// Owns the plates (mirrors upstream m_plate_list ownership).
  std::vector<std::unique_ptr<PartPlate>> m_plate_list;
  int m_current_plate = 0;

  /// Plate-grid geometry (mirrors upstream PartPlate.hpp:569-576).
  int m_plate_count = 0;
  int m_plate_cols = 0;
  int m_plate_width = 0;
  int m_plate_depth = 0;
  int m_plate_height = 0;

  /// Refresh m_plate_count + m_plate_cols from the list size (PartPlate.cpp:4862-4870).
  void updatePlateCols();

  /// Write the computed origin to every plate (PartPlate.cpp:4872-4892 core loop).
  void updatePlateOrigins();

  /// Reindexes m_plate_index on every plate to match its vector position.
  /// Called after any structural change (create/delete/reorder).
  void reindex();
};

}  // namespace OWzx
