// AiAgentService implementation — sidecar process host + NDJSON bridge.
// See AiAgentService.h for the protocol contract.

#include "AiAgentService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QTimer>

AiAgentService::AiAgentService(QObject *parent) : QObject(parent) {}

AiAgentService::~AiAgentService()
{
  // U13: the async stop() hands draining sidecars to closingProcesses_ with
  // a 3s kill timer. Timers need a live event loop, which the destructor
  // (typically app shutdown) cannot guarantee -- kill synchronously here.
  // The QProcess objects are children, so ~QObject reclaims them.
  for (QProcess *closing : std::as_const(closingProcesses_)) {
    closing->disconnect();
    closing->kill();
  }
}

void AiAgentService::configure(const Config &config) {
  config_ = config;
}

bool AiAgentService::sidecarAvailable() const {
  return QFileInfo::exists(config_.sidecarScriptPath)
      && QFileInfo::exists(config_.pythonPath);
}

bool AiAgentService::start() {
  if (running())
    return true;
  if (!sidecarAvailable()) {
    emit chatError(QStringLiteral("AI 组件未安装（缺少 ai_sidecar）"));
    return false;
  }
  if (config_.apiKey.isEmpty()) {
    emit chatError(QStringLiteral("未配置 AI API Key，请到偏好设置填写"));
    return false;
  }

  stop();

  process_ = new QProcess(this);
  process_->setProgram(config_.pythonPath);
  process_->setArguments({config_.sidecarScriptPath});
  process_->setWorkingDirectory(QFileInfo(config_.sidecarScriptPath).absolutePath());

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
  env.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
  // The sidecar talks to the loopback MCP server; on machines that export
  // HTTP(S)_PROXY without NO_PROXY, proxy-honoring HTTP clients would route
  // those requests through the proxy and time out.
  env.insert(QStringLiteral("NO_PROXY"),
             QStringLiteral("127.0.0.1,localhost,::1"));
  env.insert(QStringLiteral("no_proxy"),
             QStringLiteral("127.0.0.1,localhost,::1"));
  env.insert(QStringLiteral("OWZX_MCP_URL"), config_.mcpUrl);
  env.insert(QStringLiteral("OWZX_MCP_TOKEN"), config_.mcpToken);
  env.insert(QStringLiteral("OWZX_MODEL"), config_.model);
  env.insert(QStringLiteral("ANTHROPIC_BASE_URL"), config_.anthropicBaseUrl);
  env.insert(QStringLiteral("ANTHROPIC_AUTH_TOKEN"), config_.apiKey);
  // Do not let the bundled CLI read the machine's Claude settings.
  env.remove(QStringLiteral("CLAUDE_CODE_ENTRYPOINT"));
  process_->setProcessEnvironment(env);

  connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
    // NDJSON: split on newlines, keep the partial tail buffered.
    buffer_ += QString::fromUtf8(process_->readAllStandardOutput());
    int nl = buffer_.indexOf(QLatin1Char('\n'));
    while (nl >= 0) {
      const QString line = buffer_.left(nl).trimmed();
      buffer_ = buffer_.mid(nl + 1);
      if (!line.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (doc.isObject())
          handleEvent(doc.object());
      }
      nl = buffer_.indexOf(QLatin1Char('\n'));
    }
  });
  connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::Crashed)
      scheduleRestart();
    else
      emit chatError(QStringLiteral("AI 进程错误：%1").arg(int(error)));
  });
  connect(process_, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
    Q_UNUSED(status)
    const bool wasReady = ready_;
    ready_ = false;
    busy_ = false;
    emit stateChanged();
    if (wasReady && exitCode != 0)
      scheduleRestart();
  });

  process_->start();
  const bool started = process_->waitForStarted(5000);
  if (!started) {
    emit chatError(QStringLiteral("AI 组件启动失败"));
    process_->deleteLater();
    process_ = nullptr;
    return false;
  }
  emit stateChanged();
  return true;
}

