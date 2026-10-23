#pragma once

#include <QObject>
#include <QString>

class CrealityAuthService;

/// ModelLibraryBridge (feature/cloud-model-library, M3).
///
/// Native side of the model-library webview JS bridge, a Qt/QML port of the
/// official open-source CrealityPrint WebModelLibraryView protocol:
/// - web→native: the page calls `window.CXSWGroupInterface.postMessage(json)`
///   (a QWebChannel shim re-exports the registered object under that exact
///   name — see ModelLibraryPanel.qml); messages are
///   {"action":"toNative","message":{…}}.
/// - native→web: the shell calls `window.handleStudioCmd(json)` via
///   runJavaScript; this object emits sendToWeb(json) and the QML panel
///   performs the call.
///
/// M3 command set (upstream GUI_App.cpp handle_web_request subset):
///   get_account_info  → {command, userInfo:{token,nickName,userId,…}}
///                       (empty strings when logged out — the frontend shows
///                       its own 登录/注册 entry)
///   trigger_login_check → re-emits the command so the frontend opens its
///                       in-page login UI
///   get_client_id     → {command, clientId:<duid>}
/// Incoming messages may also carry `linkType`:
///   1   → login required: replies trigger_login_check (upstream :368-375)
///   403 → open message.linkUrl.url in the system browser (:390-435)
///   404 → token expired: emits tokenExpired (panel notifies; the account
///         face token invalidation is the auth service's invalidate())
/// M4 extends handleCommand() with the download/import commands.
class ModelLibraryBridge final : public QObject
{
  Q_OBJECT

public:
  explicit ModelLibraryBridge(CrealityAuthService *auth, QObject *parent = nullptr);

  /// Web→native entry (the CXSWGroupInterface shim forwards here).
  Q_INVOKABLE void postMessage(const QString &json);
  /// Identity payload the panel injects as `window.handleStudioCmd(...)`.
  Q_INVOKABLE void sendCommand(const QString &commandJson);

  /// DUID for get_client_id (MAC-derived with persisted-UUID fallback —
  /// injected so BackendContext owns the QSettings side).
  void setDuid(const QString &duid) { duid_ = duid; }

signals:
  /// Native→web: panel runs `window.handleStudioCmd(<json>)`.
  void sendToWeb(const QString &json);
  /// linkType 403: open the URL outside the embedded view.
  void openInSystemBrowser(const QString &url);
  /// linkType 404: session token expired (frontend shows its login entry).
  void tokenExpired();
  /// Diagnostics surface for the panel status line (M5 error states).
  void bridgeEvent(const QString &message);

private:
  void routeMessage(const QString &json);
  void handleCommand(const QString &command, const QString &messageJson);
  void reply(const QJsonObject &message);
  void reply(const QString &json);

  CrealityAuthService *auth_;
  QString duid_;
};
