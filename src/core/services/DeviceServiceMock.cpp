#include "DeviceServiceMock.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <algorithm>

DeviceServiceMock::DeviceServiceMock(QObject *parent)
    : QObject(parent)
{
  buildMockDevices();
  rebuildFilteredIndices();

  // 打印进度模拟定时器（对齐上游 PrintJob 1s 刷新间隔）
  printSimTimer_ = new QTimer(this);
  printSimTimer_->setInterval(1000);
  connect(printSimTimer_, &QTimer::timeout, this, [this]() {
    bool anyActive = false;
    for (auto it = printJobs_.begin(); it != printJobs_.end(); ) {
      auto &job = it.value();
      if (!job.active) { ++it; continue; }
      if (job.paused) { anyActive = true; ++it; continue; }

      anyActive = true;
      auto &d = devices_[it.key()];
      job.progress = qMin(100, job.progress + 1);
      job.currentLayer = qMin(job.totalLayers, job.currentLayer + 1);
      job.timeLeft = qMax(0, job.timeLeft - 18); // ~18s per tick in mock

      // Advance stage
      if (job.progress < 5)
        job.stage = "heating";
      else if (job.progress < 10)
        job.stage = "mop_first_layer";
      else if (job.progress >= 98)
        job.stage = "cooling";
      else
        job.stage = "printing";

      d.progress = job.progress;
      d.status = job.progress >= 100 ? "complete" : "printing";
      // Fluctuate temperature slightly during printing
      d.temperature = 200 + (job.progress % 15);

      if (job.progress >= 100) {
        job.active = false;
        d.taskName.clear();
      }
      ++it;
    }
    // Stop timer when no active jobs remain
    if (!anyActive)
      printSimTimer_->stop();

    emit devicesChanged();
    emit selectedDeviceChanged();
  });
}

