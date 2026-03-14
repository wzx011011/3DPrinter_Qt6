#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

/// Cloud account and device binding mock service
/// (对齐上游 NetworkAgent / WebUserLoginDialog / BindDialog / CommunicateWithCXCloud)
///
/// Mock mode simulates cloud login, device binding/unbinding, and
/// cloud sync status without real bambu_networking or wxWebView dependencies.
class CloudServiceMock final : public QObject
{
  Q_OBJECT

  /// Whether user is logged into cloud account
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loginStateChanged)
  /// Logged-in user display name
  Q_PROPERTY(QString userName READ userName NOTIFY loginStateChanged)
  /// Cloud account email or ID
  Q_PROPERTY(QString userEmail READ userEmail NOTIFY loginStateChanged)
  /// Number of cloud-bound devices
  Q_PROPERTY(int boundDeviceCount READ boundDeviceCount NOTIFY devicesChanged)
  /// Whether cloud sync is in progress
  Q_PROPERTY(bool syncing READ syncing NOTIFY syncStateChanged)
  /// Last sync timestamp string
  Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY syncStateChanged)
  /// Preset sync enabled
  Q_PROPERTY(bool presetSyncEnabled READ presetSyncEnabled WRITE setPresetSyncEnabled NOTIFY presetSyncEnabledChanged)

public:
  explicit CloudServiceMock(QObject *parent = nullptr);

  bool loggedIn() const;
  QString userName() const;
  QString userEmail() const;
  int boundDeviceCount() const;
  bool syncing() const;
  QString lastSyncTime() const;
  bool presetSyncEnabled() const;
  void setPresetSyncEnabled(bool enabled);

  /// Simulate cloud login (对齐 upstream NetworkAgent::login)
  Q_INVOKABLE void login(const QString &user, const QString &password);
  /// Simulate logout (对齐 upstream NetworkAgent::logout)
  Q_INVOKABLE void logout();
  /// Bind a device to the cloud account (对齐 upstream BindDialog bind_machine)
  Q_INVOKABLE void bindDevice(const QString &deviceName, const QString &pinCode);
  /// Unbind a device (对齐 upstream UnBindMachineDialog)
  Q_INVOKABLE void unbindDevice(int boundDeviceIndex);
  /// Get bound device info by index
  Q_INVOKABLE QVariantMap boundDeviceAt(int index) const;
  /// Trigger cloud preset sync (对齐 upstream CommunicateWithCXCloud sync)
  Q_INVOKABLE void syncPresets();

signals:
  void loginStateChanged();
  void devicesChanged();
  void syncStateChanged();
  void presetSyncEnabledChanged();
  void loginFailed(const QString &error);

private:
  struct BoundDevice {
    QString name;
    QString model;
    QString sn;
    bool online = false;
  };

  bool loggedIn_ = false;
  QString userName_;
  QString userEmail_;
  QList<BoundDevice> boundDevices_;
  bool syncing_ = false;
  QString lastSyncTime_;
  bool presetSyncEnabled_ = true;
};
