#include "PluginService.h"

#include <QSettings>

// Phase 202 (v5.6) -- PluginService implementation.
//
// Data source remains mock; this phase moves the hardcoded
// PluginManagerDialog.qml literals into C++ and adds QSettings persistence so
// enable/disable + (mock) install/uninstall survive dialog close and app
// restart. There is NO real HTTP download, NO Python / CPython, and NO plugin
// archive extraction. installPlugin()/uninstallPlugin() only flip the
// persisted installed flag so the UI layer has a working real backend; the
// download source is a documented TODO.
//
// Upstream reference: OrcaSlicer WebDownPluginDlg (wxDialog + wxWebView that
// dispatches download_plugin / install_plugin / open_plugin_folder). We mirror
// the user-facing semantics (install, enable/disable, refresh) without the
// wxWebView / real-network layer.

namespace {
// QSettings key prefix for every persisted plugin value.
constexpr const char *kSettingsPrefix = "plugins";
// Sentinel version key; bump when the persisted shape changes.
constexpr const char *kSettingsVersionKey = "plugins/version";
constexpr int kSettingsVersion = 1;
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
  // PluginManagerDialog.qml `property var plugins` literal verbatim so the
  // default visual stays byte-identical. Display strings go through tr() to
  // preserve the qsTr() translatability the old QML had.
  m_plugins.clear();

  PluginEntry networking;
  networking.name = tr("网络通信插件");
  networking.version = QStringLiteral("1.2.0");
  networking.description = tr("Bambu Lab 打印机网络通信支持");
  networking.author = QStringLiteral("OWzx");
  networking.size = tr("12.5 MB");
  networking.downloadUrl = QStringLiteral("https://plugins.owzx.example/network_plugin.zip");
  networking.localPath = QStringLiteral("plugins/network_plugin");
  networking.isInstalled = true;
  networking.isEnabled = true;
  m_plugins.append(networking);

  PluginEntry support;
  support.name = tr("高级支撑生成器");
  support.version = QStringLiteral("2.0.1");
  support.description = tr("基于树形结构的智能支撑生成");
  support.author = QStringLiteral("OWzx");
  support.size = tr("8.3 MB");
  support.downloadUrl = QStringLiteral("https://plugins.owzx.example/tree_support.zip");
  support.localPath = QStringLiteral("plugins/tree_support");
  support.isInstalled = false;
  support.isEnabled = false;
  m_plugins.append(support);

  PluginEntry ai;
  ai.name = tr("AI 切片优化");
  ai.version = QStringLiteral("0.9.0");
  ai.description = tr("基于 AI 模型的切片参数自动优化");
  ai.author = QStringLiteral("OWzx");
  ai.size = tr("45.2 MB");
  ai.downloadUrl = QStringLiteral("https://plugins.owzx.example/ai_slice.zip");
  ai.localPath = QStringLiteral("plugins/ai_slice");
  ai.isInstalled = false;
  ai.isEnabled = false;
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

  // Per-plugin enabled/installed flags are stored as parallel lists keyed by
  // index. The static catalog (name/version/description/...) is never
  // persisted -- it is re-seeded from initDefaults() -- only the mutable
  // enabled/installed state round-trips.
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
}

void PluginService::saveToSettings() const
{
  QSettings settings;
  settings.setValue(QLatin1String(kSettingsVersionKey), kSettingsVersion);

  QVariantList enabledList;
  QVariantList installedList;
  enabledList.reserve(m_plugins.size());
  installedList.reserve(m_plugins.size());
  for (const auto &p : m_plugins) {
    enabledList.append(p.isEnabled);
    installedList.append(p.isInstalled);
  }
  settings.setValue(QStringLiteral("%1/enabled").arg(kSettingsPrefix), enabledList);
  settings.setValue(QStringLiteral("%1/installed").arg(kSettingsPrefix), installedList);
}

QVariantMap PluginService::entryToMap(const PluginEntry &entry) const
{
  QVariantMap map;
  map[QStringLiteral("name")] = entry.name;
  map[QStringLiteral("version")] = entry.version;
  map[QStringLiteral("description")] = entry.description;
  map[QStringLiteral("author")] = entry.author;
  map[QStringLiteral("size")] = entry.size;
  map[QStringLiteral("downloadUrl")] = entry.downloadUrl;
  map[QStringLiteral("localPath")] = entry.localPath;
  map[QStringLiteral("isEnabled")] = entry.isEnabled;
  map[QStringLiteral("isInstalled")] = entry.isInstalled;
  // Status string mirrors the pre-Phase-202 QML literal: "已安装" when
  // installed, "可下载" when available but not installed.
  map[QStringLiteral("status")] =
      entry.isInstalled ? tr("已安装") : tr("可下载");
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
  // Mirrors the pre-Phase-202 QML: a not-installed plugin cannot be enabled.
  if (!m_plugins[idx].isInstalled)
    enabled = false;
  if (m_plugins[idx].isEnabled == enabled)
    return;
  m_plugins[idx].isEnabled = enabled;
  saveToSettings();
  emit stateChanged();
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
  saveToSettings();
  emit stateChanged();
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
  saveToSettings();
  emit stateChanged();
  return true;
}

void PluginService::refreshPluginList()
{
  initDefaults();
  loadFromSettings();
  emit stateChanged();
}
