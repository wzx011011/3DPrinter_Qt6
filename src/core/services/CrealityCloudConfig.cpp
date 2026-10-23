#include "CrealityCloudConfig.h"

#include <QDateTime>
#include <QNetworkInterface>
#include <QSettings>
#include <QUrl>
#include <QUuid>

// ── Endpoint assembly (upstream GUI.cpp:633-720 get_cloud_api_url /
//    get_cloud_webaddress) ────────────────────────────────────────────────
// Dev channels keep the http:// scheme exactly as the reference client does
// (they are internal test hosts behind plain http).

QString CrealityCloudConfig::host(Region region, const char *cnHost, const char *intlHost)
{
    return region == Region::CN ? QString::fromLatin1(cnHost) : QString::fromLatin1(intlHost);
}

QString CrealityCloudConfig::apiUrl(Region region, Channel channel)
{
    switch (channel) {
    case Channel::Dev:
        // Upstream: ENV_CN_DEV only exposes a CN dev API host.
        return QStringLiteral("http://api-dev.crealitycloud.cn");
    case Channel::Alpha:
        return QStringLiteral("https://admin-pre.") + host(region, "crealitycloud.cn", "crealitycloud.com");
    case Channel::Release:
    default:
        return QStringLiteral("https://api.") + host(region, "crealitycloud.cn", "crealitycloud.com");
    }
}

QString CrealityCloudConfig::webAddress(Region region, Channel channel)
{
    switch (channel) {
    case Channel::Dev:
        // Upstream dev2 host exists for CN only; international dev falls back
        // to the same host (the reference client has no intl dev web host).
        return QStringLiteral("https://dev2.crealitycloud.cn/");
    case Channel::Alpha:
        return QStringLiteral("https://pre-2.") + host(region, "crealitycloud.cn", "crealitycloud.com") + QStringLiteral("/");
    case Channel::Release:
    default:
        return QStringLiteral("https://cp.") + host(region, "crealitycloud.cn", "crealitycloud.com") + QStringLiteral("/");
    }
}

QString CrealityCloudConfig::modelLibraryHome(Region region, Channel channel)
{
    // Upstream WebModelLibraryView::open_default_page (:186-189).
    return webAddress(region, channel) + QStringLiteral("model-category/3d-print-all");
}

QString CrealityCloudConfig::modelLibrarySearch(Region region, Channel channel, const QString &query)
{
    // Upstream open_search_page (:200): "search/models?q=<urlencoded>".
    return webAddress(region, channel) + QStringLiteral("search/models?q=")
           + QString::fromUtf8(QUrl::toPercentEncoding(query));
}

QString CrealityCloudConfig::loginPageUrl(Region region, Channel channel)
{
    // 登录走模型库站点页内完成（官方 WebModelLibraryView 同构设计）：商城
    // 头部自带"登录/注册"，登录态落在共享 WebEngineProfile 的 cookie 上，
    // 模型库面板直接复用。上游硬编码的 www.creality.com/pages/login 独立
    // 登录页已随官网改版下线（/pages/login 与 /login 均 404/SPA-404，验证
    // 于 2026-10-23）——这正是审计文档登记的端点漂移风险，改动收敛在本
    // 配置层。loopback OAuth 回调链路保留（oauth 换 token 端点仍然有效，
    // 供后续云预设同步账号面使用）。
    // 社区门户首页（www.crealitycloud.cn|com）带"登录/注册"入口，与商城
    // （cp.*）同域——登录 cookie 直接为模型库面板所用。商城页自身在壳嵌入
    // 模式下隐藏登录头部（实测 2026-10-23），故以门户页为登录承载页。
    return QStringLiteral("https://www.") + host(region, "crealitycloud.cn", "crealitycloud.com")
           + QStringLiteral("/");
}

QString CrealityCloudConfig::oauthRedirectUri(Region region, Channel channel, int localPort)
{
    // Upstream HttpServer.cpp creality_handle_request: the public oauth page
    // bounces the code back to our loopback listener via back_url.
    // Release → www.crealitycloud.<tld>, Alpha → pre.crealitycloud.<tld>.
    const char *prefix = channel == Channel::Alpha ? "pre" : "www";
    return QStringLiteral("https://%1.crealitycloud.")
           .arg(QLatin1String(prefix))
           + (region == Region::CN ? QStringLiteral("cn") : QStringLiteral("com"))
           + QStringLiteral("/oauth?back_url=http://localhost:%1/login").arg(localPort);
}

QString CrealityCloudConfig::oauthTokenUrl(Region region, Channel channel)
{
    return apiUrl(region, channel) + QStringLiteral("/api/cxy/account/v2/oauthLogin");
}

// ── Client identity ───────────────────────────────────────────────────────

QString CrealityCloudConfig::oauthClientId(Region region, Channel channel)
{
    // Carried over verbatim from upstream HttpServer.cpp creality_handle_request.
    switch (channel) {
    case Channel::Dev:
        return QStringLiteral("e216beaa82e75f8f9d04b2ec822bcc48");
    case Channel::Alpha:
        return region == Region::CN ? QStringLiteral("b1b46c39fce85c7982563feebe0d3277")
                                    : QStringLiteral("275d77a3cc5059089a6d39c9292de38b");
    case Channel::Release:
    default:
        return region == Region::CN ? QStringLiteral("8ea5010984fa52a298f12110af8b05d0")
                                    : QStringLiteral("f9c302ecc29c59a0a6e921ff39a073ca");
    }
}

