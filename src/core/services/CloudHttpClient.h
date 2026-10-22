#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include <functional>

/// CloudHttpClient (feature/cloud-model-library, M1).
///
/// Thin asynchronous wrapper over QNetworkAccessManager providing the two
/// shapes the Creality Cloud calls need: raw byte GET (downloads, preset
/// zips) and JSON requests with `__CXY_*` header injection. Every operation
/// reports exactly one normalized HttpResponse — QNetworkReply error codes
/// are folded into ok=false plus a human-readable errorString, so callers
/// never touch QNetworkReply.
///
/// No retry logic in v1: every cloud endpoint so far is either interactive
/// (the user retries) or idempotent-but-cheap. The download service (M4)
/// uses QNetworkReply directly because it needs progress signals.
///
/// The manager instance is injectable so tests can route requests to a
/// local QTcpServer stub (no external traffic in the test suite).
class CloudHttpClient final : public QObject
{
  Q_OBJECT

public:
  /// Normalized outcome of one request. errorCategory is one of
  /// "none" / "network" / "timeout" / "http" — QML-facing layers map the
  /// first three to a retryable-network-error UX and the last to a
  /// server-reported problem.
  struct HttpResponse
  {
    bool ok = false;
    int httpStatus = 0;
    QString errorCategory; // "none" | "network" | "timeout" | "http"
    QString errorString;
    QByteArray body;
  };

  using Callback = std::function<void(const HttpResponse &)>;
  using HeaderMap = QMap<QString, QString>;

  explicit CloudHttpClient(QObject *parent = nullptr);
  /// Takes ownership of a custom manager (tests: manager without network
  /// parent hierarchy). The client keeps sharing one manager for connection
  /// reuse, mirroring the upstream Slic3r::Http singleton behaviour.
  explicit CloudHttpClient(QNetworkAccessManager *manager, QObject *parent = nullptr);
  ~CloudHttpClient() override;

  /// Issue an HTTP GET. headers are appended verbatim (the `__CXY_*` set).
  void get(const QString &url, const HeaderMap &headers, Callback callback);
  /// Issue an HTTP POST with a JSON body (content-type application/json).
  void postJson(const QString &url, const HeaderMap &headers,
                const QJsonObject &body, Callback callback);
  /// Issue an HTTP GET that expects a JSON document (parsed into the
  /// response's bodyJson on success; parse failure → ok=false, category
  /// "http", errorString "invalid json").
  void getJson(const QString &url, const HeaderMap &headers, Callback callback);
  /// Convenience: headers as QVariantMap (the CrealityCloudConfig shape).
  static HeaderMap toHeaderMap(const QVariantMap &variantHeaders);

  /// Transfer timeout in milliseconds (default 15s, upstream REST parity).
  void setTransferTimeout(int ms) { transferTimeoutMs_ = ms; }
  int transferTimeout() const { return transferTimeoutMs_; }

private:
  HttpResponse execute(QNetworkAccessManager::Operation operation,
                       const QString &url, const HeaderMap &headers,
                       const QByteArray &body, const QString &contentType);
  void dispatch(const HttpResponse &response, const Callback &callback);

  QNetworkAccessManager *manager_ = nullptr;
  bool ownsManager_ = false;
  int transferTimeoutMs_ = 15000;
};