void DeviceServiceMock::buildMockDevices()
{
  // Build devices with AMS slots (对齐上游 AMSScreen / AMSModel)
  MockDevice k1Max;
  k1Max.name = "K1 Max";
  k1Max.model = "K1 Max";
  k1Max.sn = "CP01001A001";
  k1Max.status = "printing";
  k1Max.online = true;
  k1Max.progress = 64;
  k1Max.taskName = "Benchy_3dbenchy.gcode";
  k1Max.ip = "192.168.1.100";
  k1Max.temperature = 218;
  k1Max.signalStrength = 3;
  k1Max.amsSlots = {
    {"PLA",   "#FF5733", 850.0f, true},   // Active: orange PLA
    {"PETG",  "#FFFFFF", 720.0f, false},  // White PETG
    {"ABS",   "#1E90FF", 0.0f, false},    // Empty slot
    {"PLA",   "#228B22", 500.0f, false},  // Green PLA
  };

  MockDevice k1c;
  k1c.name = "K1C";
  k1c.model = "K1C";
  k1c.sn = "CP01002B002";
  k1c.status = "idle";
  k1c.online = true;
  k1c.progress = 0;
  k1c.taskName = "";
  k1c.ip = "192.168.1.101";
  k1c.temperature = 25;
  k1c.signalStrength = 3;
  k1c.amsSlots = {
    {"PLA",   "#00CED1", 1000.0f, true},  // Cyan PLA active
    {"TPU",   "#9370DB", 680.0f, false},  // Purple TPU
    {"",      "",        0.0f, false},    // Empty
    {"PLA+",  "#FFD700", 920.0f, false},  // Gold PLA+
  };

  MockDevice ender3v3;
  ender3v3.name = "Ender-3 V3";
  ender3v3.model = "Ender-3 V3 SE";
  ender3v3.sn = "CP01003C003";
  ender3v3.status = "offline";
  ender3v3.online = false;
  ender3v3.progress = 0;
  ender3v3.taskName = "";
  ender3v3.ip = "192.168.1.102";
  ender3v3.temperature = 22;
  ender3v3.signalStrength = 0;
  // No AMS for Ender-3

  MockDevice cr10se;
  cr10se.name = "CR-10 SE";
  cr10se.model = "CR-10 SE";
  cr10se.sn = "CP01004D004";
  cr10se.status = "printing";
  cr10se.online = true;
  cr10se.progress = 37;
  cr10se.taskName = "Calendar_Cat.gcode";
  cr10se.ip = "192.168.1.103";
  cr10se.temperature = 205;
  cr10se.signalStrength = 2;
  cr10se.amsSlots = {
    {"PLA",   "#FF1493", 780.0f, true},   // Deep pink PLA active
    {"PETG",  "#32CD32", 650.0f, false},  // Lime PETG
    {"",      "",        0.0f, false},    // Empty
    {"",      "",        0.0f, false},    // Empty
  };

  MockDevice k1;
  k1.name = "K1";
  k1.model = "K1";
  k1.sn = "CP01005E005";
  k1.status = "connecting";
  k1.online = false;
  k1.progress = 0;
  k1.taskName = "";
  k1.ip = "192.168.1.104";
  k1.temperature = 0;
  k1.signalStrength = 1;
  k1.amsSlots = {
    {"PLA",   "#8B4513", 420.0f, false},  // Brown PLA
    {"PLA",   "#4169E1", 890.0f, true},   // Royal blue PLA active
    {"PETG",  "#FF6347", 710.0f, false},  // Tomato PETG
    {"",      "",        0.0f, false},    // Empty
  };

  devices_ = {k1Max, k1c, ender3v3, cr10se, k1};

  // Mock HMS data (对齐上游 DeviceManager hms_list)
  hmsLists_[0] = {
    MockHmsItem(0, 1, 0, 3, 1401001, false, QStringLiteral("喷头温度偏高 - 建议检查散热风扇"), ""),
    MockHmsItem(0, 1, 0, 2, 1402001, false, QStringLiteral("热床温度传感器异常 - 请重新校准"), ""),
    MockHmsItem(1, 0, 0, 3, 1203001, true,  QStringLiteral("进料齿轮磨损 - 建议更换"), ""),
  };
  hmsLists_[1] = {};
  hmsLists_[3] = {
    MockHmsItem(0, 0, 0, 3, 1401001, false, QStringLiteral("打印头冷却风扇转速偏低"), ""),
    MockHmsItem(2, 0, 0, 4, 1501001, false, QStringLiteral("固件版本可更新 - 当前 v1.0.0"), ""),
  };
}

void DeviceServiceMock::rebuildFilteredIndices()
{
  filteredIndices_.clear();
  const QString filter = searchText_.trimmed().toLower();
  for (int i = 0; i < devices_.size(); ++i) {
    if (filter.isEmpty()
        || devices_[i].name.toLower().contains(filter)
        || devices_[i].model.toLower().contains(filter)
        || devices_[i].sn.toLower().contains(filter)
        || devices_[i].status.toLower().contains(filter))
    {
      filteredIndices_.append(i);
    }
  }
}

// ── Q_PROPERTY accessors ──────────────────────────────────────────

int DeviceServiceMock::deviceCount() const { return devices_.size(); }

QString DeviceServiceMock::deviceListJson() const
{
  QJsonArray arr;
  for (const auto &d : devices_) {
    QJsonObject obj;
    obj["name"]           = d.name;
    obj["model"]          = d.model;
    obj["sn"]             = d.sn;
    obj["status"]         = d.status;
    obj["online"]         = d.online;
    obj["progress"]       = d.progress;
    obj["taskName"]       = d.taskName;
    obj["ip"]             = d.ip;
    obj["temperature"]    = d.temperature;
    obj["signalStrength"] = d.signalStrength;
    arr.append(obj);
  }
  return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

int DeviceServiceMock::selectedDeviceIndex() const
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return -1;
  return filteredIndices_[selectedDeviceIndex_];
}

QString DeviceServiceMock::selectedDeviceName() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].name : QString();
}

QString DeviceServiceMock::selectedDeviceModel() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].model : QString();
}

