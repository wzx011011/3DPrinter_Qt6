#include "MonitorViewModel.h"

#include <QTimer>

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CameraServiceMock.h"

MonitorViewModel::MonitorViewModel(DeviceServiceMock *deviceService, NetworkServiceMock *networkService,
                                   CameraServiceMock *cameraService, QObject *parent)
    : QObject(parent), deviceService_(deviceService), networkService_(networkService), cameraService_(cameraService)
{
  /// 初始化监控状态：根据设备列表是否为空（对齐上游 StatusPanel 状态切换）
  if (deviceService_ && deviceService_->deviceCount() > 0)
    monitorState_ = Normal;
  else
    monitorState_ = NoPrinter;

  if (deviceService_) {
    connect(deviceService_, &DeviceServiceMock::devicesChanged,
            this, &MonitorViewModel::devicesChanged);

    /// 设备列表变化时同步更新状态机（对齐上游 MonitorBasePanel on_devices_changed）
    connect(deviceService_, &DeviceServiceMock::devicesChanged, this, [this]() {
      updateMonitorState();
    });
    connect(deviceService_, &DeviceServiceMock::selectedDeviceChanged,
            this, &MonitorViewModel::selectedDeviceChanged);
    connect(deviceService_, &DeviceServiceMock::searchTextChanged,
            this, &MonitorViewModel::searchTextChanged);
    connect(deviceService_, &DeviceServiceMock::hmsChanged,
            this, &MonitorViewModel::hmsChanged);
  }
  if (networkService_) {
    connect(networkService_, &NetworkServiceMock::networkChanged,
            this, &MonitorViewModel::networkChanged);
  }
  if (cameraService_) {
    connect(cameraService_, &CameraServiceMock::streamStatusChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::recordingStatusChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::timelapseStatusChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::resolutionChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::cameraUrlChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::errorMessageChanged,
            this, &MonitorViewModel::cameraChanged);
    connect(cameraService_, &CameraServiceMock::cameraAvailableChanged,
            this, &MonitorViewModel::cameraChanged);
    // v2.6 CAM-03：帧令牌变化 → cameraChanged（驱动 QML Image 重新拉取 image://camera/live）
    connect(cameraService_, &CameraServiceMock::frameTokenChanged,
            this, &MonitorViewModel::cameraChanged);
  }
}

// ── Device list ───────────────────────────────────────────────

int MonitorViewModel::filteredDeviceCount() const
{
  return deviceService_ ? deviceService_->filteredDeviceCount() : 0;
}

QString MonitorViewModel::searchText() const
{
  return deviceService_ ? deviceService_->searchText() : QString();
}

void MonitorViewModel::setSearchText(const QString &text)
{
  if (deviceService_)
    deviceService_->setSearchText(text);
}

// ── Selected device ───────────────────────────────────────────

int MonitorViewModel::selectedDeviceIndex() const
{
  return deviceService_ ? deviceService_->selectedDeviceIndex() : -1;
}

QString MonitorViewModel::selectedDeviceName() const
{
  return deviceService_ ? deviceService_->selectedDeviceName() : QString();
}

QString MonitorViewModel::selectedDeviceModel() const
{
  return deviceService_ ? deviceService_->selectedDeviceModel() : QString();
}

bool MonitorViewModel::selectedDeviceOnline() const
{
  return deviceService_ ? deviceService_->selectedDeviceOnline() : false;
}

QString MonitorViewModel::selectedDeviceStatus() const
{
  return deviceService_ ? deviceService_->selectedDeviceStatus() : QString();
}

int MonitorViewModel::selectedDeviceProgress() const
{
  return deviceService_ ? deviceService_->selectedDeviceProgress() : 0;
}

QString MonitorViewModel::selectedDeviceTaskName() const
{
  return deviceService_ ? deviceService_->selectedDeviceTaskName() : QString();
}

