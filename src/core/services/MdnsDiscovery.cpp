#include "MdnsDiscovery.h"

#include <QDateTime>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTimer>
#include <QUdpSocket>

namespace {

constexpr quint16 kMdnsPort = 5353;
const QHostAddress kMdnsGroup{QStringLiteral("224.0.0.251")};
constexpr int kMaxDatagram = 9000;

/// Append one DNS name element to \p out. Labels are length-prefixed; a
/// label length byte of 0 terminates the name.
void appendNameLabel(QByteArray &out, const QString &label)
{
    const QByteArray utf = label.toUtf8();
    out.append(char(utf.size()));
    out.append(utf);
}

QByteArray buildEnumerationQuery()
{
    QByteArray out;
    out.append('\0').append('\0');   // ID 0 (mDNS mandates 0)
    out.append('\0').append('\0');   // flags: standard query
    out.append('\0').append('\001'); // QDCOUNT = 1
    out.append('\0').append('\0');   // ANCOUNT = 0
    out.append('\0').append('\0');   // NSCOUNT = 0
    out.append('\0').append('\0');   // ARCOUNT = 0
    // Question: _services._dns-sd._udp.local. PTR IN
    appendNameLabel(out, QStringLiteral("_services"));
    appendNameLabel(out, QStringLiteral("_dns-sd"));
    appendNameLabel(out, QStringLiteral("_udp"));
    appendNameLabel(out, QStringLiteral("local"));
    out.append('\0');
    out.append('\0').append(char(12)); // QTYPE PTR = 12
    out.append('\0').append('\001');   // QCLASS IN = 1
    return out;
}

/// Decode one DNS name starting at \p pos, following compression pointers
/// (max hops guard against loops). Returns the dotted name; \p pos advances
/// past the name in the original buffer.
QString parseDnsName(const QByteArray &data, int &pos, int depth = 0)
{
    QString name;
    int cursor = pos;
    if (depth > 8 || cursor >= data.size())
        return name;
    bool jumped = false;
    int nextAfterJump = pos;
    forever {
        if (cursor >= data.size())
            return name;
        const int len = int(quint8(data.at(cursor)));
        if (len == 0) {
            cursor += 1;
            if (!jumped)
                pos = cursor;
            break;
        }
        if ((len & 0xC0) == 0xC0) {
            // compression pointer (RFC 1035 §4.1.4)
            if (cursor + 1 >= data.size())
                return name;
            const int target = ((len & 0x3F) << 8) | quint8(data.at(cursor + 1));
            if (!jumped)
                pos = cursor + 2;
            cursor = target;
            jumped = true;
            if (++depth > 8)
                return name;
            continue;
        }
        if (!name.isEmpty())
            name += QLatin1Char('.');
        name += QString::fromUtf8(data.mid(cursor + 1, len));
        cursor += 1 + len;
        if (!jumped)
            pos = cursor;
    }
    return name;
}

} // namespace

MdnsDiscovery::MdnsDiscovery(QObject *parent)
    : QObject(parent), socket_(new QUdpSocket(this))
{
    connect(socket_, &QUdpSocket::readyRead, this,
            &MdnsDiscovery::readPendingDatagrams);
}

MdnsDiscovery::~MdnsDiscovery() = default;

bool MdnsDiscovery::hasCrealityPrefix(const QString &ptrName)
{
    // Upstream collects PTR targets CONTAINING one of the Creality prefixes
    // (PrinterMgrView.cpp:2156-2159, cxmdns.cpp:154-159) — service instance
    // names start with an underscore ("_creality007125._tcp.local"), so a
    // startsWith check would never match.
    return ptrName.contains(QLatin1String("creality"), Qt::CaseInsensitive)
           || ptrName.contains(QLatin1String("CXSWBox"), Qt::CaseInsensitive);
}

bool MdnsDiscovery::decodeCrealityInstance(const QString &instanceName,
                                           QString *typeCode, int *moonrakerPort)
{
    // Upstream correct_device: `_creality(\d{2})(\d{4}).+` — group 1 is the
    // machine type code, group 2 the Moonraker port (PrinterMgrView.cpp:2173).
    static const QRegularExpression re(
        QStringLiteral("_creality(\\d{2})(\\d{4})"));
    const QRegularExpressionMatch m = re.match(instanceName);
    if (!m.hasMatch())
        return false;
    if (typeCode)
        *typeCode = m.captured(1);
    if (moonrakerPort) {
        *moonrakerPort = m.captured(2).toInt();
    }
    return true;
}

