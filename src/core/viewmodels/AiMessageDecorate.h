#pragma once

// AiMessageDecorate — shared presentation decoration for AI chat surfaces
// (decision record: docs/ai-control.md). The harness speaks English tool
// names and JSON payloads; end users are not programmers and should never
// see either. These helpers translate registry tool calls into Chinese
// labels, plain-language parameter summaries, consequence sentences and a
// friendly one-line result so both the QML sidebar and the WebEngine chat
// page (QWebChannel bridge) render identical human-facing cards.

#include <QJsonObject>
#include <QVariantMap>

namespace OWzx::AiMessageDecorate {

// Strip the SDK's mcp__owzx__ prefix ("mcp__owzx__load_model" -> "load_model").
QString prettyToolName(const QString &name);

// Chinese action label per registry tool (falls back to the raw name).
QString toolLabel(const QString &name);

// Plain-language parameter summary; empty when there is nothing worth a line
// (file name for load/export, key=value for config, "第 N 个对象", page name).
QString toolDetail(const QString &name, const QVariantMap &input);

// Consequence sentence for destructive tools (permission card body); empty
// for non-destructive tools.
QString riskText(const QString &name, const QVariantMap &input);

// True for the six read-only query tools (rendered as quiet strips).
bool isReadOnlyTool(const QString &name);

// One-line user-facing result extracted from the tool's JSON payload; empty
// when the payload has nothing worth surfacing. Failures pass the reason
// through verbatim.
QString friendlyResult(bool ok, const QString &summary);

// Attach every human-facing field to a message entry map (adds toolName,
// toolLabel, toolDetail, riskText, readOnly, toolInput).
QVariantMap decorateToolEntry(QVariantMap entry, const QString &rawName,
                              const QJsonObject &input);

}  // namespace OWzx::AiMessageDecorate
