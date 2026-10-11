#include "PluginService.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QUrl>

// Phase 202 (v5.6) -- PluginService implementation.
//
// Data source remains mock. The registry rows carry the upstream
// PluginsDialog row schema (source variant, installed/latest version,
// update status, types, changelog, capability subtree with JSON configs)
// and every mutation reports its result through statusMessage() so the
// dialog's status bar (upstream status_message command) can render it.
// There is NO real HTTP download, NO Python / CPython, and NO plugin
// archive extraction. Q10-2: installPlugin()/updatePlugin() therefore
// report "not implemented" honestly instead of flipping a persisted
// installed flag that no package backs; uninstallPlugin() only clears the
// persisted state of catalog rows. Per-plugin state persists under
// "plugins/state" keyed by the row's stable id (localPath) -- see the v3
// note beside kSettingsVersion.
//
// Upstream reference: OrcaSlicer PluginsDialog (build_plugin_dialog_item /
// evaluate_action_policy in PluginsDialog.cpp). We mirror the user-facing
// semantics (enable/disable, refresh, context actions) without the web
// view / real-network layer.

namespace {
// QSettings key prefix for every persisted plugin value.
constexpr const char *kSettingsPrefix = "plugins";
// Sentinel version key; bump when the persisted shape changes.
constexpr const char *kSettingsVersionKey = "plugins/version";
// v2: added per-capability enabled flags + capability config overrides.
// v3 (Q10-2): per-plugin state is keyed by the row's STABLE id (localPath --
// the vendor package identity: unique, untranslated, immutable across app
// versions) instead of the catalog array position. The legacy parallel lists
// silently re-attached every plugin's state to a DIFFERENT row whenever the
// catalog reordered or gained/lost an entry across versions.
constexpr int kSettingsVersion = 3;
// Legacy v1/v2 parallel-list keys, removed after the one-time migration.
constexpr const char *kLegacyKeys[] = {
    "enabled", "installed", "installed_versions", "update", "caps", "configs"};

// Q10-2: single source of the built-in per-capability default configs. Both
// initDefaults() (catalog seeding) and restoreCapabilityConfig() (the restore
// action) read this table -- the literals previously lived at both sites and
// could drift.
QString builtinCapabilityConfig(const QString &capabilityName)
{
  // NOTE: compare with QStringLiteral -- QLatin1String cannot represent
  // non-ASCII capability names (they would mojibake and never match).
  if (capabilityName == QStringLiteral("设备状态推送"))
    return QStringLiteral("{\n  \"poll_interval\": 5\n}");
  if (capabilityName == QStringLiteral("远程监控"))
    return QStringLiteral("{\n  \"snapshot_quality\": 80\n}");
  return QString();
}

QString settingsKey(const char *leaf)
{
  return QStringLiteral("%1/%2").arg(QLatin1String(kSettingsPrefix), QLatin1String(leaf));
}
}  // namespace

PluginService::PluginService(QObject *parent)
  : QObject(parent)
{
  initDefaults();
  loadFromSettings();
}

PluginService::~PluginService() = default;

