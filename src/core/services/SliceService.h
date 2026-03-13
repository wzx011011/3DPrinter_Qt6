#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <atomic>

class ProjectServiceMock;

#ifdef HAS_LIBSLIC3R
namespace Slic3r
{
  class Print;
}
#endif

class SliceService final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY slicingChanged)
  Q_PROPERTY(QString statusLabel READ statusLabel NOTIFY progressChanged)
  Q_PROPERTY(QString outputPath READ outputPath NOTIFY sliceFinished)

public:
  explicit SliceService(ProjectServiceMock *projectService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  QString statusLabel() const;
  QString outputPath() const;
  QString estimatedTimeLabel() const;
  QString resultWeightLabel() const;
  QString resultPlateLabel() const;
  int resultPlateIndex() const;

  Q_INVOKABLE void startSlice(const QString &projectName);
  Q_INVOKABLE void startSlicePlate(int plateIndex);
  Q_INVOKABLE void cancelSlice();
  Q_INVOKABLE bool loadGCodeFromPrevious(const QString &gcodeFilePath);
  Q_INVOKABLE bool exportGCodeToPath(const QString &targetPath);

signals:
  void progressChanged();
  void slicingChanged();
  void progressUpdated(int percent, const QString &label);
  void sliceFinished(const QString &estimatedTime);
  void sliceFailed(const QString &message);

private:
  ProjectServiceMock *projectService_ = nullptr;
  int progress_ = 0;
  bool slicing_ = false;
  QString statusLabel_ = QStringLiteral("等待切片");
  QString outputPath_;
  QString estimatedTimeLabel_;
  QString resultWeightLabel_;
  QString resultPlateLabel_;
  int resultPlateIndex_ = -1;
  std::shared_ptr<std::atomic_bool> activeCancelFlag_;
#ifdef HAS_LIBSLIC3R
  std::atomic<Slic3r::Print *> activePrint_{nullptr};
#endif

  void clearStoredResult();
};
