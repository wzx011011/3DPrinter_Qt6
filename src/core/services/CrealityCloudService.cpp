#include "CrealityCloudService.h"

#include <QSettings>

CrealityCloudService::CrealityCloudService(CrealityAuthService *auth, QObject *parent)
    : ICloudDataPlane(parent), auth_(auth)
{
  if (!auth_)
    return;
  // Fan the auth service's identity signals into the interface's signals so
  // HomeViewModel (and anything else bound to the seam) re-evaluates.
  connect(auth_, &CrealityAuthService::identityChanged, this,
          &ICloudDataPlane::loginStateChanged);
  connect(auth_, &CrealityAuthService::loginFailed, this, &ICloudDataPlane::loginFailed);

  QSettings settings;
  presetSyncEnabled_ = settings.value(QStringLiteral("Cloud/Creality/PresetSyncEnabled"), false).toBool();
}

bool CrealityCloudService::loggedIn() const
{
  return auth_ ? auth_->loggedIn() : false;
}

QString CrealityCloudService::userName() const
{
  return auth_ ? auth_->userName() : QString();
}

QString CrealityCloudService::userEmail() const
{
  // The OAuth surface exposes a numeric userId, not an email — keep empty
  // instead of echoing the id into an email-shaped field (QML renders the
  // name; the email line stays blank honestly).
  return {};
}

void CrealityCloudService::login(const QString &user, const QString &password)
{
  Q_UNUSED(user);
  Q_UNUSED(password);
  emit loginFailed(QObject::tr("创想账号需通过网页登录（OAuth），请使用“登录创想账号”入口"));
}

void CrealityCloudService::logout()
{
  if (auth_)
    auth_->logout();
}

int CrealityCloudService::boundDeviceCount() const
{
  return 0;
}

QVariantMap CrealityCloudService::boundDeviceAt(int index) const
{
  Q_UNUSED(index);
  return {};
}

void CrealityCloudService::bindDevice(const QString &deviceName, const QString &pinCode)
{
  // Cloud device management needs the device-list API + MQTT channel (later
  // milestone). No-op: the QML bind entry stays mock-backend-only.
  Q_UNUSED(deviceName);
  Q_UNUSED(pinCode);
}

void CrealityCloudService::unbindDevice(int boundDeviceIndex)
{
  Q_UNUSED(boundDeviceIndex);
}

bool CrealityCloudService::syncing() const
{
  return false;
}

QString CrealityCloudService::lastSyncTime() const
{
  return {};
}

bool CrealityCloudService::presetSyncEnabled() const
{
  return presetSyncEnabled_;
}

void CrealityCloudService::setPresetSyncEnabled(bool enabled)
{
  if (presetSyncEnabled_ == enabled)
    return;
  presetSyncEnabled_ = enabled;
  QSettings settings;
  settings.setValue(QStringLiteral("Cloud/Creality/PresetSyncEnabled"), enabled);
  emit presetSyncEnabledChanged();
}

void CrealityCloudService::syncPresets()
{
  // Cloud preset sync is the later milestone (CommunicateWithCXCloud port).
  // Deliberately no syncStateChanged(true) theatre.
}
