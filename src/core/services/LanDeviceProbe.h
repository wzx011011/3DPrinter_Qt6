#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSslConfiguration>
#include <QString>
#include <QVariantMap>

/// LanDeviceProbe (feature/lan-printers, L1).
///
/// HTTP(S) /info probe for LAN Creality printers — Qt port of the official
/// open-source LanDeviceProbe (CrealityPrint print_manage/Device/
/// LanDeviceProbe.cpp):
/// - Modern devices answer `GET https://IP:443/info` (TLS with the vendor CA;
///   host name and validity are NOT verified — LAN semantics) and must
///   include `model`, `mac`, `wssPort`, `videoPort` (+ optional `videoToken`);
/// - Fallback `GET http://IP:80/info`; legacy devices answer with an `ssid`
///   field instead (oldPrinter);
/// - Anything missing `model`/`mac` is rejected as invalid.
///
/// The parser is a pure static function (unit-test entry); network access is
/// via an injectable QNetworkAccessManager so tests run against a loopback
/// stub. No external traffic in tests.
class LanDeviceProbe final : public QObject
{
  Q_OBJECT

public:
  struct ProbeResult
  {
    bool ok = false;
    bool secureConnection = false; // answered via HTTPS
    bool oldPrinter = false;       // legacy firmware (ssid field)
    QString address;
    QString model;
    QString mac;
    int wssPort = 0;
    int videoPort = 0;
    QString videoToken;
    QJsonObject rawInfo;           // full /info body (upstream raw_info parity)
    QString errorString;
  };

  using ProbeCallback = std::function<void(const ProbeResult &)>;

  explicit LanDeviceProbe(QObject *parent = nullptr);
  /// Injectable manager for tests (loopback stub); ownership stays with the
  /// caller.
  explicit LanDeviceProbe(QNetworkAccessManager *manager, QObject *parent = nullptr);
  ~LanDeviceProbe() override;

  /// Probe one device. Emits probeFinished() and invokes \p callback exactly
  /// once. HTTPS first (with the D7-lenient TLS profile), HTTP fallback.
  Q_INVOKABLE void probe(const QString &ip, ProbeCallback callback);
  /// Convenience overload for QML (result also delivered via probeFinished).
  Q_INVOKABLE void probe(const QString &ip) { probe(ip, {}); }

  /// Pure /info body validation & extraction (unit-test entry).
  static ProbeResult parseInfoBody(const QByteArray &body, bool secure,
                                   const QString &address);
  /// Test hook: skip the HTTPS attempt entirely and go straight to the HTTP
  /// fallback (the test binary mixes paho-mqtt static symbols with the
  /// system libcrypto — a real TLS handshake there crashes in OpenSSL).
  /// Never set by the shipping app.
  Q_INVOKABLE void setHttpsDisabled(bool disabled) { httpsDisabled_ = disabled; }

  /// The D7-lenient TLS profile: vendor CA accepted, host name and validity
  /// checks off (upstream LanDeviceProbe.cpp:146-149 parity). Exposed for
  /// tests to assert the policy shape.
  static QSslConfiguration lenientTlsConfiguration();

signals:
  /// Emitted for both QML callers and the callback path.
  void probeFinished(const QVariantMap &result);

private:
  void runHttpFallback(const QString &ip, const ProbeCallback &callback);
  void deliver(const ProbeResult &result, const ProbeCallback &callback);

  QNetworkAccessManager *manager_ = nullptr;
  bool ownsManager_ = false;
  bool httpsDisabled_ = false;
};