// v2.7 P2-A: MQTT 连接参数 + 实时遥测转发（DeviceServiceMock → MonitorViewModel → QML）
QString MonitorViewModel::selectedDeviceAccessCode() const
{
  return deviceService_ ? deviceService_->selectedDeviceAccessCode() : QString();
}
int MonitorViewModel::selectedDeviceMqttPort() const
{
  return deviceService_ ? deviceService_->selectedDeviceMqttPort() : 8883;
}
void MonitorViewModel::setSelectedDeviceAccessCode(const QString &code, int port)
{
  if (deviceService_) deviceService_->setSelectedDeviceAccessCode(code, port);
}
int MonitorViewModel::selectedDeviceBedTemperature() const
{
  return deviceService_ ? deviceService_->selectedDeviceBedTemperature() : 0;
}
int MonitorViewModel::selectedDeviceNozzleTargetTemp() const
{
  return deviceService_ ? deviceService_->selectedDeviceNozzleTargetTemp() : 0;
}
int MonitorViewModel::selectedDeviceBedTargetTemp() const
{
  return deviceService_ ? deviceService_->selectedDeviceBedTargetTemp() : 0;
}
int MonitorViewModel::selectedDeviceCurrentLayerNum() const
{
  return deviceService_ ? deviceService_->selectedDeviceCurrentLayerNum() : 0;
}
int MonitorViewModel::selectedDeviceTotalLayerNum() const
{
  return deviceService_ ? deviceService_->selectedDeviceTotalLayerNum() : 0;
}
int MonitorViewModel::selectedDeviceRemainingTime() const
{
  return deviceService_ ? deviceService_->selectedDeviceRemainingTime() : 0;
}
bool MonitorViewModel::mqttConnected() const
{
  return deviceService_ ? deviceService_->isMqttConnected() : false;
}
QString MonitorViewModel::selectedDeviceIp() const
{
  return deviceService_ ? deviceService_->selectedDeviceIp() : QString();
}

int MonitorViewModel::selectedDeviceTemperature() const
{
  return deviceService_ ? deviceService_->selectedDeviceTemperature() : 0;
}

int MonitorViewModel::selectedDeviceSignalStrength() const
{
  return deviceService_ ? deviceService_->selectedDeviceSignalStrength() : 0;
}

bool MonitorViewModel::selectedDeviceChamberLightOn() const
{
  return deviceService_ ? deviceService_->selectedDeviceChamberLightOn() : false;
}

bool MonitorViewModel::selectedDeviceWorkLightOn() const
{
  return deviceService_ ? deviceService_->selectedDeviceWorkLightOn() : false;
}

bool MonitorViewModel::selectedDeviceCameraRecording() const
{
  return deviceService_ ? deviceService_->selectedDeviceCameraRecording() : false;
}

bool MonitorViewModel::selectedDeviceCameraTimelapse() const
{
  return deviceService_ ? deviceService_->selectedDeviceCameraTimelapse() : false;
}

// ── Network ───────────────────────────────────────────────────

bool MonitorViewModel::networkOnline() const
{
  return networkService_ ? networkService_->online() : false;
}

int MonitorViewModel::latencyMs() const
{
  return networkService_ ? networkService_->latencyMs() : 0;
}

// ── Actions ───────────────────────────────────────────────────

QVariantMap MonitorViewModel::deviceAt(int filteredIndex) const
{
  return deviceService_ ? deviceService_->deviceAt(filteredIndex) : QVariantMap();
}

void MonitorViewModel::selectDevice(int filteredIndex)
{
  if (deviceService_)
    deviceService_->selectDevice(filteredIndex);
}

void MonitorViewModel::refresh()
{
  if (deviceService_) deviceService_->refresh();
  if (networkService_) networkService_->probe();
}

// ── Device interaction (对齐上游 DeviceManager / MachineObject) ──

void MonitorViewModel::scanDevices()
{
  if (deviceService_) deviceService_->scanDevices();
}

