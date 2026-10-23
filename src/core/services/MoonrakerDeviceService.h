#pragma once

#include "DeviceServiceMock.h"
#include "LanDeviceProbe.h"
#include "MdnsDiscovery.h"

#include <QJsonObject>
#include <QList>
#include <QTimer>
#include <QWebSocket>

class QNetworkAccessManager;

/// MoonrakerDeviceService (feature/lan-printers, L2).
///
/// LAN-printer IDeviceDataPlane implementation — Qt port of the official
/// open-source CrealityPrint LAN stack (AGPL-3.0):
/// - 发现：MdnsDiscovery（`_creality<类型码><Moonraker端口>`）+ LanDeviceProbe
///   （/info，HTTPS 优先回退 HTTP）
/// - 状态：4408 固件机型 `ws://IP:9999` 推送（JSON method/set|get）；
///   Moonraker 机型 HTTP 轮询 `/printer/objects/query`（3s）
/// - 上传：4408 `POST http://IP:80/upload/<name>`（body 含 OK）；Moonraker
///   `POST http://IP:7125/server/files/upload`（form 字段 file）
/// - 启动：Moonraker `POST /printer/print/start?filename=`；4408 ws
///   `{"method":"set","params":{"opGcodeFile":"printprt:<path>"}}`
/// - 控制：`{pause:1|0}` / `{stop:1}`（4408）；`/printer/print/pause|resume|cancel`
///
/// 诚实语义（审计纪律）：LAN 后端无种子演示设备；未连接时控制命令无副作用；
/// sendPrintViaFtp 名字保留（接口兼容）但实现是 Moonraker/4408 HTTP 上传，
/// 失败必发 sendPrintFailed。
///
/// 范围：仅局域网。不实现创想云设备通道（云 MQTT/绑定/任务）——相关接口
/// 方法返回诚实空值。
class MoonrakerDeviceService final : public IDeviceDataPlane
{
  Q_OBJECT
  Q_PROPERTY(int deviceCount READ deviceCount NOTIFY devicesChanged)
  Q_PROPERTY(int filteredDeviceCount READ filteredDeviceCount NOTIFY devicesChanged)
  Q_PROPERTY(QString deviceListJson READ deviceListJson NOTIFY devicesChanged)
  Q_PROPERTY(bool scanning READ scanning NOTIFY devicesChanged)
  Q_PROPERTY(QString selectedDeviceName READ selectedDeviceName NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceModel READ selectedDeviceModel NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceSerial READ selectedDeviceSerial NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceOnline READ selectedDeviceOnline NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceStatus READ selectedDeviceStatus NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceProgress READ selectedDeviceProgress NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceTaskName READ selectedDeviceTaskName NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceIp READ selectedDeviceIp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceTemperature READ selectedDeviceTemperature NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceBedTemperature READ selectedDeviceBedTemperature NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceNozzleTargetTemp READ selectedDeviceNozzleTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceBedTargetTemp READ selectedDeviceBedTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceTotalLayerNum READ selectedDeviceTotalLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceCurrentLayerNum READ selectedDeviceCurrentLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceRemainingTime READ selectedDeviceRemainingTime NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool mqttConnected READ isMqttConnected NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY devicesChanged)

public:
  explicit MoonrakerDeviceService(QObject *parent = nullptr);
  ~MoonrakerDeviceService() override;

  // ── IDeviceDataPlane：设备清单 ──
  int deviceCount() const override;
  QVariantMap deviceAt(int filteredIndex) const override;
  Q_INVOKABLE void selectDevice(int filteredIndex) override;
  Q_INVOKABLE bool addManualDevice(const QString &name, const QString &ip,
                                   const QString &accessCode,
                                   int port = 8883) override;
  Q_INVOKABLE bool isDemoDevice(int) const override { return false; }
  QString deviceListJson() const override;
  int filteredDeviceCount() const override;
  QString searchText() const override;
  void setSearchText(const QString &text) override;
  int selectedDeviceIndex() const override { return selectedIndex_; }
  int selectedFilteredIndex() const override { return selectedIndex_; }

  // ── 生命周期（发现/持久化/连接）──
  Q_INVOKABLE void scanDevices() override;
  Q_INVOKABLE void refresh() override {}
  Q_INVOKABLE void connectDevice(int filteredIndex) override;
  Q_INVOKABLE void disconnectDevice(int filteredIndex) override;
  bool scanning() const { return scanning_; }

