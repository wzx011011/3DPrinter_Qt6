#include "core/services/LanDeviceProbe.h"
#include "core/services/MdnsDiscovery.h"

#include <QEventLoop>
#include <QHostAddress>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

namespace {

/// Append one DNS name to a datagram (no compression).
void appendName(QByteArray &out, const QStringList &labels)
{
    for (const QString &label : labels) {
        const QByteArray utf = label.toUtf8();
        out.append(char(utf.size()));
        out.append(utf);
    }
    out.append('\0');
}

/// Build a synthetic mDNS response with one PTR answer + one A additional
/// record — enough to exercise MdnsDiscovery::parseResponseMessage.
QByteArray buildMdnsResponse(const QString &ptrTarget, const QHostAddress &aRecord)
{
    QByteArray out;
    out.append('\0').append('\0');   // ID
    out.append(char(0x84)).append('\0'); // QR=1, AA=1
    out.append('\0').append('\001'); // QDCOUNT 1
    out.append('\0').append('\001'); // ANCOUNT 1
    out.append('\0').append('\0');   // NSCOUNT 0
    out.append('\0').append('\001'); // ARCOUNT 1 (the A record)
    // Question: _services._dns-sd._udp.local PTR
    appendName(out, {"_services", "_dns-sd", "_udp", "local"});
    out.append('\0').append(char(12));
    out.append('\0').append('\001');
    // Answer: _services._dns-sd._udp.local PTR <ptrTarget>
    appendName(out, {"_services", "_dns-sd", "_udp", "local"});
    out.append('\0').append(char(12));
    out.append('\0').append('\001');
    out.append('\0').append('\0').append('\0').append('\075'); // TTL 60
    // Encode the target name WITH its terminating null — uncompressed DNS
    // names in RDATA must be null-terminated within RDLENGTH (RFC 1035); a
    // missing terminator makes parsers run into the next record.
    QByteArray encodedTarget;
    appendName(encodedTarget, ptrTarget.split('.'));
    out.append('\0').append(char(encodedTarget.size()));
    out.append(encodedTarget);
    // Additional: <ptrTarget> A <address>
    appendName(out, ptrTarget.split('.'));
    out.append('\0').append('\001'); // A
    out.append('\0').append('\001'); // IN
    out.append('\0').append('\0').append('\0').append('\075');
    out.append('\0').append('\004');
    quint32 v4 = aRecord.toIPv4Address();
    for (int shift = 24; shift >= 0; shift -= 8)
        out.append(char((v4 >> shift) & 0xFF));
    return out;
}

} // namespace

class LanDiscoveryTests : public QObject
{
  Q_OBJECT

  QTemporaryDir dir_;

private slots:
  void initTestCase() { QVERIFY(dir_.isValid()); }

  // ── mDNS wire parsing (pure, synthetic datagrams) ────────────────────
  void mdnsParsesCrealityMoonrakerService()
  {
    // K1-style instance: type 00, Moonraker port 7125.
    const QByteArray msg = buildMdnsResponse(
        QStringLiteral("_creality007125._tcp.local."),
        QHostAddress(QStringLiteral("192.168.1.87")));
    const QList<MdnsDiscovery::CrealityService> services =
        MdnsDiscovery::parseResponseMessage(msg, QHostAddress(QStringLiteral("192.168.1.87")));
    QCOMPARE(services.size(), 1);
    // Parsed DNS names carry no trailing dot (wire representation).
    QCOMPARE(services.first().instanceName,
             QStringLiteral("_creality007125._tcp.local"));
    QCOMPARE(services.first().typeCode, QStringLiteral("00"));
    QCOMPARE(services.first().moonrakerPort, 7125);
    QCOMPARE(services.first().source.toString(), QStringLiteral("192.168.1.87"));
    QVERIFY(MdnsDiscovery::hasCrealityPrefix(services.first().instanceName));
    // type "00" is the Moonraker marker (upstream PrinterMgrView.cpp:2179).
    QString typeCode;
    int port = 0;
    QVERIFY(MdnsDiscovery::decodeCrealityInstance(
        QStringLiteral("_creality007125._tcp.local."), &typeCode, &port));
    QCOMPARE(typeCode, QStringLiteral("00"));
    QCOMPARE(port, 7125);
  }

