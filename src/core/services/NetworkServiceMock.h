#pragma once

#include <QObject>

// v2.6 SSDP-02: 真实化 NetworkService（替代 mock）
namespace owzx { class SsdpDiscovery; }

class NetworkServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool online READ online NOTIFY networkChanged)
  Q_PROPERTY(int latencyMs READ latencyMs NOTIFY networkChanged)
  // v2.6 SSDP: 发现的设备数
  Q_PROPERTY(int discoveredDeviceCount READ discoveredDeviceCount NOTIFY networkChanged)

public:
  explicit NetworkServiceMock(QObject *parent = nullptr);
  ~NetworkServiceMock();

  bool online() const;
  int latencyMs() const;
  int discoveredDeviceCount() const { return discoveredCount_; }

  /// 网络连通性检测（v2.6: 真实 ICMP ping + 连通性判断）
  Q_INVOKABLE void probe();
  /// v2.6 SSDP-02: 启动局域网设备发现（5秒超时）
  Q_INVOKABLE void discoverDevices();
  /// 获取发现的设备 IP 列表
  Q_INVOKABLE QStringList discoveredIps() const;
  /// 获取发现的设备详情
  Q_INVOKABLE QVariantMap discoveredDeviceAt(int index) const;

signals:
  void networkChanged();
  /// v2.6 SSDP: 发现新设备
  void deviceDiscovered(const QVariantMap &device);
  /// v2.6 SSDP: 发现阶段完成
  void discoveryComplete(int count);

private:
  bool online_ = true;
  int latencyMs_ = 18;
  int tick_ = 0;
  int discoveredCount_ = 0;

  // v2.6 SSDP: 设备发现器
  owzx::SsdpDiscovery *ssdp_ = nullptr;
};
