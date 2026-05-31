#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QHash>
#include <QVariant>
#include <memory>
#include <atomic>

class ProjectServiceMock;

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
  int resultLayerCount = 0;
  double totalFilamentMm = 0.0;
};

class SliceService final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY slicingChanged)
  Q_PROPERTY(QString statusLabel READ statusLabel NOTIFY progressChanged)
  Q_PROPERTY(QString outputPath READ outputPath NOTIFY sliceFinished)

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
  /// 耗材用量（长度，对齐上游 SliceInfoPanel filament used）
  QString resultFilamentLabel() const;
  /// 切片层数
  int resultLayerCount() const;
  /// 预估成本（对齐上游 PrintEstimatedStatistics）
  QString resultCostLabel() const;
  /// 总耗材长度（mm，用于平均速度计算）
  double resultTotalFilamentMm() const;

  /// Per-plate result accessors
  Q_INVOKABLE bool hasPlateResult(int plateIndex) const;
  Q_INVOKABLE QString plateEstimatedTime(int plateIndex) const;
  Q_INVOKABLE QString plateWeight(int plateIndex) const;
  Q_INVOKABLE QString plateFilament(int plateIndex) const;
  Q_INVOKABLE QString plateCost(int plateIndex) const;
  Q_INVOKABLE int plateLayerCount(int plateIndex) const;

  Q_INVOKABLE void startSlice(const QString &projectName);
  Q_INVOKABLE void startSlicePlate(int plateIndex);
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE bool loadGCodeFromPrevious(const QString &gcodeFilePath);
  Q_INVOKABLE bool exportGCodeToPath(const QString &targetPath);

  /// 注入合并后的预设配置（对齐上游 PresetBundle::full_fff_config）
  /// 在 startSlice() 前调用，将 user-selected preset values 传入 slice engine
  void setMergedPresetConfig(const QHash<QString, QVariant> &config);

  void clearPlateResults();
  void removePlateResult(int plateIndex);

signals:
  void stateChanged();
  void progressChanged();
  void slicingChanged();
  void progressUpdated(int percent, const QString &label);
  void sliceFinished(const QString &estimatedTime);
  void sliceFailed(const QString &message);

private:
  ProjectServiceMock *projectService_ = nullptr;
  int progress_ = 0;
  bool slicing_ = false;
  State sliceState_ = State::Idle;
  QString statusLabel_ = QStringLiteral("等待切片");
  QString outputPath_;
  QString estimatedTimeLabel_;
  QString resultWeightLabel_;
  QString resultPlateLabel_;
  int resultPlateIndex_ = -1;
  QString resultFilamentLabel_;
  int resultLayerCount_ = 0;
  QString resultCostLabel_;
  std::shared_ptr<std::atomic_bool> activeCancelFlag_;
#ifdef HAS_LIBSLIC3R
  std::atomic<Slic3r::Print *> activePrint_{nullptr};
#endif

  QMap<int, PlateSliceResult> plateResults_;
  QHash<QString, QVariant> mergedPresetConfig_; ///< 从 ConfigViewModel 注入的合并预设值

  void clearStoredResult();
};
