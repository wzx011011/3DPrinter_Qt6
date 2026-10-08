#pragma once

#include <QObject>
#include <QDateTime>
#include <QStringList>

class DeviceServiceMock;
class NetworkServiceMock;
class CameraServiceMock;

/// 监控状态（对齐上游 StatusPanel 状态）：0=NoPrinter, 1=Connecting, 2=Disconnected, 3=Normal
enum MonitorState { NoPrinter = 0, Connecting = 1, Disconnected = 2, Normal = 3 };

class MonitorViewModel final : public QObject
{
  Q_OBJECT

  /// 监控页面状态机（对齐上游 StatusPanel / MonitorBasePanel 状态切换）
  Q_PROPERTY(int monitorState READ monitorState NOTIFY monitorStateChanged)

  // ── Device list (filtered) ──────────────────────────────────
  Q_PROPERTY(int filteredDeviceCount READ filteredDeviceCount NOTIFY devicesChanged)
  Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

  // ── Selected device detail ──────────────────────────────────
  Q_PROPERTY(int selectedDeviceIndex READ selectedDeviceIndex NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceName READ selectedDeviceName NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceModel READ selectedDeviceModel NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceSerial READ selectedDeviceSerial NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceOnline READ selectedDeviceOnline NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceStatus READ selectedDeviceStatus NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceProgress READ selectedDeviceProgress NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceTaskName READ selectedDeviceTaskName NOTIFY selectedDeviceChanged)
  Q_PROPERTY(QString selectedDeviceIp READ selectedDeviceIp NOTIFY selectedDeviceChanged)
  /// v2.7 P2-A: MQTT 连接参数（连接对话框读取/设置）
  Q_PROPERTY(QString selectedDeviceAccessCode READ selectedDeviceAccessCode NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceMqttPort READ selectedDeviceMqttPort NOTIFY selectedDeviceChanged)
  /// v2.7 P2-A: 实时遥测（由 MQTT 解析填充，转发 DeviceServiceMock）
  Q_PROPERTY(int selectedDeviceBedTemperature READ selectedDeviceBedTemperature NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceNozzleTargetTemp READ selectedDeviceNozzleTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceBedTargetTemp READ selectedDeviceBedTargetTemp NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceCurrentLayerNum READ selectedDeviceCurrentLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceTotalLayerNum READ selectedDeviceTotalLayerNum NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceRemainingTime READ selectedDeviceRemainingTime NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool mqttConnected READ mqttConnected NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceTemperature READ selectedDeviceTemperature NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedDeviceSignalStrength READ selectedDeviceSignalStrength NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceChamberLightOn READ selectedDeviceChamberLightOn NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceWorkLightOn READ selectedDeviceWorkLightOn NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceCameraRecording READ selectedDeviceCameraRecording NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceCameraTimelapse READ selectedDeviceCameraTimelapse NOTIFY selectedDeviceChanged)
  Q_PROPERTY(bool selectedDeviceFilesystemSupported READ selectedDeviceFilesystemSupported NOTIFY selectedDeviceChanged)

  // ── Network status ──────────────────────────────────────────
  Q_PROPERTY(bool networkOnline READ networkOnline NOTIFY networkChanged)
  Q_PROPERTY(int latencyMs READ latencyMs NOTIFY networkChanged)

public:
  explicit MonitorViewModel(DeviceServiceMock *deviceService, NetworkServiceMock *networkService,
                            CameraServiceMock *cameraService, QObject *parent = nullptr);

  /// 监控状态 getter（对齐上游 StatusPanel 状态切换）
  int monitorState() const { return monitorState_; }

  int filteredDeviceCount() const;
  QString searchText() const;
  void setSearchText(const QString &text);

