#pragma once

#include <QObject>
#include <QRunnable>
#include <atomic>

/// Abstract base class for background jobs (对齐上游 GUI::Job).
/// Subclass this and override process() to implement a job.
/// Use QThreadPool (via JobManager) to run jobs in background threads.
///
/// Usage:
///   auto *job = new MyJob(params);
///   job->setAutoDelete(true);
///   JobManager::instance().enqueue(job);
class JobBase : public QObject, public QRunnable
{
  Q_OBJECT

public:
  explicit JobBase(QObject *parent = nullptr);
  ~JobBase() override = default;

  /// Job status codes (对齐上游 Job::Ctl::update_status)
  enum Status {
    Pending = 0,       ///< 等待执行
    Running = 1,       ///< 正在执行
    Finished = 100,    ///< 成功完成
    Canceled = -1,     ///< 被用户取消
    Error = -2         ///< 执行出错
  };

  /// Unique job name for UI display (对齐上游 Job name)
  QString jobName() const { return m_jobName; }
  void setJobName(const QString &name) { m_jobName = name; }

  /// Current progress percent (0-100)
  int progress() const { return m_progress.load(); }

  /// Current status message
  QString statusText() const { return m_statusText; }

  /// Check if job has been canceled
  bool isCanceled() const { return m_canceled.load(); }

  /// Request cancellation (对齐上游 Job::Ctl::was_canceled)
  void cancel() { m_canceled.store(true); }

  /// Run entry point — calls process() and handles completion.
  /// Do NOT override this; override process() instead.
  void run() override final;

signals:
  /// Progress updated (percent 0-100, status text)
  void progressUpdated(int percent, const QString &statusText);

  /// Job completed successfully
  void finished();

  /// Job failed with error
  void failed(const QString &errorMessage);

  /// Job was canceled
  void canceled();

protected:
  /// Override this to implement the job logic.
  /// Call updateProgress() periodically from within this method.
  /// Check isCanceled() to support early termination.
  virtual void process() = 0;

  /// Optional cleanup on main thread after process() completes.
  /// Called via QMetaObject::invokeMethod (QueuedConnection).
  virtual void finalize() {}

  /// Update progress from within process()
  void updateProgress(int percent, const QString &text = {});

  /// Report an error (aborts the job)
  void reportError(const QString &message);

private:
  QString m_jobName;
  std::atomic<int> m_progress{0};
  std::atomic<bool> m_canceled{false};
  QString m_statusText;
};
