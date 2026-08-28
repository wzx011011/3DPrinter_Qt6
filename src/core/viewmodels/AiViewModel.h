#pragma once

// AiViewModel — OWzx-only AI assistant chat state (decision record:
// docs/ai-control.md). Bridges AiAgentService events into a QML-ready
// message list (user bubbles, assistant text, tool cards, permission cards)
// and forwards user actions back (send / cancel / reset / answer permission).
//
// Message entry shape (QVariantMap):
//   kind: "user" | "assistant" | "tool" | "permission" | "error"
//   text: user/assistant/error message text
//   toolName / toolInput / toolOk / toolSummary : raw tool card fields
//   toolLabel / toolDetail / riskText / readOnly : human-facing presentation
//     fields (Chinese action name, plain-language parameter summary,
//     destructive-consequence sentence, read-only query flag) so QML never
//     shows raw English tool names or JSON to end users
//   toolResult : one-line human-friendly result extracted from the tool's
//     JSON payload (empty when nothing user-relevant to show)
//   callId / pending / granted : permission card fields
// QML only renders this state; all logic lives here and in the services.

#include <QJsonObject>
#include <QObject>
#include <QVariantList>

#include "core/ai/AiAgentService.h"

class AiAgentService;

class AiViewModel final : public QObject {
  Q_OBJECT
  Q_PROPERTY(QVariantList messages READ messages NOTIFY messagesChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY messagesChanged)
  Q_PROPERTY(bool available READ available NOTIFY messagesChanged)
  Q_PROPERTY(bool enabled READ enabled NOTIFY messagesChanged)
  Q_PROPERTY(bool installed READ installed NOTIFY messagesChanged)

 public:
  explicit AiViewModel(AiAgentService *service, QObject *parent = nullptr);

  QVariantList messages() const;
  bool busy() const;
  bool available() const;  // sidecar installed + service running + ready
  bool enabled() const { return enabled_; }
  bool installed() const;  // optional ai_sidecar package present on disk
  void setEnabled(bool enabled);  // host calls on preferences change

  Q_INVOKABLE bool sendMessage(const QString &text);
  Q_INVOKABLE void cancelTurn();
  Q_INVOKABLE void clearHistory();
  Q_INVOKABLE void answerPermission(int callId, bool allow);

 signals:
  void messagesChanged();

 private:
  void appendEntry(const QVariantMap &entry);

  AiAgentService *service_ = nullptr;
  QVariantList messages_;
  bool enabled_ = false;
};
