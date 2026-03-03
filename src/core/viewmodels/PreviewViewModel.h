#pragma once

#include <QObject>

class SliceServiceMock;

class PreviewViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY stateChanged)
  Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY stateChanged)

public:
  explicit PreviewViewModel(SliceServiceMock *sliceService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  QString estimatedTime() const;

signals:
  void stateChanged();

private:
  SliceServiceMock *sliceService_ = nullptr;
  QString estimatedTime_ = QStringLiteral("--:--:--");
};