  int selectedDeviceIndex() const;
  QString selectedDeviceName() const;
  QString selectedDeviceModel() const;
  QString selectedDeviceSerial() const;
  bool selectedDeviceOnline() const;
  QString selectedDeviceStatus() const;
  int selectedDeviceProgress() const;
  QString selectedDeviceTaskName() const;
  QString selectedDeviceIp() const;
  /// v2.7 P2-A: MQTT 连接参数转发
  QString selectedDeviceAccessCode() const;
  int selectedDeviceMqttPort() const;
  Q_INVOKABLE void setSelectedDeviceAccessCode(const QString &code, int port = 8883);
  /// v2.7 P2-A: 实时遥测转发
  int selectedDeviceBedTemperature() const;
  int selectedDeviceNozzleTargetTemp() const;
  int selectedDeviceBedTargetTemp() const;
  int selectedDeviceCurrentLayerNum() const;
  int selectedDeviceTotalLayerNum() const;
  int selectedDeviceRemainingTime() const;
  bool mqttConnected() const;
  int selectedDeviceTemperature() const;
  int selectedDeviceSignalStrength() const;
  bool selectedDeviceChamberLightOn() const;
  bool selectedDeviceWorkLightOn() const;
  bool selectedDeviceCameraRecording() const;
  bool selectedDeviceCameraTimelapse() const;
  bool selectedDeviceFilesystemSupported() const;

  bool networkOnline() const;
  int latencyMs() const;

  /// Get device data by filtered index (for QML Repeater delegate)
  Q_INVOKABLE QVariantMap deviceAt(int filteredIndex) const;

  /// PhysicalPrinterDialog "Browse ..." discovery roster (upstream
  /// BonjourDialog / CrealityDiscoveryDialog): every known device as
  /// {name, ip, online}, independent of the Monitor search filter so the
  /// picker never hides an announced host.
  Q_INVOKABLE QVariantList discoveredPrinters() const;

  /// Select device by filtered index
  Q_INVOKABLE void selectDevice(int filteredIndex);
  /// Select a device by stable local identity from another device workflow.
  Q_INVOKABLE bool selectDeviceByIdentity(const QString &ip, const QString &name = {});

  /// Refresh all device data (mock polling)
  Q_INVOKABLE void refresh();

  /// Device interaction (对齐上游 DeviceManager / MachineObject)
  Q_INVOKABLE void scanDevices();
  /// monitor-6: manual add passthrough (ref "+ 手动添加" form); returns false
  /// when the name is empty.
  Q_INVOKABLE bool addManualDevice(const QString &name, const QString &ip,
                                   const QString &accessCode, int port = 8883);
  Q_INVOKABLE void connectDevice(int filteredIndex);
  Q_INVOKABLE void disconnectDevice(int filteredIndex);
  Q_INVOKABLE void startPrint(int filteredIndex, const QString &gcodePath);
  /// Send-to-printer storage list of the selected device (aligns upstream
  /// MachineObject storage list consumed by SendToPrinterDialog
  /// update_storage_list, SendToPrinter.cpp:607-667). Mock derivation: the
  /// emmc (internal) entry is always reported; the sdcard (external) entry
  /// follows selectedDeviceFilesystemSupported(). Each entry is a map
  /// {"key": "emmc"|"sdcard", "enabled": bool}; the QML side maps keys to
  /// the localized labels and renders disabled entries greyed out.
  Q_INVOKABLE QVariantList selectedDeviceStorages() const;
  Q_INVOKABLE void pausePrint(int filteredIndex);
  Q_INVOKABLE void resumePrint(int filteredIndex);
  Q_INVOKABLE void stopPrint(int filteredIndex);

  /// Selected device print state (对齐上游 MachineObject 打印信息)
  Q_PROPERTY(QString selectedPrintStage READ selectedPrintStage NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedPrintLayer READ selectedPrintLayer NOTIFY selectedDeviceChanged)
  Q_PROPERTY(int selectedPrintTimeLeft READ selectedPrintTimeLeft NOTIFY selectedDeviceChanged)
  QString selectedPrintStage() const;
  int selectedPrintLayer() const;
  int selectedPrintTimeLeft() const;

  /// HMS 健康管理（对齐上游 HMSPanel / DeviceManager hms_list）
  Q_PROPERTY(int selectedHmsCount READ selectedHmsCount NOTIFY hmsChanged)
  Q_PROPERTY(int selectedUnreadHmsCount READ selectedUnreadHmsCount NOTIFY hmsChanged)
  int selectedHmsCount() const;
  int selectedUnreadHmsCount() const;
  Q_INVOKABLE QVariantMap hmsAt(int index) const;
  Q_INVOKABLE void markHmsRead(int index);

