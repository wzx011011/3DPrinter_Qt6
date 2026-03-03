#include "MonitorViewModel.h"

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"

MonitorViewModel::MonitorViewModel(DeviceServiceMock *deviceService, NetworkServiceMock *networkService, QObject *parent)
    : QObject(parent), deviceService_(deviceService), networkService_(networkService)
{
  connect(deviceService_, &DeviceServiceMock::devicesChanged, this, &MonitorViewModel::stateChanged);
  connect(networkService_, &NetworkServiceMock::networkChanged, this, &MonitorViewModel::stateChanged);
}

QStringList MonitorViewModel::deviceNames() const
{
  return deviceService_->deviceNames();
}

QString MonitorViewModel::firstDeviceState() const
{
  return deviceService_->firstDeviceState();
}

bool MonitorViewModel::online() const
{
  return networkService_->online();
}

int MonitorViewModel::latencyMs() const
{
  return networkService_->latencyMs();
}

void MonitorViewModel::refresh()
{
  deviceService_->refresh();
  networkService_->probe();
}