void PluginService::initDefaults()
{
  // The 3 default mock entries mirror the pre-Phase-202
  // PluginManagerDialog.qml literal set, enriched with the upstream
  // PluginsDialog row fields. Display strings go through tr() to preserve
  // qsTr() translatability.
  m_plugins.clear();

  PluginEntry networking;
  networking.name = tr("网络通信插件");
  networking.version = QStringLiteral("1.2.0");
  networking.description = tr("Bambu Lab 打印机网络通信支持");
  networking.author = QStringLiteral("OWzx");
  // Local vendor plugin (upstream derive_plugin_source default branch).
  networking.source = QStringLiteral("local");
  networking.types = QStringLiteral("[脚本]");
  networking.latestVersion = QStringLiteral("1.3.0");
  networking.installedVersion = QStringLiteral("1.2.0");
  networking.updateStatus = QStringLiteral("update_available");
  networking.downloadUrl = QStringLiteral("https://plugins.owzx.example/network_plugin.zip");
  networking.localPath = QStringLiteral("plugins/network_plugin");
  networking.isInstalled = true;
  networking.isEnabled = true;

  // Capability subtree (upstream item.capabilities): one enabled, one
  // disabled, so the row exercises the mixed/indeterminate check state.
  PluginCapability push;
  push.name = QStringLiteral("设备状态推送");
  push.type = tr("脚本");
  push.typeKey = QStringLiteral("script");
  push.enabled = true;
  push.canRun = true;
  push.hasConfig = true;
  // Q10-2: built-in defaults come from the shared table (single source with
  // restoreCapabilityConfig).
  push.defaultConfig = builtinCapabilityConfig(push.name);
  networking.capabilities.append(push);

  PluginCapability monitor;
  monitor.name = QStringLiteral("远程监控");
  monitor.type = tr("脚本");
  monitor.typeKey = QStringLiteral("script");
  monitor.enabled = false;
  monitor.hasConfig = true;
  monitor.defaultConfig = builtinCapabilityConfig(monitor.name);
  networking.capabilities.append(monitor);

  PluginChangelogEntry net13;
  net13.version = QStringLiteral("1.3.0");
  net13.date = QStringLiteral("2026-08-12");
  net13.changes = tr("新增打印机状态推送与远程监控能力。");
  networking.changelog.append(net13);
  PluginChangelogEntry net12;
  net12.version = QStringLiteral("1.2.0");
  net12.date = QStringLiteral("2026-05-30");
  net12.changes = tr("初始发布。");
  networking.changelog.append(net12);
  m_plugins.append(networking);

  PluginEntry support;
  support.name = tr("高级支撑生成器");
  support.version = QStringLiteral("2.0.1");
  support.description = tr("基于树形结构的智能支撑生成");
  support.author = QStringLiteral("OWzx");
  support.source = QStringLiteral("local");
  support.types = QStringLiteral("-");
  support.latestVersion = QStringLiteral("2.0.1");
  support.downloadUrl = QStringLiteral("https://plugins.owzx.example/tree_support.zip");
  support.localPath = QStringLiteral("plugins/tree_support");

  PluginChangelogEntry sup201;
  sup201.version = QStringLiteral("2.0.1");
  sup201.date = QStringLiteral("2026-07-04");
  sup201.changes = tr("修复树形支撑与悬空面的接触判定。");
  support.changelog.append(sup201);
  m_plugins.append(support);

  PluginEntry ai;
  ai.name = tr("AI 切片优化");
  ai.version = QStringLiteral("0.9.0");
  ai.description = tr("基于 AI 模型的切片参数自动优化");
  ai.author = QStringLiteral("OWzx");
  ai.source = QStringLiteral("local");
  ai.types = QStringLiteral("-");
  ai.latestVersion = QStringLiteral("0.9.0");
  ai.downloadUrl = QStringLiteral("https://plugins.owzx.example/ai_slice.zip");
  ai.localPath = QStringLiteral("plugins/ai_slice");

  PluginChangelogEntry ai09;
  ai09.version = QStringLiteral("0.9.0");
  ai09.date = QStringLiteral("2026-06-18");
  ai09.changes = tr("实验性 AI 参数优化模型。");
  ai.changelog.append(ai09);
  m_plugins.append(ai);
}