  /// Camera/Video (对齐上游 CameraPopup / MediaPlayCtrl)
  Q_PROPERTY(int cameraStreamStatus READ cameraStreamStatus NOTIFY cameraChanged)
  Q_PROPERTY(int cameraRecordingStatus READ cameraRecordingStatus NOTIFY cameraChanged)
  Q_PROPERTY(int cameraTimelapseStatus READ cameraTimelapseStatus NOTIFY cameraChanged)
  Q_PROPERTY(int cameraResolution READ cameraResolution WRITE setCameraResolution NOTIFY cameraChanged)
  Q_PROPERTY(QString cameraUrl READ cameraUrl WRITE setCameraUrl NOTIFY cameraChanged)
  Q_PROPERTY(QString cameraErrorMessage READ cameraErrorMessage NOTIFY cameraChanged)
  Q_PROPERTY(bool cameraAvailable READ cameraAvailable NOTIFY cameraChanged)
  /// v2.6 CAM-03：帧令牌（每帧 +1，QML Image cache-buster）
  Q_PROPERTY(int cameraFrameToken READ cameraFrameToken NOTIFY cameraChanged)
  int cameraStreamStatus() const;
  int cameraRecordingStatus() const;
  int cameraTimelapseStatus() const;
  int cameraResolution() const;
  void setCameraResolution(int res);
  QString cameraUrl() const;
  void setCameraUrl(const QString &url);
  QString cameraErrorMessage() const;
  bool cameraAvailable() const;
  /// v2.6 CAM-03：帧令牌（转发 CameraServiceMock::frameToken）
  int cameraFrameToken() const;
  Q_INVOKABLE void startCameraStream();
  Q_INVOKABLE void stopCameraStream();
  Q_INVOKABLE void toggleCameraRecording();
  Q_INVOKABLE void toggleCameraTimelapse();
  Q_INVOKABLE void switchCameraView();
  Q_INVOKABLE void retryCameraConnection();
  Q_INVOKABLE void takeCameraScreenshot();

  /// Device lights and recording (对齐上游 MachineObject lights / camera)
  Q_INVOKABLE void setChamberLight(bool on);
  Q_INVOKABLE void setWorkLight(bool on);
  Q_INVOKABLE void toggleDeviceRecording();
  Q_INVOKABLE void toggleDeviceTimelapse();

  /// AMS 多耗材管理（对齐上游 AMSScreen / AMSModel）
  Q_PROPERTY(int selectedAmsSlotCount READ selectedAmsSlotCount NOTIFY selectedDeviceChanged)
  int selectedAmsSlotCount() const;
  Q_INVOKABLE QVariantMap amsSlotAt(int slotIndex) const;
  Q_INVOKABLE void setActiveAmsSlot(int slotIndex);

  // ── SelectMachineDialog send flow（对齐上游 SelectMachine 三态条）──────
  // 0=prepare (Send button), 1=sending (progress), 2=finish ("Send complete");
  // forwards DeviceServiceMock::sendJobChanged.
  Q_PROPERTY(int sendJobState READ sendJobState NOTIFY sendJobChanged)
  Q_PROPERTY(int sendJobProgress READ sendJobProgress NOTIFY sendJobChanged)
  /// Failed-send detail (upstream SelectMachine.cpp:707-783 Error code /
  /// Error desc / Extra info rows); empty while no failure.
  Q_PROPERTY(QString sendJobErrorCode READ sendJobErrorCode NOTIFY sendJobChanged)
  Q_PROPERTY(QString sendJobErrorDesc READ sendJobErrorDesc NOTIFY sendJobChanged)
  Q_PROPERTY(QString sendJobErrorExtra READ sendJobErrorExtra NOTIFY sendJobChanged)
  int sendJobState() const;
  int sendJobProgress() const;
  QString sendJobErrorCode() const;
  QString sendJobErrorDesc() const;
  QString sendJobErrorExtra() const;
  /// Begin the send flow from SelectMachineDialog (prepare -> sending).
  Q_INVOKABLE void startSendJob(int filteredIndex, const QString &gcodePath);
  /// Abort the in-flight send and return to the prepare page.
  Q_INVOKABLE void cancelSendJob();

  /// SelectMachineDialog advanced print options（对齐上游 PrintOption 分段值）
  /// Current option values keyed by option key (NOTIFY drives QML bindings).
  Q_PROPERTY(QVariantMap printOptions READ printOptions NOTIFY printOptionsChanged)
  QVariantMap printOptions() const;
  Q_INVOKABLE QString printOptionValue(const QString &key) const;
  Q_INVOKABLE void setPrintOptionValue(const QString &key, const QString &value);
  Q_INVOKABLE bool printOptionSupported(const QString &key) const;

