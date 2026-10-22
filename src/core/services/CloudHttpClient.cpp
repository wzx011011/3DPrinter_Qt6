#include "CloudHttpClient.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace {

/// Fold a settled QNetworkReply into the normalized response. Shared by the
/// async paths; never called before the reply emitted finished().
///
/// Category mapping notes (both verified against Qt 6.12 behaviour):
/// - The transfer timeout does NOT surface as OperationCanceledError on all
///   backends; it may arrive as a generic network error whose errorString
///   mentions the timeout, so the text is matched as a fallback.
/// - HTTP 401 triggers the authentication machinery and fails with
///   AuthenticationRequiredError even though a well-formed HTTP response
///   arrived — mapped to "http", not "network".
CloudHttpClient::HttpResponse finishReply(QNetworkReply *reply)
{
    CloudHttpClient::HttpResponse response;
    response.httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    const QNetworkReply::NetworkError error = reply->error();
    const QString errorText = reply->errorString();
    if (error == QNetworkReply::NoError) {
        response.ok = true;
        response.errorCategory = QStringLiteral("none");
    } else if (error == QNetworkReply::OperationCanceledError
               || errorText.contains(QStringLiteral("timed out"), Qt::CaseInsensitive)
               || errorText.contains(QStringLiteral("timeout"), Qt::CaseInsensitive)) {
        response.errorCategory = QStringLiteral("timeout");
        response.errorString = QStringLiteral("request timed out");
    } else if (response.httpStatus >= 400 || errorText.contains(QStringLiteral("authentication required"), Qt::CaseInsensitive)) {
        response.errorCategory = QStringLiteral("http");
        response.errorString = errorText.isEmpty()
                                   ? QStringLiteral("HTTP %1").arg(response.httpStatus)
                                   : errorText;
    } else {
        response.errorCategory = QStringLiteral("network");
        response.errorString = errorText;
    }
    if (reply->isOpen())
        response.body = reply->readAll();
    if (response.ok && response.httpStatus >= 400) {
        response.ok = false;
        response.errorCategory = QStringLiteral("http");
        response.errorString = QStringLiteral("HTTP %1").arg(response.httpStatus);
    }
    return response;
}

} // namespace

CloudHttpClient::CloudHttpClient(QObject *parent)
    : QObject(parent),
      manager_(new QNetworkAccessManager(this)),
      ownsManager_(true)
{
}

CloudHttpClient::CloudHttpClient(QNetworkAccessManager *manager, QObject *parent)
    : QObject(parent), manager_(manager), ownsManager_(false)
{
}

CloudHttpClient::~CloudHttpClient()
{
    // manager_ is child-managed only when we created it; an injected one
    // belongs to the caller (tests).
    if (manager_ && !ownsManager_)
        manager_->deleteLater();
}

CloudHttpClient::HeaderMap CloudHttpClient::toHeaderMap(const QVariantMap &variantHeaders)
{
    HeaderMap headers;
    for (auto it = variantHeaders.constBegin(); it != variantHeaders.constEnd(); ++it)
        headers.insert(it.key(), it.value().toString());
    return headers;
}

void CloudHttpClient::get(const QString &url, const HeaderMap &headers, Callback callback)
{
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(transferTimeoutMs_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());

    QNetworkReply *reply = manager_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() {
        HttpResponse response = finishReply(reply);
        reply->deleteLater();
        if (callback)
            callback(response);
    });
}

void CloudHttpClient::postJson(const QString &url, const HeaderMap &headers,
                               const QJsonObject &body, Callback callback)
{
    QNetworkRequest request{QUrl(url)};
    request.setTransferTimeout(transferTimeoutMs_);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    QNetworkReply *reply = manager_->post(request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback = std::move(callback)]() {
        HttpResponse response = finishReply(reply);
        reply->deleteLater();
        if (callback)
            callback(response);
    });
}

void CloudHttpClient::getJson(const QString &url, const HeaderMap &headers, Callback callback)
{
    get(url, headers, [callback = std::move(callback)](const HttpResponse &raw) {
        HttpResponse response = raw;
        if (response.ok) {
            QJsonParseError parseError;
            const QJsonDocument doc = QJsonDocument::fromJson(response.body, &parseError);
            if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                response.ok = false;
                response.errorCategory = QStringLiteral("http");
                response.errorString = QStringLiteral("invalid json: ") + parseError.errorString();
            }
        }
        if (callback)
            callback(response);
    });
}