QList<MdnsDiscovery::CrealityService> MdnsDiscovery::parseResponseMessage(
    const QByteArray &datagram, const QHostAddress &from)
{
    QList<CrealityService> result;
    if (datagram.size() < 12)
        return result;

    int qd = (quint8(datagram.at(4)) << 8) | quint8(datagram.at(5));
    int an = (quint8(datagram.at(6)) << 8) | quint8(datagram.at(7));
    int pos = 12;
    for (int i = 0; i < qd && pos < datagram.size(); ++i) {
        parseDnsName(datagram, pos);
        pos += 4; // qtype + qclass
    }
    for (int i = 0; i < an && pos + 10 <= datagram.size(); ++i) {
        parseDnsName(datagram, pos);
        if (pos + 10 > datagram.size())
            break;
        const int type = (quint8(datagram.at(pos)) << 8) | quint8(datagram.at(pos + 1));
        pos += 2 + 2;                       // type + class
        pos += 4;                           // TTL
        const int rdlength = (quint8(datagram.at(pos)) << 8) | quint8(datagram.at(pos + 1));
        pos += 2;
        if (pos + rdlength > datagram.size())
            break;
        if (type == 12) { // PTR
            int rpos = pos;
            const QString target = parseDnsName(datagram, rpos);
            if (hasCrealityPrefix(target)) {
                CrealityService svc;
                svc.instanceName = target;
                svc.source = from;
                decodeCrealityInstance(target, &svc.typeCode, &svc.moonrakerPort);
                result << svc;
            }
        }
        pos += rdlength;
    }
    return result;
}

void MdnsDiscovery::start(int windowMs)
{
    if (scanning_)
        return;
    if (!socket_->bind(QHostAddress::AnyIPv4, kMdnsPort,
                       QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        emit errorOccurred(QStringLiteral("mDNS bind failed: ")
                           + socket_->errorString());
        return;
    }
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (iface.flags() & QNetworkInterface::CanMulticast)
            socket_->joinMulticastGroup(kMdnsGroup, iface);
    }
    scanning_ = true;
    emit scanningChanged();
    sendEnumerationQuery();
    QTimer::singleShot(windowMs, this, &MdnsDiscovery::finishWindow);
}

void MdnsDiscovery::stop()
{
    if (!scanning_)
        return;
    finishWindow();
}

void MdnsDiscovery::finishWindow()
{
    if (!scanning_)
        return;
    scanning_ = false;
    socket_->close();
    emit scanningChanged();
    emit discoveryFinished(seenInstances_.size());
}

void MdnsDiscovery::sendEnumerationQuery()
{
    const QByteArray query = buildEnumerationQuery();
    socket_->writeDatagram(query, kMdnsGroup, kMdnsPort);
}

void MdnsDiscovery::readPendingDatagrams()
{
    while (socket_->hasPendingDatagrams()) {
        QByteArray datagram(int(socket_->pendingDatagramSize()), Qt::Uninitialized);
        QHostAddress from;
        socket_->readDatagram(datagram.data(), datagram.size(), &from);
        // Only parse responses (QR bit set, flags high byte 0x84 typical);
        // our own queries have QR=0 and are skipped naturally by the PTR
        // answer walk (QDCOUNT=1, ANCOUNT=0).
        for (const CrealityService &svc : parseResponseMessage(datagram, from)) {
            if (seenInstances_.contains(svc.instanceName))
                continue;
            seenInstances_ << svc.instanceName;
            QVariantMap device;
            device.insert(QStringLiteral("instanceName"), svc.instanceName);
            device.insert(QStringLiteral("typeCode"), svc.typeCode);
            device.insert(QStringLiteral("moonrakerPort"), svc.moonrakerPort);
            device.insert(QStringLiteral("ip"), svc.source.toString());
            device.insert(QStringLiteral("isMoonraker"), svc.typeCode == QLatin1String("00"));
            emit deviceFound(device);
        }
    }
}
