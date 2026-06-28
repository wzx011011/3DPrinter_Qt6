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
