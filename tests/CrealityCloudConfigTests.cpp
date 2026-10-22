#include "core/services/CloudHttpClient.h"
#include "core/services/CrealityCloudConfig.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork>
#include <QtTest>

#include <atomic>

/// One-shot loopback HTTP stub for the CloudHttpClient tests. Accepts a
/// single connection on 127.0.0.1, captures the raw request bytes, then
/// either answers with a canned response or (holdOpen) stays silent to
/// exercise the transfer timeout. No external traffic.
class LoopbackHttpStub : public QObject
{
  Q_OBJECT
public:
  QString lastRequest;
  QByteArray responseBody = "{\"ok\":true}";
  int responseStatus = 200;
  bool holdOpen = false;

  bool start()
  {
    if (!server_.listen(QHostAddress::LocalHost))
      return false;
    connect(&server_, &QTcpServer::newConnection, this, [this]() {
      QTcpSocket *socket = server_.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        lastRequest += socket->readAll();
        if (!lastRequest.contains("\r\n\r\n"))
          return; // headers not complete yet
        if (holdOpen)
          return; // never answer → client transfer timeout
        const QByteArray head = "HTTP/1.1 " + QByteArray::number(responseStatus)
                                + " test\r\nContent-Type: application/json\r\nContent-Length: "
                                + QByteArray::number(responseBody.size()) + "\r\nConnection: close\r\n\r\n";
        socket->write(head + responseBody);
        socket->disconnectFromHost();
      });
      connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });
    return true;
  }
  int port() const { return server_.serverPort(); }

private:
  QTcpServer server_;
};

class CrealityCloudConfigTests : public QObject
{
  Q_OBJECT

private slots:
  // ── Endpoint matrix (upstream GUI.cpp:633-720) ───────────────────────
  void endpointMatrix()
  {
    using R = CrealityCloudConfig::Region;
    using C = CrealityCloudConfig::Channel;
    // Release: api.<tld>, cp.<tld>
    QCOMPARE(CrealityCloudConfig::apiUrl(R::CN, C::Release),
             QStringLiteral("https://api.crealitycloud.cn"));
    QCOMPARE(CrealityCloudConfig::apiUrl(R::International, C::Release),
             QStringLiteral("https://api.crealitycloud.com"));
    QCOMPARE(CrealityCloudConfig::webAddress(R::CN, C::Release),
             QStringLiteral("https://cp.crealitycloud.cn/"));
    QCOMPARE(CrealityCloudConfig::webAddress(R::International, C::Release),
             QStringLiteral("https://cp.crealitycloud.com/"));
    // Dev keeps the reference http scheme and CN-only hosts.
    QCOMPARE(CrealityCloudConfig::apiUrl(R::CN, C::Dev),
             QStringLiteral("http://api-dev.crealitycloud.cn"));
    QCOMPARE(CrealityCloudConfig::webAddress(R::CN, C::Dev),
             QStringLiteral("https://dev2.crealitycloud.cn/"));
    // Alpha rides the admin-pre hosts.
    QCOMPARE(CrealityCloudConfig::apiUrl(R::International, C::Alpha),
             QStringLiteral("https://admin-pre.crealitycloud.com"));
  }

  void modelLibraryPages()
  {
    using R = CrealityCloudConfig::Region;
    using C = CrealityCloudConfig::Channel;
    QCOMPARE(CrealityCloudConfig::modelLibraryHome(R::CN, C::Release),
             QStringLiteral("https://cp.crealitycloud.cn/model-category/3d-print-all"));
    QCOMPARE(CrealityCloudConfig::modelLibrarySearch(R::CN, C::Release, QStringLiteral("齿轮 box")),
             QStringLiteral("https://cp.crealitycloud.cn/search/models?q=%E9%BD%BF%E8%BD%AE%20box"));
  }

  // ── OAuth identity (upstream HttpServer.cpp) ─────────────────────────
  void oauthConstants()
  {
    using R = CrealityCloudConfig::Region;
    using C = CrealityCloudConfig::Channel;
    // Client ids carried verbatim from the open-source reference client.
    QCOMPARE(CrealityCloudConfig::oauthClientId(R::CN, C::Release),
             QStringLiteral("8ea5010984fa52a298f12110af8b05d0"));
    QCOMPARE(CrealityCloudConfig::oauthClientId(R::International, C::Release),
             QStringLiteral("f9c302ecc29c59a0a6e921ff39a073ca"));
    QCOMPARE(CrealityCloudConfig::oauthClientId(R::CN, C::Dev),
             QStringLiteral("e216beaa82e75f8f9d04b2ec822bcc48"));
    // Token exchange + redirect assembly.
    QCOMPARE(CrealityCloudConfig::oauthTokenUrl(R::CN, C::Release),
             QStringLiteral("https://api.crealitycloud.cn/api/cxy/account/v2/oauthLogin"));
    QCOMPARE(CrealityCloudConfig::oauthRedirectUri(R::CN, C::Release, 15888),
             QStringLiteral("https://www.crealitycloud.cn/oauth?back_url=http://localhost:15888/login"));
    QCOMPARE(CrealityCloudConfig::oauthRedirectUri(R::International, C::Release, 13666),
             QStringLiteral("https://www.crealitycloud.com/oauth?back_url=http://localhost:13666/login"));
  }

