#include "core/services/PluginOverrides.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace PluginOverrides
{
namespace
{
constexpr auto kPlugin = "plugin_key";
constexpr auto kCapability = "capability";
constexpr auto kType = "capability_type";
constexpr auto kCapConfig = "cap_config";
} // namespace

bool parse(const QString &raw, Document &document, QString &error)
{
  document = Document();
  error.clear();
  if (raw.trimmed().isEmpty())
    return true;

  QJsonParseError parseError;
  const QJsonDocument json = QJsonDocument::fromJson(raw.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError)
  {
    error = QStringLiteral("预设中保存的插件能力配置不是有效的 JSON。");
    return false;
  }
  if (!json.isArray())
  {
    error = QStringLiteral("预设中的插件能力配置不是数组，无法编辑。");
    return false;
  }
  for (const QJsonValue &value : json.array())
    document.entries.append(value.toObject());
  return true;
}

QString serialize(const Document &document)
{
  if (document.entries.isEmpty())
    return QString();
  QJsonArray array;
  for (const QJsonObject &entry : document.entries)
    array.append(entry);
  return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

int count(const QString &raw)
{
  Document document;
  QString error;
  return parse(raw, document, error) ? int(document.entries.size()) : 0;
}

QString pluginKey(const QJsonObject &entry) { return entry.value(QLatin1String(kPlugin)).toString(); }
QString capability(const QJsonObject &entry) { return entry.value(QLatin1String(kCapability)).toString(); }
QString capabilityType(const QJsonObject &entry) { return entry.value(QLatin1String(kType)).toString(); }
QJsonObject capConfig(const QJsonObject &entry) { return entry.value(QLatin1String(kCapConfig)).toObject(); }

bool setCapConfig(Document &document, int index, const QString &configText, QString &error)
{
  error.clear();
  if (index < 0 || index >= document.entries.size())
    return false;
  QJsonParseError parseError;
  const QJsonDocument json = QJsonDocument::fromJson(configText.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !json.isObject())
  {
    // Upstream PluginsConfigDialog: "The configuration is not valid JSON.
    // Your changes were not saved."
    error = QStringLiteral("配置不是有效的 JSON，修改未保存。");
    return false;
  }
  document.entries[index].insert(QLatin1String(kCapConfig), json.object());
  return true;
}

bool remove(Document &document, int index)
{
  if (index < 0 || index >= document.entries.size())
    return false;
  document.entries.removeAt(index);
  return true;
}
} // namespace PluginOverrides
