// AiViewModel implementation — see AiViewModel.h for the contract.

#include "AiViewModel.h"

#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {
// Strip the SDK's mcp__owzx__ prefix for display ("set_config_value").
QString prettyToolName(const QString &name) {
  const QString prefix = QStringLiteral("mcp__owzx__");
  return name.startsWith(prefix) ? name.mid(prefix.size()) : name;
}

// ── Presentation helpers ────────────────────────────────────────────────────
// The harness speaks English tool names and JSON payloads; end users are not
// programmers and should never see either (docs/ai-control.md card-style
// chat). These helpers translate registry tool calls into Chinese labels,
// plain-language parameter summaries and consequence sentences.

bool isReadOnlyTool(const QString &name) {
  static const QSet<QString> kReadOnly = {
      QStringLiteral("get_app_state"),    QStringLiteral("get_scene"),
      QStringLiteral("get_plate_info"),   QStringLiteral("get_slice_status"),
      QStringLiteral("get_object_info"), QStringLiteral("list_config_keys")};
  return kReadOnly.contains(name);
}

QString toolLabel(const QString &name) {
  static const QHash<QString, QString> kLabels = {
      {QStringLiteral("get_app_state"), QStringLiteral("查看应用状态")},
      {QStringLiteral("get_scene"), QStringLiteral("查看场景对象")},
      {QStringLiteral("get_plate_info"), QStringLiteral("查看板信息")},
      {QStringLiteral("get_slice_status"), QStringLiteral("查看切片进度")},
      {QStringLiteral("get_object_info"), QStringLiteral("查看对象详情")},
      {QStringLiteral("list_config_keys"), QStringLiteral("查询可调参数")},
      {QStringLiteral("load_model"), QStringLiteral("导入模型")},
      {QStringLiteral("delete_object"), QStringLiteral("删除对象")},
      {QStringLiteral("duplicate_object"), QStringLiteral("复制对象")},
      {QStringLiteral("set_object_transform"), QStringLiteral("调整变换")},
      {QStringLiteral("arrange_objects"), QStringLiteral("自动排布")},
      {QStringLiteral("orient_objects"), QStringLiteral("自动摆向")},
      {QStringLiteral("set_config_value"), QStringLiteral("修改参数")},
      {QStringLiteral("select_preset"), QStringLiteral("切换预设")},
      {QStringLiteral("slice_plate"), QStringLiteral("切片")},
      {QStringLiteral("slice_all_plates"), QStringLiteral("切片全部板")},
      {QStringLiteral("cancel_slice"), QStringLiteral("取消切片")},
      {QStringLiteral("export_gcode"), QStringLiteral("导出 G-code")},
      {QStringLiteral("switch_page"), QStringLiteral("切换页面")},
      {QStringLiteral("select_object"), QStringLiteral("选中对象")},
      {QStringLiteral("toggle_sidebar"), QStringLiteral("收起/展开侧栏")},
      {QStringLiteral("undo"), QStringLiteral("撤销")},
      {QStringLiteral("redo"), QStringLiteral("重做")},
      {QStringLiteral("save_project"), QStringLiteral("保存项目")},
      {QStringLiteral("clear_project"), QStringLiteral("清空工作区")},
  };
  return kLabels.value(name, name);
}

QString pageDisplayName(const QVariantMap &input) {
  const QVariant raw = input.value(QStringLiteral("page"));
  const QString text = raw.toString();
  QString name;
  if (text.compare(QStringLiteral("home"), Qt::CaseInsensitive) == 0)
    name = QStringLiteral("首页");
  else if (text.compare(QStringLiteral("prepare"), Qt::CaseInsensitive) == 0)
    name = QStringLiteral("准备");
  else if (text.compare(QStringLiteral("preview"), Qt::CaseInsensitive) == 0)
    name = QStringLiteral("预览");
  else if (text.compare(QStringLiteral("monitor"), Qt::CaseInsensitive) == 0 ||
           text.compare(QStringLiteral("device"), Qt::CaseInsensitive) == 0)
    name = QStringLiteral("设备");
  else if (text.compare(QStringLiteral("preferences"), Qt::CaseInsensitive) == 0)
    name = QStringLiteral("偏好设置");
  else
    name = QStringLiteral("页面 %1").arg(text);
  return name;
}