  // ── Header protocol (upstream get_extra_header / get_modellibrary_header) ──
  void extraHeadersShape()
  {
    const QVariantMap headers = CrealityCloudConfig::extraHeaders(
        QStringLiteral("tok"), QStringLiteral("uid42"), QStringLiteral("zh_CN"),
        QStringLiteral("2.4.0-dev"), QStringLiteral("AA:BB"), 28800);
    QCOMPARE(headers.value("__CXY_TOKEN_").toString(), QStringLiteral("tok"));
    QCOMPARE(headers.value("__CXY_UID_").toString(), QStringLiteral("uid42"));
    QCOMPARE(headers.value("__CXY_OS_LANG_").toString(), QStringLiteral("1"));
    QCOMPARE(headers.value("__CXY_APP_VER_").toString(), QStringLiteral("2.4.0-dev"));
    QCOMPARE(headers.value("__CXY_DUID_").toString(), QStringLiteral("AA:BB"));
    QCOMPARE(headers.value("__CXY_APP_ID_").toString(), QStringLiteral("creality_model"));
    QCOMPARE(headers.value("__CXY_BRAND_").toString(), QStringLiteral("creality"));
    QCOMPARE(headers.value("__CXY_APP_CH_").toString(), QStringLiteral("creality"));
    QCOMPARE(headers.value("__CXY_PLATFORM_").toString(), QStringLiteral("11"));
    QCOMPARE(headers.value("__CXY_TIMEZONE_").toString(), QStringLiteral("28800"));
    QCOMPARE(headers.value("__CXY_OS_VER_").toString(), QStringLiteral("linux"));
  }

  void modelLibraryHeadersShape()
  {
    const QVariantMap headers = CrealityCloudConfig::modelLibraryHeaders(
        {}, {}, QStringLiteral("en"), QStringLiteral("2.4.0-dev"), QStringLiteral("D1"));
    // Anonymous browsing sends empty token/uid (upstream parity).
    QCOMPARE(headers.value("__CXY_TOKEN_").toString(), QString());
    QCOMPARE(headers.value("__CXY_UID_").toString(), QString());
    QCOMPARE(headers.value("__CXY_OS_LANG_").toString(), QStringLiteral("0"));
    QCOMPARE(headers.value("__CXY_WEBVIEW_TYPE_").toString(), QStringLiteral("creality_print_slice"));
    // The two header builders must agree on every shared name.
    const QStringList names = CrealityCloudConfig::headerNames();
    QVERIFY(names.contains("__CXY_WEBVIEW_TYPE_"));
    QVERIFY(names.contains("__CXY_APP_ID_"));
  }

