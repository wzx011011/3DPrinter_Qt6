#pragma once

// AiAgentService — OWzx-only AI harness host (decision record:
// docs/ai-control.md). Owns the Python sidecar process (tools/ai_sidecar/
// agent.py running the Claude Agent SDK) and bridges its NDJSON stdio
// protocol to Qt signals for the chat sidebar (AiViewModel).
//
// Lifecycle: configure(...) -> start() spawns the sidecar -> "ready" event
// marks the harness connected. Turns are driven by sendMessage(); every event
// arrives as a Qt signal on the GUI thread. The sidecar talks to GLM via
// Zhipu's Anthropic-compatible endpoint (env-injected) and drives the app
// through the in-app MCP server (McpHttpServer + AppToolRegistry).
//
// Destructive-tool confirmations flow through permissionRequested() ->
// answerPermission() (the UI shows a card and the user decides).

#include <QJsonObject>
#include <QObject>
#include <QProcess>

class QProcessEnvironment;

class AiAgentService final : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool running READ running NOTIFY stateChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)

 public:
  explicit AiAgentService(QObject *parent = nullptr);

  struct Config {
    QString sidecarScriptPath;  // <appDir>/ai_sidecar/agent.py
    QString pythonPath;         // <appDir>/ai_sidecar/python/python.exe
    QString mcpUrl;             // http://127.0.0.1:<port>/mcp
    QString mcpToken;
    QString model = QStringLiteral("glm-5.3-flash");
    QString anthropicBaseUrl;   // https://open.bigmodel.cn/api/anthropic
    QString apiKey;             // GLM key (never logged)
  };

  void configure(const Config &config);
  bool sidecarAvailable() const;  // script + python exist on disk

  bool running() const { return process_ && process_->state() != QProcess::NotRunning; }
  bool busy() const { return busy_; }

 public slots:
  bool start();
  void stop();
  bool sendMessage(const QString &text);
  void cancelTurn();
  void resetConversation();
  void answerPermission(int callId, bool allow);

 signals:
  void stateChanged();
  void sidecarReady(const QString &sdkVersion);
  void assistantTextChanged(const QString &text);
  void toolUseStarted(const QString &id, const QString &name, const QJsonObject &input);
  void toolUseFinished(const QString &id, bool ok, const QString &summary);
  void permissionRequested(int callId, const QString &tool, const QJsonObject &input);
  void turnFinished(bool isError, int durationMs, const QString &sessionId);
  void chatError(const QString &message);

 private:
  void handleEvent(const QJsonObject &event);
  void scheduleRestart();  // crash auto-restart (max 3, then chatError)

  Config config_;
  QProcess *process_ = nullptr;
  QString buffer_;  // partial NDJSON line from the sidecar's stdout
  bool busy_ = false;
  bool ready_ = false;
  int restartCount_ = 0;
};