void PluginService::loadFromSettings()
{
  QSettings settings;
  const int version = settings.value(QLatin1String(kSettingsVersionKey), 0).toInt();

  if (version >= 1 && version < kSettingsVersion) {
    // ── Q10-2 one-time migration: legacy parallel index lists → id-keyed map ──
    // The legacy shape (v1/v2) stored enabled/installed/versions/update/caps/
    // configs as parallel QVariantLists whose element i belonged to catalog
    // row i. Index is the ONLY mapping the legacy format carries, so the
    // migration reads it as "index i → catalog row i" (exact while the static
    // catalog holds) and immediately re-keys the state by stable id. From
    // here on, catalog reorders / additions / removals across app versions
    // can no longer re-attach one plugin's state to another row: unknown ids
    // are ignored on load, new ids start from catalog defaults.
    const auto legacyList = [&settings](const char *leaf) {
      return settings.value(settingsKey(leaf)).toList();
    };
    const QVariantList legacyEnabled = legacyList("enabled");
    const QVariantList legacyInstalled = legacyList("installed");
    const QVariantList legacyVersions = legacyList("installed_versions");
    const QVariantList legacyUpdate = legacyList("update");

    if (!legacyEnabled.isEmpty()) {
      for (int i = 0; i < m_plugins.size() && i < legacyEnabled.size(); ++i)
        m_plugins[i].isEnabled = legacyEnabled[i].toBool();
    }
    if (!legacyInstalled.isEmpty()) {
      for (int i = 0; i < m_plugins.size() && i < legacyInstalled.size(); ++i)
        m_plugins[i].isInstalled = legacyInstalled[i].toBool();
    }
    if (!legacyVersions.isEmpty()) {
      for (int i = 0; i < m_plugins.size() && i < legacyVersions.size(); ++i)
        m_plugins[i].installedVersion = legacyVersions[i].toString();
    } else {
      for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].installedVersion =
            m_plugins[i].isInstalled ? m_plugins[i].version : QString();
    }
    if (!legacyUpdate.isEmpty()) {
      for (int i = 0; i < m_plugins.size() && i < legacyUpdate.size(); ++i)
        m_plugins[i].updateStatus = legacyUpdate[i].toString();
    }
    // The legacy caps/configs keys are NOT migrated: they were written as
    // NESTED QVariantLists, which the QSettings registry backend cannot
    // round-trip -- on read they arrive flattened to per-char/string
    // fragments (QVariant::toList() on a QString element yields one entry
    // per CHARACTER, so v2 "restored" JSON configs as garbage single chars
    // at best). Those keys carried no recoverable state; the v3 JSON map
    // (single value under one key) removes the failure mode entirely, and
    // the capabilities keep their catalog defaults here.

    saveToSettings();  // re-key by stable id (writes plugins/state + version 3)
    for (const char *leaf : kLegacyKeys)
      settings.remove(settingsKey(leaf));
    settings.setValue(QLatin1String(kSettingsVersionKey), kSettingsVersion);
    settings.sync();
    return;
  }

  if (version < kSettingsVersion) {
    // Fresh install (no persisted shape at all): keep catalog defaults,
    // persist a baseline so later edits write back cleanly.
    saveToSettings();
    return;
  }

  // v3: one JSON map under plugins/state keyed by the row's stable id.
  const QVariantMap state =
      QJsonDocument::fromJson(settings.value(settingsKey("state")).toByteArray())
          .object()
          .toVariantMap();
  for (auto &p : m_plugins) {
    const QString id = !p.localPath.isEmpty() ? p.localPath : p.name;
    const QVariantMap entry = state.value(id).toMap();
    if (entry.isEmpty())
      continue;  // Unknown id (new catalog row): keep catalog defaults.
    if (entry.contains(QLatin1String("enabled")))
      p.isEnabled = entry.value(QLatin1String("enabled")).toBool();
    if (entry.contains(QLatin1String("installed")))
      p.isInstalled = entry.value(QLatin1String("installed")).toBool();
    if (entry.contains(QLatin1String("installedVersion"))) {
      p.installedVersion = entry.value(QLatin1String("installedVersion")).toString();
    } else {
      // Upstream prefers the preserved local version, falling back to the
      // catalog one.
      p.installedVersion = p.isInstalled ? p.version : QString();
    }
    if (entry.contains(QLatin1String("updateStatus")))
      p.updateStatus = entry.value(QLatin1String("updateStatus")).toString();

    const QVariantMap caps = entry.value(QLatin1String("caps")).toMap();
    for (auto &cap : p.capabilities) {
      const QVariantMap capState = caps.value(cap.name).toMap();
      if (capState.isEmpty())
        continue;
      if (capState.contains(QLatin1String("enabled")))
        cap.enabled = capState.value(QLatin1String("enabled")).toBool();
      const QString stored = capState.value(QLatin1String("config")).toString();
      if (cap.hasConfig && !stored.isEmpty())
        cap.defaultConfig = stored;
    }
  }
}

