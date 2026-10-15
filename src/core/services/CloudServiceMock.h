#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

/// X1（审计④章）：Cloud 数据面窄接口——云账号 / 云绑定设备清单 / 云同步。
/// CloudServiceMock 与未来真实云后端（bambu_networking 的 NetworkAgent /
/// CommunicateWithCXCloud 桥）共同实现本接口；BackendContext 是唯一装配点，
/// 测试以本接口注入替身（注入缝）。
class ICloudDataPlane : public QObject
{
  Q_OBJECT

public:
  explicit ICloudDataPlane(QObject *parent = nullptr) : QObject(parent) {}
  ~ICloudDataPlane() override = default;

  // ── 账号面 ──
  virtual bool loggedIn() const = 0;
  virtual QString userName() const = 0;
  virtual QString userEmail() const = 0;
  /// 对齐 upstream NetworkAgent::login
  virtual void login(const QString &user, const QString &password) = 0;
  virtual void logout() = 0;

  // ── 云绑定设备清单（Cloud 数据面的设备角色；sn 即云侧凭据载体） ──
  virtual int boundDeviceCount() const = 0;
  virtual QVariantMap boundDeviceAt(int index) const = 0;
  /// 对齐 upstream BindDialog bind_machine
  virtual void bindDevice(const QString &deviceName, const QString &pinCode) = 0;
  virtual void unbindDevice(int boundDeviceIndex) = 0;

  // ── 云同步 ──
  virtual bool syncing() const = 0;
  virtual QString lastSyncTime() const = 0;
  virtual bool presetSyncEnabled() const = 0;
  virtual void setPresetSyncEnabled(bool enabled) = 0;
  /// 对齐 upstream CommunicateWithCXCloud sync
  virtual void syncPresets() = 0;

signals:
  void loginStateChanged();
  void devicesChanged();
  void syncStateChanged();
  void presetSyncEnabledChanged();
  void loginFailed(const QString &error);
};

/// Cloud account and device binding service
/// (对齐上游 NetworkAgent / WebUserLoginDialog / BindDialog / CommunicateWithCXCloud)
///
/// Mock 模式模拟云登录、设备绑定/解绑与云同步状态；真实 bambu_networking /
/// wxWebView 依赖未接入。X1: 实现 ICloudDataPlane 窄接口——真云后端以
/// "实现接口 + BackendContext 切换装配" 接入，不改写本类原位。
class CloudServiceMock final : public ICloudDataPlane
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

  // ── ICloudDataPlane ──
  bool loggedIn() const override;
  QString userName() const override;
  QString userEmail() const override;
  int boundDeviceCount() const override;
  bool syncing() const override;
  QString lastSyncTime() const override;
  bool presetSyncEnabled() const override;
  void setPresetSyncEnabled(bool enabled) override;
  void login(const QString &user, const QString &password) override;
  void logout() override;
  void bindDevice(const QString &deviceName, const QString &pinCode) override;
  void unbindDevice(int boundDeviceIndex) override;
  QVariantMap boundDeviceAt(int index) const override;
  void syncPresets() override;

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
