#include "SliceService.h"

#include "core/services/ProjectServiceMock.h"

#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <stdexcept>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
#include <libslic3r/Utils.hpp>
#endif

namespace
{
  QString formatDurationLabel(double seconds)
  {
    const qint64 totalSeconds = std::max<qint64>(0, qRound64(seconds));
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 secs = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
  }
}

SliceService::SliceService(ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), projectService_(projectService)
{
  if (projectService_)
  {
    connect(projectService_, &ProjectServiceMock::projectChanged, this, [this]()
            {
      if (!slicing_)
        clearStoredResult(); });
  }
}

int SliceService::progress() const
{
  return progress_;
}

bool SliceService::slicing() const
{
  return slicing_;
}

QString SliceService::statusLabel() const
{
  return statusLabel_;
}

QString SliceService::outputPath() const
{
  return outputPath_;
}

QString SliceService::estimatedTimeLabel() const
{
  return estimatedTimeLabel_;
}

QString SliceService::resultWeightLabel() const
{
  return resultWeightLabel_;
}

QString SliceService::resultPlateLabel() const
{
  return resultPlateLabel_;
}

int SliceService::resultPlateIndex() const
{
  return resultPlateIndex_;
}

QString SliceService::resultFilamentLabel() const
{
  return resultFilamentLabel_;
}

int SliceService::resultLayerCount() const
{
  return resultLayerCount_;
}

QString SliceService::resultCostLabel() const
{
  return resultCostLabel_;
}

double SliceService::resultTotalFilamentMm() const
{
  // 从 resultFilamentLabel_ 解析总长度（格式如 "3.45 m"）
  if (resultFilamentLabel_.isEmpty()) return 0.0;
  bool ok = false;
  const double val = resultFilamentLabel_.left(resultFilamentLabel_.indexOf(' ')).toDouble(&ok);
  return ok ? val * 1000.0 : 0.0; // m → mm
}

void SliceService::clearStoredResult()
{
  progress_ = 0;
  sliceState_ = State::Idle;
  statusLabel_ = QStringLiteral("等待切片");
  outputPath_.clear();
  estimatedTimeLabel_.clear();
  resultWeightLabel_.clear();
  resultPlateLabel_.clear();
  resultPlateIndex_ = -1;
  resultFilamentLabel_.clear();
  resultLayerCount_ = 0;
  resultCostLabel_.clear();
  emit progressChanged();
}

void SliceService::setMergedPresetConfig(const QHash<QString, QVariant> &config)
{
  mergedPresetConfig_ = config;
}

