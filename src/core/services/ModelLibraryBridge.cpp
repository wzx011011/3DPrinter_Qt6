#include "ModelLibraryBridge.h"

#include "CrealityAuthService.h"
#include "ModelDownloadService.h"

#include <QDesktopServices>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QUrl>

ModelLibraryBridge::ModelLibraryBridge(CrealityAuthService *auth,
                                       ModelDownloadService *downloads,
                                       QObject *parent)
    : QObject(parent), auth_(auth), downloads_(downloads)
{
}

void ModelLibraryBridge::postMessage(const QString &json)
{
  qDebug("[ModelLibraryBridge] postMessage: %s", qPrintable(json.left(300)));
  routeMessage(json);
}

void ModelLibraryBridge::sendCommand(const QString &commandJson)
{
  reply(commandJson);
}

void ModelLibraryBridge::routeMessage(const QString &json)
{
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
    emit bridgeEvent(QStringLiteral("bridge: unparsable message (%1)")
                         .arg(parseError.errorString()));
    return;
  }
  const QJsonObject root = doc.object();
  if (root.value(QStringLiteral("action")).toString()
      != QStringLiteral("toNative"))
    return; // not a toNative envelope — ignore silently (upstream parity)

  const QJsonObject message = root.value(QStringLiteral("message")).toObject();

  // linkType routing (upstream WebModelLibraryView.cpp :368-462).
  const int linkType = message.value(QStringLiteral("linkType")).toInt(0);
  if (linkType == 1) {
    // Login required: ask the frontend to show its own in-page login.
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), QStringLiteral("trigger_login_check"));
    replyMsg.insert(QStringLiteral("context"), QStringLiteral("mall_link"));
    reply(replyMsg);
    return;
  }
  if (linkType == 403) {
    const QJsonObject linkUrl = message.value(QStringLiteral("linkUrl")).toObject();
    const QString url = linkUrl.value(QStringLiteral("url")).toString();
    if (!url.isEmpty()) {
      emit openInSystemBrowser(url);
      QDesktopServices::openUrl(QUrl(url));
    }
    return;
  }
  if (linkType == 404) {
    emit tokenExpired();
    if (auth_)
      auth_->invalidate();
    return;
  }

  const QString command = message.value(QStringLiteral("command")).toString();
  if (command.isEmpty())
    return;
  handleCommand(command, json);
}

