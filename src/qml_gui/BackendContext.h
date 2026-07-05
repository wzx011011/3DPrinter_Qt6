#pragma once

#include <QObject>
#include <QColor>
#include <QTranslator>
#include <QElapsedTimer>
#include <QHash>
#include <QVector>
#include <QQueue>
#include <QDateTime>
#include <QSettings>

class SliceService;
class PresetServiceMock;
class DeviceServiceMock;
class ProjectServiceMock;
class NetworkServiceMock;
class CalibrationServiceMock;
class CameraServiceMock;
class AuxiliaryService;
class AppSettingsService;

class EditorViewModel;
class PreviewViewModel;
class MonitorViewModel;
class ConfigViewModel;
class HomeViewModel;
class SettingsViewModel;
class ProjectViewModel;
class CalibrationViewModel;
class ModelMallViewModel;
class MultiMachineViewModel;

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

class BackendContext final : public QObject
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
  Q_PROPERTY(QObject *modelMallViewModel READ modelMallViewModel CONSTANT)
  Q_PROPERTY(QObject *multiMachineViewModel READ multiMachineViewModel CONSTANT)
  Q_PROPERTY(QObject *auxiliaryService READ auxiliaryService CONSTANT)
  Q_PROPERTY(QObject *appSettings READ appSettings CONSTANT)
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
    tpPlaceholder2 = 8,   // upstream: toDebugTool; reserved for future use
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
  int tpPlaceholder2() const { return static_cast<int>(TabPosition::tpPlaceholder2); }

  // QML-friendly ViewMode accessors (Q_PROPERTY constants)
  int vmView3D() const { return static_cast<int>(ViewMode::View3D); }
  int vmPreview() const { return static_cast<int>(ViewMode::Preview); }
  int vmAssembleView() const { return static_cast<int>(ViewMode::AssembleView); }
  int currentViewMode() const { return static_cast<int>(currentViewMode_); }

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
  QObject *modelMallViewModel() const;
  QObject *multiMachineViewModel() const;
  QObject *auxiliaryService() const;
  QObject *appSettings() const;
  /// Camera image provider used by Monitor preview surfaces.
  CameraServiceMock *cameraService() const { return cameraService_; }
  bool visualCompareMode() const;

  int currentPage() const;
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
  Q_INVOKABLE int hintCount() const;
  Q_INVOKABLE int currentHintIndex() const;
  Q_INVOKABLE QString currentHintText() const;
  Q_INVOKABLE QString currentHintHypertext() const;
  Q_INVOKABLE QString currentHintFollowText() const;
  /// Show the current hint documentation link.
  Q_INVOKABLE bool openHintDocumentation() const;
  /// Upstream-aligned QML API.
  Q_INVOKABLE bool currentHintHasDocumentationLink() const;

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
  /// Show plugin manager dialog placeholder until QML implements WebDownPluginDlg.
  Q_INVOKABLE void showPluginManagerDialog();
  /// Show lite-mode dialog placeholder until QML implements EnableLiteModeDialog.
  Q_INVOKABLE void showEnableLiteModeDialog();
  bool configWizardCompleted() const;
  void setConfigWizardCompleted(bool completed);
  Q_INVOKABLE void topbarNewProject();
  Q_INVOKABLE bool topbarOpenProject(const QString &filePath);
  Q_INVOKABLE bool topbarImportModel(const QString &filePath);
  Q_INVOKABLE bool topbarSaveProject();
  Q_INVOKABLE bool topbarSaveProjectAs(const QString &filePath);
  Q_INVOKABLE int beginLatency(const QString &operation, const QString &detail = QString());
  Q_INVOKABLE void endLatency(int token);
  Q_INVOKABLE void recordLatency(const QString &operation, int elapsedMs, const QString &detail = QString());
  Q_INVOKABLE void resetLatency();

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
  void exportGCodeRequested();

private:
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

  EditorViewModel *editorViewModel_ = nullptr;
  PreviewViewModel *previewViewModel_ = nullptr;
  MonitorViewModel *monitorViewModel_ = nullptr;
  ConfigViewModel *configViewModel_ = nullptr;
  HomeViewModel *homeViewModel_ = nullptr;
  SettingsViewModel *settingsViewModel_ = nullptr;
  ProjectViewModel *projectViewModel_ = nullptr;
  CalibrationViewModel *calibrationViewModel_ = nullptr;
  ModelMallViewModel *modelMallViewModel_ = nullptr;
  MultiMachineViewModel *multiMachineViewModel_ = nullptr;

  AuxiliaryService *auxiliaryService_ = nullptr;

  bool visualCompareMode_ = false;
  int currentPage_ = 1;
  /// Phase 3: current Plater view mode. Default is View3D.
  ViewMode currentViewMode_ = ViewMode::View3D;
  // Phase 4: dockable sidebar state is loaded from QSettings and saved by setters.
  static constexpr int kSidebarSettingsVersion = 2; ///< Width persistence contract version.
  static constexpr int kSidebarLegacyDefaultWidth = 390; ///< Pre-v3.9 default migrated to compact width.
  static constexpr int kSidebarMinWidth = 312;   ///< Minimum readable Prepare sidebar width.
  static constexpr int kSidebarMaxWidth = 390;   ///< Maximum width before crowding the viewport.
  static constexpr int kSidebarDefaultWidth = 328; ///< Screenshot-ratio compact default sidebar width.
  bool sidebarCollapsed_ = false;
  int sidebarWidth_ = kSidebarDefaultWidth;
  SidebarDockArea sidebarDockArea_ = SidebarDockArea::Left;
  QString lastErrorMessage_;
  QString lastErrorTitle_;
  int lastErrorSeverity_ = -1;

  struct NotificationEntry
  {
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
  QQueue<NotificationEntry> m_notificationQueue;
  NotificationEntry m_currentNotification; ///< Currently displayed notification entry.
  QVector<NotificationEntry> m_notificationHistory; ///< Dismissed notification history.
  int m_unreadHistoryCount = 0;
  void showNextNotification();
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
