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

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <QImage>  // v3.2 Phase 30: cached plate thumbnail (Qt-native).

#ifdef HAS_LIBSLIC3R
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
  void setOrigin(const Slic3r::Vec3d& origin) { m_origin = origin; }
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
  void setSize(int w, int d, int h) { m_width = w; m_depth = d; m_height = h; }

  // ── Lock / printable / slice state machine ─────────────────────────────
  bool isLocked() const { return m_locked; }
  void setLocked(bool locked) {
    m_locked = locked;
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
  }

  // ── Thumbnail cache (v3.2 Phase 30) ────────────────────────────────────
  // Qt-native cached plate thumbnail. Invalidated on every content change so
  // the next access regenerates (mirrors upstream cache-invalidation pattern).
  QImage thumbnail() const { return m_thumbnail; }
  void setThumbnail(const QImage& img) { m_thumbnail = img; }
  bool hasThumbnail() const { return !m_thumbnail.isNull(); }

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

  int printIndex() const { return m_print_index; }
  void setPrintIndex(int idx) { m_print_index = idx; }

  // ── Instance-level membership (D-03, upstream obj_to_instance_set) ─────
  /// Returns the (objectIndex, instanceIndex) pairs belonging to this plate.
  const std::set<std::pair<int, int>>& objToInstanceSet() const {
    return m_obj_to_instance_set;
  }

  /// Adds (objIdx, instIdx) to the plate's instance membership.
  void addInstance(int objIdx, int instIdx) {
    m_obj_to_instance_set.insert({objIdx, instIdx});
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
  }

  /// Removes (objIdx, instIdx) from the plate's instance membership.
  void removeInstance(int objIdx, int instIdx) {
    m_obj_to_instance_set.erase({objIdx, instIdx});
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
  }

  /// Returns true if ANY instance of objIdx is on this plate.
  bool hasObject(int objIdx) const;

  /// Clears all instance membership (upstream clear()).
  void clearInstances() {
    m_obj_to_instance_set.clear();
    m_thumbnail = QImage();  // v3.2 Phase 30 D-30-10: invalidate cache
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
  void setFilamentMapMode(int mode) { m_filament_map_mode = FilamentMapMode(mode); }

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

  bool m_printable = true;
  bool m_locked = false;
  bool m_ready_for_slice = true;
  bool m_slice_result_valid = false;
  bool m_apply_invalid = false;
  float m_slice_percent = 0.0f;
  int m_print_index = -1;

  /// (objectIndex, instanceIndex) pairs — upstream obj_to_instance_set.
  std::set<std::pair<int, int>> m_obj_to_instance_set;

  /// Cached plate thumbnail (v3.2 Phase 30). Qt-native; invalidated on content
  /// change. Converted to Slic3r::ThumbnailData at the 3MF save boundary.
  QImage m_thumbnail;

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
