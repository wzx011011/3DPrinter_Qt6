#pragma once

// PartPlate — Qt6-native plate domain value object.
//
// Source truth: third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp:77-557 (class PartPlate).
// This is a PURE DOMAIN object (no QObject, no Qt signals) per v3.0 Phase 16 decision D-01/D-02.
//
// Field boundary (D-02): only the data/lifecycle/IO subset of upstream is mirrored.
// EXCLUDED (irrelevant to Qt6 — Qt6 has its own QML/QtQuick renderer):
//   - All GL rendering (GLModel, PickingModel, GLTexture, m_quadric, m_hover_id)
//   - All wxWidgets (wxCoord, wxGetApp)
//   - Cereal serialization (Qt6 does its own persistence)
//   - Bed geometry fields that are global to PartPlateList (m_shape, m_extruder_areas)
//     — these belong on the list container, deferred from Phase 16 scope.
//
// Membership granularity (D-03): instance-level std::set<std::pair<int,int>>
//   mirroring upstream obj_to_instance_set (PartPlate.hpp:93).
//   Can represent "some instances of one object on plate A, others on plate B".
//
// Per-plate config (D-04): native Slic3r::DynamicPrintConfig m_config under HAS_LIBSLIC3R
//   (PartPlate.hpp:159), NOT QHash<QString,QVariant>.

#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <QImage>  // v3.2 Phase 30: cached plate thumbnail (Qt-native).

#ifdef HAS_LIBSLIC3R
#include <libslic3r/BoundingBox.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/Point.hpp>
#endif

namespace OWzx {

/// Layer-sequence entry (aligns with upstream LayerPrintSequence + Qt6 MockLayerSeqEntry).
/// Moved here from ProjectServiceMock so plate state lives with the domain model.
struct LayerSeqEntry {
  int beginLayer = 2;
  int endLayer = 100;
  std::vector<int> extruderOrder;
};

/// Bed type (aligns with upstream BedType in bbs_3mf.hpp / PartPlate.cpp).
/// 0=Default, 1=PEI(smooth), 2=PEI(hightemp), 3=PTE, 4=PC, 5=EP, 6=ER, 7=Custom
enum class PlateBedType : int {
  Default = 0,
  PeiSmooth = 1,
  PeiHighTemp = 2,
  Pte = 3,
  Pc = 4,
  Ep = 5,
  Er = 6,
  Custom = 7
};

/// Print sequence (aligns with upstream PrintSequence).
enum class PlatePrintSequence : int {
  ByDefault = 0,
  ByLayer = 1,
  ByObject = 2
};

/// Spiral vase mode (aligns with upstream has_spiral_mode_config).
enum class PlateSpiralMode : int {
  Default = 0,
  On = 1,
  Off = 2
};

/// Per-plate config-override choice for first/other-layer sequences.
enum class LayerSeqChoice : int {
  Auto = 0,
  Custom = 1
};

/// Filament-map mode (v4.5 Phase 107 FMAP-02 -- widened 2-value -> 4-value).
/// Source truth: third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp:424-429
///   enum FilamentMapMode { fmmAutoForFlush, fmmAutoForMatch, fmmManual, fmmDefault };
/// Names + numeric values MUST match upstream exactly (PrintConfig.cpp:579-584
/// serializes the first three as the strings "Auto For Flush" / "Auto For Match"
/// / "Manual"; there is NO "Default" string -- see fmmDefault below).
///
/// FM-07 / fmmDefault semantics (FEATURES.md WS1): fmmDefault=3 is a per-plate
/// "inherit from global" sentinel resolved at read/apply time by upstream
/// PartPlate::get_real_filament_map_mode (PartPlate.cpp:317-328):
///   if (mode != fmmDefault) return mode;        // plate override wins
///   return global_config.filament_map_mode;     // else fall back to global
/// The Qt6 layer stores fmmDefault as-is; the UI does NOT expose it as a 4th
/// radio button (anti-feature per FEATURES.md -- upstream FilamentGroupPopup
/// mode_list is {fmmAutoForFlush, fmmAutoForMatch, fmmManual} only). The
/// resolution to a concrete mode belongs to a future Phase 108+ readback layer;
/// persistence resolves fmmDefault before writing (see ProjectServiceMock write
/// site) so the on-disk value is always one of the three concrete modes.
enum class FilamentMapMode : int {
  fmmAutoForFlush = 0,  // "Filament-Saving Mode" -- minimize flush volume.
  fmmAutoForMatch = 1,  // "Convenience Mode" -- match AMS-loaded filaments.
  fmmManual       = 2,  // "Custom Mode" -- use the explicit filament_maps array.
  fmmDefault      = 3   // per-plate "inherit from global" sentinel (not a UI radio).
};

/// Phase 111 (FMAP-04 / Phase 107 REVIEW R-01): the FM-03 read-side migration
/// predicate, factored out of ProjectServiceMock so it is unit-testable in
/// isolation (R-01: the legacy raw-int-1 -> fmmManual branch was correct by
/// inspection but never executed at runtime -- the round-trip test always took
/// the trusted coEnum branch because the write side produces typed values).
///
/// This function mirrors ONLY the legacy raw-int branch of the read-side
/// discriminator in ProjectServiceMock.cpp (the branch taken when the option
/// is a NON-coEnum int, i.e. a pre-v4.5 Qt6 file that bypassed enum typing).
/// The mapping preserves pre-v4.5 user intent:
///   raw 0 (old "Auto")   -> fmmAutoForFlush (0)
///   raw 1 (old "Manual") -> fmmManual       (2)  <-- the headline FMAP-02 fix
///   anything else         -> fmmDefault      (3)  safe fallback
/// Mapping legacy 1 -> fmmManual (NOT fmmAutoForMatch=1) is the user-visible
/// behavior change FMAP-02 ships: a pre-v4.5 "Manual" plate stays Manual after
/// reload instead of flipping to the new "Convenience Mode" (fmmAutoForMatch).
FilamentMapMode migrateLegacyFilamentMapMode(int legacyRawInt);

class PartPlate {
 public:
  PartPlate() = default;
  explicit PartPlate(int plateIndex) : m_plate_index(plateIndex) {}
  ~PartPlate() = default;

