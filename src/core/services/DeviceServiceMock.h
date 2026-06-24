#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QVariantMap>

// v2.5 DEV-02: MqttClient 集成（替代 mock 数据）
namespace owzx { class MqttClient; }
class FtpUploader;  // v2.8 P2-C: FTP 上传服务

/// AMS (Automatic Material System) slot info (对齐上游 AMSScreen / AMSModel)
struct MockAmsSlot
{
  QString filamentType;   // e.g. "PLA", "PETG", "ABS"
  QString color;          // hex color e.g. "#FF5733"
  float remainingWeight = 0.0f;  // grams remaining
  bool active = false;    // currently selected for printing
};

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
  QList<MockAmsSlot> amsSlots; // AMS filament slots (typically 4)
  // Device lights (对齐上游 MachineObject chamber_light / work_light)
  bool    chamberLightOn    = false;
  bool    workLightOn       = false;
  // Device camera recording (对齐上游 MachineObject camera_recording / timelapse)
  bool    cameraRecording   = false;
  bool    cameraTimelapse   = false;
  // v2.7 P2-A: MQTT 连接参数（Bambu LAN 协议）。access code 在加设备或连接时录入。
  QString accessCode;        ///< Bambu LAN access code（MQTT/FTP 密码，username=bblp）
  int     mqttPort    = 8883; ///< Bambu MQTT 端口（默认 8883）
  // v2.7 P2-A: 实时遥测（由 MQTT messageReceived 解析填充）
  int     bedTemperature      = 0;  ///< bed_temper（当前热床温度）
  int     nozzleTargetTemp    = 0;  ///< nozzle_target_temper（喷嘴目标温度）
  int     bedTargetTemp       = 0;  ///< bed_target_temper（热床目标温度）
  int     totalLayerNum       = 0;  ///< total_layer_num（总层数）
  int     currentLayerNum     = 0;  ///< layer_num（当前层）
  int     remainingTime       = 0;  ///< mc_remaining_time（剩余分钟）
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
  /// v2.7 P2-A: 实时遥测（由 MQTT 解析填充）
  Q_PROPERTY(int selectedDeviceBedTemperature READ selectedDeviceBedTemperature NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceNozzleTargetTemp READ selectedDeviceNozzleTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceBedTargetTemp READ selectedDeviceBedTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceTotalLayerNum READ selectedDeviceTotalLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceCurrentLayerNum READ selectedDeviceCurrentLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceRemainingTime READ selectedDeviceRemainingTime NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool mqttConnected READ isMqttConnected NOTIFY selectedDeviceChanged)
  /// v2.7 P2-A: MQTT 连接参数（连接打印机时读取）
  Q_PROPERTY(QString selectedDeviceAccessCode READ selectedDeviceAccessCode NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceMqttPort READ selectedDeviceMqttPort NOTIFY selectedDeviceChanged)
  /// Convenience: signal strength of selected device (0-3)
  Q_PROPERTY(int selectedDeviceSignalStrength READ selectedDeviceSignalStrength NOTIFY selectedDeviceChanged)
  /// Convenience: chamber light on/off of selected device
  Q_PROPERTY(bool selectedDeviceChamberLightOn READ selectedDeviceChamberLightOn NOTIFY selectedDeviceChanged)
  /// Convenience: work light on/off of selected device
  Q_PROPERTY(bool selectedDeviceWorkLightOn READ selectedDeviceWorkLightOn NOTIFY selectedDeviceChanged)
  /// Convenience: camera recording on/off of selected device
  Q_PROPERTY(bool selectedDeviceCameraRecording READ selectedDeviceCameraRecording NOTIFY selectedDeviceChanged)
  /// Convenience: camera timelapse on/off of selected device
  Q_PROPERTY(bool selectedDeviceCameraTimelapse READ selectedDeviceCameraTimelapse NOTIFY selectedDeviceChanged)
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
  /// v2.7 P2-A: 新遥测字段 getter
  int selectedDeviceBedTemperature() const;
  int selectedDeviceNozzleTargetTemp() const;
  int selectedDeviceBedTargetTemp() const;
  int selectedDeviceTotalLayerNum() const;
  int selectedDeviceCurrentLayerNum() const;
  int selectedDeviceRemainingTime() const;
  /// v2.7 P2-A: MQTT 连接参数 getter
  QString selectedDeviceAccessCode() const;
  int selectedDeviceMqttPort() const;
  /// v2.7 P2-A: 设置选中设备的 access code + 端口（连接对话框录入后调用）
  Q_INVOKABLE void setSelectedDeviceAccessCode(const QString &accessCode, int port = 8883);
  Q_INVOKABLE bool applyMqttReportPayload(const QString &payload, int deviceIndex = -1);
  static QString buildPrintCommandEnvelope(const QString &command,
                                           const QString &param,
                                           int sequenceId);
  static QString buildPrintCommandTopic(const QString &serial);
  static QString buildPrintRemotePath(const QString &gcodePath);
  int selectedDeviceSignalStrength() const;
  bool selectedDeviceChamberLightOn() const;
  bool selectedDeviceWorkLightOn() const;
  bool selectedDeviceCameraRecording() const;
  bool selectedDeviceCameraTimelapse() const;

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
  // v2.5 DEV-02: MQTT 真实连接（替代 mock 模拟）
  Q_INVOKABLE void connectViaMqtt(const QString &host, int port, const QString &accessCode);
  Q_INVOKABLE void disconnectMqtt();
  Q_INVOKABLE bool isMqttConnected() const;

  /// 打印任务控制（对齐上游 Plater print/pause/resume/stop）
  Q_INVOKABLE void startPrint(int filteredIndex, const QString &gcodePath);

  /// v2.8 P2-C: 通过 FTP 上传 .gcode 到打印机，然后发 MQTT print 命令启动打印。
  /// 仅在 MQTT 已连接时启用（需要 IP + access code）。
  /// uploadProgress 信号反馈上传百分比，uploadFinished 反馈结果。
  /// 返回 true 表示上传已启动（结果异步通知）。
  Q_INVOKABLE bool sendPrintViaFtp(int filteredIndex, const QString &gcodePath);
  Q_INVOKABLE void pausePrint(int filteredIndex);
  Q_INVOKABLE void resumePrint(int filteredIndex);
  Q_INVOKABLE void stopPrint(int filteredIndex);

  /// v2.7 P2-B: 通过 MQTT 发布 Bambu 打印控制命令。
  /// 构造 {"print":{"sequence_id":N,"command":"<cmd>",...}} 信封，
  /// 发布到 device/<serial>/request（序列号取自连接设备的 sn）。
  /// command 如 "pause"/"resume"/"stop"/"gcode_line" 等。
  /// 仅当 MQTT 已连接时实际发布；返回是否发布成功。
  /// lastPublishPayload() 暴露最后构造的 JSON（供 INT-05 测试断言）。
  Q_INVOKABLE bool publishPrintCommand(const QString &command, const QString &param = QString());
  QString lastPublishPayload() const { return lastPublishPayload_; }
  QString lastPublishTopic() const { return lastPublishTopic_; }

  /// 获取设备打印状态信息（对齐上游 MachineObject 打印状态）
  Q_INVOKABLE QString devicePrintStage(int filteredIndex) const;
  Q_INVOKABLE int devicePrintLayer(int filteredIndex) const;
  Q_INVOKABLE int devicePrintTimeLeft(int filteredIndex) const; // seconds

  /// HMS 健康管理（对齐上游 HMSPanel / DeviceManager hms_list）
  Q_INVOKABLE int selectedDeviceHmsCount() const;
  Q_INVOKABLE int selectedDeviceUnreadHmsCount() const;
  Q_INVOKABLE QVariantMap selectedDeviceHmsAt(int hmsIndex) const;
  Q_INVOKABLE void markHmsRead(int hmsIndex);

  /// 灯光和录制控制（对齐上游 MachineObject lights / camera）
  Q_INVOKABLE void setChamberLight(bool on);
  Q_INVOKABLE void setWorkLight(bool on);
  Q_INVOKABLE void toggleRecording();
  Q_INVOKABLE void toggleTimelapse();

  /// AMS 多耗材管理（对齐上游 AMSScreen / AMSModel）
  Q_INVOKABLE int selectedDeviceAmsSlotCount() const;
  Q_INVOKABLE QVariantMap selectedDeviceAmsSlotAt(int slotIndex) const;
  Q_INVOKABLE void setSelectedDeviceAmsSlot(int slotIndex);

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

  // v2.5 DEV-02: MqttClient 实例（真实 MQTT 通信）
  owzx::MqttClient *mqttClient_ = nullptr;
  int mqttConnectedDeviceIndex_ = -1;  ///< MQTT 连接的设备 filteredIndex
  // v2.7 P2-A: MQTT 连接参数缓存（连接重试 + topic 构造用）
  QString pendingMqttHost_;
  int pendingMqttPort_ = 8883;
  // v2.8 P2-C: FTP 上传服务（发送打印任务时用于上传 .gcode）
  FtpUploader *ftpUploader_ = nullptr;
  QString lastPrintRemotePath_;  ///< 最后一次上传的 FTP 远程路径（供测试）
  // v2.7 P2-B: MQTT publish 调试（供 INT-05 断言 payload/topic）
  QString lastPublishPayload_;
  QString lastPublishTopic_;
  int mqttSequenceId_ = 0;

  /// HMS 通知列表（对齐上游 DeviceManager hms_list）
  /// key = device realIndex, value = list of HMS items
  QMap<int, QList<MockHmsItem>> hmsLists_;
};
