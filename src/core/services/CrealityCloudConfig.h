#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

/// CrealityCloudConfig (feature/cloud-model-library, M1).
///
/// Single configuration layer for the Creality Cloud (创想云) integration.
/// Every remote endpoint, the WebView identity (user agent), the `__CXY_*`
/// request-header protocol, the OAuth client ids and the device id (DUID)
/// live here and nowhere else, so a server-side change or a risk-control
/// switch can be answered by editing one file.
///
/// The protocol constants are ported from the official open-source
/// CrealityPrint client (AGPL-3.0, github.com/CrealityOfficial/CrealityPrint):
/// - URL assembly:  src/slic3r/GUI/GUI.cpp get_cloud_api_url / get_cloud_webaddress
/// - headers:       src/slic3r/GUI/GUI_App.cpp get_extra_header / get_modellibrary_header
/// - language map:  src/slic3r/GUI/GUI_App.cpp I18nToLangaugeIndex
/// - OAuth:         src/slic3r/GUI/HttpServer.cpp creality_handle_request
/// A copy of the reference sources is kept in CP/creality-ref/src-ref/.
///
/// Design constraints:
/// - Pure value type (no QObject, no I/O, no clocks) so the endpoint/header
///   matrix is unit-testable offline (tests/CrealityCloudConfigTests.cpp).
/// - DUID derivation is injectable: real callers pass macAddressCandidates()
///   data; tests pass fixed strings. The persisted fallback id lives in
///   QSettings under "Cloud/DuidFallback" (generated once per install).
class CrealityCloudConfig
{
public:
  /// Region selection mirrors upstream app_config country_code: "CN" vs rest.
  enum class Region { CN, International };
  /// Build channel mirrors upstream get_vertion_type() (PROJECT_VERSION_EXTRA).
  enum class Channel { Release, Alpha, Dev };

  /// ── Endpoint assembly (upstream GUI.cpp:633-720) ────────────────────
  /// REST API base, e.g. https://api.crealitycloud.cn (Release CN).
  static QString apiUrl(Region region, Channel channel);
  /// Model library web home, e.g. https://cp.crealitycloud.cn/ (Release CN).
  static QString webAddress(Region region, Channel channel);
  /// Model library category page (default landing page).
  static QString modelLibraryHome(Region region, Channel channel);
  /// Model library search page for a query string (value will be urlencoded).
  static QString modelLibrarySearch(Region region, Channel channel, const QString &query);
  /// Login page hosted on the main site (upstream LoginDialog fixed URL).
  static QString loginPageUrl(Region region, Channel channel);
  /// Public OAuth redirect target embedding the local callback, e.g.
  /// https://www.crealitycloud.cn/oauth?back_url=http://localhost:15888/login
  static QString oauthRedirectUri(Region region, Channel channel, int localPort);
  /// OAuth code→token exchange endpoint (POST oauthLogin).
  static QString oauthTokenUrl(Region region, Channel channel);

  /// ── Client identity ─────────────────────────────────────────────────
  /// OAuth application client id (upstream HttpServer.cpp hardcoded per
  /// channel/region; carried over verbatim from the open-source client).
  static QString oauthClientId(Region region, Channel channel);
  /// WebView user agent. The remote frontend uses it to detect the shell
  /// (upstream WebModelLibraryView.cpp UpdateUserAgent). Linux variant.
  static QString webViewUserAgent(const QString &appVersion);
  /// Local OAuth callback server port (upstream HttpServer.hpp: 13666 on
  /// Windows, 15888 elsewhere; this build targets Linux first).
  static int oauthCallbackPort();

  /// ── `__CXY_*` header protocol ───────────────────────────────────────
  /// Headers for REST API calls (upstream get_extra_header). token/uid may
  /// be empty (anonymous) — the upstream client sends them empty as well.
  static QVariantMap extraHeaders(const QString &token,
                                  const QString &userId,
                                  const QString &languageCode,
                                  const QString &appVersion,
                                  const QString &duid,
                                  int timezoneOffsetSeconds);
  /// Headers mirrored into the model-library web session cookies (upstream
  /// get_modellibrary_header). Note `__CXY_WEBVIEW_TYPE_` identifies the
  /// shell to the frontend.
  static QVariantMap modelLibraryHeaders(const QString &token,
                                         const QString &userId,
                                         const QString &languageCode,
                                         const QString &appVersion,
                                         const QString &duid);
  /// Header names as constants (shared by the cookie-injection and the HTTP
  /// layers so a rename cannot desynchronize the two call sites).
  static QStringList headerNames();

  /// ── Language index (upstream I18nToLangaugeIndex) ───────────────────
  /// Normalize a BCP47-ish code ("zh-Hans" → "zh_cn") and map to the wire
  /// index ("zh_cn" → "1"). Unknown codes fall back to "0" (English) so a
  /// non-Chinese locale never renders the mall in Chinese.
  static QString languageIndex(const QString &i18nCode);

  /// ── Device id (upstream get_client_id / GetMACAddress) ──────────────
  /// Deterministic device id from the first stable interface MAC. When no
  /// usable MAC exists (the reference client ships a placeholder on Linux),
  /// callers fall back to a per-install persisted UUID.
  static QString deriveDuid(const QStringList &candidateMacAddresses,
                            const QString &persistedFallback);
  /// Compute the candidates from QNetworkInterface (non-loopback, non-empty,
  /// stable order) — separated from deriveDuid for testability.
  static QStringList macAddressCandidates();

private:
  static QString host(Region region, const char *cnHost, const char *intlHost);
};