  // ── IDeviceDataPlane：凭据（LAN 无 Bambu 访问码语义，诚实缺省）──
  QString selectedDeviceAccessCode() const override { return {}; }
  int selectedDeviceMqttPort() const override { return 0; }
  void setSelectedDeviceAccessCode(const QString &, int port = 8883) override
  { Q_UNUSED(port); }

  // ── IDeviceDataPlane：任务发送（Moonraker/4408 HTTP 上传 + 启动）──
  Q_INVOKABLE bool sendPrintViaFtp(int filteredIndex,
                                   const QString &gcodePath) override;
  Q_INVOKABLE void cancelSendPrint() override;
  bool sendPrintInFlight() const override { return uploadInFlight_; }
  Q_INVOKABLE void startPrint(int filteredIndex,
                              const QString &gcodePath) override;

  // ── IDeviceDataPlane：遥测 ──
  bool selectedDeviceOnline() const override;
  QString selectedDeviceStatus() const override;
  QString selectedDeviceIp() const override;
  bool isMqttConnected() const override;
  int selectedDeviceTemperature() const override;
  int selectedDeviceBedTemperature() const override;
  int selectedDeviceNozzleTargetTemp() const override;
  int selectedDeviceBedTargetTemp() const override;
  int selectedDeviceTotalLayerNum() const override;
  int selectedDeviceCurrentLayerNum() const override;
  int selectedDeviceRemainingTime() const override;
  bool applyMqttReportPayload(const QString &payload,
                              int deviceIndex = -1) override;

  // ── 扩展面覆盖（打印控制/便捷 getter）──
  void pausePrint(int) override;
  void resumePrint(int) override;
  void stopPrint(int) override;
  QString selectedDeviceName() const override;
  QString selectedDeviceModel() const override;
  QString selectedDeviceSerial() const override;
  int selectedDeviceProgress() const override;
  QString selectedDeviceTaskName() const override;

  /// Test/dev hook: override the ws/HTTP ports for loopback stubs (production
  /// derives them from mDNS//info).
  void setPortsOverride(int wsPort, int httpPort)
  {
    portsOverrideWs_ = wsPort;
    portsOverrideHttp_ = httpPort;
  }
  /// Test hook: /info 探测跳过 HTTPS 尝试（测试二进制的 OpenSSL 符号冲突）。
  void setProbeHttpsDisabled(bool disabled) { probeHttpsDisabled_ = disabled; }

signals:
  void scanningChanged();

private:
  struct Device
  {
    QString name;       // 默认 = model，可改名
    QString model;
    QString ip;
    QString mac;
    int httpPort = 80;  // 上传/REST
    bool secure = false;
    int wssPort = 0;    // 4408 加密 ws
    bool isMoonraker = false;
    int moonrakerPort = 0;
    bool oldPrinter = false;
    bool online = false;
    QString status = QStringLiteral("offline");
    int nozzleTemp = 0, nozzleTarget = 0;
    int bedTemp = 0, bedTarget = 0;
    int progress = 0, currentLayer = 0, totalLayer = 0;
    int remainingMin = 0;
    QString taskName;
  };

  // ── 发现 ──
  void startScan();
  void onMdnsDeviceFound(const QVariantMap &device);
  void probeAndAdd(const QString &ip, const QString &nameHint);

  // ── 状态通道 ──
  void openStatusChannel();
  void closeStatusChannel();
  void onWsTextMessage(const QString &message);
  void pollMoonrakerStatus();
  void applyStatusFields(Device &dev, const QJsonObject &params);
  void wsSet(const QJsonObject &params);

  // ── 上传/启动 ──
  bool uploadAndStart(int filteredIndex, const QString &gcodePath);

  // ── 设备记录 ──
  int matchingIndexByIp(const QString &ip) const;
  Device *selected();
  const Device *selected() const;
  int filteredToReal(int filteredIndex) const;
  void loadPersisted();
  void persist() const;
  void touch();
  QString moonrakerBase(const Device &dev) const;

  MdnsDiscovery *mdns_ = nullptr;
  LanDeviceProbe *probe_ = nullptr;
  QNetworkAccessManager *manager_ = nullptr;
  QWebSocket *ws_ = nullptr;          // 4408 状态通道（选中设备）
  QTimer *moonrakerPoller_ = nullptr; // Moonraker 轮询（选中设备）
  QList<Device> devices_;
  QStringList seenInstances_;
  int selectedIndex_ = -1;
  QString searchText_;
  bool scanning_ = false;
  bool uploadInFlight_ = false;
  int portsOverrideWs_ = 0;
  int portsOverrideHttp_ = 0;
  bool probeHttpsDisabled_ = false;
};
