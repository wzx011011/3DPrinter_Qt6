#include "ModelDownloadService.h"

#include <QDir>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {
/// Uniquify "<dir>/<baseName><suffix>" with the upstream "name (n)" rule
/// (ModelDownloader.cpp:194-236): the first occurrence keeps the plain name,
/// subsequent ones become "name (1)", "name (2)", …
QString ensureUnique(const QString &path)
{
    if (!QFileInfo::exists(path))
        return path;
    const QFileInfo info(path);
    const QString dir = info.absolutePath();
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty()
                               ? QString()
                               : QStringLiteral(".") + info.suffix();
    for (int n = 1;; ++n) {
        const QString candidate = dir + QStringLiteral("/") + base
                                  + QStringLiteral(" (%1)").arg(n) + suffix;
        if (!QFileInfo::exists(candidate))
            return candidate;
    }
}
} // namespace

ModelDownloadService::ModelDownloadService(QObject *parent)
    : QObject(parent), manager_(new QNetworkAccessManager(this))
{
}

ModelDownloadService::~ModelDownloadService() = default;

void ModelDownloadService::startDownload(const QString &url,
                                         const QString &userId,
                                         const QString &modelId,
                                         const QString &fileId,
                                         const QString &fileFormat,
                                         const QString &name)
{
    if (fileId.isEmpty() || url.isEmpty()) {
        emit downloadFailed(fileId, QStringLiteral("empty url or fileId"));
        return;
    }
    if (active_.contains(fileId)) {
        emit downloadFailed(fileId, QStringLiteral("download already running"));
        return;
    }

    // Storage layout: <root>/<userId>/models/<modelId>/<name><ext> and
    // <root>/<userId>/3mfs/<name> (upstream ModelDownloader.cpp:444-481).
    const QString ext = fileFormat.startsWith(QStringLiteral("."))
                            ? fileFormat
                            : (fileFormat.isEmpty()
                                   ? QString()
                                   : QStringLiteral(".") + fileFormat);
    QDir rootDir(downloadRoot_);
    QString directory;
    if (ext == QStringLiteral(".3mf")) {
        directory = rootDir.filePath(userId + QStringLiteral("/3mfs"));
    } else {
        directory = rootDir.filePath(userId + QStringLiteral("/models/")
                                     + (modelId.isEmpty() ? QStringLiteral("misc") : modelId));
    }
    QDir().mkpath(directory);
    // The dup-rule numbering happens at COMPLETION (upstream semantics);
    // at start we only reserve a per-fileId unique temp name so parallel
    // same-name downloads cannot collide on the .part file.
    const QString intendedPath = directory + QStringLiteral("/") + name + ext;
    const QString partPath = intendedPath + QStringLiteral(".") + fileId
                             + QStringLiteral(".part");

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager_->get(request);

    ActiveDownload entry;
    entry.reply = reply;
    entry.name = name;
    entry.intendedPath = intendedPath;
    entry.partPath = partPath;
    active_.insert(fileId, entry);
    latestFileId_ = fileId;
    latestName_ = name;
    latestProgress_ = 0;
    emit stateChanged();
    emit progressChanged();

    QFile *sink = new QFile(partPath);
    sink->open(QIODevice::WriteOnly | QIODevice::Truncate);

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, fileId](qint64 received, qint64 total) {
                auto it = active_.find(fileId);
                if (it == active_.end())
                    return;
                it->received = received;
                it->total = total;
                if (total > 0) {
                    latestProgress_ = int(received * 100 / total);
                    emit progressChanged();
                }
            });
    connect(reply, &QNetworkReply::readyRead, this, [this, fileId, sink, reply]() {
        sink->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, fileId, sink]() {
        sink->close();
        sink->deleteLater();
        auto it = active_.find(fileId);
        if (it == active_.end())
            return;
        const QString err = it->reply->error() == QNetworkReply::NoError
                                ? QString()
                                : it->reply->errorString();
        const bool canceled =
            it->reply->error() == QNetworkReply::OperationCanceledError;
        // Copy the fields out BEFORE erasing the entry (the erase
        // invalidates the iterator).
        const QString partPath = it->partPath;
        const QString intendedPath = it->intendedPath;
        it->reply->deleteLater();
        active_.erase(it);

        if (canceled) {
            QFile::remove(partPath);
            emit downloadCanceled(fileId);
            emit stateChanged();
            return;
        }
        if (!err.isEmpty()) {
            QFile::remove(partPath);
            emit downloadFailed(fileId, err);
            emit stateChanged();
            return;
        }
        // .part → final: the dup-rule (n) numbering is applied now, by
        // completion order (upstream ModelDownloader semantics).
        const QString settled = ensureUnique(intendedPath);
        if (!QFile::rename(partPath, settled)) {
            emit downloadFailed(fileId, QStringLiteral("rename failed"));
            emit stateChanged();
            return;
        }
        finishedPaths_.insert(fileId, settled);
        latestProgress_ = 100;
        emit progressChanged();
        emit downloadFinished(fileId, settled);
        emit stateChanged();
    });
}

void ModelDownloadService::cancel(const QString &fileId)
{
    auto it = active_.find(fileId);
    if (it == active_.end())
        return;
    // abort() → OperationCanceledError → the finished lambda cleans up.
    it->reply->abort();
}

void ModelDownloadService::cancelLatest()
{
    cancel(latestFileId_);
}

QVariantMap ModelDownloadService::downloadState() const
{
    QVariantList models;
    QVariantList mfs;
    for (auto it = active_.constBegin(); it != active_.constEnd(); ++it) {
        QVariantMap row;
        row.insert(QStringLiteral("fileId"), it.key());
        row.insert(QStringLiteral("name"), it->name);
        row.insert(QStringLiteral("path"), it->intendedPath);
        row.insert(QStringLiteral("progress"),
                   it->total > 0 ? int(it->received * 100 / it->total) : 0);
        (it->intendedPath.endsWith(QStringLiteral(".3mf")) ? mfs : models).append(row);
    }
    for (auto it = finishedPaths_.constBegin(); it != finishedPaths_.constEnd(); ++it) {
        QVariantMap row;
        row.insert(QStringLiteral("fileId"), it.key());
        row.insert(QStringLiteral("path"), it.value());
        row.insert(QStringLiteral("progress"), 100);
        (it.value().endsWith(QStringLiteral(".3mf")) ? mfs : models).append(row);
    }
    QVariantMap state;
    state.insert(QStringLiteral("models"), models);
    state.insert(QStringLiteral("3mfs"), mfs);
    return state;
}

void ModelDownloadService::forgetDownload(const QString &fileId)
{
    if (finishedPaths_.remove(fileId) > 0)
        emit stateChanged();
}
