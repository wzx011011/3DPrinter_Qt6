#include "SliceServiceMock.h"

#include <QTimer>

SliceServiceMock::SliceServiceMock(QObject *parent)
    : QObject(parent)
{
  timer_ = new QTimer(this);
  timer_->setInterval(80);
  connect(timer_, &QTimer::timeout, this, [this]()
          {
        progress_ += 2;
        if (progress_ > 100) {
            progress_ = 100;
        }
        emit progressChanged();

        if (progress_ >= 100) {
            timer_->stop();
            slicing_ = false;
            emit slicingChanged();
            emit sliceFinished(QStringLiteral("01:42:16"));
        } });
}

int SliceServiceMock::progress() const
{
  return progress_;
}

bool SliceServiceMock::slicing() const
{
  return slicing_;
}

void SliceServiceMock::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);
  progress_ = 0;
  emit progressChanged();
  slicing_ = true;
  emit slicingChanged();
  timer_->start();
}