void PluginService::saveToSettings() const
{
  QSettings settings;
  settings.setValue(QLatin1String(kSettingsVersionKey), kSettingsVersion);

  // Q10-2: one JSON map keyed by the row's stable id. Reorders / additions /
  // removals of catalog rows can no longer re-attach one plugin's persisted
  // state to a different row.
  QVariantMap state;
  for (const auto &p : m_plugins) {
    const QString id = !p.localPath.isEmpty() ? p.localPath : p.name;
    QVariantMap caps;
    for (const auto &cap : p.capabilities) {
      QVariantMap capState;
      capState[QStringLiteral("enabled")] = cap.enabled;
      // Only capabilities with an editor persist an override; "" = default.
      capState[QStringLiteral("config")] =
          cap.hasConfig ? cap.defaultConfig : QString();
      caps[cap.name] = capState;
    }
    QVariantMap entry;
    entry[QStringLiteral("enabled")] = p.isEnabled;
    entry[QStringLiteral("installed")] = p.isInstalled;
    entry[QStringLiteral("installedVersion")] = p.installedVersion;
    entry[QStringLiteral("updateStatus")] = p.updateStatus;
    entry[QStringLiteral("caps")] = caps;
    state[id] = entry;
  }
  settings.setValue(settingsKey("state"),
                    QJsonDocument(QJsonObject::fromVariantMap(state))
                        .toJson(QJsonDocument::Compact));
}

QVariantMap PluginService::capabilityToMap(const PluginCapability &capability) const
{
  QVariantMap map;
  map[QStringLiteral("name")] = capability.name;
  map[QStringLiteral("type")] = capability.type;
  map[QStringLiteral("typeKey")] = capability.typeKey;
  map[QStringLiteral("enabled")] = capability.enabled;
  map[QStringLiteral("canToggle")] = capability.canToggle;
  map[QStringLiteral("canRun")] = capability.canRun;
  map[QStringLiteral("hasConfig")] = capability.hasConfig;
  return map;
}

QVariantMap PluginService::entryToMap(const PluginEntry &entry) const
{
  QVariantMap map;
  map[QStringLiteral("name")] = entry.name;
  map[QStringLiteral("version")] = entry.version;
  map[QStringLiteral("description")] = entry.description;
  map[QStringLiteral("author")] = entry.author;
  map[QStringLiteral("source")] = entry.source;
  map[QStringLiteral("types")] = entry.types;
  map[QStringLiteral("latestVersion")] = entry.latestVersion;
  map[QStringLiteral("installedVersion")] = entry.installedVersion;
  map[QStringLiteral("updateStatus")] = entry.updateStatus;
  map[QStringLiteral("downloadUrl")] = entry.downloadUrl;
  map[QStringLiteral("localPath")] = entry.localPath;
  map[QStringLiteral("isEnabled")] = entry.isEnabled;
  map[QStringLiteral("isInstalled")] = entry.isInstalled;

  QVariantList caps;
  caps.reserve(entry.capabilities.size());
  for (const auto &cap : entry.capabilities)
    caps.append(capabilityToMap(cap));
  map[QStringLiteral("capabilities")] = caps;

  QVariantList changelog;
  changelog.reserve(entry.changelog.size());
  for (const auto &entryItem : entry.changelog) {
    QVariantMap row;
    row[QStringLiteral("version")] = entryItem.version;
    row[QStringLiteral("date")] = entryItem.date;
    row[QStringLiteral("changes")] = entryItem.changes;
    changelog.append(row);
  }
  map[QStringLiteral("changelog")] = changelog;

  // Status text + stable key the QML table colors by (upstream StatusCell:
  // status-activated/error/loading/inactive variants). The mock registry
  // never reports Loading/Error; the keys are reserved for the real
  // plugin-manager integration.
  QString status;
  QString statusKey;
  if (!entry.isInstalled) {
    status = tr("未安装");
    statusKey = QStringLiteral("inactive");
  } else if (entry.isEnabled) {
    status = tr("已激活");
    statusKey = QStringLiteral("activated");
  } else {
    status = tr("未激活");
    statusKey = QStringLiteral("inactive");
  }
  map[QStringLiteral("status")] = status;
  map[QStringLiteral("statusKey")] = statusKey;
  return map;
}

