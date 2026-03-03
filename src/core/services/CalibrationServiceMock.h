#pragma once
#include <QObject>

class QTimer;

// Mock calibration service — simulates progress ticking 0→100 over ~20s
// Replace with real CalibrationService when engine is integrated
class CalibrationServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)

public:
  explicit CalibrationServiceMock(QObject *parent = nullptr);
  ~CalibrationServiceMock() override;

  int progress() const { return m_progress; }
  bool isRunning() const { return m_isRunning; }

  Q_INVOKABLE void startCalibration(int itemIndex);
  Q_INVOKABLE void cancelCalibration();

signals:
  void progressChanged();
  void isRunningChanged();
  void calibrationFinished(bool success);

private slots:
  void onTick();

private:
  int m_progress = 0;
  bool m_isRunning = false;
  int m_currentItem = -1;
  QTimer *m_timer = nullptr;
};
