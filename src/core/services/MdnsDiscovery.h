#pragma once

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariantMap>

class QUdpSocket;

/// MdnsDiscovery (feature/lan-printers, L1).
///
/// LAN printer discovery via mDNS service-type enumeration, ported from the
/// official open-source CrealityPrint client (AGPL-3.0, print_manage/utils/
/// cxmdns.cpp + PrinterMgrView.cpp correct_device):
/// - Send a PTR query for `_services._dns-sd._udp.local` on multicast 5353;
/// - Collect answer PTR names whose first label carries one of the Creality
///   prefixes ("creality", "Creality", "CXSWBox");
/// - Instance names matching `_creality<2-digit-type><4-digit-port>` are
///   Creality network printers: group 1 "00" = Moonraker-capable (K1 family),
///   group 2 = the Moonraker port; the device IP is the datagram source
///   address (upstream cxmdns.cpp:144,166-167 parity).
///
/// The DNS wire parsing lives in parseResponseMessage()/extractCrealityPtr(),
/// pure static functions so synthetic datagrams can drive the unit tests
/// without any real network (mirrors the SSDP parser test pattern).
///
/// Scope: LAN only. No cloud discovery, no service registration.
class MdnsDiscovery final : public QObject
{
  Q_OBJECT
  /// True while the discovery window is open.
  Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
  struct CrealityService
  {
    QString instanceName;   // full PTR target, e.g. "_creality007125._tcp.local."
    QString typeCode;       // 2 digits ("00" = Moonraker capable)
    int moonrakerPort = 0;  // 4 digits, 0 when not parseable
    QHostAddress source;    // datagram source address (the printer's IP)
  };

  explicit MdnsDiscovery(QObject *parent = nullptr);
  ~MdnsDiscovery() override;

  /// Open the multicast socket and send one service-enumeration query.
  /// Results stream in via deviceFound(); discoveryFinished() closes the
  /// window after \p windowMs (default 3000, upstream uses ~2s poll cycles).
  Q_INVOKABLE void start(int windowMs = 3000);
  Q_INVOKABLE void stop();
  bool scanning() const { return scanning_; }

  /// Parse one raw DNS message (pure; unit-test entry). Returns every PTR
  /// record whose target name contains a Creality prefix, with the
  /// `_creality` fields decoded.
  static QList<CrealityService> parseResponseMessage(const QByteArray &datagram,
                                                     const QHostAddress &from);
  /// True when the first DNS label carries one of the Creality prefixes.
  static bool hasCrealityPrefix(const QString &ptrName);
  /// Decode "_creality007125._tcp.local." → {typeCode "00", port 7125}.
  static bool decodeCrealityInstance(const QString &instanceName,
                                     QString *typeCode, int *moonrakerPort);

signals:
  /// One discovered Creality network printer (deduplicated by instance name):
  /// {instanceName, typeCode, moonrakerPort, ip, isMoonraker}.
  void deviceFound(const QVariantMap &device);
  void discoveryFinished(int foundCount);
  void scanningChanged();
  void errorOccurred(const QString &message);

private slots:
  void readPendingDatagrams();
  void finishWindow();

private:
  void sendEnumerationQuery();

  QUdpSocket *socket_ = nullptr;
  bool scanning_ = false;
  QStringList seenInstances_;
};