// ── Property getters ──────────────────────────────────────────

int PluginService::pluginCount() const
{
  return m_plugins.size();
}

QStringList PluginService::pluginNames() const
{
  QStringList names;
  names.reserve(m_plugins.size());
  for (const auto &p : m_plugins)
    names.append(p.name);
  return names;
}

QVariantList PluginService::plugins() const
{
  QVariantList result;
  result.reserve(m_plugins.size());
  for (const auto &p : m_plugins)
    result.append(entryToMap(p));
  return result;
}

QVariantMap PluginService::pluginAt(int idx) const
{
  if (idx < 0 || idx >= m_plugins.size())
    return {};
  return entryToMap(m_plugins[idx]);
}

// ── Per-plugin state ──────────────────────────────────────────

bool PluginService::isPluginEnabled(int idx) const
{
  if (idx < 0 || idx >= m_plugins.size())
    return false;
  return m_plugins[idx].isEnabled;
}

void PluginService::setPluginEnabled(int idx, bool enabled)
{
  if (idx < 0 || idx >= m_plugins.size())
    return;
  // A not-installed plugin cannot be enabled.
  if (!m_plugins[idx].isInstalled)
    enabled = false;
  if (m_plugins[idx].isEnabled == enabled)
    return;
  m_plugins[idx].isEnabled = enabled;
  saveToSettings();
  emit stateChanged();
  emit statusMessage(
      enabled ? tr("已启用 %1").arg(m_plugins[idx].name)
              : tr("已停用 %1").arg(m_plugins[idx].name),
      QStringLiteral("success"));
}

void PluginService::togglePlugin(int idx)
{
  if (idx < 0 || idx >= m_plugins.size())
    return;
  setPluginEnabled(idx, !m_plugins[idx].isEnabled);
}

// ── Install lifecycle ─────────────────────────────────────────

bool PluginService::installPlugin(int idx)
{
  if (idx < 0 || idx >= m_plugins.size())
    return false;
  if (m_plugins[idx].isInstalled)
    return false;
  // Q10-2: report the truth instead of faking success. The previous
  // "mock install" flipped the persisted installed flag and reported
  // "已安装" while NO package exists on disk (no HTTP fetch, no archive
  // extraction, no Python) -- the user kept a state that lied about the
  // machine. Until a real download source lands, install reports
  // "not implemented" and flips nothing.
  emit statusMessage(tr("插件安装尚未实现：本版本没有可用的插件下载源"),
                     QStringLiteral("warn"));
  return false;
}

bool PluginService::uninstallPlugin(int idx)
{
  if (idx < 0 || idx >= m_plugins.size())
    return false;
  if (!m_plugins[idx].isInstalled)
    return false;
  m_plugins[idx].isInstalled = false;
  m_plugins[idx].isEnabled = false;
  m_plugins[idx].installedVersion.clear();
  saveToSettings();
  emit stateChanged();
  emit statusMessage(tr("已卸载 %1").arg(m_plugins[idx].name),
                     QStringLiteral("success"));
  return true;
}

bool PluginService::updatePlugin(int idx)
{
  if (idx < 0 || idx >= m_plugins.size())
    return false;
  if (!m_plugins[idx].isInstalled)
    return false;
  if (m_plugins[idx].updateStatus != QLatin1String("update_available"))
    return false;
  // Q10-2: same truth rule as installPlugin -- no real package download /
  // replace exists, so an update must report "not implemented" instead of
  // re-seating a version string that no package backs.
  emit statusMessage(tr("插件更新尚未实现：本版本没有可用的插件下载源"),
                     QStringLiteral("warn"));
  return false;
}

void PluginService::refreshPluginList()
{
  initDefaults();
  loadFromSettings();
  emit stateChanged();
  emit statusMessage(tr("插件列表已刷新"), QStringLiteral("info"));
}

// ── Capability subtree ────────────────────────────────────────

