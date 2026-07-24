# Phase 202 Plan: Plugin Manager UI Real Backend (mock -> Service)

**Milestone:** v5.6
**Scope:** UI real backend only. The hardcoded mock data in
`PluginManagerDialog.qml` is moved into a C++ `PluginService`, and the
per-plugin enabled/installed state is persisted to local QSettings so it
survives dialog close and application restart. This is explicitly NOT a
Python scripting framework and NOT a real network downloader.

## Goal

Eliminate the 100%-hardcoded mock QML in `PluginManagerDialog.qml` (a
`property var plugins` literal with 3 fixed entries and zero behavior) by
introducing a `PluginService` C++ object that owns the plugin registry and
the mutable enable/installed state. The data source remains mock; the value
this phase adds is: (1) the UI no longer bakes in literals, (2) enable/disable
and install/uninstall actions actually do something and persist, (3) a clear
extension seam exists for a future real-download implementation.

## Background

`PluginManagerDialog.qml` (pre-Phase-202) declared a `property var plugins`
literal with 3 entries (network communication plugin / advanced support
generator / AI slice optimization). The enable checkbox had no
`onCheckedChanged` handler, the download/uninstall buttons had no `onClicked`,
and nothing persisted -- the dialog was pure display.

The dialog is opened via `BackendContext::showPluginManagerDialog()`
(BackendContext.cpp), which emits `showPluginManagerDialogRequested`;
`main.qml` connects that to `pluginManagerDialog.open()`.
`BackendContext` is the existing context property that already exposes every
service/viewmodel (`editorViewModel`, `amsMaterialsViewModel`, ...) as
`Q_PROPERTY(QObject*)`.

### Upstream reference (read-only)

OrcaSlicer `src/slic3r/GUI/WebDownPluginDlg.{hpp,cpp}` is a `wxDialog`
hosting a `wxWebView` over a bundled `resources/web/guide/6/index.html`. The
web page posts JSON commands back through `OnScriptMessage`; the relevant
ones are:
- `Begin_Download_network_plugin` -> `DownloadPlugin()` -> `wxGetApp().download_plugin("plugins", "network_plugin.zip", ...)`
- `begin_install_plugin` -> `InstallPlugin()` -> `wxGetApp().install_plugin(...)`
- `netplugin_download_cancel` -> cancel
- `restart_studio` -> restart networking
- `open_plugin_folder` -> open the `<userdataDir>/plugins` folder
- `ShowStatusPercent` -> post download/install percent back to the web page

`WebDownPluginDlg` therefore is a **plugin download/install dialog**, not a
script engine. This phase mirrors its user-facing semantics (install,
enable/disable, refresh) without the wxWebView / real-network layer.

## Hard scope constraints (do NOT cross)

1. **No Python / CPython.** This is the plugin download/management UI, not a
   script framework. There is no `ScriptEngine`, no Python binding, no
   `Python.h`. CMakeLists.txt gains zero Python dependencies.
2. **No real network download.** `installPlugin()` is a **mock state flip**
   (it sets `isInstalled = true` and persists). There is no HTTP client, no
   archive extraction, no `<UserDataDir>/plugins` write. The real download
   source (a plugin repository URL + HTTP + zip extraction) is a documented
   TODO for a later phase. The `downloadUrl` field on each entry is reserved
   for that future wiring but is never fetched.

## Files

- Add: `src/core/services/PluginService.h`
  - `final` QObject following the project service conventions (camelCase
    methods, `m_`-prefixed members, single `stateChanged` NOTIFY for all
    `Q_PROPERTY`s -- mirrors `AmsMaterialsViewModel` / `EditorViewModel`).
  - Private `PluginEntry` struct: name, version, description, author, size,
    downloadUrl, localPath, isEnabled, isInstalled.
  - `Q_PROPERTY`: `pluginCount`, `pluginNames` (QStringList), `plugins`
    (QVariantList of entry maps).
  - `Q_INVOKABLE` API: `pluginAt(int)`, `isPluginEnabled(int)`,
    `setPluginEnabled(int,bool)`, `togglePlugin(int)`, `installPlugin(int)`,
    `uninstallPlugin(int)`, `refreshPluginList()`.
