#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantMap>

/// Rich mock device record exposed to QML via QVariantMap.
/// Roles consumed by MonitorPage device model:
///   name, model (printer type), sn, status (string), online (bool),
///   progress (0-100), taskName, ip, temperature, signalStrength (0-3)
struct MockDevice
{
  QString name;
  QString model;
  QString sn;
  QString status;     // e.g. "printing", "idle", "offline", "connecting"
  bool    online      = false;
  int     progress    = 0;
  QString taskName;   // current print task or empty
  QString ip;
  int     temperature = 0;
  int     signalStrength = 0; // 0=none, 1=weak, 2=medium, 3=strong
};

/// HMS notification item (对齐上游 DeviceManager HMSItem / HMSPanel HMSNotifyItem)
struct MockHmsItem
{
  int moduleId = 0;      // component module ID
  int moduleNum = 0;     // module instance
  int partId = 0;        // specific part
  int msgLevel = 3;      // 1=Fatal, 2=Serious, 3=Common, 4=Info
  int msgCode = 0;       // error code
  bool alreadyRead = false;
  QString message;       // human-readable message
  QString wikiUrl;        // optional wiki link

  MockHmsItem() = default;
  MockHmsItem(int modId, int modNum, int pId, int level, int code, bool read,
              const QString &msg, const QString &wiki)
      : moduleId(modId), moduleNum(modNum), partId(pId), msgLevel(level),
        msgCode(code), alreadyRead(read), message(msg), wikiUrl(wiki) {}
};

class DeviceServiceMock final : public QObject
{
  Q_OBJECT
  /// Number of devices in the mock list (QML can use this as Repeater model count)
  Q_PROPERTY(int deviceCount READ deviceCount NOTIFY devicesChanged)
  /// JSON array string of all devices (fallback for simple binding)
  Q_PROPERTY(QString deviceListJson READ deviceListJson NOTIFY devicesChanged)
  /// Selected device index (-1 = none)
  Q_PROPERTY(int selectedDeviceIndex READ selectedDeviceIndex NOTIFY selectedDeviceChanged)
  /// Convenience: name of selected device
  Q_PROPERTY(QString selectedDeviceName READ selectedDeviceName NOTIFY selectedDeviceChanged)
  /// Convenience: model type of selected device
  Q_PROPERTY(QString selectedDeviceModel READ selectedDeviceModel NOTIFY selectedDeviceChanged)
  /// Convenience: online status of selected device
  Q_PROPERTY(bool selectedDeviceOnline READ selectedDeviceOnline NOTIFY selectedDeviceChanged)
  /// Convenience: status text of selected device
  Q_PROPERTY(QString selectedDeviceStatus READ selectedDeviceStatus NOTIFY selectedDeviceChanged)
  /// Convenience: print progress of selected device (0-100)
  Q_PROPERTY(int selectedDeviceProgress READ selectedDeviceProgress NOTIFY selectedDeviceChanged)
  /// Convenience: task name of selected device
  Q_PROPERTY(QString selectedDeviceTaskName READ selectedDeviceTaskName NOTIFY selectedDeviceChanged)
  /// Convenience: IP of selected device
  Q_PROPERTY(QString selectedDeviceIp READ selectedDeviceIp NOTIFY selectedDeviceChanged)
  /// Convenience: temperature of selected device
  Q_PROPERTY(int selectedDeviceTemperature READ selectedDeviceTemperature NOTIFY selectedDeviceChanged)
  /// Convenience: signal strength of selected device (0-3)
  Q_PROPERTY(int selectedDeviceSignalStrength READ selectedDeviceSignalStrength NOTIFY selectedDeviceChanged)
  /// Search/filter text
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
  /// Number of devices after filtering
  Q_PROPERTY(int filteredDeviceCount READ filteredDeviceCount NOTIFY devicesChanged)
  /// Overall network online status
  Q_PROPERTY(bool networkOnline READ networkOnline NOTIFY devicesChanged)

public:
  explicit DeviceServiceMock(QObject *parent = nullptr);