void PluginService::setCapabilityEnabled(int idx, const QString &name, bool enabled)
{
  if (idx < 0 || idx >= m_plugins.size())
    return;
  for (auto &cap : m_plugins[idx].capabilities) {
    if (cap.name != name || !cap.canToggle)
      continue;
    if (cap.enabled == enabled)
      return;
    cap.enabled = enabled;
    saveToSettings();
    emit stateChanged();
    emit statusMessage(
        enabled ? tr("已启用能力 %1").arg(name)
                : tr("已停用能力 %1").arg(name),
        QStringLiteral("success"));
    return;
  }
}

void PluginService::runScriptCapability(int idx, const QString &name)
{
  // MOCK: no plugin host exists, so nothing is executed. Reports the intent
  // where the real run_script_plugin_capability flow will plug in.
  if (idx < 0 || idx >= m_plugins.size())
    return;
  for (const auto &cap : m_plugins[idx].capabilities) {
    if (cap.name == name && cap.canRun) {
      emit statusMessage(tr("已请求运行能力 %1").arg(name),
                         QStringLiteral("info"));
      return;
    }
  }
}

QVariantMap PluginService::capabilityConfig(int idx, const QString &name) const
{
  QVariantMap result;
  result[QStringLiteral("config")] = QString();
  result[QStringLiteral("error")] = QString();
  if (idx < 0 || idx >= m_plugins.size()) {
    result[QStringLiteral("error")] = tr("插件不存在");
    return result;
  }
  for (const auto &cap : m_plugins[idx].capabilities) {
    if (cap.name != name)
      continue;
    result[QStringLiteral("config")] = cap.defaultConfig;
    return result;
  }
  result[QStringLiteral("error")] = tr("能力不存在");
  return result;
}

QVariantMap PluginService::saveCapabilityConfig(int idx, const QString &name, const QString &jsonText)
{
  const auto fail = [](const QString &message) {
    QVariantMap result;
    result[QStringLiteral("ok")] = false;
    result[QStringLiteral("error")] = message;
    return result;
  };
  if (idx < 0 || idx >= m_plugins.size())
    return fail(tr("插件不存在"));

  // The native side is the authority on validity (upstream
  // save_capability_config re-validates before persisting).
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
  if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    return fail(tr("无效 JSON: %1").arg(parseError.errorString()));

  for (auto &cap : m_plugins[idx].capabilities) {
    if (cap.name != name || !cap.hasConfig)
      continue;
    const QJsonObject obj = doc.object();
    cap.defaultConfig =
        QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    saveToSettings();
    emit stateChanged();
    emit statusMessage(tr("已保存能力 %1 的配置").arg(name),
                       QStringLiteral("success"));
    QVariantMap result;
    result[QStringLiteral("ok")] = true;
    result[QStringLiteral("error")] = QString();
    return result;
  }
  return fail(tr("能力不存在或不可配置"));
}

QVariantMap PluginService::restoreCapabilityConfig(int idx, const QString &name)
{
  const auto fail = [](const QString &message) {
    QVariantMap result;
    result[QStringLiteral("ok")] = false;
    result[QStringLiteral("error")] = message;
    return result;
  };
  if (idx < 0 || idx >= m_plugins.size())
    return fail(tr("插件不存在"));
  for (auto &cap : m_plugins[idx].capabilities) {
    if (cap.name != name || !cap.hasConfig)
      continue;
    // Q10-2: restores the capability's own built-in defaults from the SAME
    // shared table initDefaults() seeds from -- the literals previously
    // lived here a second time and could drift from the seeding site.
    cap.defaultConfig = builtinCapabilityConfig(cap.name);
    saveToSettings();
    emit stateChanged();
    emit statusMessage(tr("已恢复能力 %1 的默认配置").arg(name),
                       QStringLiteral("success"));
    QVariantMap result;
    result[QStringLiteral("ok")] = true;
    result[QStringLiteral("config")] = cap.defaultConfig;
    result[QStringLiteral("error")] = QString();
    return result;
  }
  return fail(tr("能力不存在或不可配置"));
}

// ── Row context menu ──────────────────────────────────────────