  void languageIndexMap()
  {
    // Exact entries from the upstream I18nToLangaugeIndex table.
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("en")), QStringLiteral("0"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("zh_cn")), QStringLiteral("1"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("zh_tw")), QStringLiteral("2"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("zh_hk")), QStringLiteral("3"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("ru_ru")), QStringLiteral("4"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("ko")), QStringLiteral("5"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("ja_jp")), QStringLiteral("10"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("fr_FR")), QStringLiteral("9"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("pt_br")), QStringLiteral("15"));
    // Normalization: '-'→'_', locale modifiers stripped, zh variants folded.
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("zh-Hans")), QStringLiteral("1"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("zh_CN.UTF-8")), QStringLiteral("1"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("en_GB@latin")), QStringLiteral("0"));
    // Primary-language fallback and English default for unknown codes.
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("de_AT")), QStringLiteral("8"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QStringLiteral("xx_yy")), QStringLiteral("0"));
    QCOMPARE(CrealityCloudConfig::languageIndex(QString()), QStringLiteral("0"));
  }

  void userAgentFormat()
  {
    const QString ua = CrealityCloudConfig::webViewUserAgent(QStringLiteral("2.4.0-dev"));
    // The remote frontend detects the shell via this exact marker set.
    QVERIFY(ua.contains(QStringLiteral("creality_print_slice/2.4.0-dev")));
    QVERIFY(ua.contains(QStringLiteral("WebView/141.0.0.0")));
    QVERIFY(ua.startsWith(QStringLiteral("Mozilla/5.0 (X11; Linux x86_64)")));
  }

  void duidDerivation()
  {
    // First usable MAC wins; all-zero placeholders are skipped; fallback
    // covers the no-MAC case (reference client ships "xxxmac" on Linux —
    // we fall back to a persisted UUID instead).
    QCOMPARE(CrealityCloudConfig::deriveDuid({QStringLiteral("AA:BB:CC:DD:EE:FF")},
                                             QStringLiteral("uuid-1")),
             QStringLiteral("AA:BB:CC:DD:EE:FF"));
    QCOMPARE(CrealityCloudConfig::deriveDuid({QStringLiteral("00:00:00:00:00:00"),
                                              QStringLiteral("11:22:33:44:55:66")},
                                             QStringLiteral("uuid-1")),
             QStringLiteral("11:22:33:44:55:66"));
    QCOMPARE(CrealityCloudConfig::deriveDuid({}, QStringLiteral("uuid-1")),
             QStringLiteral("uuid-1"));
    // Live candidates are non-loopback interface MACs (may be empty in a
    // container); whatever they are, they must not crash and must not be
    // all-zero.
    const QStringList live = CrealityCloudConfig::macAddressCandidates();
    for (const QString &mac : live)
      QVERIFY(mac != QStringLiteral("00:00:00:00:00:00"));
  }

  // ── CloudHttpClient against the loopback stub ────────────────────────
  void httpClientGetJson()
  {
    LoopbackHttpStub stub;
    QVERIFY(stub.start());
    CloudHttpClient client;
    QEventLoop loop;
    CloudHttpClient::HttpResponse captured;
    client.getJson(QStringLiteral("http://127.0.0.1:%1/api/x").arg(stub.port()),
                   {{QStringLiteral("__CXY_TOKEN_"), QStringLiteral("tok")}},
                   [&](const CloudHttpClient::HttpResponse &r) {
                     captured = r;
                     loop.quit();
                   });
    loop.exec();
    QVERIFY(captured.ok);
    QCOMPARE(captured.httpStatus, 200);
    QCOMPARE(captured.errorCategory, QStringLiteral("none"));
    QVERIFY(stub.lastRequest.startsWith("GET /api/x HTTP/1.1"));
    // Qt 6's HTTP backend normalizes custom header names to lowercase on the
    // wire (RFC 7230 case-insensitivity). The assertion follows the wire form.
    QVERIFY(stub.lastRequest.toLower().contains("__cxy_token_: tok"));
    QVERIFY(stub.lastRequest.contains("Host: 127.0.0.1"));
  }

  void httpClientPostJsonBody()
  {
    LoopbackHttpStub stub;
    QVERIFY(stub.start());
    CloudHttpClient client;
    QEventLoop loop;
    CloudHttpClient::HttpResponse captured;
    client.postJson(QStringLiteral("http://127.0.0.1:%1/login").arg(stub.port()), {},
                    QJsonObject{{"code", QStringLiteral("abc")}, {"clientId", QStringLiteral("cid")}},
                    [&](const CloudHttpClient::HttpResponse &r) {
                      captured = r;
                      loop.quit();
                    });
    loop.exec();
    QVERIFY(captured.ok);
    QVERIFY(stub.lastRequest.startsWith("POST /login HTTP/1.1"));
    QVERIFY(stub.lastRequest.contains("Content-Type: application/json"));
    const QJsonDocument sent = QJsonDocument::fromJson(
        stub.lastRequest.section("\r\n\r\n", 1).toUtf8());
    QCOMPARE(sent.object().value("code").toString(), QStringLiteral("abc"));
    QCOMPARE(sent.object().value("clientId").toString(), QStringLiteral("cid"));
  }

  void httpClientTimeout()
  {
    LoopbackHttpStub stub;
    stub.holdOpen = true;
    QVERIFY(stub.start());
    CloudHttpClient client;
    client.setTransferTimeout(300);
    QEventLoop loop;
    CloudHttpClient::HttpResponse captured;
    client.getJson(QStringLiteral("http://127.0.0.1:%1/slow").arg(stub.port()), {},
                   [&](const CloudHttpClient::HttpResponse &r) {
                     captured = r;
                     loop.quit();
                   });
    loop.exec();
    QVERIFY(!captured.ok);
    QCOMPARE(captured.errorCategory, QStringLiteral("timeout"));
  }

  void httpClientHttpError()
  {
    LoopbackHttpStub stub;
    stub.responseStatus = 401;
    stub.responseBody = "{\"error\":\"unauthorized\"}";
    QVERIFY(stub.start());
    CloudHttpClient client;
    QEventLoop loop;
    CloudHttpClient::HttpResponse captured;
    client.getJson(QStringLiteral("http://127.0.0.1:%1/secure").arg(stub.port()), {},
                   [&](const CloudHttpClient::HttpResponse &r) {
                     captured = r;
                     loop.quit();
                   });
    loop.exec();
    QVERIFY(!captured.ok);
    QCOMPARE(captured.errorCategory, QStringLiteral("http"));
    QCOMPARE(captured.httpStatus, 401);
    QVERIFY(!captured.errorString.isEmpty());
  }
};

QTEST_MAIN(CrealityCloudConfigTests)
#include "CrealityCloudConfigTests.moc"