bool DeviceServiceMock::selectedDeviceOnline() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].online : false;
}

QString DeviceServiceMock::selectedDeviceStatus() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].status : QString();
}

int DeviceServiceMock::selectedDeviceProgress() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].progress : 0;
}

QString DeviceServiceMock::selectedDeviceTaskName() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].taskName : QString();
}

QString DeviceServiceMock::selectedDeviceIp() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].ip : QString();
}

int DeviceServiceMock::selectedDeviceTemperature() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].temperature : 0;
}

int DeviceServiceMock::selectedDeviceSignalStrength() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].signalStrength : 0;
}

bool DeviceServiceMock::selectedDeviceChamberLightOn() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].chamberLightOn : false;
}

bool DeviceServiceMock::selectedDeviceWorkLightOn() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].workLightOn : false;
}

bool DeviceServiceMock::selectedDeviceCameraRecording() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].cameraRecording : false;
}

bool DeviceServiceMock::selectedDeviceCameraTimelapse() const
{
  const int idx = selectedDeviceIndex();
  return (idx >= 0 && idx < devices_.size()) ? devices_[idx].cameraTimelapse : false;
}

QString DeviceServiceMock::searchText() const { return searchText_; }

void DeviceServiceMock::setSearchText(const QString &text)
{
  if (searchText_ == text)
    return;
  searchText_ = text;
  rebuildFilteredIndices();
  // Clamp selection
  if (selectedDeviceIndex_ >= filteredIndices_.size())
    selectedDeviceIndex_ = filteredIndices_.isEmpty() ? -1 : 0;
  emit searchTextChanged();
  emit devicesChanged();
  emit selectedDeviceChanged();
}

int DeviceServiceMock::filteredDeviceCount() const { return filteredIndices_.size(); }

bool DeviceServiceMock::networkOnline() const
{
  // At least one device is online
  for (const auto &d : devices_) {
    if (d.online)
      return true;
  }
  return false;
}

QVariantMap DeviceServiceMock::deviceAt(int filteredIndex) const
{
  QVariantMap map;
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return map;

  const auto &d = devices_[filteredIndices_[filteredIndex]];
  map["name"]           = d.name;
  map["model"]          = d.model;
  map["sn"]             = d.sn;
  map["status"]         = d.status;
  map["online"]         = d.online;
  map["progress"]       = d.progress;
  map["taskName"]       = d.taskName;
  map["ip"]             = d.ip;
  map["temperature"]    = d.temperature;
  map["signalStrength"] = d.signalStrength;
  return map;
}

void DeviceServiceMock::selectDevice(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  if (selectedDeviceIndex_ == filteredIndex)
    return;
  selectedDeviceIndex_ = filteredIndex;
  emit selectedDeviceChanged();
}

void DeviceServiceMock::refresh()
{
  tick_++;

  // Simulate progress changes for devices that are printing
  for (auto &d : devices_) {
    if (d.status == "printing") {
      d.progress += 1;
      if (d.progress > 100) d.progress = 0;
      // Fluctuate nozzle temperature slightly
      d.temperature = 200 + (tick_ % 20);
    }
  }

  emit devicesChanged();
  emit selectedDeviceChanged();
}

// ── P5.2 Device interaction methods ──────────────────────────────────

void DeviceServiceMock::scanDevices()
{
  // Mock scan: briefly set connecting devices to offline, then rediscover
  for (auto &d : devices_) {
    if (d.status == "connecting") {
      d.status = "idle";
      d.online = true;
    }
  }
  emit devicesChanged();
  emit selectedDeviceChanged();
}

void DeviceServiceMock::connectDevice(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  auto &d = devices_[realIdx];

  if (d.online) return;

  // Simulate async connection
  d.status = "connecting";
  d.online = false;
  emit devicesChanged();
  emit selectedDeviceChanged();

  // Mock: complete connection after a short delay via single-shot timer
  QTimer::singleShot(1500, this, [this, realIdx]() {
    if (realIdx >= devices_.size()) return;
    devices_[realIdx].status = "idle";
    devices_[realIdx].online = true;
    devices_[realIdx].signalStrength = 3;
    emit devicesChanged();
    emit selectedDeviceChanged();
  });
}

