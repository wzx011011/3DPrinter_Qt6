#pragma once

#include "CloudServiceMock.h"
#include "CrealityAuthService.h"

/// CrealityCloudService (feature/cloud-model-library, M2).
///
/// Real-backend ICloudDataPlane implementation for the Creality Cloud,
/// satisfying the X1 seam (ServiceSeamTests asserts the base relation).
/// Assembly is toggled in BackendContext by QSettings "Cloud/Backend"
/// ("mock" | "creality", default mock) — the mock stays the default until
/// the real backend leaves the feature branch.
///
/// Scope honesty (per docs/依赖与协议边界审计.md):
/// - Account face: real, delegated to CrealityAuthService (OAuth web login).
///   The interface's password login(user, pass) has no upstream REST
///   counterpart in the open-source client — it reports failure with a
///   pointer to the web login; no fabricated success.
/// - Device face: honest-empty. Cloud-bound-device management needs the
///   device list API + MQTT channel (a later milestone); count stays 0 and
///   bind/unbind are no-ops, so the QML bind card stays unreachable instead
///   of pretending.
/// - Sync face: honest-idle. Preset sync belongs to the cloud-preset
///   milestone; syncing stays false, the toggle persists a preference only.
class CrealityCloudService final : public ICloudDataPlane
{
  Q_OBJECT
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY loginStateChanged)
  Q_PROPERTY(QString userName READ userName NOTIFY loginStateChanged)
  Q_PROPERTY(QString userEmail READ userEmail NOTIFY loginStateChanged)
  Q_PROPERTY(int boundDeviceCount READ boundDeviceCount NOTIFY devicesChanged)
  Q_PROPERTY(bool syncing READ syncing NOTIFY syncStateChanged)
  Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY syncStateChanged)
  Q_PROPERTY(bool presetSyncEnabled READ presetSyncEnabled WRITE setPresetSyncEnabled NOTIFY presetSyncEnabledChanged)

public:
  explicit CrealityCloudService(CrealityAuthService *auth, QObject *parent = nullptr);

  // ── ICloudDataPlane: account face (real, via CrealityAuthService) ──
  bool loggedIn() const override;
  QString userName() const override;
  QString userEmail() const override;
  /// Password login is not part of the open OAuth surface — fails honestly
  /// with guidance instead of pretending (audit rule: no mock fake success).
  void login(const QString &user, const QString &password) override;
  void logout() override;

  // ── ICloudDataPlane: device face (honest-empty, see class comment) ──
  int boundDeviceCount() const override;
  QVariantMap boundDeviceAt(int index) const override;
  void bindDevice(const QString &deviceName, const QString &pinCode) override;
  void unbindDevice(int boundDeviceIndex) override;

  // ── ICloudDataPlane: sync face (honest-idle, see class comment) ─────
  bool syncing() const override;
  QString lastSyncTime() const override;
  bool presetSyncEnabled() const override;
  void setPresetSyncEnabled(bool enabled) override;
  void syncPresets() override;

private:
  CrealityAuthService *auth_;
  bool presetSyncEnabled_ = false;
};