  // ── Troubleshoot Center diagnostics (upstream TroubleshootDialog) ──
  // The Qt6 TroubleshootDialog binds these helpers through its required
  // `monitorVm` property (PreferencesPage.qml). Behavior truth is
  // third_party/OrcaSlicer/src/slic3r/GUI/TroubleshootDialog.cpp.

  /// OS info / package type / CPU model in one call (upstream GetOSinfo
  /// :561-593 + GetWinVersion :615-642, GetPackageType :692-705,
  /// GetCPUinfo :745-773). Keys: osType, osInfo, packageType, cpuInfo.
  Q_INVOKABLE QVariantMap diagnosticSystemInfo() const;
  /// App diagnostics log directory: the OWzx stand-in for upstream
  /// data_dir/log is the executable directory (startup_diagnostics.log is
  /// written there by main_qml.cpp appendStartupLog).
  Q_INVOKABLE QString diagnosticLogDir() const;
  /// Log files [{name, path, bytes}] sorted newest first (upstream
  /// ClearLogs sort :1076-1090).
  Q_INVOKABLE QVariantList diagnosticLogFiles() const;
  /// Delete every log but the newest one (upstream ClearLogs :1070-1105).
  /// Returns the number of removed files.
  Q_INVOKABLE int diagnosticClearLogs();
  /// Pack the given files into <destDir>/<baseName>.zip and return the zip
  /// path ("" on failure). Minimal stored-entry ZIP writer standing in for
  /// upstream ExportAsZip/SaveAsZip :1236-1370.
  Q_INVOKABLE QString diagnosticPackZip(const QStringList &paths, const QString &destDir,
                                        const QString &baseName);
  /// Remove the system preset cache directory (upstream RebuildSystemProfiles
  /// remove_all(data_dir/system) :1004). OWzx keeps system presets as
  /// read-only resources, so the cache normally does not exist.
  /// Returns {existed: bool, removed: bool}.
  Q_INVOKABLE QVariantMap diagnosticCleanSystemProfilesCache();
  /// Open a local folder in the platform file manager (upstream
  /// BrowseFolder :1127-1191).
  Q_INVOKABLE bool diagnosticOpenFolder(const QString &path) const;
  /// App data directory (upstream data_dir analog) for Browse / cache clean.
  Q_INVOKABLE QString appDataDir() const;
  /// Persisted log severity (upstream app_config "log_severity_level",
  /// :104-111) stored via QSettings under the same key.
  Q_INVOKABLE QString logSeverityLevel() const;
  Q_INVOKABLE void setLogSeverityLevel(const QString &level);
  /// Write UTF-8 text content to a file (profiles-overview JSON export,
  /// upstream ExportAsJson :1193-1234). Returns write success.
  Q_INVOKABLE bool writeTextFile(const QString &filePath, const QString &content);

signals:
  void devicesChanged();
  void selectedDeviceChanged();
  void searchTextChanged();
  void networkChanged();
  void cameraChanged();
  void hmsChanged();
  void monitorStateChanged();
  void sendJobChanged();
  void printOptionsChanged();

private:
  DeviceServiceMock *deviceService_ = nullptr;
  NetworkServiceMock *networkService_ = nullptr;
  CameraServiceMock *cameraService_ = nullptr;

  /// 监控页面状态机（对齐上游 StatusPanel / MonitorBasePanel 状态切换）
  int monitorState_ = NoPrinter;

  /// 根据设备列表更新监控状态
  void updateMonitorState();
  /// 设置状态值并在变化时 emit signal
  void setMonitorStateValue(int newState);

  // ── Minimal ZIP writer helpers (diagnosticPackZip) ───────────────────
  struct ZipEntry
  {
    QString name;
    quint32 crc = 0;
    quint32 size = 0;
    quint16 time = 0;
    quint16 date = 0;
    quint32 offset = 0;
  };
  /// Standard CRC-32 (IEEE 802.3, polynomial 0xEDB88320) as required by
  /// the ZIP local/central headers.
  static quint32 zipCrc32(const QByteArray &data);
  /// MS-DOS packed time/date for the ZIP headers.
  static quint16 zipDosTime(const QDateTime &dt);
  static quint16 zipDosDate(const QDateTime &dt);
};