namespace
{
  /// Type-aware config value injection into DynamicPrintConfig
  /// Skips keys that don't exist in the config schema
  void injectPresetConfig(Slic3r::DynamicPrintConfig &config, const QHash<QString, QVariant> &presetValues)
  {
#ifdef HAS_LIBSLIC3R
    for (auto it = presetValues.constBegin(); it != presetValues.constEnd(); ++it)
    {
      const std::string key = it.key().toStdString();
      Slic3r::ConfigOption *opt = config.option(key, false);
      if (!opt)
        continue;

      const QVariant &value = it.value();
      bool setOk = false;

      switch (static_cast<QMetaType::Type>(value.typeId()))
      {
      case QMetaType::Double:
      {
        auto *floatOpt = dynamic_cast<Slic3r::ConfigOptionFloat *>(opt);
        if (floatOpt)
        {
          floatOpt->value = value.toDouble();
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::Int:
      {
        auto *intOpt = dynamic_cast<Slic3r::ConfigOptionInt *>(opt);
        if (intOpt)
        {
          intOpt->value = value.toInt();
          setOk = true;
          break;
        }
        auto *boolOpt = dynamic_cast<Slic3r::ConfigOptionBool *>(opt);
        if (boolOpt)
        {
          boolOpt->value = value.toInt() ? 1 : 0;
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::Bool:
      {
        auto *boolOpt = dynamic_cast<Slic3r::ConfigOptionBool *>(opt);
        if (boolOpt)
        {
          boolOpt->value = value.toBool() ? 1 : 0;
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::QVariantList:
      {
        // coFloats, coInts, coPoints, coStrings — serialize to Slic3r format
        const auto list = value.toList();
        if (list.isEmpty())
          break;
        QStringList parts;
        for (const auto &item : list)
          parts << item.toString();
        try
        {
          config.set_deserialize_strict(key, parts.join(",").toStdString(), false);
          setOk = true;
        }
        catch (...)
        {
        }
        break;
      }
      default:
        break;
      }

      if (!setOk)
      {
        const std::string strVal = value.toString().toStdString();
        if (!strVal.empty())
        {
          try
          {
            config.set_deserialize_strict(key, strVal, false);
          }
          catch (...)
          {
          }
        }
      }
    }
#endif
  }
} // anonymous namespace

void SliceService::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);

  if (slicing_)
    return;

  clearStoredResult();

  const QString sourcePath = projectService_ ? projectService_->sourceFilePath() : QString{};
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  QString targetPlateLabel;
#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> modelForSlice;
#endif
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("平板 %1").arg(targetPlateIndex + 1);
#ifdef HAS_LIBSLIC3R
    modelForSlice = projectService_->cloneCurrentPlateModel();
#endif
  }

  if (sourcePath.isEmpty())
  {
    statusLabel_ = QStringLiteral("未找到可切片模型");
    emit progressChanged();
    emit sliceFailed(statusLabel_);
    return;
  }

#ifdef HAS_LIBSLIC3R
  if (!modelForSlice || modelForSlice->objects.empty())
  {
    statusLabel_ = QStringLiteral("当前平板没有可切片对象");
    emit progressChanged();
    emit sliceFailed(statusLabel_);
    return;
  }
#endif

  slicing_ = true;
  sliceState_ = State::Slicing;
  progress_ = 0;
  statusLabel_ = QStringLiteral("准备切片");
  outputPath_.clear();
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  emit slicingChanged();
  emit progressChanged();
  emit progressUpdated(progress_, statusLabel_);

  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeCancelFlag_;

  QtConcurrent::run([receiver, cancelFlag, sourcePath, targetPlateIndex, targetPlateLabel
#ifdef HAS_LIBSLIC3R
                     , modelForSlice = std::move(modelForSlice)
#endif
                     ]() mutable
                    {
    QString errorText;
    QString outputPath;
    QString estimatedTimeLabel;
    QString resultWeightLabel;
    QString resultFilamentLabel;
    QString resultCostLabel;
    QString resultPlateLabel;
    int resultPlateIndex = -1;
    int layerCount = 0;

#ifdef HAS_LIBSLIC3R
    try
    {
      auto notify = [receiver](int progress, const QString &label) {
        if (!receiver)
          return;
        QMetaObject::invokeMethod(receiver, [receiver, progress, label]() {
          if (!receiver)
            return;
          receiver->progress_ = progress;
          receiver->statusLabel_ = label;
          emit receiver->progressChanged();
          emit receiver->progressUpdated(progress, label);
        }, Qt::QueuedConnection);
      };

      notify(2, QObject::tr("准备当前平板模型"));
      if (!modelForSlice || modelForSlice->objects.empty())
        throw std::runtime_error("当前平板没有可切片对象");
      {
        int totalVolumes = 0;
        int totalInstances = 0;
        for (const auto *obj : modelForSlice->objects) {
          if (obj) {
            totalVolumes += int(obj->volumes.size());
            totalInstances += int(obj->instances.size());
          }
        }
        if (totalVolumes == 0)
          throw std::runtime_error("cloned model has 0 volumes (mesh data missing)");
      }
      if (cancelFlag && cancelFlag->load())
        throw std::runtime_error("切片已取消");

      notify(10, QObject::tr("准备切片参数"));
      Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();

      // Inject user-selected preset values into config (对齐上游 PresetBundle::full_fff_config)
      // This overwrites factory defaults with the 3-tier merged hierarchy (printer→filament→print)
      if (receiver && !receiver->mergedPresetConfig_.isEmpty())
      {
        injectPresetConfig(config, receiver->mergedPresetConfig_);
        receiver->mergedPresetConfig_.clear(); // one-shot injection per slice
      }

      // Apply per-plate config overrides (align upstream PartPlate::config)
      if (receiver && receiver->projectService_) {
        const int plateIdx = targetPlateIndex >= 0 ? targetPlateIndex
                             : receiver->projectService_->currentPlateIndex();
        if (plateIdx >= 0 && plateIdx < receiver->projectService_->plateCount()) {
          const int bedType = receiver->projectService_->plateBedType(plateIdx);
          if (bedType > 0) {
            auto *opt = config.option("curr_bed_type");
            if (opt) opt->setInt(bedType);
          }
          const int printSeq = receiver->projectService_->platePrintSequence(plateIdx);
          if (printSeq > 0) {
            auto *opt = config.option("print_sequence");
            if (opt) opt->setInt(printSeq);
          }
          const int spiralMode = receiver->projectService_->plateSpiralMode(plateIdx);
          if (spiralMode == 1) {
            auto *opt = config.option("spiral_mode");
            if (opt) opt->setInt(1);
          } else if (spiralMode == 2) {
            auto *opt = config.option("spiral_mode");
            if (opt) opt->setInt(0);
          }
        }
      }

      Slic3r::Print print;
      receiver->activePrint_.store(&print, std::memory_order_release);

      print.set_status_callback([receiver](const Slic3r::PrintBase::SlicingStatus &st) {
        if (!receiver)
          return;
        const int p = qBound(0, st.percent, 100);
        const QString label = st.text.empty() ? QObject::tr("切片中") : QString::fromStdString(st.text);
        QMetaObject::invokeMethod(receiver, [receiver, p, label]() {
          if (!receiver)
            return;
          if (p >= 0)
            receiver->progress_ = p;
          receiver->statusLabel_ = label;
          emit receiver->progressChanged();
          emit receiver->progressUpdated(receiver->progress_, label);
        }, Qt::QueuedConnection);
      });

      notify(18, QObject::tr("应用切片参数"));

      // Set up directories for Print::apply()
      {
        const QString tempDir = QDir::tempPath();
        Slic3r::set_temporary_dir(tempDir.toStdString());
        Slic3r::set_data_dir(QCoreApplication::applicationDirPath().toStdString());
        Slic3r::set_resources_dir((QCoreApplication::applicationDirPath() + "/resources").toStdString());
      }

      config.normalize_fdm();

      // For Marlin with relative E distances, layer_gcode must reset extruder position
      // (upstream Print::validate enforces this for non-BBL printers)
      {
        auto *useRel = config.option<Slic3r::ConfigOptionBool>("use_relative_e_distances", false);
        auto *gcodeFlavor = config.option<Slic3r::ConfigOptionEnum<Slic3r::GCodeFlavor>>("gcode_flavor", false);
        auto *layerGcode = config.option<Slic3r::ConfigOptionString>("layer_change_gcode", false);
        if (useRel && useRel->value && gcodeFlavor &&
            (gcodeFlavor->value == Slic3r::gcfMarlinLegacy || gcodeFlavor->value == Slic3r::gcfMarlinFirmware) &&
            layerGcode && layerGcode->value.find("G92 E0") == std::string::npos)
        {
          layerGcode->value = "G92 E0\n" + layerGcode->value;
        }
      }

      print.apply(*modelForSlice, config);

      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("切片已取消");
      }

      // Pre-slice validation (align upstream BackgroundSlicingProcess::validate)
      {
        Slic3r::StringObjectException validationError = print.validate();
        if (!validationError.string.empty())
          throw std::runtime_error("切片验证失败: " + validationError.string);
      }

      notify(25, QObject::tr("执行切片"));
      print.process();
      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("切片已取消");
      }

      notify(92, QObject::tr("导出 G-code"));
      Slic3r::GCodeProcessorResult result;
      const QString appDir = QCoreApplication::applicationDirPath();
      const QString baseName = QFileInfo(sourcePath).completeBaseName();
      const QString fileName = QStringLiteral("%1_%2.gcode")
                                   .arg(baseName)
                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
      const QString targetPath = QDir(appDir).filePath(fileName);
      const std::string generated = print.export_gcode(targetPath.toStdString(), &result);
      outputPath = QString::fromStdString(generated);
      estimatedTimeLabel = formatDurationLabel(result.print_statistics.modes[0].time);
      const auto &printStats = print.print_statistics();
      if (printStats.total_weight > 0.0)
        resultWeightLabel = QStringLiteral("%1 g").arg(QString::number(printStats.total_weight, 'f', 2));

      // Filament length
      if (printStats.total_used_filament > 0.0)
        resultFilamentLabel = QStringLiteral("%1 m").arg(QString::number(printStats.total_used_filament / 1000.0, 'f', 2));

      // Layer count (captured for main-thread delivery)
      layerCount = 0;
      for (const Slic3r::PrintObject *obj : print.objects())
          layerCount += static_cast<int>(obj->total_layer_count());

      // Cost
      if (printStats.total_cost > 0.0)
        resultCostLabel = QStringLiteral("$%1").arg(QString::number(printStats.total_cost, 'f', 2));

      resultPlateLabel = targetPlateLabel;
      resultPlateIndex = targetPlateIndex;

      receiver->activePrint_.store(nullptr, std::memory_order_release);
    }
    catch (const std::exception &ex)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      errorText = QString::fromUtf8(ex.what());
    }
    catch (...)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      errorText = QObject::tr("切片失败");
    }
#else
    // Mock mode: simulate slicing progress with fake results
    Q_UNUSED(sourcePath);

    // Generate mock result values based on plate objects count
    int objectCount = 0;
    QString plateLabelForMock = targetPlateLabel;
    if (receiver && receiver->projectService_) {
      objectCount = receiver->projectService_->objectNames().size();
    }

    const int mockLayers = 80 + objectCount * 40;
    const double mockTimeSecs = 120.0 + objectCount * 180.0;
    estimatedTimeLabel = formatDurationLabel(mockTimeSecs);
    resultWeightLabel = QStringLiteral("%1 g").arg(12.5 + objectCount * 8.3, 0, 'f', 1);
    resultFilamentLabel = QStringLiteral("%1 m").arg(2.5 + objectCount * 1.8, 0, 'f', 1);
    const double weightKg = (12.5 + objectCount * 8.3) / 1000.0;
    resultCostLabel = QStringLiteral("$%1").arg(weightKg * 20.0, 0, 'f', 2);
    resultPlateLabel = plateLabelForMock;
    resultPlateIndex = targetPlateIndex;
    outputPath = QStringLiteral("(mock) %1_plate%2.gcode")
                   .arg(QFileInfo(sourcePath).completeBaseName())
                   .arg(targetPlateIndex + 1);

    // Simulate progress with 1-second steps
    const int totalSteps = 5;
    for (int step = 1; step <= totalSteps; ++step) {
      if (cancelFlag && cancelFlag->load())
        break;
      const int pct = step * 100 / totalSteps;
      const QStringList labels = {
        QObject::tr("准备切片参数"),
        QObject::tr("生成层切片"),
        QObject::tr("生成支撑"),
        QObject::tr("计算路径"),
        QObject::tr("导出 G-code")
      };
      QMetaObject::invokeMethod(receiver, [receiver, pct, labels, step]() {
        if (!receiver) return;
        receiver->progress_ = pct;
        receiver->statusLabel_ = labels.value(step - 1, QObject::tr("切片中"));
        emit receiver->progressChanged();
        emit receiver->progressUpdated(pct, receiver->statusLabel_);
      }, Qt::BlockingQueuedConnection);
      QThread::msleep(400);
    }
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText, estimatedTimeLabel, resultWeightLabel, resultPlateLabel, resultPlateIndex, resultFilamentLabel, resultCostLabel, layerCount]() {
      if (!receiver)
        return;

      receiver->slicing_ = false;
      receiver->activeCancelFlag_.reset();
      emit receiver->slicingChanged();

      if (cancelFlag && cancelFlag->load())
      {
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("已取消切片");
        emit receiver->progressChanged();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        return;
      }

      if (!errorText.isEmpty())
      {
        receiver->sliceState_ = State::Error;
        receiver->statusLabel_ = errorText;
        emit receiver->progressChanged();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(errorText);
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("切片完成");
      receiver->outputPath_ = outputPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel;
      receiver->resultWeightLabel_ = resultWeightLabel;
      receiver->resultPlateLabel_ = resultPlateLabel;
      receiver->resultPlateIndex_ = resultPlateIndex;
      receiver->resultFilamentLabel_ = resultFilamentLabel;
      receiver->resultCostLabel_ = resultCostLabel;
      receiver->resultLayerCount_ = layerCount;

      // Store per-plate result for multi-plate tracking
      if (resultPlateIndex >= 0) {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = estimatedTimeLabel;
        pr.resultWeightLabel = resultWeightLabel;
        pr.resultFilamentLabel = resultFilamentLabel;
        pr.resultCostLabel = resultCostLabel;
        pr.resultLayerCount = receiver->resultLayerCount_;
        pr.totalFilamentMm = 0.0;
        receiver->plateResults_[resultPlateIndex] = pr;
      }
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->sliceFinished(receiver->estimatedTimeLabel_);
    }, Qt::QueuedConnection); });
}