- Add: `src/core/services/PluginService.cpp`
  - Construct seeds the 3 default mock entries verbatim from the old QML
    literal (networking / advanced support / AI slice). Display strings go
    through `tr()` to preserve the old `qsTr()` translatability.
  - Versioned QSettings persistence under `plugins/*` (parallel
    `enabled`/`installed` QVariantLists, plus a `plugins/version` sentinel).
    Only the mutable enabled/installed state round-trips; the static catalog
    is re-seeded from `initDefaults()` on every construct/refresh.
  - `installPlugin()` / `uninstallPlugin()` flip the persisted installed flag
    (MOCK -- documented inline). `refreshPluginList()` re-seeds + reloads.
- Modify: `src/qml_gui/BackendContext.h`
  - Forward-declare `PluginService`; add `Q_PROPERTY(QObject *pluginService)`,
    accessor decl, member.
  - Update the `showPluginManagerDialog()` doc comment (no longer a placeholder).
- Modify: `src/qml_gui/BackendContext.cpp`
  - Include the header; construct `new PluginService(this)` next to
    `appSettings_`; add the accessor returning it.
- Modify: `src/qml_gui/dialogs/PluginManagerDialog.qml`
  - Remove the hardcoded `property var plugins` literal.
  - Add `property var pluginService: null` and a readonly `_plugins` view over
    `pluginService.plugins` (re-evaluated on `stateChanged`).
  - Repeater reads `modelData.*` (name/version/description/size/status/
    isInstalled/isEnabled).
  - Enable checkbox -> `pluginService.setPluginEnabled(index, checked)`
    (guarded against the binding-seed delta).
  - Download button -> `pluginService.installPlugin(index)`.
  - Uninstall button -> `pluginService.uninstallPlugin(index)`.
  - Footer gains a "刷新" (refresh) button -> `pluginService.refreshPluginList()`.
- Modify: `src/qml_gui/main.qml`
  - Set `pluginService: backend.pluginService` on the `PluginManagerDialog`.
- Modify: `CMakeLists.txt`
  - Register `PluginService.{h,cpp}` next to the other services in the source
    list (after `AppSettingsService`).

## Steps

- [x] Read upstream `WebDownPluginDlg.{hpp,cpp}` (read-only reference), the
      pre-Phase-202 `PluginManagerDialog.qml`, `BackendContext.{h,cpp}`, and
      the `AmsMaterialsViewModel` template (same mock+persist+single-signal
      pattern) to lock down conventions.
- [x] Implement `PluginService.{h,cpp}`: 3 mock entries verbatim from old QML,
      versioned QSettings persistence under `plugins/*`, single-signal NOTIFY,
      mock install/uninstall state flips.
- [x] Register the service in `BackendContext` (Q_PROPERTY + accessor + member
      + construction + include + updated doc comment).
- [x] Rewrite `PluginManagerDialog.qml`: drop the literal, bind `pluginService`,
      route enable/install/uninstall/refresh through the service.
- [x] Wire `pluginService: backend.pluginService` in `main.qml`.
- [x] Add the two new files to `CMakeLists.txt`.
- [ ] Bracket-balance check on every changed file.

## Key design decisions

1. **This is the plugin download/management UI, not a script framework.**
   Per the project decision, we align with upstream `WebDownPluginDlg` (a
   wx plugin download dialog) -- we do NOT embed Python / CPython. There is no
   `ScriptEngine` and no Python binding in this phase or in the codebase. The
   "plugin" noun here means a downloadable vendor extension (e.g. the Bambu
   Lab network plugin), exactly as upstream uses it.

2. **installPlugin is a MOCK state flip.** There is no real HTTP client, no
   plugin archive, no `<UserDataDir>/plugins` write. `installPlugin(idx)`
   sets `isInstalled = true` (and enables), `uninstallPlugin(idx)` sets it
   false (and disables), and both persist to QSettings. This gives the UI
   layer a working real backend (a C++ object the QML drives through
   `Q_INVOKABLE`) while leaving the download source as a documented TODO.
   The `downloadUrl` / `localPath` fields are reserved for the future real
   wiring but are never fetched.

