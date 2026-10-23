#pragma once

#include <QObject>
#include <QTimer>
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
  // X1 演示披露：种子假设备（编造的 192.168.1.x 条目）= true；手动添加的
  // 真实打印机 = false。真发送链路（sendPrintViaFtp）拒绝把 demo 条目当
  // FTP/MQTT 目标 host。
  bool    demo                = true;
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

/// X1（审计④章）：Device 数据面窄接口——设备清单 / 凭据 / 任务发送 / 遥测。
///
/// 刻意不搬 300+ Q_INVOKABLE 上帝面（审计风险项：接口切分不当会把上帝对象
/// 固化成上帝接口）——接口只收编真实后端（bambu_networking / 局域网打印机栈）
/// 即将到达的数据面角色；AMS/HMS/灯光/打印模拟等展示面留在具体类。迁移评估
/// 结论：mock→真的替换从"原位改写 12.5k 行具体类"变为"实现既有窄接口 +
/// 切换装配"，BackendContext 保持唯一装配点，测试以本接口注入替身（注入缝，
/// 替代锁生产 mock 假时序的旧写法）。
class IDeviceDataPlane : public QObject
{
  Q_OBJECT

public:
  explicit IDeviceDataPlane(QObject *parent = nullptr) : QObject(parent) {}
  ~IDeviceDataPlane() override = default;

  // ── 设备清单 ──
  virtual int deviceCount() const = 0;
  /// filteredIndex → 设备数据（含 demo 披露键，见 isDemoDevice）
  virtual QVariantMap deviceAt(int filteredIndex) const = 0;
  virtual void selectDevice(int filteredIndex) = 0;
  /// 手动添加设备（真实目标：用户录入的 IP + access code）
  virtual bool addManualDevice(const QString &name, const QString &ip,
                               const QString &accessCode, int port = 8883) = 0;
  /// X1 演示披露：种子假设备条目（编造的 192.168.1.x 等）返回 true。
  /// 真发送链路以本旗标拒绝把假设备当 FTP/MQTT 目标 host。
  virtual bool isDemoDevice(int filteredIndex) const = 0;

  // ── 凭据 ──
  virtual QString selectedDeviceAccessCode() const = 0;
  virtual int selectedDeviceMqttPort() const = 0;
  virtual void setSelectedDeviceAccessCode(const QString &accessCode, int port = 8883) = 0;

  // ── 任务发送（真链路：FTP 上传 + MQTT 打印命令）──
  /// U11 契约：返回 false 时一定伴随 sendPrintFailed（原因可读），绝不静默
  /// 降级成 mock 打印态。
  virtual bool sendPrintViaFtp(int filteredIndex, const QString &gcodePath) = 0;
  virtual void cancelSendPrint() = 0;
  virtual bool sendPrintInFlight() const = 0;

  // ── 遥测 ──
  virtual bool selectedDeviceOnline() const = 0;
  virtual QString selectedDeviceStatus() const = 0;
  virtual QString selectedDeviceIp() const = 0;
  virtual bool isMqttConnected() const = 0;
  virtual int selectedDeviceTemperature() const = 0;
  virtual int selectedDeviceBedTemperature() const = 0;
  virtual int selectedDeviceNozzleTargetTemp() const = 0;
  virtual int selectedDeviceBedTargetTemp() const = 0;
  virtual int selectedDeviceTotalLayerNum() const = 0;
  virtual int selectedDeviceCurrentLayerNum() const = 0;
  virtual int selectedDeviceRemainingTime() const = 0;
  /// MQTT report payload → 遥测字段（Bambu device/<serial>/report 信封）
  virtual bool applyMqttReportPayload(const QString &payload, int deviceIndex = -1) = 0;

  // ── 显示/控制扩展面（feature/lan-printers L2）──
  // MonitorViewModel 的完整转发面。默认实现 = 该后端不支持（no-op/空/0），
  // 遵循"不允许 mock 假成功"纪律：不支持的命令没有副作用、不产生假状态。
  // 窄核（上方纯虚）不变，仍是数据面契约锁。