  // ── Identity & geometry ────────────────────────────────────────────────
  int plateIndex() const { return m_plate_index; }
  void setPlateIndex(int idx) { m_plate_index = idx; }

  std::string name() const { return m_name; }
  void setName(const std::string& name) { m_name = name; }

  /// Geometry origin in world space (upstream m_origin, Vec3d).
#ifdef HAS_LIBSLIC3R
  Slic3r::Vec3d origin() const { return m_origin; }
  void setOrigin(const Slic3r::Vec3d& origin) {
    m_origin = origin;
    m_bounding_box_dirty = true;  // default rect tracks the origin (center)
  }
#else
  struct OriginFallback { double x = 0, y = 0, z = 0; };
  OriginFallback origin() const {
    return OriginFallback{m_origin_x, m_origin_y, m_origin_z};
  }
  void setOrigin(double x, double y, double z) {
    m_origin_x = x;
    m_origin_y = y;
    m_origin_z = z;
  }
#endif

  int width() const { return m_width; }
  int depth() const { return m_depth; }
  int height() const { return m_height; }
  void setSize(int w, int d, int h) {
    m_width = w;
    m_depth = d;
    m_height = h;
#ifdef HAS_LIBSLIC3R
    m_bounding_box_dirty = true;  // default rect tracks width/depth
#endif
  }

  // ── Bed shape + print-volume geometry (upstream m_shape / contains) ──────
  // Source truth: PartPlate.hpp:120-123 (m_raw_shape/m_shape/m_exclude_area),
  // PartPlate.cpp:2606-2624 (set_shape), :349-356 (calc_bounding_boxes),
  // :2694-2725 (contains/intersects).
  //
  // Upstream stores the bed shape centered at (0,0) and translates it by the
  // plate grid position (set_shape adds `position` to every raw point,
  // PartPlate.cpp:2609-2611; position = compute_shape_position = the plate
  // origin). Qt6 mirrors this: the plate origin IS the plate center, and the
  // default shape is the width x depth rectangle centered on it. A custom
  // polygon (round/oval beds) can be installed with setShape; until then the
  // default rectangle is synthesized lazily, so origin/size updates never go
  // stale (PartPlateList::updatePlateOrigins keeps moving origins).
#ifdef HAS_LIBSLIC3R
  /// World-space print-area polygon (upstream Pointfs m_shape). Empty until a
  /// custom shape is installed; queries then fall back to the default
  /// origin-centered rectangle.
  const std::vector<Slic3r::Vec2d>& shape() const { return m_shape; }