void SliceService::cancelSlice()
{
  if (!slicing_ || !activeCancelFlag_)
    return;

  sliceState_ = State::Cancelled;
  activeCancelFlag_->store(true);
#ifdef HAS_LIBSLIC3R
  if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
    active->cancel();
#endif
  emit stateChanged();
}

bool SliceService::loadGCodeFromPrevious(const QString &gcodeFilePath)
{
  if (slicing_)
    return false;

  clearStoredResult();

  const QFileInfo info(gcodeFilePath);
  const QString localPath = info.absoluteFilePath();
  if (!info.exists() || !info.isFile())
  {
    statusLabel_ = QObject::tr("G-code 文件不存在");
    emit progressChanged();
    emit sliceFailed(statusLabel_);
    return false;
  }

  slicing_ = true;
  progress_ = 0;
  statusLabel_ = QObject::tr("复用已有 G-code");
  outputPath_.clear();
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  emit slicingChanged();
  emit progressChanged();
  emit progressUpdated(progress_, statusLabel_);

  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeCancelFlag_;
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  QString targetPlateLabel;
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("平板 %1").arg(targetPlateIndex + 1);
  }

  QtConcurrent::run([receiver, cancelFlag, localPath, targetPlateIndex, targetPlateLabel]()
                    {
    QString errorText;

#ifdef HAS_LIBSLIC3R
    try
    {
      Slic3r::Print print;
      Slic3r::GCodeProcessorResult result;
      print.export_gcode_from_previous_file(localPath.toStdString(), &result);
    }
    catch (const std::exception &ex)
    {
      errorText = QString::fromUtf8(ex.what());
    }
    catch (...)
    {
      errorText = QObject::tr("复用 G-code 失败");
    }
#else
    Q_UNUSED(localPath);
    errorText = QObject::tr("当前构建未启用 libslic3r");
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, localPath, errorText, targetPlateIndex, targetPlateLabel]() {
      if (!receiver)
        return;

      receiver->slicing_ = false;
      receiver->activeCancelFlag_.reset();
      emit receiver->slicingChanged();

      if (cancelFlag && cancelFlag->load())
      {
        receiver->statusLabel_ = QObject::tr("已取消切片");
        emit receiver->progressChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        return;
      }

      if (!errorText.isEmpty())
      {
        receiver->statusLabel_ = errorText;
        emit receiver->progressChanged();
        emit receiver->sliceFailed(errorText);
        return;
      }

      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("复用已有 G-code 完成");
      receiver->outputPath_ = localPath;
      receiver->resultPlateLabel_ = targetPlateLabel;
      receiver->resultPlateIndex_ = targetPlateIndex;
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->sliceFinished(QString{});
    }, Qt::QueuedConnection); });

  return true;
}

