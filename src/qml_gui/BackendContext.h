#pragma once

#include <QObject>
#include <QColor>
#include <QTranslator>
#include <QElapsedTimer>
#include <QWebChannel>
class QNetworkAccessManager;
#include <QHash>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <QQueue>
#include <QDateTime>
#include <QSettings>

#include "core/ai/AppToolRegistry.h"

class SliceService;
class PresetServiceMock;
class DeviceServiceMock;
class ProjectServiceMock;
class NetworkServiceMock;
class CalibrationServiceMock;
class CameraServiceMock;
class AppSettingsService;
class PluginService;

class EditorViewModel;
class PreviewViewModel;
class MonitorViewModel;
class ConfigViewModel;
class HomeViewModel;
class SettingsViewModel;
class ProjectViewModel;
class CalibrationViewModel;
class MultiMachineViewModel;
class AmsMaterialsViewModel;
class AiAgentService;
class AiChatBridge;
class AiViewModel;
namespace OWzx { class McpHttpServer; }

/// Notification severity exposed to QML.
enum NotificationLevel {
  NotiInfo = 0,           ///< Informational notification.
  NotiSuccess = 1,        ///< Successful completion.
  NotiWarning = 2,        ///< Warning notification.
  NotiError = 3,          ///< Error notification.
  NotiSeriousWarning = 4, ///< High-priority warning.
  NotiHint = 5,           ///< Hint or did-you-know notification.
  NotiPrintInfo = 6, ///< Print notification.
  NotiPrintInfoShort = 7, ///< Print notification.
  NotiProgress = 8, ///< Progress notification.
  NotiSlicingProgress = 9 ///< Slicing progress notification.
};

/// Notification type values aligned with upstream NotificationManager.
enum NotificationType {
  NotiTypeCustom = 0,
  NotiTypeExportFinished = 1,
  NotiTypeSlicingProgress = 2,
  NotiTypeSlicingError = 3,
  NotiTypeSlicingWarning = 4,
  NotiTypeValidateError = 5,
  NotiTypeValidateWarning = 6,
  NotiTypePlaterError = 7,
  NotiTypePlaterWarning = 8,
  NotiTypeProgressBar = 9,
  NotiTypeDidYouKnowHint = 10,
  NotiTypeExportOngoing = 11,
  NotiTypeArrangeOngoing = 12,
  NotiTypeUpdatedItemsInfo = 13,
  NotiTypeObjectInfo = 14,
  NotiTypeProgressIndicator = 15,
};

/// Hint data exposed to QML notification surfaces.
struct HintData {
  QString id;
  QString text;
  int weight = 1;
  QString hypertext;           ///< Hyperlink target text.
  QString followText;          ///< Text shown after the hyperlink.
  QString documentationLink;   ///< Documentation URL.
  QString callbackType;        ///< link / settings / preferences
  QString callbackTarget;      ///< URL or settings key.
};

