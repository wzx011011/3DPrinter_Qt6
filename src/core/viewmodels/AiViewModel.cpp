// AiViewModel implementation — see AiViewModel.h for the contract.

#include "AiViewModel.h"

#include "AiMessageDecorate.h"


AiViewModel::AiViewModel(AiAgentService *service, QObject *parent)
    : QObject(parent), service_(service) {
  connect(service_, &AiAgentService::sidecarReady, this, [this](const QString &sdk) {
    QVariantMap entry;
    entry.insert(QStringLiteral("kind"), QStringLiteral("assistant"));
    entry.insert(QStringLiteral("text"),
                 QStringLiteral("AI 助手已就绪（harness %1）").arg(sdk));
    appendEntry(entry);
  });
  connect(service_, &AiAgentService::assistantTextChanged, this,
          [this](const QString &text) {
            QVariantMap entry;
            entry.insert(QStringLiteral("kind"), QStringLiteral("assistant"));
            entry.insert(QStringLiteral("text"), text);
            appendEntry(entry);
          });
  connect(service_, &AiAgentService::toolUseStarted, this,
          [this](const QString &id, const QString &name, const QJsonObject &input) {
            QVariantMap entry;
            entry.insert(QStringLiteral("kind"), QStringLiteral("tool"));
            entry.insert(QStringLiteral("id"), id);
            entry = OWzx::AiMessageDecorate::decorateToolEntry(std::move(entry), name, input);
            appendEntry(entry);
          });
  connect(service_, &AiAgentService::toolUseFinished, this,
          [this](const QString &id, bool ok, const QString &summary) {
            // Update the matching tool card in place.
            for (int i = messages_.size() - 1; i >= 0; --i) {
              QVariantMap entry = messages_.at(i).toMap();
              if (entry.value(QStringLiteral("kind")).toString() == QLatin1String("tool")
                  && entry.value(QStringLiteral("id")).toString() == id) {
                entry.insert(QStringLiteral("toolOk"), ok);
                entry.insert(QStringLiteral("toolSummary"), summary.left(200));
                entry.insert(QStringLiteral("toolResult"), OWzx::AiMessageDecorate::friendlyResult(ok, summary));
                messages_[i] = entry;
                break;
              }
            }
            emit messagesChanged();
          });
  connect(service_, &AiAgentService::permissionRequested, this,
          [this](int callId, const QString &tool, const QJsonObject &input) {
            QVariantMap entry;
            entry.insert(QStringLiteral("kind"), QStringLiteral("permission"));
            entry.insert(QStringLiteral("callId"), callId);
            entry.insert(QStringLiteral("pending"), true);
            entry = OWzx::AiMessageDecorate::decorateToolEntry(std::move(entry), tool, input);
            appendEntry(entry);
          });
  connect(service_, &AiAgentService::turnFinished, this,
          [this](bool isError, int durationMs, const QString &) {
            if (isError) {
              QVariantMap entry;
              entry.insert(QStringLiteral("kind"), QStringLiteral("error"));
              entry.insert(QStringLiteral("text"),
                           QStringLiteral("本轮对话异常结束（%1 ms），可重试或查看日志")
                               .arg(durationMs));
              appendEntry(entry);
            }
            emit messagesChanged();  // busy flag flip
          });
  connect(service_, &AiAgentService::chatError, this, [this](const QString &message) {
    QVariantMap entry;
    entry.insert(QStringLiteral("kind"), QStringLiteral("error"));
    entry.insert(QStringLiteral("text"), message);
    appendEntry(entry);
  });
  connect(service_, &AiAgentService::stateChanged, this, &AiViewModel::messagesChanged);
}

QVariantList AiViewModel::messages() const { return messages_; }

bool AiViewModel::busy() const { return service_ && service_->busy(); }

bool AiViewModel::available() const {
  return service_ && service_->running();
}

bool AiViewModel::installed() const {
  return service_ && service_->sidecarAvailable();
}

void AiViewModel::setEnabled(bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  emit messagesChanged();
}

bool AiViewModel::sendMessage(const QString &text) {
  const QString trimmed = text.trimmed();
  if (trimmed.isEmpty())
    return false;
  QVariantMap entry;
  entry.insert(QStringLiteral("kind"), QStringLiteral("user"));
  entry.insert(QStringLiteral("text"), trimmed);
  appendEntry(entry);
  return service_->sendMessage(trimmed);
}

void AiViewModel::cancelTurn() { service_->cancelTurn(); }

void AiViewModel::clearHistory() {
  messages_.clear();
  if (service_ && service_->running())
    service_->resetConversation();
  emit messagesChanged();
}

void AiViewModel::answerPermission(int callId, bool allow) {
  // Resolve the pending card in place.
  for (int i = messages_.size() - 1; i >= 0; --i) {
    QVariantMap entry = messages_.at(i).toMap();
    if (entry.value(QStringLiteral("kind")).toString() == QLatin1String("permission")
        && entry.value(QStringLiteral("callId")).toInt() == callId
        && entry.value(QStringLiteral("pending")).toBool()) {
      entry.insert(QStringLiteral("pending"), false);
      entry.insert(QStringLiteral("granted"), allow);
      messages_[i] = entry;
      break;
    }
  }
  emit messagesChanged();
  service_->answerPermission(callId, allow);
}

void AiViewModel::appendEntry(const QVariantMap &entry) {
  messages_.append(entry);
  // Keep the transcript bounded (the harness session holds the real context;
  // this list is presentation-only).
  while (messages_.size() > 200)
    messages_.removeFirst();
  emit messagesChanged();
}