  /// Installs a custom world-space print-area polygon and marks it custom
  /// (upstream set_shape shape half, PartPlate.cpp:2606-2624). An empty vector
  /// re-selects the default origin-centered rectangle.
  void setShape(std::vector<Slic3r::Vec2d> shape) {
    m_shape = std::move(shape);
    m_bounding_box_dirty = true;
  }

  /// Bounding box of the print area (upstream m_bounding_box, fed by
  /// calc_bounding_boxes, PartPlate.cpp:349-356 -- shape points merged at
  /// z=0). Upstream PartPlate::contains(Vec3d) tests THIS box (PartPlate.cpp:
  /// 2694-2697), not a per-point polygon sweep, so for a custom polygon the
  /// point test is its AABB.
  const Slic3r::BoundingBoxf3& boundingBox() const;

  /// Upstream PartPlate::contains(const Vec3d&) (PartPlate.cpp:2694-2697):
  /// the point lies inside the print-area bounding box. The box is built from
  /// shape points at z=0, so like upstream this holds for bed-plane points
  /// (z == 0).
  bool contains(const Slic3r::Vec3d& point) const {
    return boundingBox().contains(point);
  }

  /// Upstream PartPlate::contains(const BoundingBoxf3&) (PartPlate.cpp:2704-
  /// 2714): the box is FULLY inside the print volume. The volume spans the
  /// print-area footprint widened by BedEpsilon on X/Y, with objects allowed
  /// to protrude below the bed (z in [-1e10, +1e3], PartPlate.cpp:2707-2712).
  bool contains(const Slic3r::BoundingBoxf3& box) const;

  /// Upstream PartPlate::intersects(const BoundingBoxf3&) (PartPlate.cpp:2716-
  /// 2725): the same print volume, overlap instead of containment.
  bool intersects(const Slic3r::BoundingBoxf3& box) const;

  /// Geometric instance lookup under a bed-plane point. Qt6 composite of the
  /// bounding-box work upstream does per instance inside
  /// PartPlateList::notify_instance_update (PartPlate.cpp:4198-4250): walks
  /// this plate's obj_to_instance_set in membership order and returns the
  /// first (objectIndex, instanceIndex) whose transformed world bounding box
  /// -- supplied by instanceBounds, the same source upstream derives from
  /// ModelObject::instance_bounding_box -- contains the point. Returns
  /// {-1,-1} when no member instance matches (or the provider is unset).
  std::pair<int, int> findInstance(
      const Slic3r::Vec3d& point,
      const std::function<Slic3r::BoundingBoxf3(int objectIndex,
                                                int instanceIndex)>& instanceBounds)
      const;
#endif

  // ── Lock / printable / slice state machine ─────────────────────────────
  bool isLocked() const { return m_locked; }
  void setLocked(bool locked) {
    m_locked = locked;
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
    m_noLightThumbnail = QImage();  // G-04: variants follow the main cache
    m_topThumbnail = QImage();
    m_pickThumbnail = QImage();
  }

  // ── Thumbnail cache (v3.2 Phase 30) ────────────────────────────────────
  // Qt-native cached plate thumbnail. Invalidated on every content change so
  // the next access regenerates (mirrors upstream cache-invalidation pattern).
  QImage thumbnail() const { return m_thumbnail; }
  void setThumbnail(const QImage& img) { m_thumbnail = img; }
  bool hasThumbnail() const { return !m_thumbnail.isNull(); }

  // ── G-04: per-view thumbnail variants ──────────────────────────────────
  // Upstream caches no_light/top/picking variants per plate
  // (PartPlate::no_light_thumbnail_data / top_thumbnail_data /
  // pick_thumbnail_data, Plater.cpp:12051-12094) and stores all four
  // families into the 3MF (StoreParams::no_light_thumbnail_data etc).
  // Null = never captured; the writer skips invalid entries (bbs_3mf.cpp
  // is_valid guard), matching upstream behaviour when generation is skipped.
  QImage noLightThumbnail() const { return m_noLightThumbnail; }
  void setNoLightThumbnail(const QImage& img) { m_noLightThumbnail = img; }
  QImage topThumbnail() const { return m_topThumbnail; }
  void setTopThumbnail(const QImage& img) { m_topThumbnail = img; }
  QImage pickThumbnail() const { return m_pickThumbnail; }
  void setPickThumbnail(const QImage& img) { m_pickThumbnail = img; }

