#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

/// PluginService (Phase 202, v5.6 Plugin Manager UI Real Backend).
///
/// Replaces the 100%-hardcoded mock data that used to live in
/// PluginManagerDialog.qml (a `property var plugins` literal with 3 fixed
/// entries and zero behavior). The data source is still mock, but the
/// per-plugin enabled/installed/capability state is persisted to QSettings
/// under the "plugins/*" namespace, so enable/disable and (mock)
/// install/uninstall actions survive dialog close AND application restart.
///
/// Aligns with the upstream OrcaSlicer PluginsDialog row schema
/// (PluginsDialog.cpp build_plugin_dialog_item): source variant
/// (local/mine/subscribed/orphaned), installed vs latest version,
/// update status, display types, changelog and a capability subtree with
/// per-capability JSON config. This service deliberately does NOT embed a
/// web view, a real HTTP client, or a plugin archive extractor. There is no
/// Python / CPython and no real download source wired -- installPlugin() is
/// a state-flipping mock that documents where the real download/install
/// flow will plug in once a plugin repository URL exists.
///
/// Notification model: a single stateChanged signal fans out to every
/// Q_PROPERTY (batch notification, mirrors the AmsMaterialsViewModel /
/// EditorViewModel convention). Operation results additionally travel as
/// statusMessage(message, level) -- level is one of
/// "success" / "error" / "warn" / "info", rendered by the dialog's status
/// bar (upstream status_message web command).
class PluginService final : public QObject
{
  Q_OBJECT

  // ── Plugin registry state ───────────────────────────────────────────
  Q_PROPERTY(int pluginCount READ pluginCount NOTIFY stateChanged)
  Q_PROPERTY(QStringList pluginNames READ pluginNames NOTIFY stateChanged)
  /// Full registry rows for QML Repeater binding.
  Q_PROPERTY(QVariantList plugins READ plugins NOTIFY stateChanged)

public:
  explicit PluginService(QObject *parent = nullptr);
  ~PluginService() override;

  // ── Property getters ───────────────────────────────────────────────
  int pluginCount() const;
  QStringList pluginNames() const;
  QVariantList plugins() const;

  /// Return a single plugin row as a QVariantMap with the same keys the QML
  /// table consumes: name, version, description, author, source, types,
  /// installedVersion, latestVersion, updateStatus, changelog, capabilities,
  /// downloadUrl, localPath, isEnabled, isInstalled, status, statusKey.
  Q_INVOKABLE QVariantMap pluginAt(int idx) const;

  // ── Per-plugin state ───────────────────────────────────────────────
  Q_INVOKABLE bool isPluginEnabled(int idx) const;
  Q_INVOKABLE void setPluginEnabled(int idx, bool enabled);
  /// Toggle the enabled flag. No-op when the plugin is not installed
  /// (a not-installed row cannot be activated).
  Q_INVOKABLE void togglePlugin(int idx);

  // ── Install lifecycle ──────────────────────────────────────────────
  /// Mark the plugin as installed. MOCK ONLY: there is no real download
  /// source or HTTP client. Returns true when the state actually flips.
  /// Wire a real plugin repository + download in a later phase.
  Q_INVOKABLE bool installPlugin(int idx);
  /// Mark the plugin as not installed and disable it. Returns true when the
  /// state actually flips. Persisted.
  Q_INVOKABLE bool uninstallPlugin(int idx);
  /// Mock "Update to the latest version": re-seats installedVersion at
  /// latestVersion and clears the update flag (upstream update_plugin).
  Q_INVOKABLE bool updatePlugin(int idx);

  /// Re-seed the registry from defaults and reload persisted state. Emits
  /// stateChanged once. Aligned with the upstream refresh_plugins command.
  Q_INVOKABLE void refreshPluginList();

  // ── Capability subtree ─────────────────────────────────────────────
  /// Enable/disable one capability of one plugin (upstream
  /// toggle_plugin_capability). Persisted.
  Q_INVOKABLE void setCapabilityEnabled(int idx, const QString &name, bool enabled);
  /// MOCK script run (upstream run_script_plugin_capability): there is no
  /// plugin host to execute anything, so this only reports the intent on
  /// the status bar.
  Q_INVOKABLE void runScriptCapability(int idx, const QString &name);

