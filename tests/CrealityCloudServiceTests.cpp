#include "core/services/CloudServiceMock.h"
#include "core/services/CrealityAuthService.h"
#include "core/services/CrealityCloudService.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

#include <type_traits>

/// Loopback OAuth token-endpoint stub: answers one POST
/// /api/cxy/account/v2/oauthLogin with a canned envelope (upstream
/// HttpServer.cpp:930-941 shape: result.token / result.user_info.nickName /
/// result.userId). Records the received body for request-shape assertions.
class OAuthTokenStub : public QObject
{
  Q_OBJECT
public:
  QByteArray lastBody;
  bool failMode = false;

  bool start()
  {
    if (!server_.listen(QHostAddress::LocalHost))
      return false;
    connect(&server_, &QTcpServer::newConnection, this, [this]() {
      QTcpSocket *socket = server_.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        buffer_ += socket->readAll();
        const int splitPos = buffer_.indexOf("\r\n\r\n");
        if (splitPos == -1)
          return;
        // Extract the request body via Content-Length.
        const QByteArray head = buffer_.left(splitPos + 4);
        int contentLength = 0;
        for (const QByteArray &line : head.split('\n')) {
          if (line.toLower().startsWith("content-length:"))
            contentLength = line.mid(15).trimmed().toInt();
        }
        const QByteArray body = buffer_.mid(splitPos + 4);
        if (body.size() < contentLength)
          return; // wait for the rest
        lastBody = body;
        const char *payload = failMode
                                  ? "{\"code\":401,\"message\":\"bad code\"}"
                                  : "{\"code\":0,\"result\":{\"token\":\"tok-123\","
                                    "\"user_info\":{\"nickName\":\"创想用户\",\"avatar\":"
                                    "\"http://x/a.png\"},\"userId\":\"u-42\"}}";
        const QByteArray response = QByteArray("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ")
                                    + QByteArray::number(static_cast<qlonglong>(qstrlen(payload)))
                                    + "\r\nConnection: close\r\n\r\n" + payload;
        socket->write(response);
        socket->disconnectFromHost();
        buffer_.clear();
      });
      connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });
    return true;
  }
  int port() const { return server_.serverPort(); }

private:
  QTcpServer server_;
  QByteArray buffer_;
};

class CrealityCloudServiceTests : public QObject
{
  Q_OBJECT

  QTemporaryDir settingsDir_;

  /// Redirect QSettings into the temp dir BEFORE any service touches
  /// QSettings() (identity persistence must never pollute the developer's
  /// real ~/.config/OWzx store from a test run).
  void isolateSettings()
  {
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir_.path());
    QCoreApplication::setOrganizationName(QStringLiteral("OWzx"));
    QCoreApplication::setApplicationName(QStringLiteral("CrealityCloudServiceTests"));
  }