QVariantMap PluginService::runPluginAction(int idx, const QString &action)
{
  const auto report = [](const QString &message, const QString &level) {
    QVariantMap result;
    result[QStringLiteral("message")] = message;
    result[QStringLiteral("level")] = level;
    return result;
  };
  if (idx < 0 || idx >= m_plugins.size())
    return report(tr("插件不存在"), QStringLiteral("error"));

  // Copy the display name up front: uninstallPlugin() below mutates the
  // registry row this report describes.
  const QString pluginName = m_plugins[idx].name;
  if (action == QLatin1String("delete_plugin")) {
    // The QML layer shows the wxMessageBox-equivalent YES/NO confirm
    // before dispatching (upstream delete_local_plugin,
    // PluginsDialog.cpp:1109-1118); uninstallPlugin() carries the
    // statusMessage.
    uninstallPlugin(idx);
    return report(tr("已删除插件包 %1").arg(pluginName), QStringLiteral("success"));
  }
  if (action == QLatin1String("unsubscribe_plugin")) {
    // Same confirm gating on the QML side (upstream
    // unsubscribe_cloud_plugin, PluginsDialog.cpp:1146-1152).
    uninstallPlugin(idx);
    return report(tr("已取消订阅 %1").arg(pluginName), QStringLiteral("success"));
  }
  if (action == QLatin1String("open_folder")) {
    // Upstream open_plugin_folder warns when the plugin root cannot be
    // determined and otherwise opens it through desktop_open_any_folder
    // (PluginsDialog.cpp:1061-1071). The mock never writes the reserved
    // install path (no archive extractor), so an absent folder reports the
    // same warn instead of silently doing nothing.
    const QFileInfo folderInfo(m_plugins[idx].localPath);
    if (m_plugins[idx].localPath.isEmpty() || !folderInfo.isDir()) {
      emit statusMessage(tr("无法确定插件目录"), QStringLiteral("warn"));
      return report(tr("无法确定插件目录"), QStringLiteral("warn"));
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderInfo.absoluteFilePath()));
    const QString opened =
        tr("已打开插件目录 %1").arg(folderInfo.absoluteFilePath());
    emit statusMessage(opened, QStringLiteral("info"));
    return report(opened, QStringLiteral("info"));
  }
  if (action == QLatin1String("reinstall_plugin")) {
    // Upstream dispatches reinstall only for cloud rows and silently
    // ignores it on local rows (PluginsDialog.cpp:786-789; the policy
    // layer never offers it there either, :309-315).
    if (m_plugins[idx].source != QLatin1String("mine")
        && m_plugins[idx].source != QLatin1String("subscribed")
        && m_plugins[idx].source != QLatin1String("orphaned"))
      return report(QString(), QString());
    // Q10-2: report the truth -- installPlugin() emits the "not implemented"
    // warn (no real download source), so the action report mirrors that
    // instead of claiming a reinstall that never happened.
    installPlugin(idx);
    return report(tr("插件重新安装尚未实现：本版本没有可用的插件下载源"),
                  QStringLiteral("warn"));
  }
  if (action == QLatin1String("reload_plugin")
      || action == QLatin1String("clear_cache_reload_plugin")) {
    // Mock reload: no loader to poke, the state round-trips unchanged.
    // Report through statusMessage so the dialog status bar actually
    // receives it (the return value alone was never consumed).
    const QString reloaded = tr("已重新加载 %1").arg(pluginName);
    emit statusMessage(reloaded, QStringLiteral("success"));
    return report(reloaded, QStringLiteral("success"));
  }
  return report(tr("未知操作: %1").arg(action), QStringLiteral("error"));
}

// ── Install entry points ──────────────────────────────────────

void PluginService::installFromHub()
{
  // MOCK: the marketplace hub is not wired (no repository URL). Reports
  // where the real open_plugin_hub flow will plug in.
  emit statusMessage(tr("插件市场暂未接入，敬请期待"), QStringLiteral("info"));
}

void PluginService::installLocalPlugin()
{
  // MOCK: local package picking is not wired (no archive extractor).
  // Reports where the real install_local_plugin flow will plug in.
  emit statusMessage(tr("本地插件安装暂未接入"), QStringLiteral("info"));
}
