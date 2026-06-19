#include "SsdpDiscovery.h"

#include <QNetworkInterface>
#include <QVariant>
#include <QDebug>

namespace owzx {

// SSDP M-SEARCH 多播消息（搜索所有 3D 打印机）
const char *SsdpDiscovery::M_SEARCH_MSG =
    "M-SEARCH * HTTP/1.1\r\n"
    "HOST: 239.255.255.250:1900\r\n"
    "MAN: \"ssdp:discover\"\r\n"
    "MX: 3\r\n"
    "ST: ssdp:all\r\n"
    "\r\n";

const QHostAddress SsdpDiscovery::SSDP_ADDR("239.255.255.250");

SsdpDiscovery::SsdpDiscovery(QObject *parent)
    : QObject(parent)
    , m_socket(new QUdpSocket(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SsdpDiscovery::onTimeout);
    connect(m_socket, &QUdpSocket::readyRead, this, &SsdpDiscovery::onReadyRead);
}

SsdpDiscovery::~SsdpDiscovery()
{
    stopDiscovery();
}

void SsdpDiscovery::startDiscovery(int timeoutMs)
{
    m_devices.clear();
    m_seenIps.clear();

    // 绑定到任意端口（接收多播响应）
    if (!m_socket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
    {
        // 绑定失败，尝试不绑定直接发送（某些系统允许）
        qWarning("[SSDP] bind failed: %s", m_socket->errorString().toUtf8().constData());
    }

    // 加入多播组
    m_socket->joinMulticastGroup(SSDP_ADDR);

    // 发送 M-SEARCH（多次发送提高发现率）
    QByteArray msg(M_SEARCH_MSG);
    for (int i = 0; i < 3; ++i)
    {
        m_socket->writeDatagram(msg, SSDP_ADDR, SSDP_PORT);
    }

    qDebug("[SSDP] M-SEARCH sent (3x), waiting %dms for responses", timeoutMs);
    m_timeoutTimer->start(timeoutMs);
}

void SsdpDiscovery::stopDiscovery()
{
    if (m_timeoutTimer->isActive())
        m_timeoutTimer->stop();
    if (m_socket->state() == QAbstractSocket::BoundState)
    {
        m_socket->leaveMulticastGroup(SSDP_ADDR);
        m_socket->abort();
    }
}

void SsdpDiscovery::onReadyRead()
{
    while (m_socket->hasPendingDatagrams())
    {
        QByteArray data;
        QHostAddress sender;
        quint16 senderPort;
        data.resize(m_socket->pendingDatagramSize());
        m_socket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        // 去重（同一 IP 可能多次响应）
        QString ip = sender.toString();
        if (m_seenIps.contains(ip))
            continue;

        DiscoveredDevice dev = parseResponse(data, sender);
        if (!dev.ip.isEmpty())
        {
            m_seenIps[ip] = true;
            m_devices.append(dev);

            QVariantMap vm;
            vm["ip"] = dev.ip;
            vm["serial"] = dev.serial;
            vm["model"] = dev.model;
            vm["name"] = dev.name;
            vm["port"] = dev.port;
            vm["isBambu"] = dev.isBambu;
            emit deviceFound(vm);

            qDebug("[SSDP] found device: %s (%s) at %s",
                   dev.name.toUtf8().constData(),
                   dev.model.toUtf8().constData(),
                   dev.ip.toUtf8().constData());
        }
    }
}

void SsdpDiscovery::onTimeout()
{
    stopDiscovery();
    qDebug("[SSDP] discovery finished, found %d devices", m_devices.size());
    emit discoveryFinished(m_devices.size());
}

DiscoveredDevice SsdpDiscovery::parseResponse(const QByteArray &data, const QHostAddress &sender)
{
    DiscoveredDevice dev;
    dev.ip = sender.toString();

    // 解析 HTTP 风格的 SSDP 响应头
    QString response = QString::fromUtf8(data);
    QStringList lines = response.split("\r\n", Qt::SkipEmptyParts);

    for (const QString &line : lines)
    {
        QString lower = line.toLower();
        if (lower.startsWith("location:"))
        {
            // Location: http://192.168.1.100:80/info
            dev.ip = line.mid(9).trimmed();
            // 提取 IP（去掉 http:// 和端口）
            dev.ip = dev.ip.replace("http://", "").section(":", 0, 0);
        }
        else if (lower.startsWith("st:"))
        {
            // ST: urn:bambu:device:3dprinter:1 / urn:creality:...
            if (lower.contains("bambu")) dev.isBambu = true;
            if (lower.contains("3dprinter")) dev.model = "3D Printer";
        }
        else if (lower.startsWith("usn:"))
        {
            // USN: uuid:<serial>::urn:...
            // 提取 serial
            int uuidStart = lower.indexOf("uuid:") + 5;
            int uuidEnd = lower.indexOf("::", uuidStart);
            if (uuidStart > 4 && uuidEnd > uuidStart)
                dev.serial = line.mid(uuidStart, uuidEnd - uuidStart).toUpper();
        }
        else if (lower.startsWith("server:"))
        {
            // Server: Bambu Lab / Creality
            if (lower.contains("bambu")) dev.isBambu = true;
            if (lower.contains("creality")) dev.isBambu = false;
            dev.name = line.mid(7).trimmed();
        }
    }

    // 端口（Bambu 默认 8883 TLS，我们简化用 1883 明文）
    dev.port = dev.isBambu ? 8883 : 1883;

    return dev;
}

QVariantMap SsdpDiscovery::deviceAt(int index) const
{
    QVariantMap vm;
    if (index < 0 || index >= m_devices.size())
        return vm;
    const auto &d = m_devices[index];
    vm["ip"] = d.ip;
    vm["serial"] = d.serial;
    vm["model"] = d.model;
    vm["name"] = d.name;
    vm["port"] = d.port;
    vm["isBambu"] = d.isBambu;
    return vm;
}

QStringList SsdpDiscovery::discoveredIps() const
{
    QStringList ips;
    for (const auto &d : m_devices)
        ips << d.ip;
    return ips;
}

} // namespace owzx
