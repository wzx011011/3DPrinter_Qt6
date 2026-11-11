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

/// C5 (review 2026-11-03): every path component below comes straight from
/// the mall page's WebChannel message. Reduce each to a single, harmless
/// file-name component: no separators, drive colons, wildcard/reserved
/// characters or control characters, no leading/trailing dots or spaces (so
/// "." / ".." cannot survive), length-capped. Returns `fallback` when nothing
/// is left.
QString safeComponent(const QString &raw, const QString &fallback)
{
    static const QString kReserved = QStringLiteral("/\\:*?\"<>|");
    QString out;
    out.reserve(raw.size());
    for (const QChar c : raw) {
        if (c.unicode() < 0x20 || kReserved.contains(c))
            out.append(QLatin1Char('_'));
        else
            out.append(c);
    }
    // Interior ".." must also die: "..\..\m" becomes ".._.._m" after the
    // reserved-char pass, and the leading-dot strip cannot reach the middle
    // run. Collapse every dot-dot run anywhere in the component.
    while (out.contains(QLatin1String("..")))
        out.replace(QLatin1String(".."), QLatin1String("_"));
    while (!out.isEmpty() && (out.front() == QLatin1Char('.') || out.front().isSpace()))
        out.remove(0, 1);
    while (!out.isEmpty() && (out.back() == QLatin1Char('.') || out.back().isSpace()))
        out.chop(1);
    if (out.size() > 120)
        out.truncate(120);
    return out.isEmpty() ? fallback : out;
}

/// Model files only: a short ASCII alphanumeric extension that is not an
/// executable/script type Windows would run on open.
bool isAllowedExtension(const QString &ext)
{
    static const QStringList kBlocked = {
        QStringLiteral("exe"), QStringLiteral("bat"), QStringLiteral("cmd"),
        QStringLiteral("com"), QStringLiteral("ps1"), QStringLiteral("vbs"),
        QStringLiteral("vbe"), QStringLiteral("js"),  QStringLiteral("jse"),
        QStringLiteral("wsf"), QStringLiteral("msi"), QStringLiteral("msp"),
        QStringLiteral("lnk"), QStringLiteral("scr"), QStringLiteral("dll"),
        QStringLiteral("reg"), QStringLiteral("hta"), QStringLiteral("jar"),
        QStringLiteral("cpl"), QStringLiteral("pif"), QStringLiteral("url")};
    if (ext.isEmpty() || ext.size() > 8)
        return false;
    for (const QChar c : ext)
        if (c.unicode() >= 0x80 || !c.isLetterOrNumber())
            return false;
    return !kBlocked.contains(ext.toLower());
}

/// https everywhere; plain http only to the loopback host (local stubs).
bool isAllowedUrl(const QUrl &url)
{
    if (!url.isValid())
        return false;
    const QString scheme = url.scheme().toLower();
    if (scheme == QStringLiteral("https"))
        return true;
    const QString host = url.host().toLower();
    return scheme == QStringLiteral("http")
           && (host == QStringLiteral("127.0.0.1") || host == QStringLiteral("localhost"));
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

    const QUrl sourceUrl(url);
    if (!isAllowedUrl(sourceUrl)) {
        emit downloadFailed(fileId, QStringLiteral("refused download url (https required)"));
        return;
    }
    QString format = fileFormat;
    while (format.startsWith(QLatin1Char('.')))
        format.remove(0, 1);
    format = format.toLower();
    if (!isAllowedExtension(format)) {
        emit downloadFailed(fileId, QStringLiteral("refused file format: %1").arg(fileFormat));
        return;
    }

    // Storage layout: <root>/<userId>/models/<modelId>/<name><ext> and
    // <root>/<userId>/3mfs/<name> (upstream ModelDownloader.cpp:444-481).
    // Every component is sanitised (C5) and the result must stay under the
    // download root.
    const QString ext = QStringLiteral(".") + format;
    const QString safeUser = safeComponent(userId, QStringLiteral("anonymous"));
    const QString safeModel = safeComponent(modelId, QStringLiteral("misc"));
    const QString safeName = safeComponent(name, QStringLiteral("model"));
    const QString safeFileId = safeComponent(fileId, QStringLiteral("download"));
    QDir rootDir(downloadRoot_);
    QString directory;
    if (ext == QStringLiteral(".3mf")) {
        directory = rootDir.filePath(safeUser + QStringLiteral("/3mfs"));
    } else {
        directory = rootDir.filePath(safeUser + QStringLiteral("/models/") + safeModel);
    }
    // The dup-rule numbering happens at COMPLETION (upstream semantics);
    // at start we only reserve a per-fileId unique temp name so parallel
    // same-name downloads cannot collide on the .part file.
    const QString intendedPath =
        QDir::cleanPath(QFileInfo(directory + QStringLiteral("/") + safeName + ext).absoluteFilePath());
    const QString rootPath = QDir::cleanPath(QFileInfo(downloadRoot_).absoluteFilePath());
    if (!intendedPath.startsWith(rootPath + QLatin1Char('/'))) {
        emit downloadFailed(fileId, QStringLiteral("refused download path"));
        return;
    }
    QDir().mkpath(directory);
    const QString partPath = intendedPath + QStringLiteral(".") + safeFileId
                             + QStringLiteral(".part");

    QFile *sink = new QFile(partPath);
    if (!sink->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString reason = sink->errorString();
        delete sink;
        emit downloadFailed(fileId, QStringLiteral("cannot write %1: %2").arg(partPath, reason));
        return;
    }

    QNetworkRequest request{sourceUrl};
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
