#pragma once

// AiChatBridge — QWebChannel object exposed to the WebEngine chat page
// (decision record: docs/ai-control.md step 2: web chat UI). The page keeps
// its own DOM state; the bridge emits granular JSON events (append/update)
// built from the same decorated fields the QML sidebar renders
// (AiMessageDecorate), and forwards user actions — send / answer permission /
// cancel / clear / close — back into AiViewModel and the host QML shell.
//
// Security: only these five invokables cross the JS boundary; the page is
// local qrc content (trusted), but nothing broader (no file/shell/config
// access) is reachable from it.

#include <QObject>

class AiAgentService;
class AiViewModel;

class AiChatBridge final : public QObject {
  Q_OBJECT

 public:
  explicit AiChatBridge(AiViewModel *viewModel, AiAgentService *service,
                        QObject *parent = nullptr);

  // Page lifecycle: replay the current transcript + state after (re)connect
  // so reopening the sidebar restores history without a sidecar round trip.
  Q_INVOKABLE void pageReady();
  Q_INVOKABLE void sendMessage(const QString &text);
  Q_INVOKABLE void answerPermission(int callId, bool allow);
  Q_INVOKABLE void cancelTurn();
  Q_INVOKABLE void clearHistory();
  Q_INVOKABLE void requestClose();

 signals:
  // Page bootstrap: full transcript as a JSON array of message entries.
  void historyReplayed(const QString &messagesJson);
  // Current session state: {"busy":bool,"available":bool,"enabled":bool}.
  void stateChanged(const QString &stateJson);

  // Live events (entryJson = one decorated message entry).
  void assistantTextAppended(const QString &text);
  void toolStarted(const QString &id, const QString &entryJson);
  void toolFinished(const QString &id, bool ok, const QString &resultText,
                    const QString &summaryText);
  void permissionRequested(int callId, const QString &entryJson);
  void permissionResolved(int callId, bool granted);
  void historyCleared();
  void errorOccurred(const QString &text);

  // Shell actions (QML side connects: close collapses the sidebar panel).
  void closeRequested();

 private:
  void emitState();

  AiViewModel *viewModel_ = nullptr;
  AiAgentService *service_ = nullptr;
};