QString CrealityCloudConfig::webViewUserAgent(const QString &appVersion)
{
    // Upstream WebModelLibraryView.cpp UpdateUserAgent, Linux branch
    // (:509-528). The trailing WebView/<ver> token mirrors the embedded
    // Chromium major the reference client ships; the remote frontend uses
    // the "creality_print_slice/<ver>" marker to detect the desktop shell.
    return QStringLiteral("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
                          "(KHTML, like Gecko) Chrome/141.0.0.0 Safari/537.36 "
                          "creality_print_slice/%1 WebView/141.0.0.0").arg(appVersion);
}

int CrealityCloudConfig::oauthCallbackPort()
{
#ifdef Q_OS_WINDOWS
    return 13666;
#else
    return 15888;
#endif
}

// ── `__CXY_*` header protocol (upstream GUI_App.cpp get_extra_header /
//    get_modellibrary_header) ─────────────────────────────────────────────

QVariantMap CrealityCloudConfig::extraHeaders(const QString &token,
                                              const QString &userId,
                                              const QString &languageCode,
                                              const QString &appVersion,
                                              const QString &duid,
                                              int timezoneOffsetSeconds)
{
    QVariantMap headers;
    headers.insert(QStringLiteral("__CXY_APP_ID_"), QStringLiteral("creality_model"));
    headers.insert(QStringLiteral("__CXY_PLATFORM_"), QStringLiteral("11"));
    headers.insert(QStringLiteral("__CXY_BRAND_"), QStringLiteral("creality"));
    headers.insert(QStringLiteral("__CXY_APP_CH_"), QStringLiteral("creality"));
    headers.insert(QStringLiteral("__CXY_OS_LANG_"), languageIndex(languageCode));
    headers.insert(QStringLiteral("__CXY_DUID_"), duid);
    headers.insert(QStringLiteral("__CXY_APP_VER_"), appVersion);
    headers.insert(QStringLiteral("__CXY_TIMEZONE_"), QString::number(timezoneOffsetSeconds));
    headers.insert(QStringLiteral("__CXY_UID_"), userId);
    headers.insert(QStringLiteral("__CXY_TOKEN_"), token);
#ifdef Q_OS_WINDOWS
    headers.insert(QStringLiteral("__CXY_OS_VER_"), QStringLiteral("win"));
#elif defined(Q_OS_MACOS)
    headers.insert(QStringLiteral("__CXY_OS_VER_"), QStringLiteral("macos"));
#else
    headers.insert(QStringLiteral("__CXY_OS_VER_"), QStringLiteral("linux"));
#endif
    return headers;
}

QVariantMap CrealityCloudConfig::modelLibraryHeaders(const QString &token,
                                                     const QString &userId,
                                                     const QString &languageCode,
                                                     const QString &appVersion,
                                                     const QString &duid)
{
    QVariantMap headers;
    headers.insert(QStringLiteral("__CXY_TOKEN_"), token);
    headers.insert(QStringLiteral("__CXY_UID_"), userId);
    headers.insert(QStringLiteral("__CXY_OS_LANG_"), languageIndex(languageCode));
    headers.insert(QStringLiteral("__CXY_APP_VER_"), appVersion);
    headers.insert(QStringLiteral("__CXY_WEBVIEW_TYPE_"), QStringLiteral("creality_print_slice"));
    headers.insert(QStringLiteral("__CXY_PLATFORM_"), QStringLiteral("11"));
    headers.insert(QStringLiteral("__CXY_DUID_"), duid);
    return headers;
}

QStringList CrealityCloudConfig::headerNames()
{
    return {
        QStringLiteral("__CXY_TOKEN_"),
        QStringLiteral("__CXY_UID_"),
        QStringLiteral("__CXY_OS_LANG_"),
        QStringLiteral("__CXY_APP_VER_"),
        QStringLiteral("__CXY_WEBVIEW_TYPE_"),
        QStringLiteral("__CXY_PLATFORM_"),
        QStringLiteral("__CXY_DUID_"),
        QStringLiteral("__CXY_APP_ID_"),
        QStringLiteral("__CXY_BRAND_"),
        QStringLiteral("__CXY_APP_CH_"),
        QStringLiteral("__CXY_TIMEZONE_"),
        QStringLiteral("__CXY_OS_VER_"),
    };
}

// ── Language index (upstream I18nToLangaugeIndex, GUI_App.cpp:3757) ──────