// Plain-language summary of the tool's parameters (empty -> QML omits line).
QString toolDetail(const QString &name, const QVariantMap &input) {
  if (name == QLatin1String("load_model")) {
    const QString path = input.value(QStringLiteral("path")).toString();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
  }
  if (name == QLatin1String("delete_object") ||
      name == QLatin1String("duplicate_object") ||
      name == QLatin1String("select_object") ||
      name == QLatin1String("set_object_transform")) {
    const int index = input.value(QStringLiteral("index"), -1).toInt();
    return index >= 0 ? QStringLiteral("第 %1 个对象").arg(index + 1) : QString();
  }
  if (name == QLatin1String("set_config_value")) {
    const QString key = input.value(QStringLiteral("key")).toString();
    if (key.isEmpty())
      return QString();
    return QStringLiteral("%1：%2").arg(
        key, input.value(QStringLiteral("value")).toString());
  }
  if (name == QLatin1String("select_preset")) {
    const QString name2 = input.value(QStringLiteral("name")).toString();
    return name2.isEmpty() ? QString()
                           : QStringLiteral("%1（%2）").arg(
                                 name2, input.value(QStringLiteral("kind")).toString());
  }
  if (name == QLatin1String("export_gcode") ||
      name == QLatin1String("save_project")) {
    const QString path = input.value(QStringLiteral("path")).toString();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
  }
  if (name == QLatin1String("switch_page"))
    return pageDisplayName(input);
  return {};
}

// Consequence sentence for destructive tools (permission card body).
QString riskText(const QString &name, const QVariantMap &input) {
  if (name == QLatin1String("delete_object")) {
    const int index = input.value(QStringLiteral("index"), -1).toInt();
    return index >= 0
               ? QStringLiteral("将从当前板删除第 %1 个对象，可通过撤销恢复。").arg(index + 1)
               : QStringLiteral("将从当前板删除该对象，可通过撤销恢复。");
  }
  if (name == QLatin1String("clear_project"))
    return QStringLiteral("将移除工作区的所有对象与板，且此操作不可撤销。");
  return {};
}

// One-line user-facing result extracted from the tool's JSON payload.
// Returns an empty string when the payload has nothing worth surfacing —
// the status chip alone is cleaner for non-technical users than raw JSON.
QString friendlyResult(bool ok, const QString &summary) {
  if (!ok)
    return summary.left(200);  // failures: surface the reason verbatim
  const QJsonDocument doc = QJsonDocument::fromJson(summary.toUtf8());
  if (!doc.isObject())
    return {};
  const QJsonObject obj = doc.object();
  if (obj.contains(QLatin1String("statusLabel")))
    return obj.value(QLatin1String("statusLabel")).toString();
  if (obj.contains(QLatin1String("appliedValue")))
    return QStringLiteral("已应用：%1 = %2")
        .arg(obj.value(QLatin1String("key")).toString(),
             obj.value(QLatin1String("appliedValue")).toVariant().toString());
  if (obj.contains(QLatin1String("path"))) {
    const QString path = obj.value(QLatin1String("path")).toString();
    if (!path.isEmpty())
      return QFileInfo(path).fileName();
  }
  if (obj.contains(QLatin1String("objectCountAfter")))
    return QStringLiteral("现有 %1 个对象")
        .arg(obj.value(QLatin1String("objectCountAfter")).toInt());
  if (obj.contains(QLatin1String("name")) && obj.contains(QLatin1String("kind")))
    return QStringLiteral("已切换到 %1").arg(obj.value(QLatin1String("name")).toString());
  return {};
}

// Attach the human-facing fields shared by tool cards and permission cards.
QVariantMap decorateToolEntry(QVariantMap entry, const QString &rawName,
                              const QJsonObject &input) {
  const QString name = prettyToolName(rawName);
  const QVariantMap inputMap = input.toVariantMap();
  entry.insert(QStringLiteral("toolName"), name);
  entry.insert(QStringLiteral("toolLabel"), toolLabel(name));
  entry.insert(QStringLiteral("toolDetail"), toolDetail(name, inputMap));
  entry.insert(QStringLiteral("riskText"), riskText(name, inputMap));
  entry.insert(QStringLiteral("readOnly"), isReadOnlyTool(name));
  entry.insert(QStringLiteral("toolInput"), inputMap);
  return entry;
}
}  // namespace

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
            entry = decorateToolEntry(std::move(entry), name, input);
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
                entry.insert(QStringLiteral("toolResult"), friendlyResult(ok, summary));
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
            entry = decorateToolEntry(std::move(entry), tool, input);
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
