#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <QVector>
#include <QPointF>
#include <memory>
#include <atomic>
#include <vector>

// Phase 108 (FMAP-01): PartPlate.h declares the OWzx::FilamentMapMode enum
// (widened to the upstream 4-value model in Phase 107) used by the
// FilamentMapResult POD below. Included here so the POD's mode field has the
// complete type on every consumer of this header (EditorViewModel, tests).
#include "core/model/PartPlate.h"

class ProjectServiceMock;
class AppSettingsService;

#ifdef HAS_LIBSLIC3R
namespace Slic3r
{
  class Print;
}
#endif

struct PlateSliceResult {
  QString estimatedTimeLabel;
  QString resultWeightLabel;
  QString resultFilamentLabel;
  QString resultCostLabel;
  QString outputPath;
  int resultLayerCount = 0;
  double totalFilamentMm = 0.0;
  int source = 0;
};

/// Phase 100 (WTREAD-01): Wipe-tower geometry captured BY VALUE inside the
/// SliceService worker after print.process() succeeds and before the Print is
/// invalidated. No Print* or WipeTowerData* may escape the worker (Frozen
/// Decision 1 from 99-GAP-MATRIX.md). The fields mirror upstream
/// Print::WipeTowerData (Print.hpp:740-786) and the width/position derivation
/// in Print.cpp:2871-2873 (bbx span + rib_offset). The Qt renderer uses X/Z as
/// the bed plane (upstream Y maps to Qt Z), matching buildWipeTowerVertices at
/// GizmoGeometry.cpp:449.
///
/// Phase 109 (WTMESH-01/02): the POD is EXTENDED (not replaced) with the real
/// mesh field hasRealMesh + meshVertices. The existing v4.4 dim fields above
/// are UNCHANGED so the Option A fallback (Phase 99 Frozen Decision 2) still
/// works when the engine did not populate wipe_tower_mesh_data. The new fields
/// default to false/empty so a v4.4-style dims-only capture leaves
/// hasRealMesh=false (the renderer takes the Option A else branch). When the
/// mesh is populated, meshVertices holds the flattened XYZ floats of the
/// convex hull of the merged real_wipe_tower_mesh + real_brim_mesh (mirrors
/// upstream 3DScene.cpp:906-914: mesh.merge(brim_mesh) then
/// mesh.convex_hull_3d() then its.vertices). NO TriangleMesh* or its* escapes
/// the worker -- meshVertices is a pure float vector (Frozen Decision 1
/// extended).
struct WipeTowerGeometry {
  /// Mirrors Print::has_wipe_tower() (Print.hpp:988). When false (single-material
  /// / enable_prime_tower off), the renderer must show no wipe-tower (WTREAD-02).
  bool valid = false;
  /// Tower origin X in the bed plane (bbx.min.x() + rib_offset.x()).
  float x = 0.f;
  /// Tower origin Z in the bed plane (bbx.min.y() + rib_offset.y()).
  float z = 0.f;
  /// Tower width (bbx.max.x() - bbx.min.x()).
  float width = 0.f;
  /// Tower depth (Print::get_wipe_tower_depth()).
  float depth = 0.f;
  /// Tower height (Print::wipe_tower_data().height).
  float height = 0.f;
  /// Tower brim width (Print::wipe_tower_data().brim_width).
  float brimWidth = 0.f;
  /// Rib offset X (Print::get_rib_offset().x()).
  float ribOffsetX = 0.f;
  /// Rib offset Y (Print::get_rib_offset().y()).
  float ribOffsetY = 0.f;
  /// Phase 109 (WTMESH-01): true ONLY when the engine populated
  /// Print::wipe_tower_data().wipe_tower_mesh_data (Print.hpp:766), so the
  /// merged+convex-hull mesh is available for real-mesh rendering (Option B).
  /// When false, meshVertices is empty and the renderer must take the Option A
  /// dimensioned-box path (Phase 99 Frozen Decision 2 baseline).
  bool hasRealMesh = false;
  /// Phase 109 (WTMESH-02): flattened XYZ TRIANGLE SOUP expanded from the
  /// convex hull of the merged real_wipe_tower_mesh + real_brim_mesh. Layout:
  /// [x0,y0,z0, x1,y1,z1, x2,y2,z2, ...] -- 9 floats per triangle (3 verts ×
  /// XYZ), size divisible by 9. The v4.4 W1 corner-to-center convention is
  /// ALREADY applied to the captured x/z above (SliceService.cpp adds half the
  /// bed-plane extents); this vector stores the upstream mesh vertices as-is
  /// (the bed-plane transform upstream Y -> Qt Z is applied in the renderer
  /// builder, not here). Empty when hasRealMesh is false. NO TriangleMesh* or
  /// its* is captured -- the worker EXPANDS shell.its.indices into this flat
  /// soup (its.vertices alone is a deduplicated point cloud that would render
  /// as garbage GL_TRIANGLES; the indices carry the triangle connectivity --
  /// Phase 109 REVIEW CRITICAL-1). Frozen Decision 1 extended.
  std::vector<float> meshVertices;
};