  bool isPrintable() const { return m_printable; }
  void setPrintable(bool printable) { m_printable = printable; }

  bool readyForSlice() const { return m_ready_for_slice; }
  void setReadyForSlice(bool ready) { m_ready_for_slice = ready; }

  bool sliceResultValid() const { return m_slice_result_valid; }
  void setSliceResultValid(bool valid) { m_slice_result_valid = valid; }

  bool applyInvalid() const { return m_apply_invalid; }
  void setApplyInvalid(bool invalid) { m_apply_invalid = invalid; }

  float slicePercent() const { return m_slice_percent; }
  void setSlicePercent(float pct) { m_slice_percent = pct; }

  /// Slice state machine gate (upstream semantics): slice is allowed only when
  /// the plate is marked ready and not flagged apply-invalid.
  bool canSlice() const { return m_ready_for_slice && !m_apply_invalid; }

  // -- All-plates print/export readiness predicates (B3) --------------------
  // Upstream truth: PartPlate.hpp:426-441 + PartPlate.cpp:2435-2477, consumed
  // by the PartPlateList aggregates is_all_slice_results_ready_for_print /
  // is_all_slice_result_ready_for_export (PartPlate.cpp:5000-5044).

  /// Upstream is_slice_result_ready_for_print (PartPlate.hpp:426) also
  /// requires a G-code result whose toolpath stays on the bed; Qt6 models
  /// only the validity half today, so readiness reduces to the validity flag
  /// here. The flag is kept in sync with SliceService's result store.
  bool isSliceResultReadyForPrint() const { return m_slice_result_valid; }

  /// Plate-granularity proxy for upstream is_all_instances_unprintable
  /// (PartPlate.cpp:2458): Qt6 carries printability at plate level instead of
  /// per ModelInstance. True also for an empty membership, matching the
  /// upstream loop initialization.
  bool isAllInstancesUnprintable() const { return !m_printable; }

  /// Plate-granularity proxy for upstream has_printable_instances
  /// (PartPlate.cpp:2435): a printable plate holding at least one member
  /// instance that is not flagged outside the bed.
  bool hasPrintableInstances() const {
    if (!m_printable) return false;
    for (const auto& key : m_obj_to_instance_set) {
      if (m_instance_outside_set.count(key) == 0) return true;
    }
    return false;
  }

  /// Mirrors upstream is_slice_result_ready_for_export (PartPlate.hpp:437).
  bool isSliceResultReadyForExport() const {
    return isSliceResultReadyForPrint() && hasPrintableInstances();
  }

  int printIndex() const { return m_print_index; }
  void setPrintIndex(int idx) { m_print_index = idx; }

  // ── Instance-level membership (D-03, upstream obj_to_instance_set) ─────
  /// Returns the (objectIndex, instanceIndex) pairs belonging to this plate.
  const std::set<std::pair<int, int>>& objToInstanceSet() const {
    return m_obj_to_instance_set;
  }

  /// Returns instances that intersect this plate but are not fully contained.
  const std::set<std::pair<int, int>>& instanceOutsideSet() const {
    return m_instance_outside_set;
  }

  bool containsInstance(int objIdx, int instIdx) const {
    return m_obj_to_instance_set.count({objIdx, instIdx}) != 0;
  }

  bool containsInstanceTotally(int objIdx, int instIdx) const {
    return containsInstance(objIdx, instIdx)
        && m_instance_outside_set.count({objIdx, instIdx}) == 0;
  }

  /// Updates the upstream instance_outside_set and slice-readiness state.
  void setInstanceOutside(int objIdx, int instIdx, bool outside) {
    const std::pair<int, int> key{objIdx, instIdx};
    if (outside && containsInstance(objIdx, instIdx))
      m_instance_outside_set.insert(key);
    else
      m_instance_outside_set.erase(key);
    updateSliceReadiness();
  }

