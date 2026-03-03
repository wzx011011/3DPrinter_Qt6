#pragma once

#include <QObject>
#include <QStringList>

class DeviceServiceMock;
class NetworkServiceMock;

class MonitorViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList deviceNames READ deviceNames NOTIFY stateChanged)
  Q_PROPERTY(QString firstDeviceState READ firstDeviceState NOTIFY stateChanged)
  Q_PROPERTY(bool online READ online NOTIFY stateChanged)
  Q_PROPERTY(int latencyMs READ latencyMs NOTIFY stateChanged)

public:
  explicit MonitorViewModel(DeviceServiceMock *deviceService, NetworkServiceMock *networkService, QObject *parent = nullptr);

  QStringList deviceNames() const;
  QString firstDeviceState() const;
  bool online() const;
  int latencyMs() const;

  Q_INVOKABLE void refresh();

signals:
  void stateChanged();

private:
  DeviceServiceMock *deviceService_ = nullptr;
  NetworkServiceMock *networkService_ = nullptr;
};