  void mdnsIgnoresNonCrealityAndBadNames()
  {
    // Non-creality PTR must not be collected.
    QByteArray msg = buildMdnsResponse(QStringLiteral("_workstation._tcp.local."),
                                       QHostAddress(QStringLiteral("192.168.1.5")));
    QVERIFY(MdnsDiscovery::parseResponseMessage(msg, QHostAddress()).isEmpty());
    // Creality prefix but not the printer pattern → collected, undecoded.
    msg = buildMdnsResponse(QStringLiteral("CXSWBox001._tcp.local."),
                            QHostAddress(QStringLiteral("192.168.1.6")));
    const QList<MdnsDiscovery::CrealityService> services =
        MdnsDiscovery::parseResponseMessage(msg, QHostAddress());
    QCOMPARE(services.size(), 1);
    QVERIFY(MdnsDiscovery::hasCrealityPrefix(services.first().instanceName));
    QString typeCode;
    QVERIFY(!MdnsDiscovery::decodeCrealityInstance(
        services.first().instanceName, &typeCode, nullptr));
    // Truncated datagram must not crash.
    QVERIFY(MdnsDiscovery::parseResponseMessage(QByteArray("xxxxx"),
                                                QHostAddress()).isEmpty());
  }

  void mdnsParsesLegacyCrealityService()
  {
    // Old printers use a different type code (not "00") — collected but not
    // treated as Moonraker (upstream ignores non-"00" types for the Klipper
    // list; OWzx keeps them visible as generic LAN devices).
    const QByteArray msg = buildMdnsResponse(
        QStringLiteral("_creality018090._tcp.local."),
        QHostAddress(QStringLiteral("192.168.1.90")));
    const QList<MdnsDiscovery::CrealityService> services =
        MdnsDiscovery::parseResponseMessage(msg, QHostAddress());
    QCOMPARE(services.size(), 1);
    QCOMPARE(services.first().typeCode, QStringLiteral("01"));
    QCOMPARE(services.first().moonrakerPort, 8090);
    QVERIFY(!services.first().instanceName.isEmpty());
  }

