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
/// entries and zero behavior). The data source is still mock, but it now
/// lives in C++ and the per-plugin enabled/installed state is persisted to
/// QSettings under the "plugins/*" namespace, so enable/disable and
/// (mock) install/uninstall actions survive dialog close AND application
/// restart.
///
/// Aligns with the upstream OrcaSlicer WebDownPluginDlg (a wxDialog that
/// hosts a wxWebView over a bundled HTML index and dispatches
/// download_plugin / install_plugin / open_plugin_folder commands). This
/// service deliberately does NOT embed wxWebView, a real HTTP client, or a
/// plugin archive extractor. There is no Python / CPython and no real
/// download source wired -- installPlugin() is a state-flipping mock that
/// documents where the real download/install flow will plug in once a
/// plugin repository URL exists.
///
/// Notification model: a single stateChanged signal fans out to every
/// Q_PROPERTY (batch notification, mirrors the AmsMaterialsViewModel /
/// EditorViewModel convention).
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
  /// Repeater consumes: name, version, description, author, size, downloadUrl,
  /// localPath, isEnabled, isInstalled, status.
  Q_INVOKABLE QVariantMap pluginAt(int idx) const;

  // ── Per-plugin state ───────────────────────────────────────────────
  Q_INVOKABLE bool isPluginEnabled(int idx) const;
  Q_INVOKABLE void setPluginEnabled(int idx, bool enabled);
  /// Toggle the enabled flag. No-op when the plugin is not installed
  /// (mirrors the pre-Phase-202 QML which disabled the checkbox until
  /// the row was installed).
  Q_INVOKABLE void togglePlugin(int idx);

  // ── Install lifecycle ──────────────────────────────────────────────
  /// Mark the plugin as installed. MOCK ONLY: there is no real download
  /// source or HTTP client. Returns true when the state actually flips.
  /// Wire a real plugin repository + download in a later phase.
  Q_INVOKABLE bool installPlugin(int idx);
  /// Mark the plugin as not installed and disable it. Returns true when the
  /// state actually flips. Persisted.
  Q_INVOKABLE bool uninstallPlugin(int idx);

  /// Re-seed the registry from defaults and reload persisted state. Emits
  /// stateChanged once. Aligned with the upstream WebDownPluginDlg refresh
  /// intent (the HTML page polls plugin status via RunScript).
  Q_INVOKABLE void refreshPluginList();

signals:
  /// Batch notification for every Q_PROPERTY (EditorViewModel convention).
  void stateChanged();

private:
  struct PluginEntry
  {
    QString name;           // Display name (translatable)
    QString version;        // Semver string, e.g. "1.2.0"
    QString description;    // Short description (translatable)
    QString author;         // Author / vendor string
    QString size;           // Human-readable size, e.g. "12.5 MB"
    QString downloadUrl;    // Future download source URL (not fetched yet)
    QString localPath;      // Reserved install path (not written yet)
    bool isEnabled = false;    // Persisted enabled flag
    bool isInstalled = false;  // Persisted installed flag
  };

  // Seed the 3 default mock plugins (verbatim from pre-Phase-202 QML).
  void initDefaults();
  // Apply persisted enabled/installed overrides from QSettings.
  void loadFromSettings();
  // Persist current enabled/installed state to QSettings under plugins/*.
  void saveToSettings() const;
  // QVariantMap projection of one entry (the shape QML consumes).
  QVariantMap entryToMap(const PluginEntry &entry) const;

  QList<PluginEntry> m_plugins;
};