/// Phase 108 (FMAP-01): filament-map auto-recommendation readback captured BY
/// VALUE inside the SliceService worker after print.process() succeeds and
/// before the Print is invalidated. Mirrors the v4.4 WipeTowerGeometry capture
/// pattern (Frozen Decision 1 from 99-GAP-MATRIX.md): no Print* or libslic3r
/// reference type may escape the worker. The maps field mirrors upstream
/// Print::get_filament_maps() (Print.cpp:3051, declared Print.hpp:996), which
/// returns the result of the auto-recommendation computed inside print.process()
/// at Print.cpp:2484-2491 (ToolOrdering::get_recommended_filament_maps + the
/// +1 transform, so the values are 1-based group ids). The mode field is the
/// Phase 107 OWzx::FilamentMapMode enum (numeric values identical to upstream
/// Slic3r::FilamentMapMode, so a static_cast<int> round-trips losslessly).
/// When valid is false (user picked Manual, so mode >= fmmManual and no
/// auto-map was computed), receivers must not surface an auto recommendation.
struct FilamentMapResult {
  /// True ONLY when the upstream auto-recommendation actually ran, i.e. when
  /// the resolved mode was < fmmManual (Print.cpp:2485). When the user picked
  /// Manual, the engine does not compute an auto-map, so valid stays false and
  /// there is no recommendation to surface (mirrors WTREAD-02 gate logic).
  bool valid = false;
  /// The resolved per-plate filament-map mode (OWzx::FilamentMapMode). When
  /// capturing, the worker resolves fmmDefault against the resolved mode read
  /// from the print config (Print::get_filament_map_mode, Print.cpp:3056) so
  /// the value stored here is always one of the three concrete modes.
  OWzx::FilamentMapMode mode = OWzx::FilamentMapMode::fmmDefault;
  /// The auto-recommended per-extruder mapping (1-based group ids), as returned
  /// by Print::get_filament_maps(). Empty when valid is false.
  std::vector<int> maps;
};

class SliceService final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY slicingChanged)
  Q_PROPERTY(QString statusLabel READ statusLabel NOTIFY progressChanged)
  Q_PROPERTY(QString outputPath READ outputPath NOTIFY resultChanged)