class BackendContext final : public QObject, public OWzx::AppToolUiProvider
{
  Q_OBJECT
  Q_PROPERTY(QObject *editorViewModel READ editorViewModel CONSTANT)
  Q_PROPERTY(QObject *previewViewModel READ previewViewModel CONSTANT)
  Q_PROPERTY(QObject *monitorViewModel READ monitorViewModel CONSTANT)
  Q_PROPERTY(QObject *configViewModel READ configViewModel CONSTANT)
  Q_PROPERTY(QObject *homeViewModel READ homeViewModel CONSTANT)
  Q_PROPERTY(QObject *settingsViewModel READ settingsViewModel CONSTANT)
  Q_PROPERTY(QObject *projectViewModel READ projectViewModel CONSTANT)
  Q_PROPERTY(QObject *calibrationViewModel READ calibrationViewModel CONSTANT)
  Q_PROPERTY(QObject *multiMachineViewModel READ multiMachineViewModel CONSTANT)
  // Phase 199 (WIZ-01): expose the preset data service to QML so the
  // ConfigWizard can enumerate vendors / printer models / materials /
  // bed surfaces via Q_INVOKABLE. Returned as QObject* to avoid pulling
  // the PresetServiceMock header into every QML-facing include.
  Q_PROPERTY(QObject *presetServiceMock READ presetServiceMock CONSTANT)
  // Phase 239 (ENGN-03): expose the slice engine service the same way the
  // other services are exposed (QObject* CONSTANT). Lets QML/tests observe
  // the engine-level validateWarning / export signals directly.
  Q_PROPERTY(QObject *sliceService READ sliceService CONSTANT)
  // Phase 201 (v5.6 AMS Architecture Cleanup): mock->viewmodel for AMSSettingsDialog.
  Q_PROPERTY(QObject *amsMaterialsViewModel READ amsMaterialsViewModel CONSTANT)
  // Phase 202 (v5.6 Plugin Manager UI Real Backend): mock->service for
  // PluginManagerDialog. Owns the plugin registry + persisted enable/install
  // state. installPlugin is a mock (no real download source).
  Q_PROPERTY(QObject *pluginService READ pluginService CONSTANT)
  Q_PROPERTY(QObject *appSettings READ appSettings CONSTANT)
  // AI 助手（OWzx-only，docs/ai-control.md）：聊天侧栏 ViewModel + 控制面
  // 活跃状态（MCP 服务器 + sidecar 是否已按偏好设置启动）。
  // aiChatBridge 是暴露给 WebEngine 聊天页的 QWebChannel 对象（网页版
  // 聊天 UI，替代 QML 卡片渲染）。
  Q_PROPERTY(QObject *aiViewModel READ aiViewModel CONSTANT)
  Q_PROPERTY(QObject *aiChatBridge READ aiChatBridge CONSTANT)
  Q_PROPERTY(bool aiControlActive READ aiControlActive NOTIFY stateChanged)
  Q_PROPERTY(bool visualCompareMode READ visualCompareMode CONSTANT)
  // Phase 51: shell-level action gate properties (SHELL-03) - forward to EditorViewModel/PreviewViewModel.
  Q_PROPERTY(bool canImport READ canImport NOTIFY stateChanged)
  Q_PROPERTY(bool canSlice READ canSlice NOTIFY stateChanged)
  Q_PROPERTY(bool isSlicing READ isSlicing NOTIFY stateChanged)
  Q_PROPERTY(bool canExport READ canExport NOTIFY stateChanged)
  Q_PROPERTY(bool canSave READ canSave NOTIFY stateChanged)
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY stateChanged)
  Q_PROPERTY(bool isBusy READ isBusy NOTIFY stateChanged)
  // Phase 51: state-dependent blocked-reason labels (mirror sliceActionLabel pattern).
  Q_PROPERTY(QString exportActionLabel READ exportActionLabel NOTIFY stateChanged)
  Q_PROPERTY(QString exportActionHint READ exportActionHint NOTIFY stateChanged)
  Q_PROPERTY(QString saveActionLabel READ saveActionLabel NOTIFY stateChanged)
  Q_PROPERTY(QString saveActionHint READ saveActionHint NOTIFY stateChanged)
  Q_PROPERTY(int currentPage READ currentPage NOTIFY currentPageChanged)
  Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorChanged)
  Q_PROPERTY(int lastErrorSeverity READ lastErrorSeverity NOTIFY errorChanged)
  Q_PROPERTY(QString lastErrorTitle READ lastErrorTitle NOTIFY errorChanged)
  Q_PROPERTY(int pendingNotificationCount READ pendingNotificationCount NOTIFY errorChanged)
  // Phase 240 (NOTI-01): stacked notification surface. Upstream
  // NotificationManager renders every live PopNotification at once
  // (NotificationManager.cpp:2531-2554 render_notifications), importance
  // ordered (NotificationManager.cpp:2633-2639 sort_notifications). This list
  // mirrors it for QML: index 0 is the MOST important entry (top of the
  // stack), matching upstream ErrorNotificationLevel at the "Top most
  // position" (NotificationManager.hpp:158-180).
  Q_PROPERTY(QVariantList notificationStack READ notificationStack NOTIFY errorChanged)
  /// Progress value for the currently displayed notification.
  Q_PROPERTY(int currentNotificationProgress READ currentNotificationProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationHasProgress READ currentNotificationHasProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationPersistent READ currentNotificationPersistent NOTIFY errorChanged)
  Q_PROPERTY(int currentNotificationType READ currentNotificationType NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowExport READ currentNotificationShowExport NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowPreview READ currentNotificationShowPreview NOTIFY errorChanged)
  /// Notification state exposed to QML.
  Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)
  Q_PROPERTY(int unreadHistoryCount READ unreadHistoryCount NOTIFY historyChanged)
  Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(bool hintsEnabled READ hintsEnabled WRITE setHintsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(int autoDismissSec READ autoDismissSec WRITE setAutoDismissSec NOTIFY settingsChanged)
  Q_PROPERTY(bool showProgressNotifications READ showProgressNotifications WRITE setShowProgressNotifications NOTIFY settingsChanged)
  Q_PROPERTY(QString latencyBrief READ latencyBrief NOTIFY latencyChanged)
  Q_PROPERTY(QString lastLatencyOperation READ lastLatencyOperation NOTIFY latencyChanged)
  Q_PROPERTY(int lastLatencyMs READ lastLatencyMs NOTIFY latencyChanged)
  // Theme and scaling properties exposed to QML.
  Q_PROPERTY(double uiScale READ uiScale NOTIFY uiScaleChanged)
  Q_PROPERTY(QColor bgColor READ bgColor NOTIFY themeChanged)
  Q_PROPERTY(QColor surfaceColor READ surfaceColor NOTIFY themeChanged)
  Q_PROPERTY(QColor sidebarColor READ sidebarColor NOTIFY themeChanged)
  Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)
  /// Display title derived from project name/path state.
  Q_PROPERTY(QString displayProjectTitle READ displayProjectTitle NOTIFY displayProjectTitleChanged)
  /// Whether the first-run configuration wizard has completed.
  Q_PROPERTY(bool configWizardCompleted READ configWizardCompleted WRITE setConfigWizardCompleted NOTIFY configWizardCompletedChanged)
  // TabPosition enum values are mirrored as integer properties for QML.
  // Q_ENUM can be fragile through context-property access in Qt 6.10.
  // Keep integer aliases so QML can compare page ids without enum lookup issues.
  // Aligned with upstream MainFrame tab ids.
  Q_PROPERTY(int tpHome READ tpHome CONSTANT)
  Q_PROPERTY(int tp3DEditor READ tp3DEditor CONSTANT)
  Q_PROPERTY(int tpPreview READ tpPreview CONSTANT)
  Q_PROPERTY(int tpDevice READ tpDevice CONSTANT)
  Q_PROPERTY(int tpMultiDevice READ tpMultiDevice CONSTANT)
  Q_PROPERTY(int tpProject READ tpProject CONSTANT)
  Q_PROPERTY(int tpCalibration READ tpCalibration CONSTANT)
  Q_PROPERTY(int tpPlaceholder1 READ tpPlaceholder1 CONSTANT)
  Q_PROPERTY(int tpPreferences READ tpPreferences CONSTANT)
  Q_PROPERTY(int tpPlaceholder2 READ tpPlaceholder2 CONSTANT)
  // Phase 3: ViewMode ids mirror Plater canvas modes.
  // Upstream-aligned QML API.
  Q_PROPERTY(int vmView3D READ vmView3D CONSTANT)
  Q_PROPERTY(int vmPreview READ vmPreview CONSTANT)
  Q_PROPERTY(int vmAssembleView READ vmAssembleView CONSTANT)
  /// Current Plater view mode derived from page and canvas state.
  Q_PROPERTY(int currentViewMode READ currentViewMode NOTIFY currentViewModeChanged)

  // Phase 4: persisted sidebar state mirrors upstream collapsed-sidebar behavior.
  // Sidebar state is persisted under the owzx/sidebar QSettings namespace.
  Q_PROPERTY(bool sidebarCollapsed READ sidebarCollapsed NOTIFY sidebarCollapsedChanged)
  Q_PROPERTY(int sidebarWidth READ sidebarWidth NOTIFY sidebarWidthChanged)
  Q_PROPERTY(int sidebarDockArea READ sidebarDockArea NOTIFY sidebarDockAreaChanged)
  // QML-facing constant accessors.
  Q_PROPERTY(int sidebarMinWidth READ sidebarMinWidth CONSTANT)
  Q_PROPERTY(int sidebarMaxWidth READ sidebarMaxWidth CONSTANT)
  Q_PROPERTY(int sdaLeft READ sdaLeft CONSTANT)
  Q_PROPERTY(int sdaRight READ sdaRight CONSTANT)

