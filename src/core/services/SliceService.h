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

  /// v2.7 P0：设置热床形状（对齐 CLI 成功路径 CliRunner.cpp:397-399）。
  /// 用 set_key_value + ConfigOptionPoints 直接创建 option，绕过 injectPresetConfig
  /// 的 set_deserialize_strict 路径（后者对 bed_shape/printable_area 别名不可靠）。
  /// 坐标单位 mm，内部 ×1000 转 μm（coord_t）。
  void setBedShape(const QVector<QPointF> &pointsMm);

  /// v2.7 P1：设置校准参数（对齐上游 Print::set_calib_params）。
  /// 在 startSlice() 前调用。worker 在 print.apply() 后、process() 前注入，
  /// 使 GCode::do_export 走 Calib_PA_Line / Calib_Flow_Rate / Calib_Temp_Tower 分支，
  /// 自动生成校准 G-code。mode=Calib_None 表示普通切片。
  /// 路径 B（镜像上游 CalibUtils::send_to_print）：不直接构造 CalibPressureAdvanceLine
  /// （其唯一构造点在 GCode.cpp:3250 do_export 内部），而是设 Print.calib_params
  /// 跑完整 slice→export 流水线。
  void setCalibParams(int mode, double start, double end, double step,
                      bool printNumbers = false, int testModel = 0);

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
  /// v2.7 P0：热床形状（mm，由 setBedShape 设置）。空表示用 full_print_config 默认。
  /// startSlice 时通过 set_key_value("bed_shape", ConfigOptionPoints) 注入（镜像 CLI）。
  QVector<QPointF> bedShape_;

  /// v2.7 P1：校准参数（由 setCalibParams 设置）。mode=0(Calib_None) 表示普通切片。
  /// worker 在 print.apply() 后注入到 Print，使 GCode::do_export 走校准分支。
  struct CalibConfig
  {
    int mode = 0;        ///< CalibMode (0=None, 1=PA_Line, 5=Flow_Rate, 6=Temp_Tower)
    double start = 0.0;
    double end = 0.0;
    double step = 0.0;
    bool printNumbers = false;
    int testModel = 0;
  } calibConfig_;

  void clearStoredResult();
};