void SliceService::startSlicePlate(int plateIndex)
{
  if (slicing_)
    return;
  if (projectService_)
    projectService_->setCurrentPlateIndex(plateIndex);
  startSlice(projectService_ ? projectService_->projectName() : QString{});
}

bool SliceService::exportGCodeToPath(const QString &targetPath)
{
  if (outputPath_.isEmpty())
  {
    statusLabel_ = QObject::tr("没有可导出的 G-code，请先切片");
    emit progressChanged();
    return false;
  }

  const QFileInfo srcInfo(outputPath_);
  if (!srcInfo.exists() || !srcInfo.isFile())
  {
    statusLabel_ = QObject::tr("G-code 源文件不存在");
    emit progressChanged();
    return false;
  }

  if (QFile::exists(targetPath))
    QFile::remove(targetPath);

  if (!QFile::copy(outputPath_, targetPath))
  {
    statusLabel_ = QObject::tr("导出 G-code 失败");
    emit progressChanged();
    return false;
  }

  statusLabel_ = QObject::tr("已导出: %1").arg(QFileInfo(targetPath).fileName());
  emit progressChanged();
  return true;
}

bool SliceService::hasPlateResult(int plateIndex) const
{
  return plateResults_.contains(plateIndex);
}

QString SliceService::plateEstimatedTime(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->estimatedTimeLabel : QString();
}

QString SliceService::plateWeight(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultWeightLabel : QString();
}

QString SliceService::plateFilament(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultFilamentLabel : QString();
}

QString SliceService::plateCost(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultCostLabel : QString();
}

int SliceService::plateLayerCount(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultLayerCount : 0;
}

void SliceService::clearPlateResults()
{
  plateResults_.clear();
}

void SliceService::removePlateResult(int plateIndex)
{
  plateResults_.remove(plateIndex);
}