public:
  enum class State {
    Idle,
    Slicing,
    Exporting,
    Completed,
    Cancelled,
    Error
  };
  Q_ENUM(State)

  enum class ResultSource {
    None = 0,
    ModelSlice = 1,
    PreviousGCode = 2
  };
  Q_ENUM(ResultSource)

  State sliceState() const { return sliceState_; }
  explicit SliceService(ProjectServiceMock *projectService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  QString statusLabel() const;
  QString outputPath() const;
  QString estimatedTimeLabel() const;
  QString resultWeightLabel() const;
  QString resultPlateLabel() const;
  int resultPlateIndex() const;
  /// Filament usage length, aligned with upstream SliceInfoPanel filament used.
  QString resultFilamentLabel() const;
  /// Sliced layer count.
  int resultLayerCount() const;
  /// Estimated print cost, aligned with upstream PrintEstimatedStatistics.
  QString resultCostLabel() const;
  /// Total filament length in millimeters, used by average speed calculations.
  double resultTotalFilamentMm() const;

  /// Per-plate result accessors
  Q_INVOKABLE bool hasPlateResult(int plateIndex) const;
  Q_INVOKABLE QString plateEstimatedTime(int plateIndex) const;
  Q_INVOKABLE QString plateWeight(int plateIndex) const;
  Q_INVOKABLE QString plateFilament(int plateIndex) const;
  Q_INVOKABLE QString plateCost(int plateIndex) const;
  Q_INVOKABLE int plateLayerCount(int plateIndex) const;
  Q_INVOKABLE QString plateOutputPath(int plateIndex) const;
  Q_INVOKABLE int plateResultSource(int plateIndex) const;
  Q_INVOKABLE bool activatePlateResult(int plateIndex);

  Q_INVOKABLE void startSlice(const QString &projectName);
  Q_INVOKABLE void startSlicePlate(int plateIndex);
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE bool loadGCodeFromPrevious(const QString &gcodeFilePath);
  Q_INVOKABLE QString defaultExportGCodeFileName(int plateIndex = -1) const;
  Q_INVOKABLE bool exportGCodeToPath(const QString &targetPath);
  Q_INVOKABLE bool exportPlateGCodeToPath(int plateIndex, const QString &targetPath);
  Q_INVOKABLE bool exportAllPlateGCodeToDirectory(const QString &directoryPath, const QString &baseName = QString());
  Q_INVOKABLE void clearResults();

  /// Inject merged preset config before startSlice(), aligned with upstream PresetBundle::full_fff_config.
  /// Values come from the current user-selected printer, filament, and print presets.
  void setMergedPresetConfig(const QHash<QString, QVariant> &config);

  /// v2.7 P0: set bed shape in millimeters, aligned with CLI path CliRunner.cpp:397-399.
  /// Uses ConfigOptionPoints directly because bed_shape is not suitable for the generic preset path.
  /// Coordinates are millimeters and converted to internal coord_t units during injection.

  void setBedShape(const QVector<QPointF> &pointsMm);

  /// v2.7 P1: set calibration params, aligned with upstream Print::set_calib_params.
  /// Called before startSlice(); the worker injects params after print.apply() and before process().
  /// GCode::do_export then generates calibration G-code for supported calibration modes.
  /// mode=0 means normal slicing.
  void setCalibParams(int mode, double start, double end, double step,
                      bool printNumbers = false, int testModel = 0);

  /// v2.8 W3: inject AppSettingsService for persisted bed size lookup.
  void setAppSettings(AppSettingsService *appSettings) { appSettings_ = appSettings; }

  void clearPlateResults();
  void removePlateResult(int plateIndex);

signals:
  void stateChanged();
  void progressChanged();
  void slicingChanged();
  void progressUpdated(int percent, const QString &label);
  void resultChanged();
  void sliceResultCleared();
  void sliceFinished(const QString &estimatedTime);
  /// Phase 100 (WTREAD-01): delivers the captured-by-value wipe-tower geometry
  /// to the GUI thread. Emitted on the success branch of the sliceFinished
  /// queued-invokeMethod lambda (SliceService.cpp worker). When valid is false
  /// (single-material / enable_prime_tower off), receivers must gate to
  /// showWipeTower=false (WTREAD-02).
  void wipeTowerGeometryReady(const WipeTowerGeometry &geometry);
  /// Phase 108 (FMAP-01): delivers the captured-by-value filament-map
  /// auto-recommendation to the GUI thread. Emitted on the success branch of
  /// the sliceFinished queued-invokeMethod lambda (SliceService.cpp worker),
  /// on the SAME gate as wipeTowerGeometryReady (cancel/error branches do NOT
  /// emit). When result.valid is false (user picked Manual, so no auto-map was
  /// computed by the engine), receivers must not surface an auto recommendation.
  void filamentMapReady(const FilamentMapResult &result);
  void sliceFailed(const QString &message);
  void exportStarted(const QString &stage);
  void exportFinished(const QString &filePath);
  void exportFailed(const QString &message);

private:
  ProjectServiceMock *projectService_ = nullptr;
  int progress_ = 0;
  bool slicing_ = false;
  State sliceState_ = State::Idle;
  QString statusLabel_ = QStringLiteral("Waiting to slice");
  QString outputPath_;
  QString estimatedTimeLabel_;
  QString resultWeightLabel_;
  QString resultPlateLabel_;
  int resultPlateIndex_ = -1;
  QString resultFilamentLabel_;
  int resultLayerCount_ = 0;
  QString resultCostLabel_;
  int activeTargetPlateIndex_ = -1;
  std::shared_ptr<std::atomic_bool> activeCancelFlag_;
#ifdef HAS_LIBSLIC3R
  std::atomic<Slic3r::Print *> activePrint_{nullptr};
#endif

  QMap<int, PlateSliceResult> plateResults_;
  QHash<QString, QVariant> mergedPresetConfig_; ///< Merged preset values injected from ConfigViewModel.
  /// Bed shape in millimeters. Empty means using full_print_config defaults.
  /// startSlice injects it via set_key_value("bed_shape", ConfigOptionPoints).
  QVector<QPointF> bedShape_;

  /// Calibration params set by setCalibParams. mode=0 means normal slicing.
  /// The worker injects these params after print.apply() so GCode::do_export uses calibration branches.
  struct CalibConfig
  {
    int mode = 0;        ///< CalibMode (0=None, 1=PA_Line, 5=Flow_Rate, 6=Temp_Tower)
    double start = 0.0;
    double end = 0.0;
    double step = 0.0;
    bool printNumbers = false;
    int testModel = 0;
  } calibConfig_;

  /// App settings service for persisted bed size.
  AppSettingsService *appSettings_ = nullptr;

  void clearStoredResult();
  void clearActiveTargetResult();
  void storePlateResult(int plateIndex, const PlateSliceResult &result);
  void setExportStatus(State state, int progress, const QString &label);
  bool exportSourceToPath(const QString &sourcePath, const QString &targetPath, const QString &displayName);
};
