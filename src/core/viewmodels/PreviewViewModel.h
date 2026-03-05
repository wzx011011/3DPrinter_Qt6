#pragma once

#include <QObject>
#include <QByteArray>
#include <QVariantList>

class QTimer;

class SliceService;

class PreviewViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY stateChanged)
  Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY stateChanged)
  Q_PROPERTY(int layerCount READ layerCount NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMin READ currentLayerMin NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMax READ currentLayerMax NOTIFY stateChanged)
  Q_PROPERTY(int moveCount READ moveCount NOTIFY stateChanged)
  Q_PROPERTY(int currentMove READ currentMove NOTIFY stateChanged)
  Q_PROPERTY(QByteArray gcodePreviewData READ gcodePreviewData NOTIFY stateChanged)
  Q_PROPERTY(QVariantList legendItems READ legendItems NOTIFY stateChanged)
  Q_PROPERTY(QString totalTime READ totalTime NOTIFY stateChanged)
  Q_PROPERTY(QString filamentUsed READ filamentUsed NOTIFY stateChanged)
  Q_PROPERTY(QStringList viewModes READ viewModes CONSTANT)
  Q_PROPERTY(int viewModeIndex READ viewModeIndex WRITE setViewModeIndex NOTIFY stateChanged)

public:
  explicit PreviewViewModel(SliceService *sliceService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  QString estimatedTime() const;
  int layerCount() const { return layerCount_; }
  int currentLayerMin() const { return currentLayerMin_; }
  int currentLayerMax() const { return currentLayerMax_; }
  int moveCount() const { return moveCount_; }
  int currentMove() const { return currentMove_; }
  const QByteArray &gcodePreviewData() const { return gcodePreviewData_; }
  QVariantList legendItems() const { return legendItems_; }
  QString totalTime() const { return totalTime_; }
  QString filamentUsed() const { return filamentUsed_; }
  QStringList viewModes() const;
  int viewModeIndex() const { return viewModeIndex_; }

  Q_INVOKABLE void setLayerRange(int minLayer, int maxLayer);
  Q_INVOKABLE void setCurrentMove(int move);
  Q_INVOKABLE void playAnimation();
  Q_INVOKABLE void pauseAnimation();
  Q_INVOKABLE void setViewModeIndex(int index);

signals:
  void stateChanged();

private:
  void rebuildFromGCode(const QString &filePath);
  QVariantMap legendItem(const QString &label, const QString &color, int count) const;

  SliceService *sliceService_ = nullptr;
  QString estimatedTime_ = QStringLiteral("--:--:--");
  int layerCount_ = 0;
  int currentLayerMin_ = 0;
  int currentLayerMax_ = 0;
  int moveCount_ = 0;
  int currentMove_ = 0;
  QByteArray gcodePreviewData_;
  QVariantList legendItems_;
  QString totalTime_ = QStringLiteral("--:--:--");
  QString filamentUsed_ = QStringLiteral("--");
  int viewModeIndex_ = 0;
  QTimer *playTimer_ = nullptr;
};