  /// Return the stored JSON config text of one capability, falling back to
  /// its default when nothing was saved yet. Map: {config, error}.
  Q_INVOKABLE QVariantMap capabilityConfig(int idx, const QString &name) const;
  /// Validate + persist a capability's JSON config (upstream
  /// save_capability_config). Map: {ok, error, config}.
  Q_INVOKABLE QVariantMap saveCapabilityConfig(int idx, const QString &name, const QString &jsonText);
  /// Drop the saved override and return to the capability default (upstream
  /// restore_capability_config). Map: {ok, error, config}.
  Q_INVOKABLE QVariantMap restoreCapabilityConfig(int idx, const QString &name);

  // ── Row context menu (upstream plugin_menu_action) ─────────────────
  /// Run one context-menu action on one row. Action ids mirror
  /// PluginsDialog.cpp evaluate_action_policy: delete_plugin,
  /// unsubscribe_plugin, open_folder, reload_plugin, reinstall_plugin,
  /// clear_cache_reload_plugin. Returns {message, level} for the status
  /// bar; every user-visible branch also emits statusMessage() so the bar
  /// is reachable without consuming the return value. The destructive
  /// delete/unsubscribe YES/NO confirm lives in the QML layer (upstream
  /// wxMessageBox in delete_local_plugin / unsubscribe_cloud_plugin) and
  /// runs before this call. open_folder opens the real folder through the
  /// desktop service when it exists and reports the upstream warn when
  /// the plugin root cannot be determined. reinstall_plugin is a silent
  /// no-op on non-cloud rows (upstream PluginsDialog.cpp:786-789).
  Q_INVOKABLE QVariantMap runPluginAction(int idx, const QString &action);

  // ── Install entry points (upstream hub / local package flows) ──────
  /// MOCK: the marketplace hub is not wired (no repository URL exists).
  /// Reports an info status message and documents the future hook.
  Q_INVOKABLE void installFromHub();
  /// MOCK: local package picking is not wired (no archive extractor).
  /// Reports an info status message and documents the future hook.
  Q_INVOKABLE void installLocalPlugin();

signals:
  /// Batch notification for every Q_PROPERTY (EditorViewModel convention).
  void stateChanged();
  /// Result of one user operation, rendered by the dialog status bar.
  /// Mirrors the upstream status_message web command payload shape.
  void statusMessage(const QString &message, const QString &level);

private:
  struct PluginCapability
  {
    QString name;           // Capability identity within the plugin
    QString type;           // Display type label ("脚本" etc.)
    QString typeKey;        // Stable type key ("script" etc.)
    bool enabled = false;   // Persisted per-capability enabled flag
    bool canToggle = true;  // Upstream can_toggle
    bool canRun = false;    // Upstream CapabilityCanRun
    bool hasConfig = false; // Exposes a JSON config editor
    QString defaultConfig;  // JSON text returned when no override is stored
  };

  struct PluginChangelogEntry
  {
    QString version;
    QString date;      // ISO date string, pre-formatted for display
    QString changes;
  };

  struct PluginEntry
  {
    QString name;             // Display name (translatable)
    QString version;          // Catalog semver string, e.g. "1.2.0"
    QString description;      // Short description (translatable)
    QString author;           // Author / vendor string
    QString source;           // "local" | "mine" | "subscribed" | "orphaned"
    QString types;            // Display types, "[a, b]" or "-" (upstream GetPluginTypes)
    QString latestVersion;    // Newest catalog version (update target)
    QString installedVersion; // Empty while not installed (upstream has_local_package)
    QString updateStatus;     // "normal" | "update_available" | "unauthorized"
    QString downloadUrl;      // Future download source URL (not fetched yet)
    QString localPath;        // Reserved install path (not written yet)
    QList<PluginCapability> capabilities;
    QList<PluginChangelogEntry> changelog;
    bool isEnabled = false;    // Persisted enabled flag
    bool isInstalled = false;  // Persisted installed flag
  };

  // Seed the default mock plugins (the 3 pre-Phase-202 entries, enriched
  // with the upstream PluginsDialog row fields).
  void initDefaults();
  // Apply persisted enabled/installed state from QSettings.
  void loadFromSettings();
  // Persist current state to QSettings under plugins/*.
  void saveToSettings() const;
  // QVariantMap projection of one entry (the shape QML consumes).
  QVariantMap entryToMap(const PluginEntry &entry) const;
  // QVariantMap projection of one capability row.
  QVariantMap capabilityToMap(const PluginCapability &capability) const;

  QList<PluginEntry> m_plugins;
};