void MonitorViewModel::connectDevice(int filteredIndex)
{
  // v2.7 P2-A: 真实 MQTT 连接（当设备有 access code + IP）+ mock fallback。
  // 对齐上游 DeviceManager connect 逻辑：先设 Connecting，真实连接异步回调决定最终状态。
  setMonitorStateValue(Connecting);

  if (!deviceService_) {
    setMonitorStateValue(Disconnected);
    return;
  }

  // 检查设备是否有 MQTT 连接参数（access code + IP）
  deviceService_->selectDevice(filteredIndex);
  const QString ip = deviceService_->selectedDeviceIp();
  const QString accessCode = deviceService_->selectedDeviceAccessCode();
  const int port = deviceService_->selectedDeviceMqttPort();
  const bool hasMqttParams = !ip.isEmpty() && !accessCode.isEmpty();

  if (hasMqttParams) {
    // v2.7 P2-A: 真实 MQTT 连接路径。connectViaMqtt 异步连接，成功后订阅 device/+/report，
    // telemetry 通过 messageReceived 实时更新（selectedDeviceTemperature 等 Q_PROPERTY）。
    qDebug("[Monitor] real MQTT connect to %s:%d (access code set)", ip.toUtf8().constData(), port);
    QMetaObject::invokeMethod(deviceService_, "connectViaMqtt", Qt::QueuedConnection,
                              Q_ARG(QString, ip), Q_ARG(int, port), Q_ARG(QString, accessCode));
    // 监听 MQTT 连接状态（2.5s 超时决定最终状态，对齐 mock 的 1.5s + 网络 RTT）
    QTimer::singleShot(2500, this, [this]() {
      if (deviceService_ && deviceService_->isMqttConnected()) {
        setMonitorStateValue(Normal);
      } else {
        // MQTT 连接失败/超时 → fallback 到 mock（保留演示能力）或 Disconnected
        qDebug("[Monitor] MQTT connect timeout, fallback to mock");
        setMonitorStateValue(Disconnected);
      }
    });
  } else {
    // Mock fallback：无 access code 或无 IP（演示/测试模式）
    QTimer::singleShot(1500, this, [this, filteredIndex]() {
      deviceService_->connectDevice(filteredIndex);
      if (deviceService_->selectedDeviceOnline())
        setMonitorStateValue(Normal);
      else
        setMonitorStateValue(Disconnected);
    });
  }
}

void MonitorViewModel::disconnectDevice(int filteredIndex)
{
  if (deviceService_) {
    deviceService_->disconnectDevice(filteredIndex);
    /// 断开后进入 Disconnected 状态（对齐上游 StatusPanel on_disconnect）
    setMonitorStateValue(Disconnected);
  }
}

void MonitorViewModel::startPrint(int filteredIndex, const QString &gcodePath)
{
  if (deviceService_) deviceService_->startPrint(filteredIndex, gcodePath);
}

void MonitorViewModel::pausePrint(int filteredIndex)
{
  if (deviceService_) deviceService_->pausePrint(filteredIndex);
}

void MonitorViewModel::resumePrint(int filteredIndex)
{
  if (deviceService_) deviceService_->resumePrint(filteredIndex);
}

void MonitorViewModel::stopPrint(int filteredIndex)
{
  if (deviceService_) deviceService_->stopPrint(filteredIndex);
}

// ── Selected device print state ────────────────────────────────

QString MonitorViewModel::selectedPrintStage() const
{
  return deviceService_ ? deviceService_->devicePrintStage(deviceService_->selectedFilteredIndex()) : QString();
}

int MonitorViewModel::selectedPrintLayer() const
{
  return deviceService_ ? deviceService_->devicePrintLayer(deviceService_->selectedFilteredIndex()) : 0;
}

int MonitorViewModel::selectedPrintTimeLeft() const
{
  return deviceService_ ? deviceService_->devicePrintTimeLeft(deviceService_->selectedFilteredIndex()) : 0;
}

// ── HMS 健康管理（对齐上游 HMSPanel / DeviceManager hms_list） ──

int MonitorViewModel::selectedHmsCount() const
{
  return deviceService_ ? deviceService_->selectedDeviceHmsCount() : 0;
}

int MonitorViewModel::selectedUnreadHmsCount() const
{
  return deviceService_ ? deviceService_->selectedDeviceUnreadHmsCount() : 0;
}

QVariantMap MonitorViewModel::hmsAt(int index) const
{
  return deviceService_ ? deviceService_->selectedDeviceHmsAt(index) : QVariantMap();
}

void MonitorViewModel::markHmsRead(int index)
{
  if (deviceService_) {
    deviceService_->markHmsRead(index);
    emit hmsChanged();
  }
}