void DeviceServiceMock::disconnectDevice(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  auto &d = devices_[realIdx];

  if (!d.online) return;

  // Stop any active print job on this device
  if (printJobs_.contains(realIdx)) {
    printJobs_[realIdx].active = false;
    printJobs_[realIdx].paused = false;
  }

  d.status = "offline";
  d.online = false;
  d.signalStrength = 0;
  d.progress = 0;
  d.taskName.clear();
  d.temperature = 0;
  emit devicesChanged();
  emit selectedDeviceChanged();
}

void DeviceServiceMock::startPrint(int filteredIndex, const QString &gcodePath)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  auto &d = devices_[realIdx];

  if (!d.online || d.status == "printing")
    return;

  // Create print job state
  PrintJobState job;
  job.gcodePath = gcodePath.isEmpty() ? "MockPrint.gcode" : gcodePath;
  job.progress = 0;
  job.totalLayers = 200;
  job.currentLayer = 0;
  job.timeLeft = 3600; // 1 hour mock
  job.active = true;
  job.paused = false;
  job.stage = "heating";
  printJobs_[realIdx] = job;

  d.status = "printing";
  d.progress = 0;
  d.taskName = job.gcodePath;
  d.temperature = 200;

  // Start simulation timer if not already running
  if (printSimTimer_ && !printSimTimer_->isActive())
    printSimTimer_->start();

  emit devicesChanged();
  emit selectedDeviceChanged();
}

void DeviceServiceMock::pausePrint(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  if (!printJobs_.contains(realIdx)) return;

  auto &job = printJobs_[realIdx];
  if (!job.active || job.paused) return;

  job.paused = true;
  devices_[realIdx].status = "paused";

  emit devicesChanged();
  emit selectedDeviceChanged();
}

void DeviceServiceMock::resumePrint(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  if (!printJobs_.contains(realIdx)) return;

  auto &job = printJobs_[realIdx];
  if (!job.active || !job.paused) return;

  job.paused = false;
  devices_[realIdx].status = "printing";

  emit devicesChanged();
  emit selectedDeviceChanged();
}

void DeviceServiceMock::stopPrint(int filteredIndex)
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[filteredIndex];
  if (!printJobs_.contains(realIdx)) return;

  auto &job = printJobs_[realIdx];
  job.active = false;
  job.paused = false;

  auto &d = devices_[realIdx];
  d.status = "idle";
  d.progress = 0;
  d.taskName.clear();
  d.temperature = 0;

  emit devicesChanged();
  emit selectedDeviceChanged();
}

QString DeviceServiceMock::devicePrintStage(int filteredIndex) const
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return QString();
  const int realIdx = filteredIndices_[filteredIndex];
  auto it = printJobs_.constFind(realIdx);
  if (it == printJobs_.constEnd()) return QString();
  return it->stage;
}

int DeviceServiceMock::devicePrintLayer(int filteredIndex) const
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return 0;
  const int realIdx = filteredIndices_[filteredIndex];
  auto it = printJobs_.constFind(realIdx);
  if (it == printJobs_.constEnd()) return 0;
  return it->currentLayer;
}

int DeviceServiceMock::devicePrintTimeLeft(int filteredIndex) const
{
  if (filteredIndex < 0 || filteredIndex >= filteredIndices_.size())
    return 0;
  const int realIdx = filteredIndices_[filteredIndex];
  auto it = printJobs_.constFind(realIdx);
  if (it == printJobs_.constEnd()) return 0;
  return it->timeLeft;
}

// ── HMS 健康管理（对齐上游 HMSPanel / DeviceManager hms_list） ──

int DeviceServiceMock::selectedDeviceHmsCount() const
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return 0;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  return hmsLists_.value(realIdx).size();
}