  // 列表/过滤/选中
  virtual QString deviceListJson() const { return QStringLiteral("[]"); }
  virtual int filteredDeviceCount() const { return deviceCount(); }
  virtual QString searchText() const { return {}; }
  virtual void setSearchText(const QString &) {}
  virtual int selectedDeviceIndex() const { return -1; }
  virtual int selectedFilteredIndex() const { return -1; }
  // 生命周期
  virtual void scanDevices() {}
  virtual void refresh() {}
  virtual void connectDevice(int) {}
  virtual void disconnectDevice(int) {}
  // 打印控制
  virtual void startPrint(int, const QString &) {}
  virtual void pausePrint(int) {}
  virtual void resumePrint(int) {}
  virtual void stopPrint(int) {}
  virtual void setChamberLight(bool) {}
  virtual void setWorkLight(bool) {}
  virtual void toggleRecording() {}
  virtual void toggleTimelapse() {}
  // 发送任务状态机（SelectMachineDialog/PrintDialog 面）
  virtual void startSendJob(int, const QString &) {}
  virtual void cancelSendJob() {}
  virtual int sendJobState() const { return 0; }
  virtual int sendJobProgress() const { return 0; }
  virtual QString sendJobErrorCode() const { return {}; }
  virtual QString sendJobErrorDesc() const { return {}; }
  virtual QString sendJobErrorExtra() const { return {}; }
  // 打印选项
  virtual QVariantMap printOptions() const { return {}; }
  virtual QString printOptionValue(const QString &) const { return {}; }
  virtual void setPrintOptionValue(const QString &, const QString &) {}
  virtual bool printOptionSupported(const QString &) const { return false; }
  // 便捷 getter
  virtual QString selectedDeviceName() const { return {}; }
  virtual QString selectedDeviceModel() const { return {}; }
  virtual QString selectedDeviceSerial() const { return {}; }
  virtual int selectedDeviceProgress() const { return 0; }
  virtual QString selectedDeviceTaskName() const { return {}; }
  virtual int selectedDeviceSignalStrength() const { return 0; }
  virtual bool selectedDeviceChamberLightOn() const { return false; }
  virtual bool selectedDeviceWorkLightOn() const { return false; }
  virtual bool selectedDeviceCameraRecording() const { return false; }
  virtual bool selectedDeviceCameraTimelapse() const { return false; }
  virtual bool selectedDeviceFilesystemSupported() const { return false; }
  // 设备级打印状态（对齐上游 per-device 查询）
  virtual QString devicePrintStage(int) const { return {}; }
  virtual int devicePrintLayer(int) const { return 0; }
  virtual int devicePrintTimeLeft(int) const { return 0; }
  // HMS/AMS（mock 展示面；真后端默认空 = 诚实不支持）
  virtual void markHmsRead(int) {}
  virtual int selectedDeviceHmsCount() const { return 0; }
  virtual QVariantMap selectedDeviceHmsAt(int) const { return {}; }
  virtual int selectedDeviceUnreadHmsCount() const { return 0; }
  virtual int selectedDeviceAmsSlotCount() const { return 0; }
  virtual QVariantMap selectedDeviceAmsSlotAt(int) const { return {}; }
  virtual void setSelectedDeviceAmsSlot(int) {}

signals:
  void devicesChanged();
  void selectedDeviceChanged();
  // ── 自有面信号（MonitorViewModel 连接面；mock 与 LAN 后端均可发）──
  void searchTextChanged();
  void hmsChanged();
  void sendJobChanged();
  void printOptionsChanged();
  // ── U11: send-print（FTP+MQTT）真实链路进度与终态 ──
  void sendPrintUploadStarted();
  void sendPrintUploadProgress(int percent);
  void sendPrintSucceeded();
  void sendPrintFailed(const QString &reason);
  void sendPrintCancelled();
};

