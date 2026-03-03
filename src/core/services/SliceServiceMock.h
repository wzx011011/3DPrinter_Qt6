#pragma once

#include <QObject>

class QTimer;

class SliceServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY slicingChanged)

public:
  explicit SliceServiceMock(QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;

  Q_INVOKABLE void startSlice(const QString &projectName);

signals:
  void progressChanged();
  void slicingChanged();
  void sliceFinished(const QString &estimatedTime);

private:
  int progress_ = 0;
  bool slicing_ = false;
  QTimer *timer_ = nullptr;
};