public:
  /// TabPosition mirrors upstream MainFrame tab ids.
  /// OWzx renames monitor and reserves auxiliary/debug pages as placeholders.
  /// Navigation requests follow upstream Notebook tab selection semantics.
  enum class TabPosition
  {
    tpHome = 0,
    tp3DEditor = 1,
    tpPreview = 2,
    tpDevice = 3,         // upstream: tpMonitor; OWzx rename (CONTEXT.md D-ARCH-02)
    tpMultiDevice = 4,
    tpProject = 5,
    tpCalibration = 6,
    tpPlaceholder1 = 7,   // upstream: tpAuxiliary; reserved for future use
    tpPreferences = 8,    // upstream PreferencesDialog, hosted as a Qt page
    tpPlaceholder2 = tpPreferences, // compatibility alias for the former slot name
  };
  Q_ENUM(TabPosition)

  /// Plater canvas mode exposed to QML.
  /// Values follow upstream view3D / preview / assemble_view canvas modes.
  /// AssembleView remains out of scope for this milestone.
  enum class ViewMode
  {
    View3D = 0, ///< Prepare 3D canvas.
    Preview = 1, ///< G-code preview canvas.
    AssembleView = 2, ///< Assemble canvas placeholder.
  };
  Q_ENUM(ViewMode)

  /// Sidebar dock area exposed for persisted Prepare sidebar placement.
  /// Left is the upstream default; right docking is retained for persistence.
  /// Dragging between dock areas is out of scope for this milestone.
  enum class SidebarDockArea
  {
    Left = 0,
    Right = 1,
  };
  Q_ENUM(SidebarDockArea)

  // QML-friendly accessors returning the enum value as int (Q_PROPERTY constants)
  int tpHome() const { return static_cast<int>(TabPosition::tpHome); }
  int tp3DEditor() const { return static_cast<int>(TabPosition::tp3DEditor); }
  int tpPreview() const { return static_cast<int>(TabPosition::tpPreview); }
  int tpDevice() const { return static_cast<int>(TabPosition::tpDevice); }
  int tpMultiDevice() const { return static_cast<int>(TabPosition::tpMultiDevice); }
  int tpProject() const { return static_cast<int>(TabPosition::tpProject); }
  int tpCalibration() const { return static_cast<int>(TabPosition::tpCalibration); }
  int tpPlaceholder1() const { return static_cast<int>(TabPosition::tpPlaceholder1); }
  int tpPreferences() const { return static_cast<int>(TabPosition::tpPreferences); }
  int tpPlaceholder2() const { return tpPreferences(); }

  // QML-friendly ViewMode accessors (Q_PROPERTY constants)
  int vmView3D() const { return static_cast<int>(ViewMode::View3D); }
  int vmPreview() const { return static_cast<int>(ViewMode::Preview); }
  int vmAssembleView() const { return static_cast<int>(ViewMode::AssembleView); }
  int currentViewMode() const { return static_cast<int>(currentViewMode_); }

  // Phase 90 (ASMROUTE-01): Plater-level canvas-type routing helper.
  // Mirrors upstream get_current_canvas3D()->get_canvas_type()
  // (GLCanvas3D.hpp:509-513 ECanvasType). The ViewMode enum is the routing
  // anchor: View3D<->CanvasView3D(0), Preview<->CanvasPreview(1),
  // AssembleView<->CanvasAssembleView(2). Exposed as an int (matching
  // RhiViewport::CanvasType values) so EditorViewModel can branch selection /
  // gizmo / undo-redo routing on the active canvas without a layering
  // dependency on RhiViewport.h.
  int currentCanvasType() const { return static_cast<int>(currentViewMode_); }

  // Phase 4: Sidebar Dockable accessors
  bool sidebarCollapsed() const { return sidebarCollapsed_; }
  int sidebarWidth() const { return sidebarWidth_; }
  int sidebarDockArea() const { return static_cast<int>(sidebarDockArea_); }
  int sidebarMinWidth() const { return kSidebarMinWidth; }
  int sidebarMaxWidth() const { return kSidebarMaxWidth; }
  int sdaLeft() const { return static_cast<int>(SidebarDockArea::Left); }
  int sdaRight() const { return static_cast<int>(SidebarDockArea::Right); }

  explicit BackendContext(QObject *parent = nullptr);

  QObject *editorViewModel() const;
  QObject *previewViewModel() const;
  QObject *monitorViewModel() const;
  QObject *configViewModel() const;
  QObject *homeViewModel() const;
  QObject *settingsViewModel() const;
  QObject *projectViewModel() const;
  QObject *calibrationViewModel() const;
  QObject *multiMachineViewModel() const;
  QObject *amsMaterialsViewModel() const;
  /// Plugin registry + persisted enable/install state (Phase 202). Mock data
  /// source; installPlugin is a state flip, not a real download.
  QObject *pluginService() const;
  /// Phase 199 (WIZ-01): preset data service for ConfigWizard enumeration.
  QObject *presetServiceMock() const;
  QObject *sliceService() const;
  QObject *appSettings() const;
  /// Camera image provider used by Monitor preview surfaces.
  CameraServiceMock *cameraService() const { return cameraService_; }
  bool visualCompareMode() const;

  // AI 助手（OWzx-only，docs/ai-control.md）
  QObject *aiViewModel() const;
  QObject *aiChatBridge() const;
  // Chat panel wiring: QML hands over its QQmlWebChannel element (the only
  // channel type WebEngineView.webChannel accepts); the bridge is then
  // registered through the public QWebChannel base API. QML-side
  // registeredObjects cannot be used — in Qt 6 it requires the attached
  // WebChannel.id property on the registered object, which a C++-owned
  // bridge cannot declare.
  Q_INVOKABLE void attachAiChatChannel(QObject *channel);
  bool aiControlActive() const { return aiControlActive_; }
  /// (Re)applies the AI preferences: starts/stops the loopback MCP server and
  /// the sidecar harness. Called from the constructor and on
  /// SettingsViewModel::settingsChanged.
  void applyAiSettings();
  // ── OWzx::AppToolUiProvider (drives the registry's UI tools) ─────────────
  bool switchPage(int position) override;
  bool toggleSidebar() override;
  int currentPage() const;  // also implements AppToolUiProvider::currentPage
  // Phase 51: shell action gates (forward to EditorViewModel/PreviewViewModel)
  bool canImport() const;
  bool canSlice() const;
  bool isSlicing() const;
  bool canExport() const;
  bool canSave() const;
  bool canUndo() const;
  bool canRedo() const;
  bool isBusy() const;
  QString exportActionLabel() const;
  QString exportActionHint() const;
  QString saveActionLabel() const;
  QString saveActionHint() const;
  /// Set current Plater canvas mode and emit currentViewModeChanged.
  void setCurrentViewMode(int mode);
  double uiScale() const { return m_uiScale; }
  QColor bgColor() const { return m_bgColor; }
  QColor surfaceColor() const { return m_surfaceColor; }
  QColor sidebarColor() const { return m_sidebarColor; }
  QColor borderColor() const { return m_borderColor; }
  QString displayProjectTitle() const;

  Q_INVOKABLE void setCurrentPage(int page);
  /// Request a top-level tab change.
  /// Emits tabSelectRequested before updating currentPage.
  /// Invalid tab positions are rejected with a warning.
  Q_INVOKABLE void requestSelectTab(int position);
  /// Request a Plater canvas view-mode change.
  /// Emits viewModeChangeRequested before updating currentViewMode.
  Q_INVOKABLE void requestChangeViewMode(int mode);
  // Phase 4: dockable sidebar state aligns with upstream collapsed sidebar behavior.
  Q_INVOKABLE void requestToggleSidebar();           ///< Toggle sidebar collapsed state.
  Q_INVOKABLE void requestSetSidebarCollapsed(bool c);  ///< Set persisted sidebar collapsed state.
  Q_INVOKABLE void requestSetSidebarWidth(int w);       ///< Set persisted sidebar width clamped to [min,max].
  Q_INVOKABLE void requestSetSidebarDockArea(int area); ///< Set persisted sidebar dock area.
  Q_INVOKABLE void postError(const QString &message, int severity = 0);
  Q_INVOKABLE void postNotification(const QString &message, const QString &title = {}, int severity = 0);
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void dismissNotification();
  /// Phase 240 (NOTI-01): dismiss one stacked notification by its id (the
  /// QML toast delegates call this from their per-entry close button /
  /// auto-dismiss timer). Upstream equivalent: PopNotification::close().
  Q_INVOKABLE void dismissNotificationById(int id);
  Q_INVOKABLE void confirmNotificationById(int id);
  Q_INVOKABLE void cancelNotificationById(int id);

  /// Convenience notification helpers aligned with upstream NotificationManager.
  Q_INVOKABLE void postSlicingProgress(int percent, const QString &stage = {});
  Q_INVOKABLE void postSlicingComplete(); ///< Post slicing-complete notification.
  Q_INVOKABLE void postExportFinished(const QString &filePath);
  Q_INVOKABLE void postExportOngoing(const QString &stage = {});
  Q_INVOKABLE void postPlaterWarning(const QString &message);
  Q_INVOKABLE void postPlaterError(const QString &message);
  Q_INVOKABLE void postValidateError(const QString &message);
  Q_INVOKABLE void postValidateWarning(const QString &message);
  Q_INVOKABLE void postArrangeOngoing(int percent);

  /// Post a hint notification aligned with upstream DidYouKnowHint behavior.
  Q_INVOKABLE void postHint();
  Q_INVOKABLE void nextHint();
  Q_INVOKABLE void prevHint();
  /// Phase 241 (PAGE-01): advance the hint cursor WITHOUT posting a
  /// notification — the HomePage Daily Tips card drives the same hint
  /// database (hints.json, upstream DailyTips/MarkdownTip) through this
  /// entry point. Emits dailyTipChanged.
  Q_INVOKABLE void showDailyTip();
  Q_INVOKABLE int hintCount() const;
  Q_INVOKABLE int currentHintIndex() const;
  Q_INVOKABLE QString currentHintText() const;
  Q_INVOKABLE QString currentHintHypertext() const;
  Q_INVOKABLE QString currentHintFollowText() const;
  /// Show the current hint documentation link.
  Q_INVOKABLE bool openHintDocumentation() const;
  /// Upstream-aligned QML API.
  Q_INVOKABLE bool currentHintHasDocumentationLink() const;

  /// Phase 241 (PAGE-04): apply the persisted startup-page preference
  /// (showHomePage + defaultPage, upstream app_config "show_home_page" /
  /// "default_page") to currentPage. Called once from main_qml.cpp after
  /// BackendContext construction, before QML loads.
  Q_INVOKABLE void applyStartupPagePreference();
  /// Phase 241 (PAGE-04): write one project backup into the app-data backup
  /// directory (upstream backup_switch "Auto-Backup ... for restoring from
  /// the occasional crash", Preferences.cpp:1179 — OWzx delta: periodic
  /// save-on-change backup at minute granularity instead of the upstream
  /// seconds-level crash timer). Returns the backup file path, or an empty
  /// string when there is nothing to back up / the save failed.
  Q_INVOKABLE QString triggerProjectBackup();

  Q_INVOKABLE void openSettings(); // H3
  // Phase 52 PREPSB-02: forward sidebar settings request (interim no-op log;
  // Phase 56 wires the independent dialog). category: "printer"/"filament"/"process".
  Q_INVOKABLE void forwardSettingsRequest(const QString &category);
  /// Upstream-aligned QML API.
  Q_INVOKABLE void showConfigWizard();
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showBedShapeDialog();
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showEditGCodeDialog(const QString &key = {}, const QString &value = {});
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showAMSSettingsDialog();
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showFirmwareDialog();
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showSpeedLimitDialog();
  /// Upstream-aligned QML API.
  Q_INVOKABLE void showWipeTowerDialog();
  /// Request a QML-owned dialog or workflow surface.
  Q_INVOKABLE void showPrintHostDialog();
  /// Open the plugin manager dialog (Phase 202: backed by pluginService_).
  Q_INVOKABLE void showPluginManagerDialog();
  /// Show lite-mode dialog placeholder until QML implements EnableLiteModeDialog.
  Q_INVOKABLE void showEnableLiteModeDialog();
  /// Phase 236 (DLG-01): request the Export Preset Bundle dialog
  /// (File > Export Preset Bundle, upstream ExportPresetBundleDialog).
  Q_INVOKABLE void showExportPresetBundleDialog();
  /// Phase 236 (DLG-03): request the system information dialog
  /// (Help menu, upstream SysInfoDialog).
  Q_INVOKABLE void showSysInfoDialog();
  /// Phase 236 (DLG-03): runtime environment dump for SysInfoDialog —
  /// compile-time constants, graphics API / surface format / GL strings
  /// (when a context is current), and key configuration paths.
  Q_INVOKABLE QVariantMap systemInfo() const;
  bool configWizardCompleted() const;
  void setConfigWizardCompleted(bool completed);
  Q_INVOKABLE void topbarNewProject();
  Q_INVOKABLE bool topbarOpenProject(const QString &filePath);
  Q_INVOKABLE bool topbarImportModel(const QString &filePath);
  /// Phase 237 (VIEW-02): Import Configs entry (upstream MainFrame::
  /// load_config_file, MainFrame.cpp:3204-3247). Loads a .json preset bundle
  /// through PresetServiceMock::importBundle and posts an import-result
  /// notification. Returns true when at least one preset was imported.
  Q_INVOKABLE bool topbarImportConfigs(const QString &filePath);
  Q_INVOKABLE bool topbarSaveProject();
  Q_INVOKABLE bool topbarSaveProjectAs(const QString &filePath);
  Q_INVOKABLE int beginLatency(const QString &operation, const QString &detail = QString());
  Q_INVOKABLE void endLatency(int token);
  Q_INVOKABLE void recordLatency(const QString &operation, int elapsedMs, const QString &detail = QString());
  Q_INVOKABLE void resetLatency();

  // ── Dead-control elimination: real behaviors for the former placeholder
  // menu items / buttons (upstream MainFrame help menu + NetworkTestDialog +
  // PrintHostDialog test). ──
  /// Upstream Help > Check for Update (MainFrame.cpp:2160,
  /// check_new_version_sf): async GitHub latest-release query; posts a
  /// notification and emits updateCheckFinished with the comparison result.
  Q_INVOKABLE void checkForUpdates();
  /// True while a checkForUpdates round-trip is in flight (drives busy state
  /// on the Help item and the Preferences "check now" button).
  Q_PROPERTY(bool updateCheckRunning READ updateCheckRunning NOTIFY updateCheckRunningChanged)
  bool updateCheckRunning() const { return updateCheckRunning_; }
  /// Upstream Help > Show Configuration Folder (MainFrame.cpp:2146,
  /// desktop_open_datadir_folder): opens the writable AppData folder.
  Q_INVOKABLE bool openConfigFolder();
  /// Upstream NetworkTestDialog connectivity probe: DNS resolve + HTTPS GET
  /// latency. Emits networkTestFinished.
  Q_INVOKABLE void runNetworkTest();
  /// Upstream PrintHostDialog "test connection": HTTP GET against the
  /// configured host. Emits printHostTestFinished.
  Q_INVOKABLE void testPrintHost(const QString &hostUrl);

  QString lastErrorMessage() const;
  int lastErrorSeverity() const;
  QString lastErrorTitle() const;
  int pendingNotificationCount() const;
  int currentNotificationProgress() const;
  bool currentNotificationHasProgress() const;
  bool currentNotificationPersistent() const;
  int currentNotificationType() const;
  bool currentNotificationShowExport() const;
  bool currentNotificationShowPreview() const;
  /// Notification state changed.
  Q_INVOKABLE int historyCount() const;
  Q_INVOKABLE int unreadHistoryCount() const;
  Q_INVOKABLE QString historyMessage(int index) const;
  Q_INVOKABLE QString historyTitle(int index) const;
  Q_INVOKABLE int historySeverity(int index) const;
  Q_INVOKABLE QString historyTime(int index) const;
  Q_INVOKABLE void clearHistory();
  Q_INVOKABLE void markHistoryRead();
  /// Phase 240 (NOTI-01): the visible notification stack, most important
  /// first (see the notificationStack Q_PROPERTY). Each element is a
  /// QVariantMap: id/message/title/severity/type/persistent/hasProgress/
  /// progressValue/repeatCount/showExportButton/showPreviewButton.
  QVariantList notificationStack() const;
  /// Phase 240 (NOTI-01): number of simultaneously visible notifications.
  int visibleNotificationCount() const { return m_activeNotifications.size(); }
  bool notificationsEnabled() const { return m_notificationsEnabled; }
  void setNotificationsEnabled(bool v);
  bool hintsEnabled() const { return m_hintsEnabled; }
  void setHintsEnabled(bool v);
  int autoDismissSec() const { return m_autoDismissSec; }
  void setAutoDismissSec(int sec);
  bool showProgressNotifications() const { return m_showProgressNotifications; }
  void setShowProgressNotifications(bool v);
  QString latencyBrief() const;
  QString lastLatencyOperation() const;
  int lastLatencyMs() const;