class DeviceServiceMock final : public IDeviceDataPlane
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
  /// Convenience: serial number of selected device
  Q_PROPERTY(QString selectedDeviceSerial READ selectedDeviceSerial NOTIFY selectedDeviceChanged)
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
  /// Whether the selected device exposes a browsable and mutable filesystem.
  Q_PROPERTY(bool selectedDeviceFilesystemSupported READ selectedDeviceFilesystemSupported NOTIFY selectedDeviceChanged)
  /// Search/filter text
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
  /// Number of devices after filtering
  Q_PROPERTY(int filteredDeviceCount READ filteredDeviceCount NOTIFY devicesChanged)
  /// Overall network online status
  Q_PROPERTY(bool networkOnline READ networkOnline NOTIFY devicesChanged)

  // ── SelectMachineDialog send-job state machine ─────────────────────────
  // Mirrors the upstream SelectMachine.cpp bottom wxSimplebook pages:
  // 0 = prepare (Send button), 1 = sending (status bar progress), 2 = finish
  // ("Send complete"). Upstream SelectMachine.cpp:651-705.
  Q_PROPERTY(int sendJobState READ sendJobState NOTIFY sendJobChanged)
  /// Send/upload progress percent (0-100) while sendJobState == 1
  Q_PROPERTY(int sendJobProgress READ sendJobProgress NOTIFY sendJobChanged)
  /// Failure detail for the failed-info area (empty while no failure).
  /// Upstream SelectMachine.cpp:707-783 (Error code / Error desc / Extra info).
  Q_PROPERTY(QString sendJobErrorCode READ sendJobErrorCode NOTIFY sendJobChanged)
  Q_PROPERTY(QString sendJobErrorDesc READ sendJobErrorDesc NOTIFY sendJobChanged)
  Q_PROPERTY(QString sendJobErrorExtra READ sendJobErrorExtra NOTIFY sendJobChanged)

