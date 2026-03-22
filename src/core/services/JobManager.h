#pragma once

#include <QObject>
#include <QThreadPool>

class JobBase;

/// Job manager wrapping QThreadPool (对齐上游 GUI::Worker).
/// Provides job queue management, cancellation, and state tracking.
///
/// Usage:
///   JobManager::instance().enqueue(new MyJob());
///   JobManager::instance().cancelAll();
class JobManager final : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int activeJobCount READ activeJobCount NOTIFY activeJobCountChanged)
  Q_PROPERTY(bool hasActiveJobs READ hasActiveJobs NOTIFY activeJobCountChanged)
  Q_PROPERTY(QString currentJobName READ currentJobName NOTIFY activeJobCountChanged)
  Q_PROPERTY(int currentJobProgress READ currentJobProgress NOTIFY currentJobProgressChanged)

public:
  static JobManager &instance();

  /// Enqueue a job for background execution.
  /// JobManager does NOT take ownership — job must have autoDelete or be managed elsewhere.
  void enqueue(JobBase *job);

  /// Cancel all active jobs
  Q_INVOKABLE void cancelAll();

  /// Cancel a specific job by name
  Q_INVOKABLE void cancelJob(const QString &jobName);

  int activeJobCount() const { return m_activeJobs.load(); }
  bool hasActiveJobs() const { return m_activeJobs.load() > 0; }
  QString currentJobName() const { return m_currentJobName; }
  int currentJobProgress() const { return m_currentJobProgress; }

  /// Maximum concurrent thread count
  int maxThreads() const;
  void setMaxThreads(int count);

signals:
  void activeJobCountChanged();
  void currentJobProgressChanged(int percent, const QString &jobName);
  void jobFinished(const QString &jobName);
  void jobFailed(const QString &jobName, const QString &error);

private:
  Q_DISABLE_COPY(JobManager)
  explicit JobManager(QObject *parent = nullptr);

  QThreadPool *pool() { return &m_pool; }

  QThreadPool m_pool;
  std::atomic<int> m_activeJobs{0};
  QString m_currentJobName;
  int m_currentJobProgress = 0;
};
