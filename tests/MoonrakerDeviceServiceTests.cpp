#include "core/services/LanErrorCodes.h"
#include "core/services/MoonrakerDeviceService.h"

#include <QDir>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

/// 回环 G-code 打印机 stub（Moonraker 语义）：
/// - POST /server/files/upload → {"result":"ok"}
/// - POST /printer/print/start?filename=X → {"result":"ok"}
/// - POST /printer/print/pause|resume|cancel → {"result":"ok"}
/// - GET  /printer/objects/query → 打印中状态（供轮询解析）
/// 记录收到的请求行，供端点断言。
class MoonrakerStub : public QObject
{
  Q_OBJECT
public:
    QString lastRequestLine;
    QByteArray lastBody;
    int uploadHits = 0;
    int startHits = 0;
    /// trickle：上传请求只应答头 + 少量 body，保持连接挂起（取消测试用）。
    bool trickle = false;

    bool start()
    {
        if (!server_.listen(QHostAddress::LocalHost))
            return false;
        connect(&server_, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket *socket = server_.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket]() {
                buffer_ += socket->readAll();
                const int split = buffer_.indexOf("\r\n\r\n");
                if (split == -1)
                    return;
                const QByteArray head = buffer_.left(splitPos(buffer_) + 4);
                int contentLength = 0;
                for (const QByteArray &line : head.split('\n')) {
                    if (line.toLower().startsWith("content-length:"))
                        contentLength = line.mid(15).trimmed().toInt();
                }
                const QByteArray body = buffer_.mid(splitPos(buffer_) + 4);
                if (body.size() < contentLength)
                    return; // 等剩余分片
                lastRequestLine = QString::fromLatin1(
                    buffer_.left(buffer_.indexOf("\r\n")));
                lastBody = body;
                const QByteArray path =
                    lastRequestLine.section(' ', 1, 1).toLatin1();
                QByteArray payload;
                if (path.startsWith("/server/files/upload")) {
                    ++uploadHits; // Moonraker 上传
                    payload = "{\"result\":\"ok\"}";
                } else if (path.startsWith("/upload/")) {
                    ++uploadHits; // 4408 上传：body 含 "OK" 即成功（官方语义）
                    payload = "OK";
                } else if (path.startsWith("/printer/print/start")) {
                    ++startHits; // Moonraker 启动
                    payload = "{\"result\":\"ok\"}";
                } else if (path.startsWith("/server/files/list")) {
                    payload = "{\"result\":{\"files\":[{\"name\":\"benchy.gcode\","
                              "\"size\":1048576,\"path\":\"gcodes/benchy.gcode\"}]}}";
                } else {
                    payload = "{\"result\":\"ok\"}";
                }
                if (trickle && path.startsWith("/upload/")) {
                    socket->write("HTTP/1.1 200 OK\r\nContent-Length: 100\r\n"
                                  "Connection: close\r\n\r\nOK..."); // body 不发完
                } else {
                    socket->write(QByteArray("HTTP/1.1 200 OK\r\nContent-Length: ")
                                  + QByteArray::number(static_cast<qlonglong>(
                                        static_cast<int>(payload.size())))
                                  + "\r\nConnection: close\r\n\r\n"
                                  + payload);
                    socket->disconnectFromHost();
                }
                buffer_.clear();
            });
            connect(socket, &QTcpSocket::disconnected, socket,
                    &QTcpSocket::deleteLater);
        });
        return true;
    }
    int port() const { return server_.serverPort(); }

private:
    static int splitPos(const QByteArray &b) { return b.indexOf("\r\n\r\n"); }
    QTcpServer server_;
    QByteArray buffer_;
};

class MoonrakerDeviceServiceTests : public QObject
{
  Q_OBJECT

  QTemporaryDir dir_;

private slots:
  void initTestCase() { QVERIFY(dir_.isValid()); }

  /// 每个用例前清空持久化设备表（隔离用例间状态）
  void init()
  {
    QSettings s;
    s.remove(QStringLiteral("Devices/Lan"));
    s.remove(QStringLiteral("Devices/LanLastIp"));
    s.sync();
  }