signals:
  void currentPageChanged();
  /// Phase 51 SHELL-03: bulk shell-state refresh signal for action gates + labels.
  void stateChanged();
  /// Phase 241 (PAGE-01): the hint cursor moved (showDailyTip / nextHint /
  /// prevHint). The HomePage Daily Tips card rebinds on this signal.
  void dailyTipChanged();
  /// Emitted when a tab selection is requested.
  /// Current Plater view mode derived from page and canvas state.
  void tabSelectRequested(int position);
  /// Upstream-aligned QML API.
  void viewModeChangeRequested(int mode);
  /// Emitted when the Plater view mode changes.
  void currentViewModeChanged();
  // Phase 4: dockable sidebar state changed.
  void sidebarCollapsedChanged();
  void sidebarWidthChanged();
  void sidebarDockAreaChanged();
  void errorChanged();
  void latencyChanged();
  void uiScaleChanged();
  void themeChanged();
  void languageChanged();
  void displayProjectTitleChanged();
  void historyChanged();
  void settingsChanged();
  void configWizardCompletedChanged();
  void showConfigWizardRequested();
  /// Phase 52 PREPSB-02 + Phase 56: request to open an independent
  /// settings dialog for a category ("printer" / "filament" / "process").
  /// BackendContext::forwardSettingsRequest() calls setActivePresetTier(category)
  /// FIRST, then emits this signal. QML side connects to show the dialog.
  void settingsRequested(const QString &category);
  void showBedShapeDialogRequested();
  void showEditGCodeDialogRequested(const QString &key, const QString &value);
  void showAMSSettingsDialogRequested();
  void showFirmwareDialogRequested();
  void showSpeedLimitDialogRequested();
  void showWipeTowerDialogRequested();
  void showPrintHostDialogRequested();
  void showPluginManagerDialogRequested();
  void showEnableLiteModeDialogRequested();
  /// Phase 236 (DLG-01): open the Export Preset Bundle dialog.
  void showExportPresetBundleDialogRequested();
  /// Phase 236 (DLG-03): open the system information dialog.
  void showSysInfoDialogRequested();
  /// Phase 236 (DLG-03): objects landed outside the bed after a load/drop —
  /// open the RecenterDialog (upstream outside-bed prompt).
  void recenterPromptRequested();
  /// Phase 237 (VIEW-04): a freshly imported object tripped the upstream
  /// saved-unit heuristic — the shell opens the unit-conversion confirm.
  /// unitHint: 1 = meters (x1000), 2 = imperial (x25.4).
  void unitConversionPromptRequested(int objectIndex, int unitHint, const QString &objectName);
  void exportGCodeRequested();
  /// Dead-control elimination: result of checkForUpdates (GitHub latest
  /// release vs the running version). ok=false carries the failure reason.
  void updateCheckFinished(bool ok, bool updateAvailable, const QString &message);
  /// Dead-control elimination: NetworkTestDialog probe result.
  void networkTestFinished(bool dnsOk, bool online, int latencyMs, const QString &detail);
  /// Dead-control elimination: PrintHostDialog connection test result.
  void printHostTestFinished(bool ok, const QString &detail);
  void updateCheckRunningChanged();

