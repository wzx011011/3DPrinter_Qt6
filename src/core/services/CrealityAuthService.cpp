#include "CrealityAuthService.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QSettings>

namespace {
const char kSettingsGroup[] = "Cloud/Creality";
const char kKeyToken[] = "token";
const char kKeyUserId[] = "userId";
const char kKeyNickname[] = "nickname";
const char kKeyAvatar[] = "avatar";
} // namespace

CrealityAuthService::CrealityAuthService(QObject *parent)
    : QObject(parent),
      callbackServer_(new OAuthCallbackServer(this)),
      http_(new CloudHttpClient(this))
{
    restoreIdentity();
    connect(callbackServer_, &OAuthCallbackServer::codeReceived, this,
            &CrealityAuthService::exchangeCode);
    connect(callbackServer_, &OAuthCallbackServer::errorOccurred, this,
            [this](const QString &message) {
                emit loginFailed(QStringLiteral("本地回调服务: ") + message);
            });
}

CrealityAuthService::~CrealityAuthService() = default;

void CrealityAuthService::setRegion(CrealityCloudConfig::Region region)
{
    region_ = region;
}

void CrealityAuthService::setChannel(CrealityCloudConfig::Channel channel)
{
    channel_ = channel;
}

int CrealityAuthService::effectiveCallbackPort() const
{
    return callbackPortOverride_ > 0 ? callbackPortOverride_
                                     : CrealityCloudConfig::oauthCallbackPort();
}

QString CrealityAuthService::loginUrl() const
{
    return CrealityCloudConfig::loginPageUrl(region_, channel_);
}

void CrealityAuthService::beginLogin()
{
    if (!callbackServer_->start(effectiveCallbackPort())) {
        // A second instance holding the port is the realistic failure; the
        // flow cannot proceed without the loopback capture.
        setLoginInProgress(false);
        return;
    }
    setLoginInProgress(true);
}

void CrealityAuthService::cancelLogin()
{
    callbackServer_->stop();
    setLoginInProgress(false);
}

void CrealityAuthService::logout()
{
    clearIdentity();
}

void CrealityAuthService::invalidate()
{
    if (token_.isEmpty())
        return;
    token_.clear();
    persistIdentity();
    emit identityChanged();
}

void CrealityAuthService::exchangeCode(const QString &code)
{
    const int port = effectiveCallbackPort();
    QJsonObject body;
    body.insert(QStringLiteral("code"), code);
    body.insert(QStringLiteral("clientId"), CrealityCloudConfig::oauthClientId(region_, channel_));
    body.insert(QStringLiteral("redirecturi"),
                CrealityCloudConfig::oauthRedirectUri(region_, channel_, port));

    const auto headers = CloudHttpClient::toHeaderMap(CrealityCloudConfig::extraHeaders(
        /*token*/ {}, /*userId*/ {}, QLocale::system().name(),
        QCoreApplication::applicationVersion(), /*duid*/ {}, 0));

    const QString tokenUrl = tokenUrlOverride_.isEmpty()
                                 ? CrealityCloudConfig::oauthTokenUrl(region_, channel_)
                                 : tokenUrlOverride_;
    http_->postJson(tokenUrl, headers, body,
                    [this](const CloudHttpClient::HttpResponse &response) {
                        if (!response.ok) {
                            emit loginFailed(response.errorCategory == QStringLiteral("timeout")
                                                 ? tr("登录超时，请重试")
                                                 : tr("网络错误：%1").arg(response.errorString));
                            setLoginInProgress(false);
                            return;
                        }
                        const QJsonDocument doc = QJsonDocument::fromJson(response.body);
                        const QJsonObject root = doc.object();
                        // Upstream envelope: {code:0, result:{token, user_info:{nickName,
                        // avatar}, userId}} — userId lives inside result (HttpServer.cpp
                        // r["userId"] = data_node["userId"]); a top-level userId is
                        // accepted as a defensive fallback.
                        if (root.value(QStringLiteral("code")).toInt() != 0) {
                            emit loginFailed(tr("登录被拒绝（code=%1）")
                                                 .arg(root.value(QStringLiteral("code")).toInt()));
                            setLoginInProgress(false);
                            return;
                        }
                        const QJsonObject result =
                            root.value(QStringLiteral("result")).toObject();
                        token_ = result.value(QStringLiteral("token")).toString();
                        userId_ = result.value(QStringLiteral("userId")).toString(
                            root.value(QStringLiteral("userId")).toString());
                        const QJsonObject userInfo =
                            result.value(QStringLiteral("user_info")).toObject();
                        nickname_ = userInfo.value(QStringLiteral("nickName")).toString();
                        avatar_ = userInfo.value(QStringLiteral("avatar")).toString();
                        if (token_.isEmpty()) {
                            emit loginFailed(tr("登录响应缺少 token"));
                            setLoginInProgress(false);
                            return;
                        }
                        persistIdentity();
                        callbackServer_->stop();
                        setLoginInProgress(false);
                        emit identityChanged();
                        emit loginSucceeded();
                    });
}

void CrealityAuthService::restoreIdentity()
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    token_ = settings.value(kKeyToken).toString();
    userId_ = settings.value(kKeyUserId).toString();
    nickname_ = settings.value(kKeyNickname).toString();
    avatar_ = settings.value(kKeyAvatar).toString();
    settings.endGroup();
}

void CrealityAuthService::persistIdentity() const
{
    QSettings settings;
    settings.beginGroup(kSettingsGroup);
    settings.setValue(kKeyToken, token_);
    settings.setValue(kKeyUserId, userId_);
    settings.setValue(kKeyNickname, nickname_);
    settings.setValue(kKeyAvatar, avatar_);
    settings.endGroup();
    settings.sync();
}

void CrealityAuthService::clearIdentity()
{
    token_.clear();
    userId_.clear();
    nickname_.clear();
    avatar_.clear();
    persistIdentity();
    emit identityChanged();
}

void CrealityAuthService::setLoginInProgress(bool inProgress)
{
    if (loginInProgress_ == inProgress)
        return;
    loginInProgress_ = inProgress;
    emit loginInProgressChanged();
}
