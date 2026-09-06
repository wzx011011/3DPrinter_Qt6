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
#include <functional>
#include <memory>
#include <vector>

#include <QList>  // for objectIndicesOnPlate bridge query (matches downstream Qt6 API shape)

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>  // for setModel backref used by rebuildPlatesAfterArrangement
#endif

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

  /// Public maintenance hook (v3.2 Phase 29): recompute every plate's origin
  /// from its current grid index. Called automatically by createPlate/
  /// deletePlate/movePlate/setPlateSize/rebuildPlatesAfterArrangement, but
  /// exposed so callers that reconstruct the plate list out-of-band (e.g. the
  /// 3MF load path) can refresh origins defensively.
  void refreshPlateOrigins() { updatePlateOrigins(); }

  // ── Multi-plate arrangement rebuild (v3.2 Phase 29, ARRANGE-02/03) ──────
  // rebuildPlatesAfterArrangement mirrors upstream rebuild_plates_after_arrangement
  // (PartPlate.cpp:6096-6139), but decodes plate index from each instance's
  // world translation via computePlateIndex instead of bbox intersection
  // (D-29-5/D-29-7) because ModelArrange.cpp:98 resets bed_idx to 0.
  //
  // exceptLocked: when true, locked plates' instance memberships are preserved
  //   (cleared only on non-locked plates) — ARRANGE-03 locked exclusion.
  // recyclePlates: when true, trailing empty non-locked plates are deleted
  //   (upstream recycle_plates default). Plate 0 and locked plates are never
  //   deleted. Requires setModel() to have been called.
  void rebuildPlatesAfterArrangement(bool exceptLocked, bool recyclePlates);

#ifdef HAS_LIBSLIC3R
  /// Sets the libslic3r Model backref used by rebuildPlatesAfterArrangement to
  /// enumerate model_->objects[*].instances[*] world translations.
  void setModel(Slic3r::Model* model) { m_model = model; }
#endif

  // ── Lifecycle (re-backing targets for ProjectServiceMock, PLATE-06) ────
  /// Creates a new plate with an auto-incremented index and stable print identity.
  /// Returns nullptr if kMaxPlateCount would be exceeded (mirrors upstream
  /// create_plate guard). The print identity is never reused or changed by
  /// reorder/delete operations, matching upstream m_print_index semantics.
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

  /// Returns the plate containing an instance, or -1 when it is not assigned.
  int findInstance(int objectIndex, int instanceIndex) const;
  /// Returns the plate that fully contains an instance, or -1 when it is outside.
  int findInstanceBelongs(int objectIndex, int instanceIndex) const;
  /// Upstream naming is retained: true when at least one plate can be sliced.
  bool isAllPlatesReadyForSlice() const;

  // ── Print-volume geometry (upstream PartPlateList contains/intersects/
  //    find_instance overloads) ─────────────────────────────────────────────
#ifdef HAS_LIBSLIC3R
  /// Provider of an instance's transformed world-space bounding box. Upstream
  /// derives these from m_model inside notify_instance_update
  /// (PartPlate.cpp:4198-4250, ModelObject::instance_bounding_box); Qt6
  /// injects the source so PartPlateList stays free of the Model type.
  using InstanceBoundsFn =
      std::function<Slic3r::BoundingBoxf3(int objectIndex, int instanceIndex)>;
  void setInstanceBoundsFn(InstanceBoundsFn fn) {
    m_instance_bounds_fn = std::move(fn);
  }

  /// Result of the geometric cross-plate point lookup.
  struct InstanceHit {
    int plateIndex = -1;
    int objectIndex = -1;
    int instanceIndex = -1;
  };

  /// Upstream PartPlateList::find_instance(BoundingBoxf3&) (PartPlate.cpp:
  /// 4131-4149): the first plate whose print volume intersects the box,
  /// -1 when none does. Note the upstream asymmetry: the list-level overload
  /// INTERSECTS, while per-plate contains() is full containment
  /// (PartPlate.cpp:4130 "only judges whether it is intersect with plate").
  int findInstance(const Slic3r::BoundingBoxf3& boundingBox) const;

  /// Cross-plate geometric point lookup with the upstream find_instance loop
  /// structure (PartPlate.cpp:4110-4129: first plate wins): the first
  /// PRINTABLE plate holding a member instance whose transformed bounds
  /// contain the point. Unprintable plates never match: upstream keeps the
  /// unprintable shared plate OUT of m_plate_list (PartPlate.hpp:541) so
  /// find_instance cannot return it; Qt6 models that plate as the plate-level
  /// printable flag and filters it here. Requires setInstanceBoundsFn.
  InstanceHit findInstanceAt(const Slic3r::Vec3d& point) const;

  /// Upstream PartPlateList::contains(const BoundingBoxf3&) (PartPlate.cpp:
  /// 3956-3964): true when ANY plate fully contains the box.
  bool contains(const Slic3r::BoundingBoxf3& boundingBox) const;

  /// Upstream PartPlateList::intersects(const BoundingBoxf3&) (PartPlate.cpp:
  /// 3948-3954): true when ANY plate's print volume overlaps the box.
  bool intersects(const Slic3r::BoundingBoxf3& boundingBox) const;
#endif

  // -- All-plates print/export readiness aggregates (B3) --------------------
  // Upstream PartPlate.cpp:4989-5044; consumed by the MainFrame.cpp:1372-1392
  // style print/export gates. Loop structure mirrors upstream exactly.
  /// Every plate's slice result is valid (PartPlate.cpp:4989).
  bool isAllSliceResultsValid() const;
  /// Every non-empty printable plate must be print-ready and at least one
  /// plate ready (PartPlate.cpp:5000).
  bool isAllSliceResultsReadyForPrint() const;
  /// Print variant plus printable instances on every ready plate
  /// (PartPlate.cpp:5022).
  bool isAllSliceResultReadyForExport() const;

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
  /// Next upstream-style print identity. It is monotonic for this list and
  /// intentionally survives plate deletion so identities are never reused.
  int m_next_print_index = 0;

  /// Plate-grid geometry (mirrors upstream PartPlate.hpp:569-576).
  int m_plate_count = 0;
  int m_plate_cols = 0;
  int m_plate_width = 0;
  int m_plate_depth = 0;
  int m_plate_height = 0;

#ifdef HAS_LIBSLIC3R
  /// libslic3r Model backref for rebuildPlatesAfterArrangement (set via setModel).
  Slic3r::Model* m_model = nullptr;

  /// Injected instance world-bounds source for findInstanceAt (see above).
  InstanceBoundsFn m_instance_bounds_fn;
#endif

  /// Refresh m_plate_count + m_plate_cols from the list size (PartPlate.cpp:4862-4870).
  void updatePlateCols();

  /// Write the computed origin to every plate (PartPlate.cpp:4872-4892 core loop).
  void updatePlateOrigins();

  /// Reindexes m_plate_index on every plate to match its vector position.
  /// Called after any structural change (create/delete/reorder).
  void reindex();
};

}  // namespace OWzx