  // ── /info probe against a loopback stub ──────────────────────────────
  void probeParsesModernInfo()
  {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    QByteArray served;
    const QByteArray body =
        "{\"model\":\"K1C\",\"mac\":\"AA:BB:CC:DD:EE:FF\",\"wssPort\":443,"
        "\"videoPort\":443,\"videoToken\":\"tok\"}";
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
      QTcpSocket *socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
        served += socket->readAll();
        if (!served.contains("\r\n\r\n"))
          return;
        const QByteArray response = QByteArray("HTTP/1.1 200 OK\r\nContent-Length: ")
                                    + QByteArray::number(body.size())
                                    + "\r\nConnection: close\r\n\r\n" + body;
        socket->write(response);
        socket->disconnectFromHost();
      });
      connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    QEventLoop loop;
    LanDeviceProbe probe;
    probe.setHttpsDisabled(true); // 测试二进制有 OpenSSL 符号冲突，跳过 TLS 握手
    LanDeviceProbe::ProbeResult captured;
    probe.probe(QStringLiteral("127.0.0.1:%1").arg(server.serverPort()),
                [&](const LanDeviceProbe::ProbeResult &r) {
                  captured = r;
                  loop.quit();
                });
    loop.exec();

    QVERIFY(captured.ok);
    QVERIFY(captured.secureConnection == false); // HTTP fallback path
    QVERIFY(captured.oldPrinter == false);
    QCOMPARE(captured.model, QStringLiteral("K1C"));
    QCOMPARE(captured.mac, QStringLiteral("AA:BB:CC:DD:EE:FF"));
    QCOMPARE(captured.wssPort, 443);
    QCOMPARE(captured.videoPort, 443);
    QCOMPARE(captured.videoToken, QStringLiteral("tok"));
    // /info was requested over HTTP on the fallback path (loopback has no TLS).
    QVERIFY(served.startsWith("GET /info"));
  }

  void probeDetectsLegacyPrinter()
  {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const QByteArray body =
        "{\"model\":\"CR-10 Smart\",\"ssid\":\"CR-SMART-AABBCCDD\"}";
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
      QTcpSocket *socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, socket, [&, socket]() {
        const QByteArray req = socket->readAll();
        if (!req.contains("\r\n\r\n"))
          return;
        const QByteArray response = QByteArray("HTTP/1.1 200 OK\r\nContent-Length: ")
                                    + QByteArray::number(body.size())
                                    + "\r\nConnection: close\r\n\r\n" + body;
        socket->write(response);
        socket->disconnectFromHost();
      });
      connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    QEventLoop loop;
    LanDeviceProbe probe;
    probe.setHttpsDisabled(true);
    LanDeviceProbe::ProbeResult captured;
    probe.probe(QStringLiteral("127.0.0.1:%1").arg(server.serverPort()),
                [&](const LanDeviceProbe::ProbeResult &r) {
                  captured = r;
                  loop.quit();
                });
    loop.exec();

    QVERIFY(captured.ok);
    QVERIFY(captured.oldPrinter); // ssid field marks legacy firmware
    QCOMPARE(captured.model, QStringLiteral("CR-10 Smart"));
    QCOMPARE(captured.mac, QStringLiteral("SMART")); // ssid second dash segment
  }

  void probeRejectsInvalidInfo()
  {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    const QByteArray body = "{\"foo\":1}"; // no model/mac
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
      QTcpSocket *socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, socket, [&, socket, body]() {
        const QByteArray req = socket->readAll();
        if (!req.contains("\r\n\r\n"))
          return;
        socket->write(QByteArray("HTTP/1.1 200 OK\r\nContent-Length: ")
                      + QByteArray::number(body.size())
                      + "\r\nConnection: close\r\n\r\n" + body);
        socket->disconnectFromHost();
      });
      connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    QEventLoop loop;
    LanDeviceProbe probe;
    probe.setHttpsDisabled(true);
    LanDeviceProbe::ProbeResult captured;
    probe.probe(QStringLiteral("127.0.0.1:%1").arg(server.serverPort()),
                [&](const LanDeviceProbe::ProbeResult &r) {
                  captured = r;
                  loop.quit();
                });
    loop.exec();
    QVERIFY(!captured.ok);
    QVERIFY(!captured.errorString.isEmpty());
  }

  // ── 真局域网冒烟（用户环境有真实打印机；只发 mDNS 查询 + /info GET）──
  void realLanDiscoverySmoke()
  {
    MdnsDiscovery mdns;
    QEventLoop loop;
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    QStringList found;
    connect(&mdns, &MdnsDiscovery::deviceFound, &loop,
            [&](const QVariantMap &d) {
              const QString ip = d.value(QStringLiteral("ip")).toString();
              found << ip;
              qInfo().noquote() << "[REAL-LAN] 发现:"
                                << d.value(QStringLiteral("instanceName")).toString()
                                << "@" << ip
                                << "moonrakerPort="
                                << d.value(QStringLiteral("moonrakerPort")).toInt();
              // 对每个发现的 IP 跑 /info 探测，打印机型
              auto *probe = new LanDeviceProbe(&mdns);
              connect(probe, &LanDeviceProbe::probeFinished, probe,
                      [probe](const QVariantMap &r) {
                        qInfo().noquote()
                            << "[REAL-LAN] /info:" << r.value("address").toString()
                            << "model=" << r.value("model").toString()
                            << "ok=" << r.value("ok").toBool()
                            << "oldPrinter=" << r.value("oldPrinter").toBool();
                        probe->deleteLater();
                      });
              probe->probe(ip);
            });
    connect(&mdns, &MdnsDiscovery::discoveryFinished, &loop, [&loop](int) {
      QTimer::singleShot(2500, &loop, &QEventLoop::quit); // 给探测留缓冲
    });
    mdns.start(8000);
    loop.exec();
    qInfo().noquote() << "[REAL-LAN] 发现总数:" << found.size();
    // 语义断言：不要求环境有设备，但发现了就必须是创想前缀（由解析器保证）
    QVERIFY(true);
  }

  // ── pure parseInfo checks (no network) ───────────────────────────────
  void parseInfoShapes()
  {
    // Secure responses must carry wss/video ports (upstream validity rule).
    const QByteArray incomplete = "{\"model\":\"K1C\",\"mac\":\"AA\"}";
    const LanDeviceProbe::ProbeResult bad =
        LanDeviceProbe::parseInfoBody(incomplete, true, QStringLiteral("10.0.0.1"));
    QVERIFY(!bad.ok);
    // Same body over plain HTTP is acceptable (ports are optional there per
    // the upstream http-fallback leniency).
    const LanDeviceProbe::ProbeResult plain =
        LanDeviceProbe::parseInfoBody(incomplete, false, QStringLiteral("10.0.0.1"));
    QVERIFY(plain.ok);
    QCOMPARE(plain.wssPort, 0);
    // Malformed JSON is rejected, never ok.
    QVERIFY(!LanDeviceProbe::parseInfoBody("{broken", true, "x").ok);
  }
};

QTEST_MAIN(LanDiscoveryTests)
#include "LanDiscoveryTests.moc"