  int deviceCount() const;
  QString deviceListJson() const;

  int selectedDeviceIndex() const;
  QString selectedDeviceName() const;
  QString selectedDeviceModel() const;
  bool selectedDeviceOnline() const;
  QString selectedDeviceStatus() const;
  int selectedDeviceProgress() const;
  QString selectedDeviceTaskName() const;
  QString selectedDeviceIp() const;
  int selectedDeviceTemperature() const;
  int selectedDeviceSignalStrength() const;

  QString searchText() const;
  void setSearchText(const QString &text);

  int filteredDeviceCount() const;
  bool networkOnline() const;

  /// Retrieve a device by filtered index (used by QML Repeater delegate)
  Q_INVOKABLE QVariantMap deviceAt(int filteredIndex) const;

  /// Select device by filtered index
  Q_INVOKABLE void selectDevice(int filteredIndex);

  /// Raw filtered index of the selected device (for bridging print state queries)
  int selectedFilteredIndex() const { return selectedDeviceIndex_; }

  /// Trigger mock data refresh (simulates polling)
  Q_INVOKABLE void refresh();

  /// 模拟设备扫描（对齐上游 SSDP discovery）
  Q_INVOKABLE void scanDevices();

  /// 连接/断开设备（对齐上游 DeviceManager connect / disconnect）
  Q_INVOKABLE void connectDevice(int filteredIndex);
  Q_INVOKABLE void disconnectDevice(int filteredIndex);

  /// 打印任务控制（对齐上游 Plater print/pause/resume/stop）
  Q_INVOKABLE void startPrint(int filteredIndex, const QString &gcodePath);
  Q_INVOKABLE void pausePrint(int filteredIndex);
  Q_INVOKABLE void resumePrint(int filteredIndex);
  Q_INVOKABLE void stopPrint(int filteredIndex);

  /// 获取设备打印状态信息（对齐上游 MachineObject 打印状态）
  Q_INVOKABLE QString devicePrintStage(int filteredIndex) const;
  Q_INVOKABLE int devicePrintLayer(int filteredIndex) const;
  Q_INVOKABLE int devicePrintTimeLeft(int filteredIndex) const; // seconds

  /// HMS 健康管理（对齐上游 HMSPanel / DeviceManager hms_list）
  Q_INVOKABLE int selectedDeviceHmsCount() const;
  Q_INVOKABLE int selectedDeviceUnreadHmsCount() const;
  Q_INVOKABLE QVariantMap selectedDeviceHmsAt(int hmsIndex) const;
  Q_INVOKABLE void markHmsRead(int hmsIndex);

signals:
  void devicesChanged();
  void selectedDeviceChanged();
  void searchTextChanged();
  void hmsChanged();

private:
  void buildMockDevices();
  void rebuildFilteredIndices();

  QList<MockDevice> devices_;
  QList<int> filteredIndices_; ///< indices into devices_ that pass the search filter
  int selectedDeviceIndex_ = 0;
  QString searchText_;
  int tick_ = 0;

  /// 打印模拟状态（对齐上游 PrintJob / MachineObject 打印状态机）
  struct PrintJobState {
    QString gcodePath;
    QString stage;       // e.g. "准备中", "打印中", "暂停", "完成"
    int progress = 0;
    int totalLayers = 0;
    int currentLayer = 0;
    int timeLeft = 0;    // seconds
    bool active = false;
    bool paused = false;
  };
  QMap<int, PrintJobState> printJobs_; ///< device filteredIndex → print state
  QTimer *printSimTimer_ = nullptr;

  /// HMS 通知列表（对齐上游 DeviceManager hms_list）
  /// key = device realIndex, value = list of HMS items
  QMap<int, QList<MockHmsItem>> hmsLists_;
};
