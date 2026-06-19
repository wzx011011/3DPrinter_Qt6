#include "NetworkServiceMock.h"
#include "SsdpDiscovery.h"

#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QUrl>

NetworkServiceMock::NetworkServiceMock(QObject *parent)
    : QObject(parent)
{
}

NetworkServiceMock::~NetworkServiceMock()
{
    if (ssdp_) delete ssdp_;
}

bool NetworkServiceMock::online() const { return online_; }
int NetworkServiceMock::latencyMs() const { return latencyMs_; }

void NetworkServiceMock::probe()
{
    // v2.6 SSDP-02: 真实网络检测（ping 公网 DNS）
    // 简化: 用 QNetworkAccessManager 测连通性 + 测延迟
    tick_++;

    // 尝试连接 8.8.8.8（Google DNS）测连通性
    QElapsedTimer timer;
    timer.start();

    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl("http://www.baidu.com"));  // 国内可达
    req.setTransferTimeout(3000);

    QNetworkReply *reply = nam->head(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, nam, timer]() {
        latencyMs_ = int(timer.elapsed());
        online_ = (reply->error() == QNetworkReply::NoError);
        reply->deleteLater();
        nam->deleteLater();
        emit networkChanged();
        qDebug("[Network] probe: online=%d latency=%dms", online_, latencyMs_);
    });
}

void NetworkServiceMock::discoverDevices()
{
    // v2.6 SSDP-02: 启动 SSDP 设备发现
    if (!ssdp_) {
        ssdp_ = new owzx::SsdpDiscovery(this);
        connect(ssdp_, &owzx::SsdpDiscovery::deviceFound, this, [this](const QVariantMap &dev) {
            discoveredCount_++;
            emit deviceDiscovered(dev);
            emit networkChanged();
        });
        connect(ssdp_, &owzx::SsdpDiscovery::discoveryFinished, this, [this](int count) {
            qDebug("[Network] SSDP discovery finished: %d devices", count);
            emit discoveryComplete(count);
        });
    }

    discoveredCount_ = 0;
    ssdp_->startDiscovery(5000);
    qDebug("[Network] SSDP discovery started (5s timeout)");
}

QStringList NetworkServiceMock::discoveredIps() const
{
    if (!ssdp_) return {};
    return ssdp_->discoveredIps();
}

QVariantMap NetworkServiceMock::discoveredDeviceAt(int index) const
{
    if (!ssdp_) return {};
    return ssdp_->deviceAt(index);
}
