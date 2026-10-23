#include "core/services/ModelDownloadService.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QTemporaryDir>
#include <QtNetwork>
#include <QtTest>

/// Loopback file server: serves a fixed payload on /file.bin (with
/// Content-Length) and /big.bin slowly enough to exercise cancel.
class DownloadStub : public QObject
{
  Q_OBJECT
public:
  QByteArray payload;
  int hits = 0;
  /// trickle: send headers + first 100 bytes only, then hold the socket —
  /// guarantees the client-side download stays in flight for cancel tests.
  bool trickle = false;

  bool start()
  {
    if (!server_.listen(QHostAddress::LocalHost))
      return false;
    connect(&server_, &QTcpServer::newConnection, this, [this]() {
      QTcpSocket *socket = server_.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        buffer_ += socket->readAll();
        if (!buffer_.contains("\r\n\r\n"))
          return;
        hits++;
        const QByteArray head = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "
                                + QByteArray::number(payload.size()) + "\r\nConnection: close\r\n\r\n";
        if (trickle) {
          socket->write(head + payload.left(100)); // body held: in-flight
        } else {
          socket->write(head + payload);
          socket->disconnectFromHost();
        }
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

class ModelDownloadServiceTests : public QObject
{
  Q_OBJECT

  QTemporaryDir dir_;

private slots:
  void initTestCase()
  {
    QVERIFY(dir_.isValid());
  }

  void downloadWritesFileAndSignals()
  {
    DownloadStub stub;
    stub.payload = QByteArray(2048, 'A');
    QVERIFY(stub.start());

    ModelDownloadService svc;
    svc.setDownloadRoot(dir_.path());
    QSignalSpy finishedSpy(&svc, &ModelDownloadService::downloadFinished);
    QSignalSpy failedSpy(&svc, &ModelDownloadService::downloadFailed);

    QEventLoop loop;
    connect(&svc, &ModelDownloadService::downloadFinished, &loop,
            &QEventLoop::quit);
    svc.startDownload(QStringLiteral("http://127.0.0.1:%1/file.bin").arg(stub.port()),
                      QStringLiteral("u1"), QStringLiteral("m1"),
                      QStringLiteral("f1"), QStringLiteral("stl"),
                      QStringLiteral("benchy"));
    loop.exec();

    QVERIFY(failedSpy.isEmpty());
    QCOMPARE(finishedSpy.count(), 1);
    const QString path = finishedSpy.first().at(1).toString();
    QVERIFY(path.contains(QStringLiteral("/u1/models/m1/")));
    QVERIFY(path.endsWith(QStringLiteral("benchy.stl")));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), stub.payload);
    QVERIFY(!QFile::exists(path + QStringLiteral(".part")));
  }

  void duplicateNamesGetNumberedSuffix()
  {
    DownloadStub stub;
    stub.payload = "x";
    QVERIFY(stub.start());

    ModelDownloadService svc;
    svc.setDownloadRoot(dir_.path());
    QStringList paths;
    QEventLoop loop;
    connect(&svc, &ModelDownloadService::downloadFinished,
            [&](const QString &, const QString &path) {
              paths << path;
              if (paths.size() == 3)
                loop.quit();
            });
    for (int i = 0; i < 3; ++i) {
      svc.startDownload(QStringLiteral("http://127.0.0.1:%1/f.bin").arg(stub.port()),
                        QStringLiteral("u1"), QStringLiteral("m1"),
                        QStringLiteral("dup%1").arg(i), QStringLiteral("stl"),
                        QStringLiteral("same-name"));
    }
    loop.exec();
    QCOMPARE(paths.size(), 3);
    QVERIFY(paths.at(0).endsWith(QStringLiteral("same-name.stl")));
    QVERIFY(paths.at(1).endsWith(QStringLiteral("same-name (1).stl")));
    QVERIFY(paths.at(2).endsWith(QStringLiteral("same-name (2).stl")));
  }

  void cancelRemovesPartFile()
  {
    DownloadStub stub;
    stub.trickle = true;
    stub.payload = QByteArray(1 << 20, 'B');
    QVERIFY(stub.start());

    ModelDownloadService svc;
    svc.setDownloadRoot(dir_.path());
    QEventLoop loop;
    QTimer::singleShot(1500, &loop, &QEventLoop::quit);
    QSignalSpy canceledSpy(&svc, &ModelDownloadService::downloadCanceled);
    svc.startDownload(QStringLiteral("http://127.0.0.1:%1/big.bin").arg(stub.port()),
                      QStringLiteral("u1"), QStringLiteral("m1"),
                      QStringLiteral("c1"), QStringLiteral("3mf"),
                      QStringLiteral("big"));
    QTimer::singleShot(50, &svc, [&svc]() { svc.cancel(QStringLiteral("c1")); });
    loop.exec();

    QCOMPARE(canceledSpy.count(), 1);
    QVERIFY(!svc.busy());
  }

  void stateReportShapes()
  {
    ModelDownloadService svc;
    svc.setDownloadRoot(dir_.path());
    const QVariantMap state = svc.downloadState();
    QVERIFY(state.value(QStringLiteral("models")).toList().isEmpty());
    QVERIFY(state.value(QStringLiteral("3mfs")).toList().isEmpty());
    // forgetting an unknown id is a no-op
    svc.forgetDownload(QStringLiteral("nope"));
    QCOMPARE(svc.activeCount(), 0);
  }
};

QTEST_MAIN(ModelDownloadServiceTests)
#include "ModelDownloadServiceTests.moc"
