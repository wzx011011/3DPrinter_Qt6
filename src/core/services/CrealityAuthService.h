#pragma once

#include "CloudHttpClient.h"
#include "CrealityCloudConfig.h"
#include "OAuthCallbackServer.h"

#include <QObject>
#include <QString>

/// CrealityAuthService (feature/cloud-model-library, M2).
///
/// Account-face service for the Creality Cloud (创想云) OAuth flow, ported
/// from the official open-source CrealityPrint client (AGPL-3.0):
///   1. beginLogin() starts the loopback callback server (OAuthCallbackServer)
///      and exposes the login page URL (https://www.creality.com/login)
///      for the QML web dialog to load.
///   2. The site's OAuth redirect lands on
///      https://www.crealitycloud.<tld>/oauth?code=…&back_url=localhost:…/login,
///      whose page bounces to back_url carrying the code; the callback server
///      captures it (codeReceived).
///   3. exchangeCode() POSTs {code, clientId, redirecturi} to
///      /api/cxy/account/v2/oauthLogin (upstream HttpServer.cpp:865) and
///      stores {token, userId, nickName, avatar} on success.
///
/// Identity persists in QSettings group "Cloud/Creality" and is restored at
/// construction (upstream persists user_info.json + AppConfig cloud section;
/// plaintext-in-QSettings is the accepted project convention — audit doc
/// risk register). invalidate() implements the upstream code==4 token-expiry
/// path: identity cleared, re-login required.
///
/// Region/channel come from AppSettingsService.cloudRegion() via setRegion();
/// the default matches the release build (Release channel, locale-derived
/// region wired by BackendContext).
class CrealityAuthService final : public QObject
{
  Q_OBJECT
  /// True while a valid (non-empty) token is held.
  Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY identityChanged)
  /// Display name (result.user_info.nickName).
  Q_PROPERTY(QString userName READ userName NOTIFY identityChanged)
  /// Avatar URL (result.user_info.avatar), may be empty.
  Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY identityChanged)
  /// Cloud user id (result.userId).
  Q_PROPERTY(QString userId READ userId NOTIFY identityChanged)
  /// True between beginLogin() and success/failure/cancel.
  Q_PROPERTY(bool loginInProgress READ loginInProgress NOTIFY loginInProgressChanged)

public:
  explicit CrealityAuthService(QObject *parent = nullptr);
  ~CrealityAuthService() override;

  /// Wire region (and optionally an explicit channel for dev builds) before
  /// beginLogin(). BackendContext passes AppSettingsService.cloudRegion().
  void setRegion(CrealityCloudConfig::Region region);
  void setChannel(CrealityCloudConfig::Channel channel);
  /// Test/dev hook: override the token-exchange endpoint (e.g. a loopback
  /// stub). Empty (default) uses the CrealityCloudConfig URL. Never set by
  /// the shipping app.
  void setTokenUrlOverride(const QString &url) { tokenUrlOverride_ = url; }
  /// Test/dev hook: override the loopback callback port (0 = config default,
  /// 15888 on Linux) so tests never collide with a running instance.
  void setCallbackPortOverride(int port) { callbackPortOverride_ = port; }

  // ── Identity getters ────────────────────────────────────────────────
  bool loggedIn() const { return !token_.isEmpty(); }
  QString userName() const { return nickname_; }
  QString avatarUrl() const { return avatar_; }
  QString userId() const { return userId_; }
  bool loginInProgress() const { return loginInProgress_; }
  QString token() const { return token_; }

  /// Login page for the embedded web dialog (fixed upstream URL).
  Q_INVOKABLE QString loginUrl() const;
  /// Start the OAuth flow: begin listening on the loopback callback port.
  /// The QML layer opens the web dialog right after calling this.
  Q_INVOKABLE void beginLogin();
  /// Abort an in-flight login (dialog closed by the user).
  Q_INVOKABLE void cancelLogin();
  /// Clear the stored identity (upstream logout semantics).
  Q_INVOKABLE void logout();

  /// Token-expiry path (cloud service saw code==4): drop the token but keep
  /// the user id/nickname for display until re-login.
  void invalidate();

signals:
  void identityChanged();
  void loginInProgressChanged();
  /// Human-readable exchange failure (network/HTTP/server rejection).
  void loginFailed(const QString &message);
  /// Emitted after a successful token exchange — the QML dialog closes.
  void loginSucceeded();

private:
  void exchangeCode(const QString &code);
  void restoreIdentity();
  void persistIdentity() const;
  void clearIdentity();
  void setLoginInProgress(bool inProgress);
  /// Callback port after the test override (config default when 0).
  int effectiveCallbackPort() const;

  CrealityCloudConfig::Region region_ = CrealityCloudConfig::Region::CN;
  CrealityCloudConfig::Channel channel_ = CrealityCloudConfig::Channel::Release;
  QString tokenUrlOverride_;
  int callbackPortOverride_ = 0;
  OAuthCallbackServer *callbackServer_;
  CloudHttpClient *http_;
  QString token_;
  QString userId_;
  QString nickname_;
  QString avatar_;
  bool loginInProgress_ = false;
};