void AiAgentService::stop() {
  if (!process_)
    return;
  // U13: signal-driven close. The old handshake froze the GUI thread for up
  // to 4s (waitForBytesWritten(1000) + waitForFinished(3000)). The service
  // state now flips to stopped immediately; the draining sidecar keeps only
  // self-contained connections (kill timer + deleteLater), so no signal can
  // re-enter the service after the disconnect below.
  QProcess *closing = process_;
  process_ = nullptr;
  ready_ = false;
  busy_ = false;
  buffer_.clear();
  closing->disconnect(this);
  if (closing->state() != QProcess::NotRunning) {
    closingProcesses_.insert(closing);
    // Graceful handshake, just without the blocking waits: the shutdown
    // line flushes on the event loop, stdin EOF covers sidecars that never
    // parse it, terminate() asks again, and the child QTimer escalates to
    // kill() after the same 3s grace the old waitForFinished(3000) used.
    closing->write("{\"type\":\"shutdown\"}\n");
    closing->closeWriteChannel();
    closing->terminate();
    auto *killTimer = new QTimer(closing);
    killTimer->setSingleShot(true);
    connect(killTimer, &QTimer::timeout, closing, &QProcess::kill);
    connect(closing, &QProcess::finished, this, [this, closing, killTimer]() {
      closingProcesses_.remove(closing);
      killTimer->stop();
    });
    connect(closing, &QProcess::finished, closing, &QObject::deleteLater);
    killTimer->start(3000);
  } else {
    closing->deleteLater();
  }
  emit stateChanged();
}

bool AiAgentService::sendMessage(const QString &text) {
  if (!running() || !ready_) {
    emit chatError(QStringLiteral("AI 助手未就绪"));
    return false;
  }
  if (busy_)
    return false;
  QJsonObject cmd;
  cmd.insert(QStringLiteral("type"), QStringLiteral("send"));
  cmd.insert(QStringLiteral("text"), text);
  process_->write(QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n");
  busy_ = true;
  emit stateChanged();
  return true;
}

void AiAgentService::cancelTurn() {
  if (running() && busy_) {
    QJsonObject cmd;
    cmd.insert(QStringLiteral("type"), QStringLiteral("cancel"));
    process_->write(QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n");
  }
}

void AiAgentService::resetConversation() {
  if (!running())
    return;
  QJsonObject cmd;
  cmd.insert(QStringLiteral("type"), QStringLiteral("reset"));
  process_->write(QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n");
}

void AiAgentService::answerPermission(int callId, bool allow) {
  if (!running())
    return;
  QJsonObject cmd;
  cmd.insert(QStringLiteral("type"), QStringLiteral("answer_permission"));
  cmd.insert(QStringLiteral("callId"), callId);
  cmd.insert(QStringLiteral("allow"), allow);
  process_->write(QJsonDocument(cmd).toJson(QJsonDocument::Compact) + "\n");
}

void AiAgentService::handleEvent(const QJsonObject &event) {
  const QString type = event.value(QStringLiteral("type")).toString();
  if (type == QStringLiteral("ready")) {
    ready_ = true;
    restartCount_ = 0;
    emit sidecarReady(event.value(QStringLiteral("sdk")).toString());
    emit stateChanged();
  } else if (type == QStringLiteral("assistant_text")) {
    emit assistantTextChanged(event.value(QStringLiteral("text")).toString());
  } else if (type == QStringLiteral("tool_use")) {
    emit toolUseStarted(event.value(QStringLiteral("id")).toString(),
                        event.value(QStringLiteral("name")).toString(),
                        event.value(QStringLiteral("input")).toObject());
  } else if (type == QStringLiteral("tool_result")) {
    emit toolUseFinished(event.value(QStringLiteral("id")).toString(),
                         event.value(QStringLiteral("ok")).toBool(true),
                         event.value(QStringLiteral("summary")).toString());
  } else if (type == QStringLiteral("permission_request")) {
    emit permissionRequested(event.value(QStringLiteral("callId")).toInt(),
                             event.value(QStringLiteral("tool")).toString(),
                             event.value(QStringLiteral("input")).toObject());
  } else if (type == QStringLiteral("turn_done")) {
    busy_ = false;
    emit stateChanged();
    emit turnFinished(event.value(QStringLiteral("isError")).toBool(false),
                      event.value(QStringLiteral("durationMs")).toInt(),
                      event.value(QStringLiteral("sessionId")).toString());
  } else if (type == QStringLiteral("error")) {
    emit chatError(event.value(QStringLiteral("message")).toString());
  }
}

void AiAgentService::scheduleRestart() {
  if (restartCount_ >= 3) {
    emit chatError(QStringLiteral("AI 组件多次异常退出，已停止自动重启"));
    return;
  }
  ++restartCount_;
  process_->deleteLater();
  process_ = nullptr;
  ready_ = false;
  busy_ = false;
  emit stateChanged();
  // Re-spawn on the next event-loop tick (give the OS a moment to release
  // the executable lock).
  QMetaObject::invokeMethod(
      this, [this]() { start(); }, Qt::QueuedConnection);
}
