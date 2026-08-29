// AiChatBridge implementation — see AiChatBridge.h.

#include "AiChatBridge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/ai/AiAgentService.h"
#include "core/viewmodels/AiMessageDecorate.h"
#include "core/viewmodels/AiViewModel.h"

namespace {

QString jsonToString(const QJsonObject &obj) {
  return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

}  // namespace

AiChatBridge::AiChatBridge(AiViewModel *viewModel, AiAgentService *service,
                           QObject *parent)
    : QObject(parent), viewModel_(viewModel), service_(service) {
  // QML WebChannel publishes registeredObjects under their objectName; the
  // chat page looks the bridge up as channel.objects.bridge.
  setObjectName(QStringLiteral("bridge"));
  connect(service_, &AiAgentService::sidecarReady, this, [this](const QString &) {
    emitState();
    emit assistantTextAppended(
        QStringLiteral("AI 助手已就绪。你好！可以用大白话指挥我：加载模型、调参数、切片、排查问题都行。"));
  });
  connect(service_, &AiAgentService::assistantTextChanged, this,
          &AiChatBridge::assistantTextAppended);
  connect(service_, &AiAgentService::toolUseStarted, this,
          [this](const QString &id, const QString &name, const QJsonObject &input) {
            QVariantMap entry;
            entry.insert(QStringLiteral("kind"), QStringLiteral("tool"));
            entry.insert(QStringLiteral("id"), id);
            entry = OWzx::AiMessageDecorate::decorateToolEntry(std::move(entry), name, input);
            emit toolStarted(id,
                             jsonToString(QJsonObject::fromVariantMap(entry)));
          });
  connect(service_, &AiAgentService::toolUseFinished, this,
          [this](const QString &id, bool ok, const QString &summary) {
            emit toolFinished(id, ok,
                              OWzx::AiMessageDecorate::friendlyResult(ok, summary),
                              summary.left(300));
          });
  connect(service_, &AiAgentService::permissionRequested, this,
          [this](int callId, const QString &tool, const QJsonObject &input) {
            QVariantMap entry;
            entry.insert(QStringLiteral("kind"), QStringLiteral("permission"));
            entry.insert(QStringLiteral("callId"), callId);
            entry.insert(QStringLiteral("pending"), true);
            entry = OWzx::AiMessageDecorate::decorateToolEntry(std::move(entry), tool, input);
            emit permissionRequested(
                callId, jsonToString(QJsonObject::fromVariantMap(entry)));
          });
  connect(service_, &AiAgentService::turnFinished, this,
          [this](bool isError, int, const QString &) {
    if (isError)
      emit errorOccurred(QStringLiteral("本轮对话异常结束，可重试。"));
    emitState();
  });
  connect(service_, &AiAgentService::chatError, this,
          &AiChatBridge::errorOccurred);
  connect(service_, &AiAgentService::stateChanged, this,
          &AiChatBridge::emitState);
}

void AiChatBridge::pageReady() {
  // Replay the current transcript (the harness session keeps real context;
  // this restores what the user saw before the panel was closed/reloaded).
  QJsonArray array;
  const QVariantList messages = viewModel_ ? viewModel_->messages() : QVariantList{};
  for (const QVariant &v : messages)
    array.append(QJsonObject::fromVariantMap(v.toMap()));
  emit historyReplayed(QString::fromUtf8(
      QJsonDocument(array).toJson(QJsonDocument::Compact)));
  emitState();
}

void AiChatBridge::sendMessage(const QString &text) {
  if (!viewModel_)
    return;
  viewModel_->sendMessage(text);
  emitState();
}

void AiChatBridge::answerPermission(int callId, bool allow) {
  if (!viewModel_)
    return;
  viewModel_->answerPermission(callId, allow);
  emit permissionResolved(callId, allow);
}

void AiChatBridge::cancelTurn() {
  if (viewModel_)
    viewModel_->cancelTurn();
}

void AiChatBridge::clearHistory() {
  if (!viewModel_)
    return;
  viewModel_->clearHistory();
  emit historyCleared();
}

void AiChatBridge::requestClose() { emit closeRequested(); }

void AiChatBridge::emitState() {
  QJsonObject state;
  state.insert(QStringLiteral("busy"), viewModel_ ? viewModel_->busy() : false);
  state.insert(QStringLiteral("available"),
               service_ && service_->running());
  state.insert(QStringLiteral("installed"),
               service_ && service_->sidecarAvailable());
  state.insert(QStringLiteral("enabled"), viewModel_ && viewModel_->enabled());
  emit stateChanged(jsonToString(state));
}
