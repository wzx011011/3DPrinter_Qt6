#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QNetworkAccessManager;
class QNetworkReply;

/// ModelDownloadService (feature/cloud-model-library, M4).
///
/// Native downloader behind the model-library bridge's
/// `3mf_download_start` / `models_download_start` commands — the Qt port of
/// the official CrealityPrint ModelDownloader, simplified per the M4 plan:
/// - No 200ms polling / cloud_download_data.json cache: progress is a
///   QNetworkReply::downloadProgress signal driving QML directly; the
///   download-history JSON and resume support are a later milestone.
/// - Storage layout parity: <root>/<userId>/models/<modelId>/<name><ext> and
///   <root>/<userId>/3mfs/<name>, duplicate names become "name (n).ext"
///   (upstream ModelDownloader.cpp:194-236 rule).
/// - Progress reporting per fileId plus an aggregate for the QML dialog.
///
/// Completion does NOT import by itself — ownership of the "close dialog,
/// then import" ordering stays with BackendContext (upstream
/// CloudDownloadProgressDialog closes before CallAfter-ing the import).
class ModelDownloadService final : public QObject
{
  Q_OBJECT
  /// Number of in-flight downloads.
  Q_PROPERTY(int activeCount READ activeCount NOTIFY stateChanged)
  /// Aggregate progress (0-100) of the most recently started active
  /// download — the QML dialog shows one row per official parity; the
  /// aggregate keeps multi-file model groups honest without a list UI.
  Q_PROPERTY(int progressPercent READ progressPercent NOTIFY progressChanged)
  /// Display name of the most recently started download.
  Q_PROPERTY(QString currentName READ currentName NOTIFY stateChanged)
  /// True while at least one download is running.
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)

public:
  explicit ModelDownloadService(QObject *parent = nullptr);
  ~ModelDownloadService() override;

  /// Directory files are written under (AppSettings modelDownloadPath).
  void setDownloadRoot(const QString &root) { downloadRoot_ = root; }
  QString downloadRoot() const { return downloadRoot_; }

  /// Start one file download (bridge command surface).
  Q_INVOKABLE void startDownload(const QString &url,
                                 const QString &userId,
                                 const QString &modelId,
                                 const QString &fileId,
                                 const QString &fileFormat,
                                 const QString &name);
  /// Cancel one in-flight download by fileId (no-op when unknown).
  Q_INVOKABLE void cancel(const QString &fileId);
  /// Cancel the most recently started download (QML dialog cancel button).
  Q_INVOKABLE void cancelLatest();
  /// Reply payload for `models_download_state` / `3mfs_download_state`:
  /// {models:[{fileId,progress,path,…}],3mfs:[…]} — subset upstream parity.
  Q_INVOKABLE QVariantMap downloadState() const;
  /// Drop one finished entry from the tracked history (upstream
  /// models_download_delete / 3mfs_download_delete). Removes nothing on disk
  /// beyond the tracked record (the mall re-downloads on demand).
  Q_INVOKABLE void forgetDownload(const QString &fileId);

  int activeCount() const { return int(active_.size()); }
  int progressPercent() const { return latestProgress_; }
  QString currentName() const { return latestName_; }
  bool busy() const { return !active_.isEmpty(); }

signals:
  /// One download settled successfully (file on disk at finalPath). Emitted
  /// for 3MF and model files alike; BackendContext decides the import.
  void downloadFinished(const QString &fileId, const QString &finalPath);
  void downloadFailed(const QString &fileId, const QString &error);
  void downloadCanceled(const QString &fileId);
  void stateChanged();
  void progressChanged();

private:
  struct ActiveDownload
  {
    QNetworkReply *reply = nullptr;
    QString name;
    QString intendedPath;   // target name (dup rule re-applied at finish)
    QString partPath;       // unique per fileId — parallel same-name
                            // downloads never share a temp file
    qint64 received = 0;
    qint64 total = 0;
  };


  QNetworkAccessManager *manager_ = nullptr;
  QString downloadRoot_;
  QHash<QString, ActiveDownload> active_;
  QHash<QString, QString> finishedPaths_; // fileId → final path (history)
  int latestProgress_ = 0;
  QString latestName_;
  QString latestFileId_;
};
