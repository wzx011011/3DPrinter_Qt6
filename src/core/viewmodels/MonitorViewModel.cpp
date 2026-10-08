#include "MonitorViewModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QCoreApplication>
#include <QDataStream>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>
#include <QVariant>

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
    // SelectMachineDialog send-job state machine forward（三态条 prepare/sending/finish）
    connect(deviceService_, &DeviceServiceMock::sendJobChanged,
            this, &MonitorViewModel::sendJobChanged);
    // SelectMachineDialog print option values forward（分段选择器 NOTIFY）
    connect(deviceService_, &DeviceServiceMock::printOptionsChanged,
            this, &MonitorViewModel::printOptionsChanged);
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

QString MonitorViewModel::selectedDeviceSerial() const
{
  return deviceService_ ? deviceService_->selectedDeviceSerial() : QString();
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

bool MonitorViewModel::selectedDeviceFilesystemSupported() const
{
  return deviceService_ ? deviceService_->selectedDeviceFilesystemSupported() : false;
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

QVariantList MonitorViewModel::discoveredPrinters() const
{
  // PhysicalPrinterDialog "Browse ..." discovery mock: the full unfiltered
  // device roster (upstream BonjourDialog lists every announced printer,
  // unaffected by any UI filter). DeviceServiceMock exposes the roster as
  // JSON; parse it into light {name, ip, online} entries for the QML picker.
  QVariantList result;
  if (!deviceService_)
    return result;
  const QJsonDocument doc = QJsonDocument::fromJson(deviceService_->deviceListJson().toUtf8());
  const QJsonArray arr = doc.array();
  for (const QJsonValue &value : arr) {
    const QJsonObject obj = value.toObject();
    QVariantMap entry;
    entry.insert(QStringLiteral("name"), obj.value(QStringLiteral("name")).toString());
    entry.insert(QStringLiteral("ip"), obj.value(QStringLiteral("ip")).toString());
    entry.insert(QStringLiteral("online"), obj.value(QStringLiteral("online")).toBool());
    result.append(entry);
  }
  return result;
}

void MonitorViewModel::selectDevice(int filteredIndex)
{
  if (deviceService_)
    deviceService_->selectDevice(filteredIndex);
}

bool MonitorViewModel::selectDeviceByIdentity(const QString &ip, const QString &name)
{
  if (!deviceService_)
    return false;

  for (int i = 0; i < filteredDeviceCount(); ++i) {
    const QVariantMap device = deviceAt(i);
    const bool matchesIdentity = !ip.isEmpty()
        ? device.value(QStringLiteral("ip")).toString() == ip
        : (!name.isEmpty() && device.value(QStringLiteral("name")).toString() == name);
    if (matchesIdentity) {
      selectDevice(i);
      return true;
    }
  }
  return false;
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

bool MonitorViewModel::addManualDevice(const QString &name, const QString &ip,
                                       const QString &accessCode, int port)
{
  return deviceService_ != nullptr
             ? deviceService_->addManualDevice(name, ip, accessCode, port)
             : false;
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

QVariantList MonitorViewModel::selectedDeviceStorages() const
{
  // Upstream renders one radio per device-reported storage and disables
  // non-emmc entries when no SD card is present (SendToPrinter.cpp:630-635).
  // The mock service exposes the equivalent state through
  // selectedDeviceFilesystemSupported() (SD-card listing is not implemented
  // yet), so the external entry is reported but stays disabled.
  QVariantList storages;
  QVariantMap internalStorage;
  internalStorage.insert(QStringLiteral("key"), QStringLiteral("emmc"));
  internalStorage.insert(QStringLiteral("enabled"), true);
  storages.append(internalStorage);
  QVariantMap externalStorage;
  externalStorage.insert(QStringLiteral("key"), QStringLiteral("sdcard"));
  externalStorage.insert(QStringLiteral("enabled"), selectedDeviceFilesystemSupported());
  storages.append(externalStorage);
  return storages;
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

// ── SelectMachineDialog send flow（对齐上游 SelectMachine 三态条） ──────

int MonitorViewModel::sendJobState() const
{
  return deviceService_ ? deviceService_->sendJobState() : 0;
}

int MonitorViewModel::sendJobProgress() const
{
  return deviceService_ ? deviceService_->sendJobProgress() : 0;
}

QString MonitorViewModel::sendJobErrorCode() const
{
  return deviceService_ ? deviceService_->sendJobErrorCode() : QString();
}

QString MonitorViewModel::sendJobErrorDesc() const
{
  return deviceService_ ? deviceService_->sendJobErrorDesc() : QString();
}

QString MonitorViewModel::sendJobErrorExtra() const
{
  return deviceService_ ? deviceService_->sendJobErrorExtra() : QString();
}

void MonitorViewModel::startSendJob(int filteredIndex, const QString &gcodePath)
{
  if (deviceService_) deviceService_->startSendJob(filteredIndex, gcodePath);
}

void MonitorViewModel::cancelSendJob()
{
  if (deviceService_) deviceService_->cancelSendJob();
}

QVariantMap MonitorViewModel::printOptions() const
{
  return deviceService_ ? deviceService_->printOptions() : QVariantMap();
}

QString MonitorViewModel::printOptionValue(const QString &key) const
{
  return deviceService_ ? deviceService_->printOptionValue(key) : QString();
}

void MonitorViewModel::setPrintOptionValue(const QString &key, const QString &value)
{
  if (deviceService_) deviceService_->setPrintOptionValue(key, value);
}

bool MonitorViewModel::printOptionSupported(const QString &key) const
{
  return deviceService_ ? deviceService_->printOptionSupported(key) : false;
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

// ── Troubleshoot Center diagnostics (upstream TroubleshootDialog) ──────────

QVariantMap MonitorViewModel::diagnosticSystemInfo() const
{
  // Upstream composes the left-column system-info panel from GetOSinfo /
  // GetPackageType / GetCPUinfo (TroubleshootDialog.cpp:150-161); the RAM,
  // GPU and monitor lines are derived QML-side from backend.systemInfo()
  // (BackendContext::systemInfo) and the QML screen list.
  QVariantMap info;
#if defined(Q_OS_WINDOWS)
  info.insert(QStringLiteral("osType"), QStringLiteral("Windows"));
#elif defined(Q_OS_LINUX)
  info.insert(QStringLiteral("osType"), QStringLiteral("Linux"));
#elif defined(Q_OS_MACOS)
  info.insert(QStringLiteral("osType"), QStringLiteral("macOS"));
#else
  info.insert(QStringLiteral("osType"), QSysInfo::productType());
#endif

#if defined(Q_OS_WINDOWS)
  // Upstream GetWinVersion (TroubleshootDialog.cpp:615-642) maps the build
  // number from RtlGetVersion to the marketing major; QSysInfo::kernelVersion()
  // already reports "10.0.26200" on Windows. DisplayVersion registry value
  // with ReleaseId fallback replaces GetWinDisplayVersion (:596-614).
  const int build = QSysInfo::kernelVersion().section(QLatin1Char('.'), -1).toInt();
  const QString win = build >= 22000 ? QStringLiteral("11")
                        : build >= 10240 ? QStringLiteral("10")
                        : build >= 9200  ? QStringLiteral("8")
                        : build >= 7601  ? QStringLiteral("7")
                                         : QStringLiteral("?");
  const QSettings reg(
      QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
      QSettings::NativeFormat);
  QString display = reg.value(QStringLiteral("DisplayVersion")).toString();
  if (display.isEmpty())
    display = reg.value(QStringLiteral("ReleaseId")).toString();
  info.insert(QStringLiteral("osInfo"),
              display.isEmpty()
                  ? QStringLiteral("Windows %1 %2").arg(win).arg(build)
                  : QStringLiteral("Windows %1 %2 %3").arg(win, display).arg(build));
#else
  info.insert(QStringLiteral("osInfo"), QSysInfo::prettyProductName());
#endif

  // Upstream GetPackageType (:692-705): dev build path -> "Local Build",
  // Uninstall.exe next to the executable -> "Installed", else "Portable".
  const QString exePath = QCoreApplication::applicationFilePath();
  if (exePath.contains(QStringLiteral("build"), Qt::CaseInsensitive)) {
    info.insert(QStringLiteral("packageType"), QStringLiteral("Local Build"));
  } else if (QFileInfo::exists(QFileInfo(exePath).absolutePath()
                               + QStringLiteral("/Uninstall.exe"))) {
    info.insert(QStringLiteral("packageType"), QStringLiteral("Installed"));
  } else {
    info.insert(QStringLiteral("packageType"), QStringLiteral("Portable"));
  }

#if defined(Q_OS_WINDOWS)
  // Upstream get_cpu_info_from_registry (:762-773): ProcessorNameString.
  const QSettings cpuReg(
      QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
      QSettings::NativeFormat);
  const QString cpu = cpuReg.value(QStringLiteral("ProcessorNameString")).toString();
  info.insert(QStringLiteral("cpuInfo"),
              cpu.isEmpty() ? QStringLiteral("Unknown") : cpu.trimmed());
#else
  info.insert(QStringLiteral("cpuInfo"), QSysInfo::currentCpuArchitecture());
#endif
  return info;
}

QString MonitorViewModel::diagnosticLogDir() const
{
  // startup_diagnostics.log lives next to the executable (main_qml.cpp
  // appendStartupLog) -- the OWzx stand-in for upstream data_dir/log.
  return QCoreApplication::applicationDirPath();
}

QVariantList MonitorViewModel::diagnosticLogFiles() const
{
  // Newest first (upstream ClearLogs sorts by last_write_time descending,
  // TroubleshootDialog.cpp:1087-1090).
  QVariantList out;
  const QFileInfoList logs = QDir(diagnosticLogDir())
                                 .entryInfoList({QStringLiteral("*.log")}, QDir::Files,
                                                QDir::Time);
  for (const QFileInfo &fi : logs) {
    QVariantMap entry;
    entry.insert(QStringLiteral("name"), fi.fileName());
    entry.insert(QStringLiteral("path"), QDir::toNativeSeparators(fi.absoluteFilePath()));
    entry.insert(QStringLiteral("bytes"), static_cast<double>(fi.size()));
    out.append(entry);
  }
  return out;
}

int MonitorViewModel::diagnosticClearLogs()
{
  // Upstream ClearLogs (:1070-1105) deletes every log but the newest one.
  const QFileInfoList logs = QDir(diagnosticLogDir())
                                 .entryInfoList({QStringLiteral("*.log")}, QDir::Files,
                                                QDir::Time);
  int removed = 0;
  for (int i = 1; i < logs.size(); ++i) {
    if (QFile::remove(logs.at(i).absoluteFilePath()))
      ++removed;
  }
  return removed;
}

quint32 MonitorViewModel::zipCrc32(const QByteArray &data)
{
  static quint32 table[256];
  static bool initialized = false;
  if (!initialized) {
    for (quint32 i = 0; i < 256; ++i) {
      quint32 c = i;
      for (int k = 0; k < 8; ++k)
        c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
      table[i] = c;
    }
    initialized = true;
  }
  quint32 crc = 0xFFFFFFFFu;
  for (char raw : data) {
    const auto byte = static_cast<quint8>(raw);
    crc = table[(crc ^ byte) & 0xFFu] ^ (crc >> 8);
  }
  return crc ^ 0xFFFFFFFFu;
}

quint16 MonitorViewModel::zipDosTime(const QDateTime &dt)
{
  const QTime t = dt.time();
  return static_cast<quint16>((t.hour() << 11) | (t.minute() << 5) | (t.second() / 2));
}

quint16 MonitorViewModel::zipDosDate(const QDateTime &dt)
{
  const QDate d = dt.date();
  return static_cast<quint16>(((d.year() - 1980) << 9) | (d.month() << 5) | d.day());
}

QString MonitorViewModel::diagnosticPackZip(const QStringList &paths, const QString &destDir,
                                            const QString &baseName)
{
  // Minimal ZIP (stored entries, no compression) standing in for upstream
  // SaveAsZip (TroubleshootDialog.cpp:1333-1370): OWzx only packs flat
  // diagnostic log files plus the project file, so directory recursion is
  // not required.
  QDir().mkpath(destDir);
  const QString zipPath = destDir + QStringLiteral("/%1.zip").arg(baseName);
  if (QFileInfo::exists(zipPath))
    QFile::remove(zipPath);

  QFile zip(zipPath);
  if (!zip.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return {};

  QList<ZipEntry> entries;
  QDataStream out(&zip);
  out.setByteOrder(QDataStream::LittleEndian);

  for (const QString &path : paths) {
    const QFileInfo fi(path);
    QFile source(path);
    if (!fi.exists() || !source.open(QIODevice::ReadOnly))
      continue;
    const QByteArray payload = source.readAll();
    ZipEntry entry;
    entry.name = fi.fileName();
    entry.crc = zipCrc32(payload);
    entry.size = static_cast<quint32>(payload.size());
    const QDateTime modified = fi.lastModified();
    entry.time = zipDosTime(modified);
    entry.date = zipDosDate(modified);
    entry.offset = static_cast<quint32>(zip.pos());

    out << quint32(0x04034b50)  // local file header signature
        << quint16(20)          // version needed to extract
        << quint16(0)           // flags: sizes known up front
        << quint16(0)           // method: stored
        << entry.time << entry.date
        << entry.crc << entry.size << entry.size
        << quint16(entry.name.toUtf8().size()) << quint16(0);
    zip.write(entry.name.toUtf8());
    zip.write(payload);
    entries.append(entry);
  }

  const quint32 centralOffset = static_cast<quint32>(zip.pos());
  for (const ZipEntry &entry : entries) {
    const QByteArray nameUtf8 = entry.name.toUtf8();
    out << quint32(0x02014b50)  // central directory header signature
        << quint16(20) << quint16(20)
        << quint16(0) << quint16(0)
        << entry.time << entry.date
        << entry.crc << entry.size << entry.size
        << quint16(nameUtf8.size()) << quint16(0) << quint16(0)
        << quint16(0) << quint16(0) << quint32(0)
        << entry.offset;
    zip.write(nameUtf8);
  }
  const quint32 centralSize = static_cast<quint32>(zip.pos()) - centralOffset;
  out << quint32(0x06054b50)    // end of central directory signature
      << quint16(0) << quint16(0)
      << quint16(entries.size()) << quint16(entries.size())
      << centralSize << centralOffset << quint16(0);
  zip.close();
  return zipPath;
}

QVariantMap MonitorViewModel::diagnosticCleanSystemProfilesCache()
{
  // Upstream deletes data_dir/system after the restart confirmation
  // (TroubleshootDialog.cpp:1004). OWzx system presets are read-only
  // resources loaded from the profiles directory, so the cache directory
  // normally does not exist; the QML layer reports that honestly instead of
  // restarting the app for nothing.
  QVariantMap result;
  result.insert(QStringLiteral("existed"), false);
  result.insert(QStringLiteral("removed"), false);
  const QString sysDir = appDataDir() + QStringLiteral("/system");
  if (!QFileInfo::exists(sysDir))
    return result;
  result.insert(QStringLiteral("existed"), true);
  result.insert(QStringLiteral("removed"), QDir(sysDir).removeRecursively());
  return result;
}

bool MonitorViewModel::diagnosticOpenFolder(const QString &path) const
{
  // Upstream BrowseFolder (TroubleshootDialog.cpp:1127-1191) opens the
  // directory with the platform default application.
  return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QString MonitorViewModel::appDataDir() const
{
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString MonitorViewModel::logSeverityLevel() const
{
  // Upstream reads app_config "log_severity_level" (:104-105); the OWzx
  // AppConfig-lite sink is QSettings under the same key. The Qt logger
  // category rules consume the persisted value on the next launch.
  return QSettings()
      .value(QStringLiteral("log_severity_level"), QStringLiteral("info"))
      .toString();
}

void MonitorViewModel::setLogSeverityLevel(const QString &level)
{
  QSettings().setValue(QStringLiteral("log_severity_level"), level);
  QSettings().sync();
}

bool MonitorViewModel::writeTextFile(const QString &filePath, const QString &content)
{
  // Failure removes the partial file (upstream ExportAsJson
  // TroubleshootDialog.cpp:1212-1229).
  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return false;
  const QByteArray payload = content.toUtf8();
  const bool ok = file.write(payload) == payload.size();
  file.close();
  if (!ok)
    QFile::remove(filePath);
  return ok;
}