3. **Persistence is the value-add.** Pre-Phase-202, the enable checkbox and
   download/uninstall buttons were display-only; nothing persisted. The
   service writes every successful state change to QSettings
   (`plugins/enabled`, `plugins/installed`, plus a `plugins/version`
   sentinel). This turns "buttons do nothing" into "stateful mock". The
   persistence is purely local (QSettings on disk) and touches no network.

4. **Single-signal NOTIFY (`stateChanged`).** Mirrors `EditorViewModel` and
   `AmsMaterialsViewModel`: every `Q_PROPERTY` carries `NOTIFY stateChanged`
   and every mutating API emits it once after the change + persist. This
   keeps the QML side simple (one binding refresh point via the readonly
   `_plugins` view) and matches an established project convention rather than
   introducing per-property signals.

5. **Mock catalog re-seeded, not persisted.** The static catalog (name,
   version, description, author, size, downloadUrl, localPath) is re-seeded
   from `initDefaults()` on every construct and on `refreshPluginList()`.
   Only the mutable enabled/installed flags round-trip through QSettings.
   This keeps the catalog versionable in code (a future plugin repository
   manifest replaces `initDefaults()`) without a migration headache.

6. **Display strings use C++ `tr()`.** The old QML used `qsTr("网络通信插件")`
   etc. Moving the literals to C++ would have lost translatability, so the
   defaults are wrapped in `tr()` -- Qt extracts them into the same `.ts`
   pipeline. Version strings (`"1.2.0"`) and author/URL/path codes are
   intentionally NOT translated, matching the original QML split.

7. **QML binding-seed guard on the checkbox.** The enable checkbox binds
   `checked: modelData.isEnabled` and also has an `onCheckedChanged` that
   calls `setPluginEnabled`. To avoid a feedback loop when the binding seeds
   the value, the handler compares `checked !== modelData.isEnabled` before
   acting, so only a real user-initiated delta reaches the service.

## Known limitations / deferred

- **No real download.** `installPlugin` does not fetch `downloadUrl` or write
  `localPath`. Wiring a real plugin repository (HTTP GET + zip extraction to
  `<UserDataDir>/plugins`) is a follow-up phase; the service API shape already
  supports it (the `downloadUrl`/`localPath` fields exist).
- **No version check / update flow.** Upstream `WebUpdatePlugin.{cpp,hpp}`
  exists alongside `WebDownPluginDlg` for update detection; that is out of
  scope here. `refreshPluginList()` only reloads the local registry.
- **Catalog is fixed at 3 mock entries.** A future plugin-marketplace browse
  (the footer "从插件市场浏览更多插件" hint text) would need a dynamic catalog
  sourced from a remote manifest; not needed for the UI real backend.
- **No restart prompt after install.** Upstream emits `restart_studio` to
  reload networking; this mock does not require a restart and does not prompt.
- **Status string is a simple binary.** `status` is "已安装" when installed,
  "可下载" otherwise. A richer progress state (downloading X%, installing...)
  needs a per-entry state machine deferred to the real-download phase.

## Verify

- [ ] Build: `PluginService.{h,cpp}` compiles and links; the dialog's
      `pluginService` binding resolves at runtime.
- [ ] Manual: open the plugin manager dialog; the 3 default mock entries
      match the pre-Phase-202 visual (same names, versions, descriptions,
      sizes, statuses).
- [ ] Manual: toggle the enable checkbox on the installed networking plugin,
      close and reopen -- the toggle survives (QSettings).
- [ ] Manual: click "下载" on the advanced-support row -- it flips to
      installed + enabled; click "卸载" -- it flips back; both survive
      close/reopen.
- [ ] Manual: click "刷新" -- the registry reloads without visual regression.
- [ ] Manual: restart the app -- persisted enabled/installed state is still
      present.
