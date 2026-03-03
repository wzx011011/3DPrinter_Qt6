#pragma once

#include <QObject>
#include <QStringList>

class DeviceServiceMock final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList deviceNames READ deviceNames NOTIFY devicesChanged)
  Q_PROPERTY(QString firstDeviceState READ firstDeviceState NOTIFY devicesChanged)

public:
  explicit DeviceServiceMock(QObject *parent = nullptr);

  QStringList deviceNames() const;
  QString firstDeviceState() const;

  Q_INVOKABLE void refresh();

signals:
  void devicesChanged();

private:
  QStringList deviceNames_;
  QStringList deviceStates_;
  int tick_ = 0;
};
