#pragma once

#include <QObject>
#include <QString>
#include <QTcpServer>

/// OAuthCallbackServer (feature/cloud-model-library, M2).
///
/// Minimal loopback HTTP listener capturing the OAuth authorization code,
/// ported from the upstream CrealityPrint HttpServer /login handler
/// (creality_handle_request, HttpServer.cpp:844-960). The remote login page
/// (hosted in the embedded webview) ends its OAuth flow by navigating to
///   http://localhost:<port>/login?code=...&result_url=...
/// This server answers that navigation with a 302 redirect and hands the
/// code to CrealityAuthService for the token exchange. Only GET /login is
/// understood; every other request gets 404 and an immediate close.
///
/// Deliberately QTcpServer-based instead of QtHttpServer: the parsed surface
/// is one request line + one 302 response, and this keeps the build free of
/// an additional Qt module. Binds LocalHost only. Port parity with the
/// reference client (15888 on Linux/macOS, 13666 on Windows) so the public
/// oauth redirect templates keep matching.
class OAuthCallbackServer final : public QObject
{
  Q_OBJECT
  /// Whether the loopback listener is currently accepting connections.
  Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)

public:
  explicit OAuthCallbackServer(QObject *parent = nullptr);
  ~OAuthCallbackServer() override;

  /// Bind the loopback listener. Fails (false + errorOccurred) when the
  /// port is taken by another instance.
  bool start(int port);
  void stop();
  bool listening() const;

  /// Page the browser is bounced to after the code is captured
  /// (upstream bounces to the oauth page's result_url with ?result=…).
  void setSuccessRedirectUrl(const QString &url);
  void setFailureRedirectUrl(const QString &url);

signals:
  /// The authorization code captured from GET /login?code=…
  void codeReceived(const QString &code);
  void listeningChanged();
  /// Socket/bind level failure (never a login failure).
  void errorOccurred(const QString &message);

private:
  void handleConnection();

  QTcpServer *server_ = nullptr;
  QString successRedirectUrl_ = QStringLiteral("https://www.crealitycloud.cn/oauth?result=success");
  QString failureRedirectUrl_ = QStringLiteral("https://www.crealitycloud.cn/oauth?result=fail");
};