public:
  explicit DeviceServiceMock(QObject *parent = nullptr);

  // ── IDeviceDataPlane：设备清单（override 由接口锁定契约；QML 可达面
  //    保持 Q_INVOKABLE 不变） ──
  int deviceCount() const override;
  Q_INVOKABLE QVariantMap deviceAt(int filteredIndex) const override;
  Q_INVOKABLE void selectDevice(int filteredIndex) override;
  Q_INVOKABLE bool addManualDevice(const QString &name, const QString &ip,
                                   const QString &accessCode, int port = 8883) override;
  Q_INVOKABLE bool isDemoDevice(int filteredIndex) const override;

  // ── IDeviceDataPlane：凭据 ──
  QString selectedDeviceAccessCode() const override;
  int selectedDeviceMqttPort() const override;
  Q_INVOKABLE void setSelectedDeviceAccessCode(const QString &accessCode, int port = 8883) override;

  // ── IDeviceDataPlane：任务发送 ──
  Q_INVOKABLE bool sendPrintViaFtp(int filteredIndex, const QString &gcodePath) override;
  Q_INVOKABLE void cancelSendPrint() override;
  bool sendPrintInFlight() const override { return ftpSendInProgress_; }

  // ── IDeviceDataPlane：遥测 ──
  bool selectedDeviceOnline() const override;
  QString selectedDeviceStatus() const override;
  QString selectedDeviceIp() const override;
  Q_INVOKABLE bool isMqttConnected() const override;
  int selectedDeviceTemperature() const override;
  int selectedDeviceBedTemperature() const override;
  int selectedDeviceNozzleTargetTemp() const override;
  int selectedDeviceBedTargetTemp() const override;
  int selectedDeviceTotalLayerNum() const override;
  int selectedDeviceCurrentLayerNum() const override;
  int selectedDeviceRemainingTime() const override;
  Q_INVOKABLE bool applyMqttReportPayload(const QString &payload, int deviceIndex = -1) override;

  // ── DeviceServiceMock 自有面（QML/Q_PROPERTY/Q_INVOKABLE） ──
  QString deviceListJson() const;

  int selectedDeviceIndex() const;
  QString selectedDeviceName() const;
  QString selectedDeviceModel() const;
  QString selectedDeviceSerial() const;
  int selectedDeviceProgress() const;
  QString selectedDeviceTaskName() const;
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
  bool selectedDeviceFilesystemSupported() const;

  QString searchText() const;
  void setSearchText(const QString &text);

  int filteredDeviceCount() const;
  bool networkOnline() const;

  // ── SelectMachineDialog send-job state machine ─────────────────────────
  int sendJobState() const { return sendJobState_; }
  int sendJobProgress() const { return sendJobProgress_; }
  QString sendJobErrorCode() const { return sendJobErrorCode_; }
  QString sendJobErrorDesc() const { return sendJobErrorDesc_; }
  QString sendJobErrorExtra() const { return sendJobErrorExtra_; }
  /// Begin the mock send flow for SelectMachineDialog (prepare -> sending).
  /// The progress ticks asynchronously; on completion the mock device starts
  /// printing the file (startPrint), mirroring the upstream send-then-print
  /// sequence (Plater.cpp:12921-12928 open_machine_select_dialog flow).
  Q_INVOKABLE void startSendJob(int filteredIndex, const QString &gcodePath);
  /// Abort an in-flight mock send and return to the prepare page
  /// (upstream on_cancel stops the send worker, SelectMachine.cpp:139).
  Q_INVOKABLE void cancelSendJob();

  // ── SelectMachineDialog advanced print options ─────────────────────────
  // Upstream PrintOption segmented values (SelectMachine.cpp:565-614):
  // "timelapse" (On/Off), "bed_leveling", "flow_cali", "nozzle_offset_cali"
  // (Auto/On/Off), "pa_value" (On/Off).
  /// Current option values keyed by option key (NOTIFY drives QML bindings).
  Q_PROPERTY(QVariantMap printOptions READ printOptions NOTIFY printOptionsChanged)
  QVariantMap printOptions() const;
  Q_INVOKABLE QString printOptionValue(const QString &key) const;
  Q_INVOKABLE void setPrintOptionValue(const QString &key, const QString &value);
  /// Feature visibility per device capability. Upstream creates all five
  /// options but initially hides four of them (SelectMachine.cpp:646-649);
  /// the mock exposes only timelapse until per-device capability lands.
  Q_INVOKABLE bool printOptionSupported(const QString &key) const;

  // deviceAt / selectDevice / addManualDevice：声明已收编进 IDeviceDataPlane
  // 覆写组（X1），Q_INVOKABLE 保留在覆写上——此处不再重复声明。

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

  /// 打印任务控制（对齐上游 Plater print/pause/resume/stop）
  Q_INVOKABLE void startPrint(int filteredIndex, const QString &gcodePath);

  /// v2.8 P2-C: 通过 FTP 上传 .gcode 到打印机，然后发 MQTT print 命令启动打印。
  /// 仅在 MQTT 已连接时启用（需要 IP + access code）。
  /// sendPrintUploadProgress 信号反馈上传百分比，终态经
  /// sendPrintSucceeded/sendPrintFailed/sendPrintCancelled 通知。
  /// 契约见 IDeviceDataPlane::sendPrintViaFtp（U11：返回 false 必伴随
  /// sendPrintFailed；X1：demo 假设备条目拒绝作为 FTP/MQTT 目标）。
  /// U11: 中止在途的 send-print FTP 上传（PrintDialog 取消按钮）。
  /// 无在途任务时为安全 no-op；取消结果经 sendPrintCancelled 通知。
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
  // X1: devicesChanged / selectedDeviceChanged / sendPrint* / searchTextChanged
  // / hmsChanged / sendJobChanged / printOptionsChanged 均由 IDeviceDataPlane
  // 基类声明（连接经元对象继承解析）。

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
  // U11: send-print 在途/取消标记。ftpSendInProgress_ 覆盖 uploadStarted →
  // uploadFinished 窗口，startPrint 以它防重入（此前在途期间二次点击会
  // fall-through 到 mock 任务，造成双重 printing 状态）。
  bool ftpSendInProgress_ = false;
  bool ftpSendCancelRequested_ = false;
  // v2.7 P2-B: MQTT publish 调试（供 INT-05 断言 payload/topic）
  QString lastPublishPayload_;
  QString lastPublishTopic_;
  int mqttSequenceId_ = 0;

  /// HMS 通知列表（对齐上游 DeviceManager hms_list）
  /// key = device realIndex, value = list of HMS items
  QMap<int, QList<MockHmsItem>> hmsLists_;

  // ── SelectMachineDialog send-job state machine ─────────────────────────
  QTimer *sendJobTimer_ = nullptr;
  int sendJobState_ = 0;      // 0=prepare, 1=sending, 2=finish
  int sendJobProgress_ = 0;   // 0-100
  int sendJobDeviceIndex_ = -1;
  QString sendJobGcodePath_;
  QString sendJobErrorCode_;
  QString sendJobErrorDesc_;
  QString sendJobErrorExtra_;

  // Advanced print option segmented values (upstream PrintOption keys)
  QMap<QString, QString> printOptions_;
};
