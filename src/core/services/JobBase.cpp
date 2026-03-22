#include "JobBase.h"

#include <QMetaObject>

JobBase::JobBase(QObject *parent)
    : QObject(parent), QRunnable()
{
}

void JobBase::run()
{
  try
  {
    updateProgress(0, QObject::tr("Starting %1...").arg(m_jobName));
    process();
    if (isCanceled())
    {
      QMetaObject::invokeMethod(this, [this]() { emit canceled(); }, Qt::QueuedConnection);
    }
    else
    {
      QMetaObject::invokeMethod(this, [this]()
      {
        finalize();
        emit finished();
      }, Qt::QueuedConnection);
    }
  }
  catch (const std::exception &e)
  {
    QMetaObject::invokeMethod(this, [this, msg = QString::fromUtf8(e.what())]()
    { emit failed(msg); }, Qt::QueuedConnection);
  }
  catch (...)
  {
    QMetaObject::invokeMethod(this, [this]()
    { emit failed(QObject::tr("Unknown error")); }, Qt::QueuedConnection);
  }
}

void JobBase::updateProgress(int percent, const QString &text)
{
  m_progress.store(qBound(0, percent, 100));
  if (!text.isNull())
    m_statusText = text;
  // Use QueuedConnection for thread-safe signal emission
  QMetaObject::invokeMethod(this, [this, p = m_progress.load(), t = m_statusText]()
  { emit progressUpdated(p, t); }, Qt::QueuedConnection);
}

void JobBase::reportError(const QString &message)
{
  m_progress.store(Error);
  QMetaObject::invokeMethod(this, [this, msg = message]()
  { emit failed(msg); }, Qt::QueuedConnection);
}