void ModelLibraryBridge::handleCommand(const QString &command, const QString &messageJson)
{
  Q_UNUSED(messageJson);
  if (command == QLatin1String("get_account_info")) {
    // Upstream GUI_App.cpp:8077 shape: the frontend reads nickName/avatar/
    // userId/token from userInfo. Logged-out → empty strings (the frontend
    // then shows its own 登录/注册 header entry — verified 2026-10-23).
    QJsonObject userInfo;
    userInfo.insert(QStringLiteral("token"), auth_ ? auth_->token() : QString());
    userInfo.insert(QStringLiteral("nickName"), auth_ ? auth_->userName() : QString());
    userInfo.insert(QStringLiteral("avatar"), QString());
    userInfo.insert(QStringLiteral("userId"), auth_ ? auth_->userId() : QString());
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), command);
    replyMsg.insert(QStringLiteral("userInfo"), userInfo);
    reply(replyMsg);
    return;
  }
  if (command == QLatin1String("trigger_login_check")) {
    // Echo back so the frontend opens its in-page login UI
    // (upstream GUI_App.cpp:8035).
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), command);
    replyMsg.insert(QStringLiteral("context"), QStringLiteral("3mf_download_login"));
    reply(replyMsg);
    return;
  }
  if (command == QLatin1String("get_client_id")) {
    // Upstream GUI_App.cpp:8293 (MAC address, first-boot UUID fallback).
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), command);
    replyMsg.insert(QStringLiteral("clientId"), duid_);
    reply(replyMsg);
    return;
  }
  if (command.endsWith(QLatin1String("_download_start")) && downloads_) {
    // Upstream GUI_App.cpp:8375/8400: message carries userId/modelId and
    // files — a single object for 3mf_download_start, an array for
    // models_download_start.
    const QJsonObject msg = QJsonDocument::fromJson(messageJson.toUtf8())
                                .object()
                                .value(QStringLiteral("message"))
                                .toObject();
    const QString userId = msg.value(QStringLiteral("userId")).toString();
    const QString modelId = msg.value(QStringLiteral("modelId")).toString();
    QJsonArray files = msg.value(QStringLiteral("files")).toArray();
    if (files.isEmpty()) {
      const QJsonObject single = msg.value(QStringLiteral("files")).toObject();
      if (!single.isEmpty())
        files.append(single);
    }
    for (const QJsonValue &fv : files) {
      const QJsonObject f = fv.toObject();
      downloads_->startDownload(f.value(QStringLiteral("url")).toString(),
                                userId, modelId,
                                f.value(QStringLiteral("fileId")).toString(),
                                f.value(QStringLiteral("fileFormat")).toString(),
                                f.value(QStringLiteral("name")).toString());
    }
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), command);
    replyMsg.insert(QStringLiteral("result"), 0);
    reply(replyMsg);
    emit switchToEditorRequested(); // upstream ModelLibraryRoutes.cpp:32
    return;
  }
  if (command.endsWith(QLatin1String("_download_state"))) {
    QJsonObject replyMsg;
    replyMsg.insert(QStringLiteral("command"), command);
    const QVariantMap state = downloads_ ? downloads_->downloadState()
                                         : QVariantMap();
    replyMsg.insert(QStringLiteral("models"),
                    QJsonArray::fromVariantList(
                        state.value(QStringLiteral("models")).toList()));
    replyMsg.insert(QStringLiteral("3mfs"),
                    QJsonArray::fromVariantList(
                        state.value(QStringLiteral("3mfs")).toList()));
    reply(replyMsg);
    return;
  }
  if (command == QLatin1String("models_download_delete")
      || command == QLatin1String("3mfs_download_delete")) {
    const QJsonObject msg = QJsonDocument::fromJson(messageJson.toUtf8())
                                .object()
                                .value(QStringLiteral("message"))
                                .toObject();
    if (downloads_)
      downloads_->forgetDownload(msg.value(QStringLiteral("fileId")).toString());
    return;
  }
  if (command == QLatin1String("3mf_download_import")) {
    // Upstream GUI_App.cpp:8490: plater->load_project(path).
    const QJsonObject msg = QJsonDocument::fromJson(messageJson.toUtf8())
                                .object()
                                .value(QStringLiteral("message"))
                                .toObject();
    const QString path = msg.value(QStringLiteral("path")).toString();
    if (!path.isEmpty())
      emit importProjectRequested(path);
    return;
  }
  if (command == QLatin1String("models_download_import")) {
    // Upstream GUI_App.cpp:8510: plater->load_files(paths, LoadModel).
    const QJsonObject msg = QJsonDocument::fromJson(messageJson.toUtf8())
                                .object()
                                .value(QStringLiteral("message"))
                                .toObject();
    QStringList paths;
    const QJsonArray arr = msg.value(QStringLiteral("paths")).toArray();
    for (const QJsonValue &v : arr)
      paths << v.toString();
    if (paths.isEmpty()) {
      const QString single = msg.value(QStringLiteral("path")).toString();
      if (!single.isEmpty())
        paths << single;
    }
    if (!paths.isEmpty())
      emit importModelFilesRequested(paths);
    return;
  }
  // Unknown commands are logged, not fatal (the frontend degrades).
  emit bridgeEvent(QStringLiteral("bridge: unhandled command %1").arg(command));
}

void ModelLibraryBridge::reply(const QJsonObject &message)
{
  emit sendToWeb(QString::fromUtf8(QJsonDocument(message).toJson(QJsonDocument::Compact)));
}

void ModelLibraryBridge::reply(const QString &json)
{
  emit sendToWeb(json);
}
