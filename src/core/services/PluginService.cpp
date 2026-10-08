#include "PluginService.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>

// Phase 202 (v5.6) -- PluginService implementation.
//
// Data source remains mock. The registry rows carry the upstream
// PluginsDialog row schema (source variant, installed/latest version,
// update status, types, changelog, capability subtree with JSON configs)
// and every mutation reports its result through statusMessage() so the
// dialog's status bar (upstream status_message command) can render it.
// There is NO real HTTP download, NO Python / CPython, and NO plugin
// archive extraction. installPlugin()/uninstallPlugin()/updatePlugin()
// only flip the persisted installed/version state; the download source is
// a documented TODO.
//
// Upstream reference: OrcaSlicer PluginsDialog (build_plugin_dialog_item /
// evaluate_action_policy in PluginsDialog.cpp). We mirror the user-facing
// semantics (install, enable/disable, update, refresh, context actions)
// without the web view / real-network layer.

namespace {
// QSettings key prefix for every persisted plugin value.
constexpr const char *kSettingsPrefix = "plugins";
// Sentinel version key; bump when the persisted shape changes.
constexpr const char *kSettingsVersionKey = "plugins/version";
// v2: adds per-capability enabled flags + capability config overrides.
constexpr int kSettingsVersion = 2;
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
  push.defaultConfig = QStringLiteral("{\n  \"poll_interval\": 5\n}");
  networking.capabilities.append(push);

  PluginCapability monitor;
  monitor.name = QStringLiteral("远程监控");
  monitor.type = tr("脚本");
  monitor.typeKey = QStringLiteral("script");
  monitor.enabled = false;
  monitor.hasConfig = true;
  monitor.defaultConfig = QStringLiteral("{\n  \"snapshot_quality\": 80\n}");
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
  if (version < kSettingsVersion) {
    // Shape changed or fresh install: keep defaults, persist a baseline so
    // later edits write back cleanly.
    saveToSettings();
    return;
  }

  // Per-plugin mutable state is stored as parallel lists keyed by index.
  // The static catalog (name/version/description/...) is never persisted --
  // it is re-seeded from initDefaults() -- only the mutable enabled /
  // installed / capability / config state round-trips.
  const QVariant enabledVar =
      settings.value(QStringLiteral("%1/enabled").arg(kSettingsPrefix));
  if (enabledVar.isValid()) {
    const QVariantList list = enabledVar.toList();
    if (list.size() == m_plugins.size()) {
      for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].isEnabled = list[i].toBool();
    }
  }

  const QVariant installedVar =
      settings.value(QStringLiteral("%1/installed").arg(kSettingsPrefix));
  if (installedVar.isValid()) {
    const QVariantList list = installedVar.toList();
    if (list.size() == m_plugins.size()) {
      for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].isInstalled = list[i].toBool();
    }
  }

  // Persisted installed version (empty while not installed). Falls back to
  // the catalog version for a fresh baseline (upstream prefers the preserved
  // local version, falling back to the descriptor one).
  const QVariant installedVerVar =
      settings.value(QStringLiteral("%1/installed_versions").arg(kSettingsPrefix));
  if (installedVerVar.isValid()) {
    const QVariantList list = installedVerVar.toList();
    if (list.size() == m_plugins.size()) {
      for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].installedVersion = list[i].toString();
    }
  } else {
    for (int i = 0; i < m_plugins.size(); ++i)
      m_plugins[i].installedVersion =
          m_plugins[i].isInstalled ? m_plugins[i].version : QString();
  }

  // The mock catalog is static, so without persisting the update flag an
  // applied update would resurrect after a restart (the catalog would
  // re-report the same "latest" forever).
  const QVariant updateVar =
      settings.value(QStringLiteral("%1/update").arg(kSettingsPrefix));
  if (updateVar.isValid()) {
    const QVariantList list = updateVar.toList();
    if (list.size() == m_plugins.size()) {
      for (int i = 0; i < m_plugins.size(); ++i)
        m_plugins[i].updateStatus = list[i].toString();
    }
  }

  const QVariant capsVar =
      settings.value(QStringLiteral("%1/caps").arg(kSettingsPrefix));
  if (capsVar.isValid()) {
    const QVariantList perPlugin = capsVar.toList();
    for (int i = 0; i < m_plugins.size() && i < perPlugin.size(); ++i) {
      const QVariantList flags = perPlugin[i].toList();
      for (int c = 0; c < m_plugins[i].capabilities.size() && c < flags.size(); ++c)
        m_plugins[i].capabilities[c].enabled = flags[c].toBool();
    }
  }

  const QVariant configsVar =
      settings.value(QStringLiteral("%1/configs").arg(kSettingsPrefix));
  if (configsVar.isValid()) {
    const QVariantList perPlugin = configsVar.toList();
    for (int i = 0; i < m_plugins.size() && i < perPlugin.size(); ++i) {
      // Overrides ride on defaultConfig: an empty string means "no
      // override", restored lazily by capabilityConfig().
      const QVariantList overrides = perPlugin[i].toList();
      for (int c = 0; c < m_plugins[i].capabilities.size() && c < overrides.size(); ++c) {
        if (!m_plugins[i].capabilities[c].hasConfig)
          continue;
        const QString stored = overrides[c].toString();
        if (!stored.isEmpty())
          m_plugins[i].capabilities[c].defaultConfig = stored;
      }
    }
  }
}