private slots:
  void initTestCase()
  {
    QVERIFY(settingsDir_.isValid());
    isolateSettings();
  }

  /// Runs before EVERY test function: drop any identity a previous test
  /// persisted, so restoreIdentity() always starts from a clean store.
  void init()
  {
    QSettings s;
    s.remove(QStringLiteral("Cloud/Creality"));
    s.sync();
  }

  // ── X1 seam: the real backend satisfies the narrow interface ─────────
  void seamInterfaceContract()
  {
    static_assert(std::is_base_of_v<ICloudDataPlane, CrealityCloudService>,
                  "CrealityCloudService must implement the X1 ICloudDataPlane seam");
    static_assert(std::is_base_of_v<ICloudDataPlane, CloudServiceMock>,
                  "CloudServiceMock must implement the X1 ICloudDataPlane seam");
  }

  // ── OAuthCallbackServer captures the code and bounces 302 ────────────
  void callbackServerCapturesCode()
  {
    OAuthCallbackServer server;
    QSignalSpy codeSpy(&server, &OAuthCallbackServer::codeReceived);
    QVERIFY(server.start(15899)); // test-fixed port; never the 15888 default
    QVERIFY(server.listening());

    QTcpSocket socket;
    QEventLoop loop;
    QObject::connect(&server, &OAuthCallbackServer::codeReceived, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, 15899);
    QVERIFY(socket.waitForConnected(2000));
    socket.write("GET /login?code=abc123&result_url=https%3A%2F%2Fx HTTP/1.1\r\n"
                 "Host: localhost:15899\r\n\r\n");
    loop.exec();
    // The 302 is written before codeReceived is emitted but arrives
    // asynchronously — wait for it with a bounded readyRead loop.
    QEventLoop respLoop;
    QTimer::singleShot(2000, &respLoop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, &respLoop, &QEventLoop::quit);
    while (socket.bytesAvailable() == 0
           && socket.state() == QAbstractSocket::ConnectedState)
        respLoop.exec();
    const QByteArray response = socket.readAll();

    QCOMPARE(codeSpy.count(), 1);
    QCOMPARE(codeSpy.first().first().toString(), QStringLiteral("abc123"));
    QVERIFY(response.startsWith("HTTP/1.1 302"));
    QVERIFY(response.contains("Location: "));
    server.stop();
    QVERIFY(!server.listening());
  }

  void callbackServerRejectsNonLoginPath()
  {
    OAuthCallbackServer server;
    QSignalSpy codeSpy(&server, &OAuthCallbackServer::codeReceived);
    QVERIFY(server.start(15899));

    QTcpSocket socket;
    QEventLoop loop;
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    QObject::connect(&socket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, 15899);
    QVERIFY(socket.waitForConnected(2000));
    socket.write("GET /other HTTP/1.1\r\nHost: localhost\r\n\r\n");
    loop.exec();
    QVERIFY(socket.readAll().startsWith("HTTP/1.1 404"));
    QCOMPARE(codeSpy.count(), 0);
    server.stop();
  }

  // ── Full OAuth exchange against the loopback token stub ──────────────
  void authServiceExchangeSucceeds()
  {
    OAuthTokenStub stub;
    QVERIFY(stub.start());

    CrealityAuthService auth;
    auth.setCallbackPortOverride(15898);
    auth.setTokenUrlOverride(QStringLiteral("http://127.0.0.1:%1/api/cxy/account/v2/oauthLogin")
                                 .arg(stub.port()));
    QVERIFY(!auth.loggedIn());

    QSignalSpy successSpy(&auth, &CrealityAuthService::loginSucceeded);
    QSignalSpy identitySpy(&auth, &CrealityAuthService::identityChanged);
    QSignalSpy failedSpy(&auth, &CrealityAuthService::loginFailed);

    auth.beginLogin();
    QVERIFY(auth.loginInProgress());

    // Simulate the remote oauth page bouncing to our loopback listener.
    QTcpSocket socket;
    QEventLoop loop;
    connect(&auth, &CrealityAuthService::loginSucceeded, &loop, &QEventLoop::quit);
    connect(&auth, &CrealityAuthService::loginFailed, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, 15898);
    QVERIFY(socket.waitForConnected(2000));
    socket.write("GET /login?code=good-code HTTP/1.1\r\nHost: localhost:15898\r\n\r\n");
    loop.exec();

    QVERIFY(failedSpy.isEmpty());
    QCOMPARE(successSpy.count(), 1);
    QVERIFY(identitySpy.count() >= 1);
    QVERIFY(auth.loggedIn());
    QCOMPARE(auth.userName(), QStringLiteral("创想用户"));
    QCOMPARE(auth.userId(), QStringLiteral("u-42"));
    QCOMPARE(auth.token(), QStringLiteral("tok-123"));
    QVERIFY(!auth.loginInProgress()); // flow finished (server stopped)

    // Request shape sent to the token endpoint (upstream HttpServer.cpp:866+).
    const QJsonDocument sent = QJsonDocument::fromJson(stub.lastBody);
    QCOMPARE(sent.object().value("code").toString(), QStringLiteral("good-code"));
    QVERIFY(!sent.object().value("clientId").toString().isEmpty());
    QVERIFY(sent.object().value("redirecturi").toString()
                .contains(QStringLiteral("back_url=http://localhost:15898/login")));

    // Identity persisted for the next launch (isolated settings store).
    QSettings check;
    check.beginGroup(QStringLiteral("Cloud/Creality"));
    QCOMPARE(check.value(QStringLiteral("token")).toString(), QStringLiteral("tok-123"));
    check.endGroup();
  }

  void authServiceExchangeFailurePath()
  {
    OAuthTokenStub stub;
    stub.failMode = true;
    QVERIFY(stub.start());

    CrealityAuthService auth;
    auth.setCallbackPortOverride(15897);
    auth.setTokenUrlOverride(QStringLiteral("http://127.0.0.1:%1/api/cxy/account/v2/oauthLogin")
                                 .arg(stub.port()));

    QSignalSpy successSpy(&auth, &CrealityAuthService::loginSucceeded);
    QSignalSpy failedSpy(&auth, &CrealityAuthService::loginFailed);

    auth.beginLogin();
    QTcpSocket socket;
    QEventLoop loop;
    connect(&auth, &CrealityAuthService::loginSucceeded, &loop, &QEventLoop::quit);
    connect(&auth, &CrealityAuthService::loginFailed, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, 15897);
    QVERIFY(socket.waitForConnected(2000));
    socket.write("GET /login?code=bad HTTP/1.1\r\nHost: localhost:15897\r\n\r\n");
    loop.exec();

    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(successSpy.count(), 0);
    QVERIFY(!auth.loggedIn());
  }

  void authServiceInvalidateAndRestore()
  {
    CrealityAuthService auth;
    auth.setCallbackPortOverride(15896);
    // Pre-seed identity through the real exchange path.
    {
      OAuthTokenStub stub;
      QVERIFY(stub.start());
      auth.setTokenUrlOverride(QStringLiteral("http://127.0.0.1:%1/oauthLogin").arg(stub.port()));
      auth.beginLogin();
      QTcpSocket socket;
      QEventLoop loop;
      connect(&auth, &CrealityAuthService::loginSucceeded, &loop, &QEventLoop::quit);
      socket.connectToHost(QHostAddress::LocalHost, 15896);
      QVERIFY(socket.waitForConnected(2000));
      socket.write("GET /login?code=k HTTP/1.1\r\nHost: localhost\r\n\r\n");
      loop.exec();
      QVERIFY(auth.loggedIn());
    }
    // Upstream code==4 path: token dropped, account facts kept.
    auth.invalidate();
    QVERIFY(!auth.loggedIn());
    QCOMPARE(auth.userName(), QStringLiteral("创想用户"));
    // A fresh instance restores the persisted (tokenless) identity.
    CrealityAuthService second;
    QVERIFY(!second.loggedIn());
    // logout clears everything.
    second.invalidate();
  }

  // ── CrealityCloudService honesty contract ────────────────────────────
  void cloudServiceHonesty()
  {
    CrealityAuthService auth;
    auth.setCallbackPortOverride(15895);
    CrealityCloudService service(&auth);

    // Account face before login.
    QVERIFY(!service.loggedIn());
    // Password login is not supported — fails with guidance, no fake success.
    QSignalSpy failedSpy(&service, &ICloudDataPlane::loginFailed);
    service.login(QStringLiteral("a@b.c"), QStringLiteral("pw"));
    QCOMPARE(failedSpy.count(), 1);

    // Device face honest-empty: no fabricated bound devices.
    QCOMPARE(service.boundDeviceCount(), 0);
    QVERIFY(service.boundDeviceAt(0).isEmpty());
    service.bindDevice(QStringLiteral("K1"), QStringLiteral("123456"));
    QCOMPARE(service.boundDeviceCount(), 0);

    // Sync face honest-idle.
    QVERIFY(!service.syncing());
    service.syncPresets();
    QVERIFY(!service.syncing());
  }

  void cloudServiceDelegatesAccountFace()
  {
    OAuthTokenStub stub;
    QVERIFY(stub.start());
    CrealityAuthService auth;
    auth.setCallbackPortOverride(15894);
    auth.setTokenUrlOverride(QStringLiteral("http://127.0.0.1:%1/oauthLogin").arg(stub.port()));
    CrealityCloudService service(&auth);

    QSignalSpy stateSpy(&service, &ICloudDataPlane::loginStateChanged);
    auth.beginLogin();
    QTcpSocket socket;
    QEventLoop loop;
    connect(&auth, &CrealityAuthService::loginSucceeded, &loop, &QEventLoop::quit);
    socket.connectToHost(QHostAddress::LocalHost, 15894);
    QVERIFY(socket.waitForConnected(2000));
    socket.write("GET /login?code=z HTTP/1.1\r\nHost: localhost\r\n\r\n");
    loop.exec();

    QVERIFY(service.loggedIn());
    QCOMPARE(service.userName(), QStringLiteral("创想用户"));
    QVERIFY(stateSpy.count() >= 1);

    service.logout();
    QVERIFY(!service.loggedIn());
  }
};

QTEST_MAIN(CrealityCloudServiceTests)
#include "CrealityCloudServiceTests.moc"