  /// Adds (objIdx, instIdx) to the plate's instance membership.
  void addInstance(int objIdx, int instIdx) {
    m_obj_to_instance_set.insert({objIdx, instIdx});
    updateSliceReadiness();
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
    m_noLightThumbnail = QImage();  // G-04: variants follow the main cache
    m_topThumbnail = QImage();
    m_pickThumbnail = QImage();
  }

  /// Removes (objIdx, instIdx) from the plate's instance membership.
  void removeInstance(int objIdx, int instIdx) {
    const std::pair<int, int> key{objIdx, instIdx};
    m_obj_to_instance_set.erase(key);
    m_instance_outside_set.erase(key);
    updateSliceReadiness();
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
    m_noLightThumbnail = QImage();  // G-04: variants follow the main cache
    m_topThumbnail = QImage();
    m_pickThumbnail = QImage();
  }

  /// Returns true if ANY instance of objIdx is on this plate.
  bool hasObject(int objIdx) const;

  /// Clears all instance membership (upstream clear()).
  void clearInstances() {
    m_obj_to_instance_set.clear();
    m_instance_outside_set.clear();
    updateSliceReadiness();
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
    m_noLightThumbnail = QImage();  // G-04: variants follow the main cache
    m_topThumbnail = QImage();
    m_pickThumbnail = QImage();
  }

  /// True if no instances belong to this plate.
  // Mirrors upstream PartPlate::empty() (PartPlate.hpp:387).
  bool empty() const { return m_obj_to_instance_set.empty(); }

  // ── Per-plate config (D-04, upstream DynamicPrintConfig m_config) ──────
#ifdef HAS_LIBSLIC3R
  Slic3r::DynamicPrintConfig& config() { return m_config; }
  const Slic3r::DynamicPrintConfig& config() const { return m_config; }
#endif

  // ── Per-plate settings (align with upstream PlateSettingsDialog) ───────
  // Bed type / print sequence / spiral mode / layer sequences.
  // These were previously scattered parallel QLists on ProjectServiceMock;
  // Phase 16 (D-05) consolidates them here.
  int bedType() const { return m_bed_type; }
  void setBedType(int bedType) { m_bed_type = bedType; }

  int printSequence() const { return m_print_sequence; }
  void setPrintSequence(int seq) { m_print_sequence = seq; }

  int spiralMode() const { return m_spiral_mode; }
  void setSpiralMode(int mode) { m_spiral_mode = mode; }

  // v3.2 Phase 31 (FMAP-01/03) + v4.5 Phase 107 (FMAP-02): manual
  // filament->extruder mapping per plate. Mirrors upstream PartPlate.hpp:262-263.
  std::vector<int> filamentMaps() const { return m_filament_maps; }
  void setFilamentMaps(const std::vector<int>& maps) { m_filament_maps = maps; }
  // Widened to the upstream 4-value FilamentMapMode (see enum above). The int
  // overloads remain so legacy callers (and the 3MF read-migration path that
  // normalizes legacy raw ints) can still set by raw value.
  FilamentMapMode filamentMapMode() const { return m_filament_map_mode; }
  void setFilamentMapMode(FilamentMapMode mode) { m_filament_map_mode = mode; }
  // Phase 110 R-02 (FP-04): validate the int at the Q_INVOKABLE boundary. A
  // QML/Q_INVOKABLE caller passing an out-of-range mode (e.g. mode=5) would
  // otherwise silently store an invalid enum, masked by the writer-side
  // default: case. Clamp out-of-range to fmmDefault (the safe per-plate
  // "inherit from global" sentinel) so the on-disk value stays one of the 3
  // concrete modes after the write-site resolution. The [0,3] bounds cover
  // all 4 enum values (fmmAutoForFlush..fmmDefault). grep-assertable guard.
  void setFilamentMapMode(int mode) {
    if (mode < 0 || mode > 3) mode = static_cast<int>(FilamentMapMode::fmmDefault);
    m_filament_map_mode = FilamentMapMode(mode);
  }

  int firstLayerSeqChoice() const { return m_first_layer_seq_choice; }
  void setFirstLayerSeqChoice(int choice) { m_first_layer_seq_choice = choice; }

  const std::vector<int>& firstLayerSeqOrder() const {
    return m_first_layer_seq_order;
  }
  void setFirstLayerSeqOrder(std::vector<int> order) {
    m_first_layer_seq_order = std::move(order);
  }