// ── Camera / Video (对齐上游 CameraPopup / MediaPlayCtrl) ──

int MonitorViewModel::cameraStreamStatus() const
{
  return cameraService_ ? cameraService_->streamStatus() : 0;
}

int MonitorViewModel::cameraRecordingStatus() const
{
  return cameraService_ ? cameraService_->recordingStatus() : 0;
}

int MonitorViewModel::cameraTimelapseStatus() const
{
  return cameraService_ ? cameraService_->timelapseStatus() : 0;
}

int MonitorViewModel::cameraResolution() const
{
  return cameraService_ ? cameraService_->resolution() : 0;
}

void MonitorViewModel::setCameraResolution(int res)
{
  if (cameraService_) cameraService_->setResolution(res);
}

QString MonitorViewModel::cameraUrl() const
{
  return cameraService_ ? cameraService_->cameraUrl() : QString();
}

void MonitorViewModel::setCameraUrl(const QString &url)
{
  if (cameraService_) cameraService_->setCameraUrl(url);
}

QString MonitorViewModel::cameraErrorMessage() const
{
  return cameraService_ ? cameraService_->errorMessage() : QString();
}

bool MonitorViewModel::cameraAvailable() const
{
  return cameraService_ ? cameraService_->cameraAvailable() : false;
}

int MonitorViewModel::cameraFrameToken() const
{
  return cameraService_ ? cameraService_->frameToken() : 0;
}

void MonitorViewModel::startCameraStream()
{
  if (cameraService_ && deviceService_) {
    cameraService_->updateForDevice(deviceService_->selectedDeviceIp(), deviceService_->selectedDeviceOnline());
    cameraService_->startStream();
  }
}

void MonitorViewModel::stopCameraStream()
{
  if (cameraService_) cameraService_->stopStream();
}

void MonitorViewModel::toggleCameraRecording()
{
  if (cameraService_) cameraService_->toggleRecording();
}

void MonitorViewModel::toggleCameraTimelapse()
{
  if (cameraService_) cameraService_->toggleTimelapse();
}

void MonitorViewModel::switchCameraView()
{
  if (cameraService_) cameraService_->switchCamera();
}

void MonitorViewModel::retryCameraConnection()
{
  if (cameraService_) cameraService_->retryConnection();
}

void MonitorViewModel::takeCameraScreenshot()
{
  if (cameraService_) cameraService_->takeScreenshot();
}

// ── Device lights and recording (对齐上游 MachineObject lights / camera) ──

void MonitorViewModel::setChamberLight(bool on)
{
  if (deviceService_) deviceService_->setChamberLight(on);
}

void MonitorViewModel::setWorkLight(bool on)
{
  if (deviceService_) deviceService_->setWorkLight(on);
}

void MonitorViewModel::toggleDeviceRecording()
{
  if (deviceService_) deviceService_->toggleRecording();
}

void MonitorViewModel::toggleDeviceTimelapse()
{
  if (deviceService_) deviceService_->toggleTimelapse();
}

// ── AMS 多耗材管理（对齐上游 AMSScreen / AMSModel） ──

int MonitorViewModel::selectedAmsSlotCount() const
{
  return deviceService_ ? deviceService_->selectedDeviceAmsSlotCount() : 0;
}

QVariantMap MonitorViewModel::amsSlotAt(int slotIndex) const
{
  return deviceService_ ? deviceService_->selectedDeviceAmsSlotAt(slotIndex) : QVariantMap();
}

void MonitorViewModel::setActiveAmsSlot(int slotIndex)
{
  if (deviceService_) deviceService_->setSelectedDeviceAmsSlot(slotIndex);
}

// ── Monitor state machine（对齐上游 StatusPanel / MonitorBasePanel 状态切换） ──

void MonitorViewModel::updateMonitorState()
{
  if (!deviceService_ || deviceService_->deviceCount() == 0) {
    setMonitorStateValue(NoPrinter);
  } else {
    setMonitorStateValue(Normal);
  }
}

void MonitorViewModel::setMonitorStateValue(int newState)
{
  if (monitorState_ == newState)
    return;
  monitorState_ = newState;
  emit monitorStateChanged();
}
