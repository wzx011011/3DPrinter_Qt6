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
#include <stdexcept>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
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

void SliceService::clearStoredResult()
{
  progress_ = 0;
  statusLabel_ = QStringLiteral("等待切片");
  outputPath_.clear();
  estimatedTimeLabel_.clear();
  resultWeightLabel_.clear();
  resultPlateLabel_.clear();
  resultPlateIndex_ = -1;
  emit progressChanged();
}

void SliceService::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);

  if (slicing_)
    return;

  clearStoredResult();

  const QString sourcePath = projectService_ ? projectService_->sourceFilePath() : QString{};
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  QString targetPlateLabel;
  std::unique_ptr<Slic3r::Model> modelForSlice;
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
  progress_ = 0;
  statusLabel_ = QStringLiteral("准备切片");
  outputPath_.clear();
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  emit slicingChanged();
  emit progressChanged();
  emit progressUpdated(progress_, statusLabel_);

  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeCancelFlag_;

  QtConcurrent::run([receiver, cancelFlag, sourcePath, targetPlateIndex, targetPlateLabel, modelForSlice = std::move(modelForSlice)]() mutable
                    {
    QString errorText;
    QString outputPath;
    QString estimatedTimeLabel;
    QString resultWeightLabel;
    QString resultPlateLabel;
    int resultPlateIndex = -1;

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
      if (cancelFlag && cancelFlag->load())
        throw std::runtime_error("切片已取消");

      notify(10, QObject::tr("准备切片参数"));
      Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();

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
      print.apply(*modelForSlice, config);
      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("切片已取消");
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
    Q_UNUSED(sourcePath);
    errorText = QObject::tr("当前构建未启用 libslic3r");
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText, estimatedTimeLabel, resultWeightLabel, resultPlateLabel, resultPlateIndex]() {
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
      receiver->statusLabel_ = QObject::tr("切片完成");
      receiver->outputPath_ = outputPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel;
      receiver->resultWeightLabel_ = resultWeightLabel;
      receiver->resultPlateLabel_ = resultPlateLabel;
      receiver->resultPlateIndex_ = resultPlateIndex;
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->sliceFinished(receiver->estimatedTimeLabel_);
    }, Qt::QueuedConnection); });
}

void SliceService::cancelSlice()
{
  if (!slicing_ || !activeCancelFlag_)
    return;

  activeCancelFlag_->store(true);
#ifdef HAS_LIBSLIC3R
  if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
    active->cancel();
#endif
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
