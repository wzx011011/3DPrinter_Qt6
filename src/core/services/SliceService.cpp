#include "SliceService.h"

#include "core/services/ProjectServiceMock.h"

#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <stdexcept>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
#endif

SliceService::SliceService(ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), projectService_(projectService)
{
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

void SliceService::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);

  if (slicing_)
    return;

  const QString sourcePath = projectService_ ? projectService_->sourceFilePath() : QString{};
  if (sourcePath.isEmpty())
  {
    statusLabel_ = QStringLiteral("未找到可切片模型");
    emit progressChanged();
    emit sliceFailed(statusLabel_);
    return;
  }

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

  QtConcurrent::run([receiver, cancelFlag, sourcePath]()
                    {
    QString errorText;
    QString outputPath;

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

      notify(2, QObject::tr("加载模型"));
      Slic3r::Model model = Slic3r::Model::read_from_file(sourcePath.toStdString());
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
      print.apply(model, config);
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

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText]() {
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
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->sliceFinished(QStringLiteral("01:42:16"));
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

  QtConcurrent::run([receiver, cancelFlag, localPath]()
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

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, localPath, errorText]() {
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
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->sliceFinished(QStringLiteral("00:00:00"));
    }, Qt::QueuedConnection); });

  return true;
}
