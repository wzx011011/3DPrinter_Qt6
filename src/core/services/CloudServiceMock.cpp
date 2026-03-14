#include "CloudServiceMock.h"

#include <QDateTime>
#include <QTimer>
#include <QVariantMap>

CloudServiceMock::CloudServiceMock(QObject *parent)
    : QObject(parent)
{
}

bool CloudServiceMock::loggedIn() const { return loggedIn_; }

QString CloudServiceMock::userName() const { return userName_; }

QString CloudServiceMock::userEmail() const { return userEmail_; }

int CloudServiceMock::boundDeviceCount() const { return boundDevices_.size(); }

bool CloudServiceMock::syncing() const { return syncing_; }

QString CloudServiceMock::lastSyncTime() const { return lastSyncTime_; }

bool CloudServiceMock::presetSyncEnabled() const { return presetSyncEnabled_; }

void CloudServiceMock::setPresetSyncEnabled(bool enabled)
{
  if (presetSyncEnabled_ == enabled)
    return;
  presetSyncEnabled_ = enabled;
  emit presetSyncEnabledChanged();
}

void CloudServiceMock::login(const QString &user, const QString &password)
{
  if (user.isEmpty() || password.isEmpty()) {
    emit loginFailed(QStringLiteral("用户名和密码不能为空"));
    return;
  }

  // Simulate async login (对齐 upstream NetworkAgent::login via bambu_networking)
  QTimer::singleShot(1200, this, [this, user]() {
    loggedIn_ = true;
    userName_ = user;
    userEmail_ = user + QStringLiteral("@creality.com");
    lastSyncTime_ = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));

    // Add some mock bound devices
    boundDevices_ = {
      {QStringLiteral("K1 Max"), QStringLiteral("K1 Max"), QStringLiteral("CP01001A001"), true},
      {QStringLiteral("Ender-3 V3 SE"), QStringLiteral("Ender-3 V3 SE"), QStringLiteral("CP01003C003"), false},
    };

    emit loginStateChanged();
    emit devicesChanged();
    emit syncStateChanged();
  });
}

void CloudServiceMock::logout()
{
  loggedIn_ = false;
  userName_.clear();
  userEmail_.clear();
  boundDevices_.clear();
  lastSyncTime_.clear();
  emit loginStateChanged();
  emit devicesChanged();
}

void CloudServiceMock::bindDevice(const QString &deviceName, const QString &pinCode)
{
  if (!loggedIn_) {
    emit loginFailed(QStringLiteral("请先登录账号"));
    return;
  }
  if (pinCode.length() < 4) {
    emit loginFailed(QStringLiteral("PIN 码格式错误"));
    return;
  }

  // Simulate async bind
  QTimer::singleShot(1500, this, [this, deviceName]() {
    BoundDevice d;
    d.name = deviceName;
    d.model = deviceName;
    d.sn = QStringLiteral("CP") + QString::number(100000 + boundDevices_.size()) + QStringLiteral("X00") + QString::number(boundDevices_.size());
    d.online = true;
    boundDevices_.append(d);
    emit devicesChanged();
  });
}

void CloudServiceMock::unbindDevice(int boundDeviceIndex)
{
  if (boundDeviceIndex < 0 || boundDeviceIndex >= boundDevices_.size())
    return;
  boundDevices_.removeAt(boundDeviceIndex);
  emit devicesChanged();
}

QVariantMap CloudServiceMock::boundDeviceAt(int index) const
{
  QVariantMap map;
  if (index < 0 || index >= boundDevices_.size())
    return map;
  const auto &d = boundDevices_[index];
  map[QStringLiteral("name")] = d.name;
  map[QStringLiteral("model")] = d.model;
  map[QStringLiteral("sn")] = d.sn;
  map[QStringLiteral("online")] = d.online;
  return map;
}

void CloudServiceMock::syncPresets()
{
  if (!loggedIn_) return;

  syncing_ = true;
  emit syncStateChanged();

  QTimer::singleShot(2000, this, [this]() {
    syncing_ = false;
    lastSyncTime_ = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"));
    emit syncStateChanged();
  });
}
