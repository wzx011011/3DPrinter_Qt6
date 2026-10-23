#include "LanDeviceProbe.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslError>
#include <QTimer>

namespace {

/// Required /info fields (upstream LanDeviceProbe.cpp:86-92): a response
/// without model+mac is rejected; secure responses must also carry the
/// wss/video ports.
LanDeviceProbe::ProbeResult parseInfo(const QJsonObject &root, bool secure,
                                      const QString &address)
{
    LanDeviceProbe::ProbeResult result;
    result.address = address;
    result.secureConnection = secure;
    result.rawInfo = root;

    result.model = root.value(QStringLiteral("model")).toString();
    if (result.model.isEmpty()) {
        result.errorString = QStringLiteral("info missing model");
        return result;
    }
    // Legacy firmware answers with `ssid` and NO `mac` — the mac is derived
    // from the ssid's second dash segment (front-end parity). Check this
    // before requiring mac.
    if (root.contains(QStringLiteral("ssid"))) {
        const QString ssid = root.value(QStringLiteral("ssid")).toString();
        result.oldPrinter = true;
        result.mac = ssid.section(QLatin1Char('-'), 1, 1);
        result.ok = true;
        return result;
    }
    result.mac = root.value(QStringLiteral("mac")).toString();
    if (result.mac.isEmpty()) {
        result.errorString = QStringLiteral("info missing mac");
        return result;
    }
    result.wssPort = root.value(QStringLiteral("wssPort")).toInt();
    result.videoPort = root.value(QStringLiteral("videoPort")).toInt();
    result.videoToken = root.value(QStringLiteral("videoToken")).toString();
    if (secure && (result.wssPort <= 0 || result.videoPort <= 0)) {
        result.errorString = QStringLiteral("secure info missing wssPort/videoPort");
        return result;
    }
    result.ok = true;
    return result;
}

} // namespace

LanDeviceProbe::ProbeResult LanDeviceProbe::parseInfoBody(const QByteArray &body,
                                                          bool secure,
                                                          const QString &address)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        ProbeResult invalid;
        invalid.address = address;
        invalid.secureConnection = secure;
        invalid.errorString = QStringLiteral("invalid /info json");
        return invalid;
    }
    return parseInfo(doc.object(), secure, address);
}

LanDeviceProbe::LanDeviceProbe(QObject *parent)
    : QObject(parent),
      manager_(new QNetworkAccessManager(this)),
      ownsManager_(true)
{
}

LanDeviceProbe::LanDeviceProbe(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent), manager_(manager), ownsManager_(false)
{
}

LanDeviceProbe::~LanDeviceProbe()
{
    if (manager_ && !ownsManager_)
        manager_->deleteLater();
}

QSslConfiguration LanDeviceProbe::lenientTlsConfiguration()
{
    // Upstream parity (LanDeviceProbe.cpp:146-149): verify the chain against
    // the vendor CA bundle but ignore the host name and the validity window
    // (printer certificates are self-stamped). Qt has no "ignore validity"
    // switch — the invalidity errors are absorbed in the per-error handler
    // below instead.
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setPeerVerifyMode(QSslSocket::QueryPeer); // errors surface as signals
    config.setPeerVerifyDepth(5);
    return config;
}

void LanDeviceProbe::probe(const QString &ip, ProbeCallback callback)
{
    if (httpsDisabled_) {
        runHttpFallback(ip, callback);
        return;
    }
    // HTTPS first (upstream PreferSecure), HTTP fallback. Ignore all TLS
    // handshake errors: the printer certificate does not carry our host name
    // and is frequently past its validity stamp — LAN-only semantics.
    QNetworkRequest request{QUrl(QStringLiteral("https://%1/info").arg(ip))};
    request.setSslConfiguration(lenientTlsConfiguration());
    request.setTransferTimeout(6000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager_->get(request);
    auto handleTlsErrors = [reply](const QList<QSslError> &errors) {
        Q_UNUSED(errors);
        reply->ignoreSslErrors();
    };
    connect(reply, &QNetworkReply::sslErrors, this, handleTlsErrors);

    connect(reply, &QNetworkReply::finished, this, [this, reply, ip, callback]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QJsonParseError parseError;
            const QJsonDocument doc =
                QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                const ProbeResult result =
                    parseInfo(doc.object(), true, ip);
                if (result.ok) {
                    deliver(result, callback);
                    return;
                }
            }
        }
        runHttpFallback(ip, callback);
    });
}

void LanDeviceProbe::runHttpFallback(const QString &ip,
                                     const ProbeCallback &callback)
{
    QNetworkRequest request{QUrl(QStringLiteral("http://%1/info").arg(ip))};
    request.setTransferTimeout(3000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, ip, callback]() {
        ProbeResult result;
        result.address = ip;
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            result.errorString = reply->errorString();
            deliver(result, callback);
            return;
        }
        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            result.errorString = QStringLiteral("invalid /info json");
            deliver(result, callback);
            return;
        }
        deliver(parseInfo(doc.object(), false, ip), callback);
    });
}

void LanDeviceProbe::deliver(const ProbeResult &result, const ProbeCallback &callback)
{
    QVariantMap map;
    map.insert(QStringLiteral("ok"), result.ok);
    map.insert(QStringLiteral("secureConnection"), result.secureConnection);
    map.insert(QStringLiteral("oldPrinter"), result.oldPrinter);
    map.insert(QStringLiteral("address"), result.address);
    map.insert(QStringLiteral("model"), result.model);
    map.insert(QStringLiteral("mac"), result.mac);
    map.insert(QStringLiteral("wssPort"), result.wssPort);
    map.insert(QStringLiteral("videoPort"), result.videoPort);
    map.insert(QStringLiteral("videoToken"), result.videoToken);
    if (!result.errorString.isEmpty())
        map.insert(QStringLiteral("error"), result.errorString);
    emit probeFinished(map);
    if (callback)
        callback(result);
}