int DeviceServiceMock::selectedDeviceUnreadHmsCount() const
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return 0;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  int count = 0;
  for (const auto &item : hmsLists_.value(realIdx)) {
    if (!item.alreadyRead) ++count;
  }
  return count;
}

QVariantMap DeviceServiceMock::selectedDeviceHmsAt(int hmsIndex) const
{
  QVariantMap map;
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return map;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  const auto &items = hmsLists_.value(realIdx);
  if (hmsIndex < 0 || hmsIndex >= items.size())
    return map;
  const auto &h = items[hmsIndex];
  map[QStringLiteral("moduleId")] = h.moduleId;
  map[QStringLiteral("moduleNum")] = h.moduleNum;
  map[QStringLiteral("partId")] = h.partId;
  map[QStringLiteral("msgLevel")] = h.msgLevel;
  map[QStringLiteral("msgCode")] = h.msgCode;
  map[QStringLiteral("alreadyRead")] = h.alreadyRead;
  map[QStringLiteral("message")] = h.message;
  map[QStringLiteral("wikiUrl")] = h.wikiUrl;
  return map;
}

void DeviceServiceMock::markHmsRead(int hmsIndex)
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  auto &items = hmsLists_[realIdx];
  if (hmsIndex < 0 || hmsIndex >= items.size())
    return;
  items[hmsIndex].alreadyRead = true;
  emit hmsChanged();
}

// ── 灯光和录制控制（对齐上游 MachineObject lights / camera） ──

void DeviceServiceMock::setChamberLight(bool on)
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return;
  auto &d = devices_[realIdx];
  if (d.chamberLightOn == on)
    return;
  d.chamberLightOn = on;
  emit selectedDeviceChanged();
}

void DeviceServiceMock::setWorkLight(bool on)
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return;
  auto &d = devices_[realIdx];
  if (d.workLightOn == on)
    return;
  d.workLightOn = on;
  emit selectedDeviceChanged();
}

void DeviceServiceMock::toggleRecording()
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return;
  auto &d = devices_[realIdx];
  d.cameraRecording = !d.cameraRecording;
  emit selectedDeviceChanged();
}

void DeviceServiceMock::toggleTimelapse()
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return;
  auto &d = devices_[realIdx];
  d.cameraTimelapse = !d.cameraTimelapse;
  emit selectedDeviceChanged();
}

// ── AMS 多耗材管理（对齐上游 AMSScreen / AMSModel） ──

int DeviceServiceMock::selectedDeviceAmsSlotCount() const
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return 0;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return 0;
  return devices_[realIdx].amsSlots.size();
}

QVariantMap DeviceServiceMock::selectedDeviceAmsSlotAt(int slotIndex) const
{
  QVariantMap map;
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return map;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return map;
  const auto &slots = devices_[realIdx].amsSlots;
  if (slotIndex < 0 || slotIndex >= slots.size())
    return map;
  const auto &slot = slots[slotIndex];
  map[QStringLiteral("filamentType")] = slot.filamentType;
  map[QStringLiteral("color")] = slot.color;
  map[QStringLiteral("remainingWeight")] = slot.remainingWeight;
  map[QStringLiteral("active")] = slot.active;
  map[QStringLiteral("slotIndex")] = slotIndex;
  return map;
}

void DeviceServiceMock::setSelectedDeviceAmsSlot(int slotIndex)
{
  if (selectedDeviceIndex_ < 0 || selectedDeviceIndex_ >= filteredIndices_.size())
    return;
  const int realIdx = filteredIndices_[selectedDeviceIndex_];
  if (realIdx < 0 || realIdx >= devices_.size())
    return;
  auto &slots = devices_[realIdx].amsSlots;
  if (slotIndex < 0 || slotIndex >= slots.size())
    return;
  // Deactivate all slots, then activate the selected one
  for (int i = 0; i < slots.size(); ++i) {
    slots[i].active = (i == slotIndex);
  }
  emit selectedDeviceChanged();
}
