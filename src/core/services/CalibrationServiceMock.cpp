#include "CalibrationServiceMock.h"
#include <QTimer>

CalibrationServiceMock::CalibrationServiceMock(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
  m_timer->setInterval(200); // tick every 200 ms → reaches 100 in ~20 s
  connect(m_timer, &QTimer::timeout, this, &CalibrationServiceMock::onTick);
}

CalibrationServiceMock::~CalibrationServiceMock() = default;

void CalibrationServiceMock::startCalibration(int itemIndex)
{
  if (m_isRunning)
    return;
  m_currentItem = itemIndex;
  m_progress = 0;
  m_isRunning = true;
  emit isRunningChanged();
  emit progressChanged();
  m_timer->start();
}

void CalibrationServiceMock::cancelCalibration()
{
  if (!m_isRunning)
    return;
  m_timer->stop();
  m_isRunning = false;
  m_progress = 0;
  emit isRunningChanged();
  emit progressChanged();
}

void CalibrationServiceMock::onTick()
{
  m_progress += 5;
  emit progressChanged();
  if (m_progress >= 100)
  {
    m_progress = 100;
    m_isRunning = false;
    m_timer->stop();
    emit isRunningChanged();
    emit calibrationFinished(true);
  }
}
