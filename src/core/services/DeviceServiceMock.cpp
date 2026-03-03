#include "DeviceServiceMock.h"

DeviceServiceMock::DeviceServiceMock(QObject *parent)
    : QObject(parent)
{
  deviceNames_ = {QStringLiteral("K1 Max"), QStringLiteral("K1C"), QStringLiteral("Ender-3 V3")};
  deviceStates_ = {QStringLiteral("打印中 64%"), QStringLiteral("空闲"), QStringLiteral("离线")};
}

QStringList DeviceServiceMock::deviceNames() const
{
  return deviceNames_;
}

QString DeviceServiceMock::firstDeviceState() const
{
  return deviceStates_.isEmpty() ? QStringLiteral("未知") : deviceStates_.first();
}

void DeviceServiceMock::refresh()
{
  tick_++;
  const int progress = 64 + (tick_ % 30);
  deviceStates_[0] = QStringLiteral("打印中 %1%").arg(progress > 100 ? 100 : progress);
  emit devicesChanged();
}
