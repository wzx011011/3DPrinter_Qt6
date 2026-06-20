#pragma once

#include <QObject>
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

  bool networkOnline() const;
  int latencyMs() const;

  /// Get device data by filtered index (for QML Repeater delegate)
  Q_INVOKABLE QVariantMap deviceAt(int filteredIndex) const;

  /// Select device by filtered index
  Q_INVOKABLE void selectDevice(int filteredIndex);

  /// Refresh all device data (mock polling)
  Q_INVOKABLE void refresh();

  /// Device interaction (对齐上游 DeviceManager / MachineObject)
  Q_INVOKABLE void scanDevices();
  Q_INVOKABLE void connectDevice(int filteredIndex);
  Q_INVOKABLE void disconnectDevice(int filteredIndex);
  Q_INVOKABLE void startPrint(int filteredIndex, const QString &gcodePath);
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

signals:
  void devicesChanged();
  void selectedDeviceChanged();
  void searchTextChanged();
  void networkChanged();
  void cameraChanged();
  void hmsChanged();
  void monitorStateChanged();

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
};