private:
  /// Shared async helper state for the update / host / network probes.
  QNetworkAccessManager *probeNam_ = nullptr;
  bool updateCheckRunning_ = false;
  QNetworkAccessManager *probeNam();
  void setUpdateCheckRunning(bool running);
  CalibrationServiceMock *calibrationService_ = nullptr;
  SliceService *sliceService_ = nullptr;
  PresetServiceMock *presetService_ = nullptr;
  DeviceServiceMock *deviceService_ = nullptr;
  ProjectServiceMock *projectService_ = nullptr;
  NetworkServiceMock *networkService_ = nullptr;
  /// Upstream-aligned QML API.
  CameraServiceMock *cameraService_ = nullptr;
  /// v2.8 W3: application-level persisted settings, including bed size.
  AppSettingsService *appSettings_ = nullptr;
  /// Phase 202 (v5.6 Plugin Manager UI Real Backend): plugin registry +
  /// persisted install/enable state. Mock data; installPlugin is a state flip.
  PluginService *pluginService_ = nullptr;

  // AI 助手（OWzx-only，docs/ai-control.md）
  /// AppToolRegistry: whole-app tool surface over the VMs/services above.
  OWzx::AppToolRegistry *aiRegistry_ = nullptr;
  /// Loopback MCP server exposing the registry (started per preferences).
  OWzx::McpHttpServer *aiMcp_ = nullptr;
  /// Sidecar harness host (Python + Claude Agent SDK + GLM).
  AiAgentService *aiAgentService_ = nullptr;
  AiViewModel *aiViewModel_ = nullptr;
  /// QWebChannel bridge for the WebEngine chat page (web chat UI).
  AiChatBridge *aiChatBridge_ = nullptr;
  bool aiControlActive_ = false;


  EditorViewModel *editorViewModel_ = nullptr;
  PreviewViewModel *previewViewModel_ = nullptr;
  MonitorViewModel *monitorViewModel_ = nullptr;
  ConfigViewModel *configViewModel_ = nullptr;
  HomeViewModel *homeViewModel_ = nullptr;
  SettingsViewModel *settingsViewModel_ = nullptr;
  ProjectViewModel *projectViewModel_ = nullptr;
  CalibrationViewModel *calibrationViewModel_ = nullptr;
  MultiMachineViewModel *multiMachineViewModel_ = nullptr;
  AmsMaterialsViewModel *amsMaterialsViewModel_ = nullptr;

  bool visualCompareMode_ = false;
  int currentPage_ = 1;
  /// Phase 3: current Plater view mode. Default is View3D.
  ViewMode currentViewMode_ = ViewMode::View3D;
  // Phase 4: dockable sidebar state is loaded from QSettings and saved by setters.
  // Phase 164 (SW-01, v5.2): bumped version + unbroken the 7-layer 392px lock —
  // min/max are now real bounds (was min==max==392, which made the
  // DockableSidebar drag handle a visible no-op per Panels-UI-REVIEW). Default
  // stays 392 to preserve the current visual; users can now resize within
  // [300, 520]. The previous "screenshot Prepare sidebar width" comment was
  // misleading — Phase 74 UI-SPEC mandates "compact" not 392px specifically.
  static constexpr int kSidebarSettingsVersion = 4; ///< Width persistence contract version.
  static constexpr int kSidebarMinWidth = 300;
  static constexpr int kSidebarMaxWidth = 520;
  static constexpr int kSidebarDefaultWidth = 320;
  bool sidebarCollapsed_ = false;
  int sidebarWidth_ = kSidebarDefaultWidth;
  SidebarDockArea sidebarDockArea_ = SidebarDockArea::Left;
  QString lastErrorMessage_;
  QString lastErrorTitle_;
  int lastErrorSeverity_ = -1;

  struct NotificationEntry
  {
    /// Phase 240 (NOTI-01): unique id so a stacked toast delegate can dismiss
    /// exactly its own entry (mirrors upstream NotificationIDProvider ids,
    /// NotificationManager.hpp:302-322).
    int id = 0;
    /// Phase 240 (NOTI-01): duplicate-compression counter. Upstream keeps a
    /// per-type counter on UpdatedItemsInfoNotification (m_types_and_counts,
    /// NotificationManager.hpp:818) and escalates repeated texts by updating
    /// the existing entry instead of pushing a new one (activate_existing,
    /// NotificationManager.cpp:2643-2675). repeatCount >= 2 makes the UI show
    /// an "xN" escalation badge.
    int repeatCount = 1;
    QString message;
    QString title;
    int severity = 0;                 ///< NotificationLevel
    int type = NotiTypeCustom;         ///< NotificationType
    bool persistent = false; ///< Persistent notifications stay visible until dismissed.
    int progressValue = 0; ///< Progress percentage from 0 to 100.
    int progressMin = 0;
    int progressMax = 100;
    bool hasProgress = false; ///< Whether the notification carries progress.
    bool requiresConfirm = false; ///< Whether the notification requires confirmation.
    int confirmAction = 0; ///< Confirmation action identifier.
    QDateTime timestamp; ///< Notification creation timestamp.
    /// Upstream-aligned QML API.
    bool showExportButton = false;
    bool showPreviewButton = false;
    /// Upstream-aligned QML API.
    bool hintHasNext = false;
    bool hintHasPrev = false;
  };
  // Phase 240 (NOTI-01): the visible stack. Upstream keeps every live
  // notification in m_pop_notifications (NotificationManager.hpp:303) and
  // renders them all at once; the previous Qt6 single-slot queue showed only
  // one at a time. Entries beyond kMaxVisibleNotifications park in
  // m_notificationQueue until a slot frees (screen-space bound, same reason
  // upstream clamps the ImGui stack to the canvas height).
  static constexpr int kMaxVisibleNotifications = 6;
  QVector<NotificationEntry> m_activeNotifications;
  int m_nextNotificationId = 1;
  QQueue<NotificationEntry> m_notificationQueue;
  NotificationEntry m_currentNotification; ///< Newest entry (compat view for the legacy single-toast API).
  QVector<NotificationEntry> m_notificationHistory; ///< Dismissed notification history.
  int m_unreadHistoryCount = 0;
  void showNextNotification();
  /// Phase 240 (NOTI-01): single push path for every post* helper. Performs
  /// duplicate compression (activate_existing semantics), importance
  /// ordering (sort_notifications semantics), overflow queueing, and the
  /// legacy current/lastError* sync. Emits errorChanged (+historyChanged
  /// when a duplicate escalated entry refreshes the history side data).
  void pushNotificationEntry(NotificationEntry entry, bool dedupByTypeOnly);
  /// Phase 240 (NOTI-01): upstream NotificationLevel ordering
  /// (NotificationManager.hpp:158-180) projected onto the Qt NotiLevel enum:
  /// progress < hint < regular < printInfo < printInfoShort < important
  /// (persistent) < warning < seriousWarning < error. Higher = more
  /// important = rendered closer to the top of the stack.
  static int notificationImportanceRank(const NotificationEntry &e);
  /// Phase 240 (NOTI-01): sync m_currentNotification + lastError* to the
  /// newest active entry (insertion order, NOT importance order) so the
  /// legacy single-toast getters keep their "last post wins" meaning.
  void syncCurrentNotificationFromStack();
  /// Phase 240 (NOTI-01): move one entry into the history ring (cap 100,
  /// same as before) + unread counter.
  void archiveNotification(const NotificationEntry &entry);
  /// Update the active notification progress value.
  Q_INVOKABLE void updateNotificationProgress(int value);
  /// Confirm the active notification.
  Q_INVOKABLE void confirmCurrentNotification();
  /// Cancel the active notification.
  Q_INVOKABLE void cancelCurrentNotification();

  // Appearance state.
  QTranslator *m_translator = nullptr;
  double m_uiScale = 1.0;
  QColor m_bgColor{"#0d0f12"};
  QColor m_surfaceColor{"#0f1217"};
  QColor m_sidebarColor{"#0f1218"};
  QColor m_borderColor{"#242a33"};

  // Notification preferences.
  bool m_notificationsEnabled = true;
  bool m_hintsEnabled = true;
  int m_autoDismissSec = 5; ///< Default auto-dismiss timeout for non-persistent notifications.
  bool m_showProgressNotifications = true; ///< Whether progress notifications are shown.

  /// First-run configuration wizard completion flag.
  bool m_configWizardCompleted = false;

  /// Hint data exposed to QML notification surfaces.
  QVector<HintData> m_hints;
  int m_currentHintIndex = -1;
  QSet<QString> m_displayedHintIds;
  QTimer *m_hintTimer = nullptr; ///< Timer for periodic hint notifications.
  /// Phase 241 (PAGE-04): periodic project backup timer. Interval follows
  /// settingsViewModel autoSaveInterval (minutes); each tick backs up the
  /// dirty project when autoSave is enabled.
  QTimer *m_backupTimer = nullptr;
  void initHintDatabase();
  void initFallbackHintDatabase();
  int selectNextHint(bool random = true);

  struct PendingLatency
  {
    QString operation;
    QString detail;
    qint64 startMs = 0;
  };

  struct OpLatencyStats
  {
    int count = 0;
    int totalMs = 0;
    int maxMs = 0;
    int lastMs = 0;
    QVector<int> samples;
  };

  QElapsedTimer m_latencyClock;
  int m_latencyNextToken = 1;
  QHash<int, PendingLatency> m_pendingLatencies;
  QHash<QString, OpLatencyStats> m_latencyStats;
  QString m_lastLatencyOperation;
  int m_lastLatencyMs = 0;

  void pushLatencySample(const QString &operation, int elapsedMs, const QString &detail);
  static int percentile95(const QVector<int> &samples);

  void applyTheme(int idx);
  void applyLanguage(int idx);
  void applyFontSize(int size);
  void applyUiScale(int idx);
};