  int otherLayersSeqChoice() const { return m_other_layers_seq_choice; }
  void setOtherLayersSeqChoice(int choice) {
    m_other_layers_seq_choice = choice;
  }

  const std::vector<LayerSeqEntry>& otherLayersSeqEntries() const {
    return m_other_layers_seq_entries;
  }
  void setOtherLayersSeqEntries(std::vector<LayerSeqEntry> entries) {
    m_other_layers_seq_entries = std::move(entries);
  }

 private:
  int m_plate_index = 0;
  std::string m_name;

  // Geometry. Under HAS_LIBSLIC3R we keep the real Vec3d; otherwise a 3-double
  // fallback so the header compiles without libslic3r.
#ifdef HAS_LIBSLIC3R
  Slic3r::Vec3d m_origin = Slic3r::Vec3d::Zero();
#else
  double m_origin_x = 0.0, m_origin_y = 0.0, m_origin_z = 0.0;
#endif
  int m_width = 0;
  int m_depth = 0;
  int m_height = 0;

#ifdef HAS_LIBSLIC3R
  /// Custom print-area polygon (upstream m_shape). Empty = the default
  /// origin-centered width x depth rectangle is used for queries.
  std::vector<Slic3r::Vec2d> m_shape;
  /// Print-area bounding box cache (upstream m_bounding_box), rebuilt lazily
  /// by calcBoundingBoxes() after shape/origin/size changes.
  mutable Slic3r::BoundingBoxf3 m_bounding_box;
  mutable bool m_bounding_box_dirty = true;
#endif

  bool m_printable = true;
  bool m_locked = false;
  bool m_ready_for_slice = true;
  bool m_slice_result_valid = false;
  bool m_apply_invalid = false;
  float m_slice_percent = 0.0f;
  int m_print_index = -1;

  /// (objectIndex, instanceIndex) pairs — upstream obj_to_instance_set.
  std::set<std::pair<int, int>> m_obj_to_instance_set;
  /// Subset of memberships that intersect but extend outside the plate.
  std::set<std::pair<int, int>> m_instance_outside_set;

  void updateSliceReadiness() {
    m_ready_for_slice = m_obj_to_instance_set.empty()
        || m_instance_outside_set.empty();
  }

  /// Cached plate thumbnail (v3.2 Phase 30). Qt-native; invalidated on content
  /// change. Converted to Slic3r::ThumbnailData at the 3MF save boundary.
  QImage m_thumbnail;
  /// G-04 (v5.17): per-view variant caches (no_light/top/picking), see above.
  QImage m_noLightThumbnail;
  QImage m_topThumbnail;
  QImage m_pickThumbnail;

#ifdef HAS_LIBSLIC3R
  /// Per-plate config override (upstream m_config, DynamicPrintConfig).
  Slic3r::DynamicPrintConfig m_config;
#endif

  // Per-plate settings (previously parallel QLists on ProjectServiceMock).
  int m_bed_type = 0;                  // PlateBedType
  int m_print_sequence = 0;            // PlatePrintSequence
  int m_spiral_mode = 0;               // PlateSpiralMode

  // v3.2 Phase 31 (FMAP-01) + v4.5 Phase 107 (FMAP-02): manual
  // filament->extruder mapping per plate. Mirrors upstream PartPlate.hpp:262-263
  // (m_filament_maps, m_filament_map_mode).
  // filament_maps[i] = the extruder index that filament i maps to (1-based,
  // matching upstream PlateData::filament_maps at bbs_3mf.hpp:98).
  // filament_map_mode: widened 2-value -> upstream 4-value FilamentMapMode
  // (fmmAutoForFlush=0 / fmmAutoForMatch=1 / fmmManual=2 / fmmDefault=3). The
  // default is fmmAutoForFlush (matches upstream PrintConfig.cpp:2509 default).
  std::vector<int> m_filament_maps;
  FilamentMapMode m_filament_map_mode = FilamentMapMode::fmmAutoForFlush;
  int m_first_layer_seq_choice = 0;    // LayerSeqChoice
  std::vector<int> m_first_layer_seq_order;
  int m_other_layers_seq_choice = 0;   // LayerSeqChoice
  std::vector<LayerSeqEntry> m_other_layers_seq_entries;
};

}  // namespace OWzx