void PluginService::saveToSettings() const
{
  QSettings settings;
  settings.setValue(QLatin1String(kSettingsVersionKey), kSettingsVersion);

  QVariantList enabledList;
  QVariantList installedList;
  QVariantList installedVerList;
  QVariantList updateList;
  QVariantList capsList;
  QVariantList configsList;
  enabledList.reserve(m_plugins.size());
  installedList.reserve(m_plugins.size());
  for (const auto &p : m_plugins) {
    enabledList.append(p.isEnabled);
    installedList.append(p.isInstalled);
    installedVerList.append(p.installedVersion);
    updateList.append(p.updateStatus);

    QVariantList capFlags;
    QVariantList capConfigs;
    capFlags.reserve(p.capabilities.size());
    capConfigs.reserve(p.capabilities.size());
    for (const auto &cap : p.capabilities) {
      capFlags.append(cap.enabled);
      // Only capabilities with an editor persist an override; "" = default.
      capConfigs.append(cap.hasConfig ? cap.defaultConfig : QString());
    }
    capsList.append(capFlags);
    configsList.append(capConfigs);
  }
  settings.setValue(QStringLiteral("%1/enabled").arg(kSettingsPrefix), enabledList);
  settings.setValue(QStringLiteral("%1/installed").arg(kSettingsPrefix), installedList);
  settings.setValue(QStringLiteral("%1/installed_versions").arg(kSettingsPrefix),
                    installedVerList);
  settings.setValue(QStringLiteral("%1/update").arg(kSettingsPrefix), updateList);
  settings.setValue(QStringLiteral("%1/caps").arg(kSettingsPrefix), capsList);
  settings.setValue(QStringLiteral("%1/configs").arg(kSettingsPrefix), configsList);
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
  // MOCK INSTALL: no real HTTP fetch, no archive extraction, no Python.
  // This flips the persisted installed flag so the UI layer has a working
  // real backend; a true download source is a documented TODO.
  if (m_plugins[idx].isInstalled)
    return false;
  m_plugins[idx].isInstalled = true;
  m_plugins[idx].isEnabled = true;
  m_plugins[idx].installedVersion = m_plugins[idx].version;
  saveToSettings();
  emit stateChanged();
  emit statusMessage(tr("已安装 %1").arg(m_plugins[idx].name),
                     QStringLiteral("success"));
  return true;
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
  // MOCK UPDATE: re-seat the installed version at the catalog latest. A
  // real package download/replace is the documented TODO.
  m_plugins[idx].installedVersion = m_plugins[idx].latestVersion;
  m_plugins[idx].version = m_plugins[idx].latestVersion;
  m_plugins[idx].updateStatus = QStringLiteral("normal");
  saveToSettings();
  emit stateChanged();
  emit statusMessage(tr("已更新 %1 到 %2").arg(m_plugins[idx].name, m_plugins[idx].latestVersion),
                     QStringLiteral("success"));
  return true;
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
    // Restores the capability's own built-in defaults; the mock registry
    // keeps them inline in initDefaults() per capability id.
    if (cap.name == QStringLiteral("设备状态推送"))
      cap.defaultConfig = QStringLiteral("{\n  \"poll_interval\": 5\n}");
    else if (cap.name == QStringLiteral("远程监控"))
      cap.defaultConfig = QStringLiteral("{\n  \"snapshot_quality\": 80\n}");
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
    uninstallPlugin(idx);
    return report(tr("已删除插件包 %1").arg(pluginName), QStringLiteral("success"));
  }
  if (action == QLatin1String("unsubscribe_plugin")) {
    uninstallPlugin(idx);
    return report(tr("已取消订阅 %1").arg(pluginName), QStringLiteral("success"));
  }
  if (action == QLatin1String("open_folder")) {
    // The reserved install path is never written by the mock (no archive
    // extractor); surface the path the real build would open.
    return report(tr("插件目录: %1").arg(m_plugins[idx].localPath),
                  QStringLiteral("info"));
  }
  if (action == QLatin1String("reinstall_plugin")
      || action == QLatin1String("reload_plugin")
      || action == QLatin1String("clear_cache_reload_plugin")) {
    // Mock reload: no loader to poke, the state round-trips unchanged.
    return report(tr("已重新加载 %1").arg(pluginName), QStringLiteral("success"));
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
