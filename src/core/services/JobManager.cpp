#include "JobManager.h"
#include "JobBase.h"

JobManager &JobManager::instance()
{
  static JobManager mgr;
  return mgr;
}

JobManager::JobManager(QObject *parent)
    : QObject(parent)
{
  // Default: use available cores, capped at 4 to avoid overwhelming system
  m_pool.setMaxThreadCount(qMin(QThreadPool::globalInstance()->maxThreadCount(), 4));
}

void JobManager::enqueue(JobBase *job)
{
  if (!job)
    return;

  m_activeJobs.fetch_add(1);
  m_currentJobName = job->jobName();
  m_currentJobProgress = 0;
  emit activeJobCountChanged();

  connect(job, &JobBase::progressUpdated, this,
          [this](int percent, const QString &text)
  {
    m_currentJobProgress = percent;
    if (!text.isEmpty())
      m_currentJobName = text;
    emit currentJobProgressChanged(percent, text);
  });

  connect(job, &JobBase::finished, this,
          [this, job]()
  {
    m_activeJobs.fetch_sub(1);
    emit jobFinished(job->jobName());
    emit activeJobCountChanged();
  });

  connect(job, &JobBase::failed, this,
          [this, job](const QString &error)
  {
    m_activeJobs.fetch_sub(1);
    emit jobFailed(job->jobName(), error);
    emit activeJobCountChanged();
  });

  connect(job, &JobBase::canceled, this,
          [this, job]()
  {
    m_activeJobs.fetch_sub(1);
    emit activeJobCountChanged();
  });

  job->setAutoDelete(false);
  m_pool.start(job);
}

void JobManager::cancelAll()
{
  m_pool.clear(); // Remove pending jobs from queue
  // Active jobs will check isCanceled() in their process() loop
}

void JobManager::cancelJob(const QString &jobName)
{
  Q_UNUSED(jobName)
  // For named cancellation, individual jobs would need tracking.
  // cancelAll() is the primary mechanism for now.
  cancelAll();
}

int JobManager::maxThreads() const
{
  return m_pool.maxThreadCount();
}

void JobManager::setMaxThreads(int count)
{
  m_pool.setMaxThreadCount(qMax(1, count));
}