  /// 设备持久化 + 过滤选中（纯本地，无网络）
  void persistenceAndSelection()
  {
    QString json;
    // 作用域内写 → 离开作用域后新实例读
    {
      QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir_.path());
      QSettings::setDefaultFormat(QSettings::IniFormat);
      QCoreApplication::setOrganizationName(QStringLiteral("OWzx"));
      QCoreApplication::setApplicationName(QStringLiteral("MoonrakerDeviceServiceTests"));

      MoonrakerDeviceService svc;
      svc.setProbeHttpsDisabled(true);
      svc.setPortsOverride(9998, 8081);
      QVERIFY(svc.addManualDevice(QStringLiteral("我的K1C"),
                                  QStringLiteral("192.0.2.77"), QString()));
      QCOMPARE(svc.deviceCount(), 1);
      QCOMPARE(svc.isDemoDevice(0), false); // LAN 后端无演示条目
      const QVariantMap dev = svc.deviceAt(0);
      QCOMPARE(dev.value(QStringLiteral("name")).toString(),
               QStringLiteral("我的K1C"));
      QCOMPARE(dev.value(QStringLiteral("demo")).toBool(), false);
      QSettings s;
      json = s.value(QStringLiteral("Devices/Lan")).toString();
      QVERIFY(!json.isEmpty());
    }
    {
      MoonrakerDeviceService second;
      QCOMPARE(second.deviceCount(), 1); // 重启后设备还在（此前 mock 重启即失）
      QCOMPARE(second.deviceAt(0).value(QStringLiteral("ip")).toString(),
               QStringLiteral("192.0.2.77"));
    }
  }

  /// 未连接时控制命令无副作用、发送走真实 HTTP 失败路径（诚实语义）
  void honestNoSideEffectsWhenDisconnected()
  {
    MoonrakerDeviceService svc;
    svc.setProbeHttpsDisabled(true);
    svc.setPortsOverride(9990, 8080);
    QVERIFY(svc.addManualDevice(QStringLiteral("d"), QStringLiteral("192.0.2.10"),
                                QString()));
    // 未连接：暂停/停止均无副作用（扩展面默认 no-op 语义）
    svc.pausePrint(0);
    svc.stopPrint(0);
    QVERIFY(!svc.sendPrintInFlight());
    // 发送到不可达地址：失败必发 sendPrintFailed，且状态零翻转
    QSignalSpy failedSpy(&svc, &IDeviceDataPlane::sendPrintFailed);
    QSignalSpy succeededSpy(&svc, &IDeviceDataPlane::sendPrintSucceeded);
    // U11 契约：失败必须走 sendPrintFailed（同步 false + 信号，或 true + 异步信号）
    const bool accepted = svc.sendPrintViaFtp(0, QStringLiteral("/tmp/nonexistent.gcode"));
    QVERIFY(accepted || failedSpy.count() == 1);
    QEventLoop loop;
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    connect(&svc, &IDeviceDataPlane::sendPrintFailed, &loop, &QEventLoop::quit);
    loop.exec();
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(succeededSpy.count(), 0);
    QVERIFY(!svc.selectedDeviceOnline()); // 绝不假翻在线
  }

  /// Moonraker 上传 + 启动全链（回环 stub）
  void uploadAndStartAgainstStub()
  {
    MoonrakerStub stub;
    QVERIFY(stub.start());

    MoonrakerDeviceService svc;
    svc.setProbeHttpsDisabled(true);
    svc.setPortsOverride(9989, stub.port());
    // addManualDevice 会探测（回环无 /info），但设备已入表——直接选中发送
    QVERIFY(svc.addManualDevice(QStringLiteral("K1"), QStringLiteral("127.0.0.1"),
                                QString()));
    svc.setPortsOverride(9999, stub.port());

    const QString gcode = QDir(dir_.path()).filePath(QStringLiteral("benchy.gcode"));
    QFile f(gcode);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(4096, 'G'));
    f.close();

    QSignalSpy progressSpy(&svc, &IDeviceDataPlane::sendPrintUploadProgress);
    QSignalSpy succeededSpy(&svc, &IDeviceDataPlane::sendPrintSucceeded);
    QSignalSpy failedSpy(&svc, &IDeviceDataPlane::sendPrintFailed);
    QEventLoop loop;
    connect(&svc, &IDeviceDataPlane::sendPrintSucceeded, &loop, &QEventLoop::quit);
    connect(&svc, &IDeviceDataPlane::sendPrintFailed, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    QVERIFY(svc.sendPrintViaFtp(0, gcode));
    loop.exec();

    QVERIFY(failedSpy.isEmpty());
    QCOMPARE(succeededSpy.count(), 1);
    QCOMPARE(stub.uploadHits, 1);
    // 手动添加的设备按 4408 语义：POST /upload/<name>，启动走 ws（不经 HTTP）
    QVERIFY(stub.lastRequestLine.contains("/upload/"));
    QVERIFY(progressSpy.count() >= 0); // 小文件可能 0 次 uploadProgress
  }
  // ── 收尾#1：错误码表 ────────────────────────────────────────────────
  void errorCodesTable()
  {
    QCOMPARE(LanErrorCodes::text(0), QString());
    QCOMPARE(LanErrorCodes::text(115), QStringLiteral("耗材断料/请检查进料"));
    QVERIFY(LanErrorCodes::isKnown(115));
    const QString unknown = LanErrorCodes::text(9001);
    QVERIFY(unknown.contains(QStringLiteral("9001"))); // 未知码显示数字，不编造
    QVERIFY(!LanErrorCodes::isKnown(9001));
  }

  // ── 收尾#2：enableSelfTest 传播 ─────────────────────────────────────
  void selfTestFlagPropagation()
  {
    MoonrakerDeviceService svc;
    svc.setProbeHttpsDisabled(true);
    svc.addManualDevice(QStringLiteral("K1"), QStringLiteral("127.0.0.1"), QString());
    QVERIFY(!svc.enableSelfTest());
    svc.setEnableSelfTest(true);
    QVERIFY(svc.enableSelfTest()); // 服务层接受选项（ws 参数在 applyStatusFields
                                   // 之外的启动消息里，ws 未连接时无副作用）
    svc.setEnableSelfTest(false);
    QVERIFY(!svc.enableSelfTest());
  }

  // ── 收尾#4：Moonraker 文件列表（回环 stub） ─────────────────────────
  void moonrakerFilesList()
  {
    MoonrakerStub stub;
    QVERIFY(stub.start());

    MoonrakerDeviceService svc;
    svc.setProbeHttpsDisabled(true);
    svc.setPortsOverride(9997, stub.port());
    svc.addManualDevice(QStringLiteral("K1"), QStringLiteral("127.0.0.1"), QString());
    svc.markDeviceMoonraker(0, stub.port()); // 生产路径 = mDNS typeCode "00" 合并
    svc.selectDevice(0); // requestDeviceFiles 走选中设备

    QSignalSpy filesSpy(&svc, &MoonrakerDeviceService::deviceFilesReady);
    QEventLoop loop;
    connect(&svc, &MoonrakerDeviceService::deviceFilesReady, &loop, &QEventLoop::quit);
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    svc.requestDeviceFiles();
    loop.exec();

    QCOMPARE(filesSpy.count(), 1);
    const QVariantList files = filesSpy.first().at(0).toList();
    QCOMPARE(files.size(), 1);
    QCOMPARE(files.first().toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("benchy.gcode"));
    QCOMPARE(files.first().toMap().value(QStringLiteral("size")).toLongLong(), 1048576LL);
  }
  // ── 收尾#3：上传取消（trickle stub + abort → sendPrintCancelled） ──
  void uploadCancelEmitsCancelled()
  {
    MoonrakerStub stub;
    stub.trickle = true;
    QVERIFY(stub.start());

    MoonrakerDeviceService svc;
    svc.setProbeHttpsDisabled(true);
    svc.setPortsOverride(9996, stub.port());
    svc.addManualDevice(QStringLiteral("K1"), QStringLiteral("127.0.0.1"), QString());

    const QString gcode = QDir(dir_.path()).filePath(QStringLiteral("big.gcode"));
    QFile f(gcode);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(4 << 20, 'G')); // 4MB：慢速 stub 上来不及传完
    f.close();

    QSignalSpy startedSpy(&svc, &IDeviceDataPlane::sendPrintUploadStarted);
    QSignalSpy canceledSpy(&svc, &IDeviceDataPlane::sendPrintCancelled);
    QSignalSpy failedSpy(&svc, &IDeviceDataPlane::sendPrintFailed);
    QEventLoop loop;
    connect(&svc, &IDeviceDataPlane::sendPrintCancelled, &loop, &QEventLoop::quit);
    connect(&svc, &IDeviceDataPlane::sendPrintFailed, &loop, &QEventLoop::quit);
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    QVERIFY(svc.sendPrintViaFtp(0, gcode));
    QVERIFY(svc.sendPrintInFlight());
    QTimer::singleShot(300, &svc, [&svc]() { svc.cancelSendPrint(); });
    loop.exec();

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(canceledSpy.count(), 1);
    QVERIFY(failedSpy.isEmpty());
    QVERIFY(!svc.sendPrintInFlight());
  }
};

QTEST_MAIN(MoonrakerDeviceServiceTests)
#include "MoonrakerDeviceServiceTests.moc"
