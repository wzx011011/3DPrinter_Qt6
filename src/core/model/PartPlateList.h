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

#include <memory>
#include <vector>

#include <QList>  // for objectIndicesOnPlate bridge query (matches downstream Qt6 API shape)

#include "PartPlate.h"

namespace OWzx {

/// Maximum number of plates, mirroring upstream MAX_PLATE_COUNT (PartPlate.hpp:36).
inline constexpr int kMaxPlateCount = 36;

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

  /// Reindexes m_plate_index on every plate to match its vector position.
  /// Called after any structural change (create/delete/reorder).
  void reindex();
};

}  // namespace OWzx
