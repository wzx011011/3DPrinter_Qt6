#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUdpSocket>
#include <QHostAddress>
#include <QHash>
#include <QTimer>

// v2.6 SSDP-01: SSDP/mDNS 局域网设备自动发现
// 对齐上游: Bonjour/SSDP 发现 Bambu/Creality 打印机
//
// 协议: UDP 多播 239.255.255.250:1900 (SSDP)
// Bambu: 响应含 ST: urn:bambu:device:3dprinter:1
// Creality: 响应含 ST: urn:creality:device:3dprinter:1

namespace owzx {

struct DiscoveredDevice {
    QString ip;           // 设备 IP
    QString serial;       // 序列号
    QString model;        // 型号 (K1 Max / Ender 等)
    QString name;         // 设备名
    int port = 1883;      // MQTT 端口
    bool isBambu = false; // Bambu vs Creality
};

class SsdpDiscovery : public QObject
{
    Q_OBJECT
public:
    explicit SsdpDiscovery(QObject *parent = nullptr);
    ~SsdpDiscovery();

    /// 启动设备发现（发送 M-SEARCH 多播，监听 5 秒响应）
    Q_INVOKABLE void startDiscovery(int timeoutMs = 5000);
    Q_INVOKABLE void stopDiscovery();

    /// 已发现的设备列表
    Q_INVOKABLE int discoveredCount() const { return m_devices.size(); }
    Q_INVOKABLE QVariantMap deviceAt(int index) const;
    Q_INVOKABLE QStringList discoveredIps() const;
    static DiscoveredDevice parseResponseDatagram(const QByteArray &data,
                                                  const QHostAddress &sender);

signals:
    /// 发现新设备
    void deviceFound(const QVariantMap &device);
    /// 发现完成（超时或手动停止）
    void discoveryFinished(int count);
    /// 发现错误
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QUdpSocket *m_socket = nullptr;
    QTimer *m_timeoutTimer = nullptr;
    QList<DiscoveredDevice> m_devices;
    QHash<QString, bool> m_seenIps; // 去重

    // SSDP M-SEARCH 请求
    static const char *M_SEARCH_MSG;
    static const QHostAddress SSDP_ADDR;
    static const int SSDP_PORT = 1900;

    // 解析 SSDP 响应
};

} // namespace owzx
