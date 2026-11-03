#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

// PluginOverrides - the per-preset plugin capability override document
// stored in print/filament/printer_plugin_config_overrides (upstream
// slic3r/plugin/PluginConfig: parse_plugin_overrides /
// serialize_plugin_overrides / CapabilityConfigDocument).
//
// The value is a JSON array of entries
//   {"plugin_key", "capability", "capability_type", "plugin_version",
//    "cap_config": {...}}
// stored as compact text, "" when no override remains. Entries that do not
// name a plugin + capability are kept verbatim (upstream m_opaque_entries).
// Unparseable text is never rewritten: callers show it read-only.
namespace PluginOverrides
{
struct Document
{
  QList<QJsonObject> entries;
};

/// Upstream parse_plugin_overrides: "" is an empty document; anything other
/// than a JSON array fails with an error message.
bool parse(const QString &raw, Document &document, QString &error);

/// Upstream serialize_plugin_overrides: compact JSON array, "" when empty.
QString serialize(const Document &document);

/// Override count shown on the field button ("配置 (N)"); 0 for invalid text.
int count(const QString &raw);

/// Entry fields for presentation.
QString pluginKey(const QJsonObject &entry);
QString capability(const QJsonObject &entry);
QString capabilityType(const QJsonObject &entry);
QJsonObject capConfig(const QJsonObject &entry);

/// Replaces cap_config of entry `index` with `configText` (a JSON object).
/// Returns false with `error` set when the text is not a JSON object.
bool setCapConfig(Document &document, int index, const QString &configText, QString &error);

/// Upstream remove_preset_override: drops the entry, the capability falls
/// back to the global configuration.
bool remove(Document &document, int index);
} // namespace PluginOverrides