QString CrealityCloudConfig::languageIndex(const QString &i18nCode)
{
    // Normalize: lowercase, '-' → '_', strip locale modifiers at the first
    // '.'/'@'/' ' (earliest of the three, like std::string::find_first_of in
    // the reference), fold the common zh variants — same rules as upstream.
    QString key = i18nCode.toLower();
    key.replace(QLatin1Char('-'), QLatin1Char('_'));
    int modifierPos = -1;
    for (const QChar sep : { QChar('.'), QChar('@'), QChar(' ') }) {
        const int pos = key.indexOf(sep);
        if (pos != -1 && (modifierPos == -1 || pos < modifierPos))
            modifierPos = pos;
    }
    if (modifierPos != -1)
        key.truncate(modifierPos);
    if (key == QLatin1String("zh_hans") || key == QLatin1String("zh_cn_hans"))
        key = QStringLiteral("zh_cn");
    if (key == QLatin1String("zh_hant") || key == QLatin1String("zh_tw_hant")
        || key == QLatin1String("zh_hk_hant"))
        key = QStringLiteral("zh_tw");

    static const QHash<QString, QString> kIndexMap = {
        { QStringLiteral("en"), QStringLiteral("0") },
        { QStringLiteral("en_us"), QStringLiteral("0") },
        { QStringLiteral("en_uk"), QStringLiteral("0") },
        { QStringLiteral("en_gb"), QStringLiteral("0") },
        { QStringLiteral("zh"), QStringLiteral("1") },
        { QStringLiteral("zh_cn"), QStringLiteral("1") },
        { QStringLiteral("zh_tw"), QStringLiteral("2") },
        { QStringLiteral("zh_hk"), QStringLiteral("3") },
        { QStringLiteral("ru"), QStringLiteral("4") },
        { QStringLiteral("ru_ru"), QStringLiteral("4") },
        { QStringLiteral("uk_ua"), QStringLiteral("4") },
        { QStringLiteral("ko"), QStringLiteral("5") },
        { QStringLiteral("ko_kr"), QStringLiteral("5") },
        { QStringLiteral("es"), QStringLiteral("7") },
        { QStringLiteral("ca_es"), QStringLiteral("7") },
        { QStringLiteral("de"), QStringLiteral("8") },
        { QStringLiteral("de_de"), QStringLiteral("8") },
        { QStringLiteral("fr"), QStringLiteral("9") },
        { QStringLiteral("fr_fr"), QStringLiteral("9") },
        { QStringLiteral("ja"), QStringLiteral("10") },
        { QStringLiteral("ja_jp"), QStringLiteral("10") },
        { QStringLiteral("pt"), QStringLiteral("11") },
        { QStringLiteral("pt_pt"), QStringLiteral("11") },
        { QStringLiteral("th"), QStringLiteral("12") },
        { QStringLiteral("th_th"), QStringLiteral("12") },
        { QStringLiteral("nl"), QStringLiteral("13") },
        { QStringLiteral("nl_nl"), QStringLiteral("13") },
        { QStringLiteral("it"), QStringLiteral("14") },
        { QStringLiteral("it_it"), QStringLiteral("14") },
        { QStringLiteral("pt_br"), QStringLiteral("15") },
        { QStringLiteral("tr"), QStringLiteral("16") },
        { QStringLiteral("ro"), QStringLiteral("17") },
        { QStringLiteral("he"), QStringLiteral("18") },
        { QStringLiteral("pl"), QStringLiteral("19") },
        { QStringLiteral("pl_pl"), QStringLiteral("19") },
        { QStringLiteral("in"), QStringLiteral("20") },
        { QStringLiteral("in_in"), QStringLiteral("20") },
        { QStringLiteral("id"), QStringLiteral("20") },
        { QStringLiteral("id_id"), QStringLiteral("20") },
        { QStringLiteral("hu"), QStringLiteral("21") },
        { QStringLiteral("hu_hu"), QStringLiteral("21") },
        { QStringLiteral("uk"), QStringLiteral("22") },
        { QStringLiteral("vi"), QStringLiteral("23") },
        { QStringLiteral("vi_vn"), QStringLiteral("23") },
    };

    if (const auto it = kIndexMap.constFind(key); it != kIndexMap.constEnd())
        return *it;
    // Primary-language fallback ("en_US" → "en").
    const int under = key.indexOf(QLatin1Char('_'));
    if (under != -1) {
        if (const auto base = kIndexMap.constFind(key.left(under)); base != kIndexMap.constEnd())
            return *base;
    }
    // Unknown → English, so a non-Chinese locale never renders the mall in
    // Chinese (upstream fallback rationale).
    return QStringLiteral("0");
}

// ── Device id (upstream get_client_id / GetMACAddress) ───────────────────

QString CrealityCloudConfig::deriveDuid(const QStringList &candidateMacAddresses,
                                        const QString &persistedFallback)
{
    for (const QString &mac : candidateMacAddresses) {
        if (!mac.isEmpty() && mac != QLatin1String("00:00:00:00:00:00"))
            return mac;
    }
    return persistedFallback;
}

QStringList CrealityCloudConfig::macAddressCandidates()
{
    QStringList candidates;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if ((iface.flags() & QNetworkInterface::IsLoopBack)
            || !(iface.flags() & QNetworkInterface::IsUp))
            continue;
        const QString mac = iface.hardwareAddress();
        if (!mac.isEmpty())
            candidates << mac;
    }
    return candidates;
}
