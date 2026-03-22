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

/// 通知级别（对齐上游 NotificationLevel）
enum NotificationLevel {
  NotiInfo = 0,           ///< 常规信息
  NotiSuccess = 1,        ///< 成功/完成
  NotiWarning = 2,        ///< 一般警告
  NotiError = 3,          ///< 错误
  NotiSeriousWarning = 4, ///< 严重警告
  NotiHint = 5,           ///< 提示/Did you know
  NotiPrintInfo = 6,      ///< 打印信息
  NotiPrintInfoShort = 7, ///< 短打印信息
  NotiProgress = 8,       ///< 进度通知
  NotiSlicingProgress = 9 ///< 切片进度（含下一步操作按钮）
};

/// 通知类型（对齐上游 NotificationManager::NotificationType）
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

/// 提示数据库条目（对齐上游 HintData）
struct HintData {
  QString id;
  QString text;
  int weight = 1;
  QString hypertext;           ///< 可点击的链接文本
  QString followText;          ///< 跟随链接后的文字
  QString documentationLink;   ///< 文档链接
  QString callbackType;        ///< link / settings / preferences
  QString callbackTarget;      ///< URL 或设置 key
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
  Q_PROPERTY(bool visualCompareMode READ visualCompareMode CONSTANT)
  Q_PROPERTY(int currentPage READ currentPage NOTIFY currentPageChanged)
  Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorChanged)
  Q_PROPERTY(int lastErrorSeverity READ lastErrorSeverity NOTIFY errorChanged)
  Q_PROPERTY(QString lastErrorTitle READ lastErrorTitle NOTIFY errorChanged)
  Q_PROPERTY(int pendingNotificationCount READ pendingNotificationCount NOTIFY errorChanged)
  /// 当前通知进度（供 QML 进度条绑定）
  Q_PROPERTY(int currentNotificationProgress READ currentNotificationProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationHasProgress READ currentNotificationHasProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationPersistent READ currentNotificationPersistent NOTIFY errorChanged)
  Q_PROPERTY(int currentNotificationType READ currentNotificationType NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowExport READ currentNotificationShowExport NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowPreview READ currentNotificationShowPreview NOTIFY errorChanged)
  /// 通知中心（对齐上游 notification_manager 滚动通知区域）
  Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)
  Q_PROPERTY(int unreadHistoryCount READ unreadHistoryCount NOTIFY historyChanged)
  Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(bool hintsEnabled READ hintsEnabled WRITE setHintsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(int autoDismissSec READ autoDismissSec WRITE setAutoDismissSec NOTIFY settingsChanged)
  Q_PROPERTY(bool showProgressNotifications READ showProgressNotifications WRITE setShowProgressNotifications NOTIFY settingsChanged)
  Q_PROPERTY(QString latencyBrief READ latencyBrief NOTIFY latencyChanged)
  Q_PROPERTY(QString lastLatencyOperation READ lastLatencyOperation NOTIFY latencyChanged)
  Q_PROPERTY(int lastLatencyMs READ lastLatencyMs NOTIFY latencyChanged)
  // 外观 / 缩放 / 主题颜色
  Q_PROPERTY(double uiScale READ uiScale NOTIFY uiScaleChanged)
  Q_PROPERTY(QColor bgColor READ bgColor NOTIFY themeChanged)
  Q_PROPERTY(QColor surfaceColor READ surfaceColor NOTIFY themeChanged)
  Q_PROPERTY(QColor sidebarColor READ sidebarColor NOTIFY themeChanged)
  Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)
  /// 项目标题（优先显示 projectName，否则从 projectPath 中提取文件名）
  Q_PROPERTY(QString displayProjectTitle READ displayProjectTitle NOTIFY displayProjectTitleChanged)
  /// 首次配置向导是否已完成（持久化到 QSettings）
  Q_PROPERTY(bool configWizardCompleted READ configWizardCompleted WRITE setConfigWizardCompleted NOTIFY configWizardCompletedChanged)

public:
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
  bool visualCompareMode() const;

  int currentPage() const;
  double uiScale() const { return m_uiScale; }
  QColor bgColor() const { return m_bgColor; }
  QColor surfaceColor() const { return m_surfaceColor; }
  QColor sidebarColor() const { return m_sidebarColor; }
  QColor borderColor() const { return m_borderColor; }
  QString displayProjectTitle() const;

  Q_INVOKABLE void setCurrentPage(int page);
  Q_INVOKABLE void postError(const QString &message, int severity = 0);
  Q_INVOKABLE void postNotification(const QString &message, const QString &title = {}, int severity = 0);
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void dismissNotification();

  /// 专用通知类型（对齐上游 NotificationManager 专用通知）
  Q_INVOKABLE void postSlicingProgress(int percent, const QString &stage = {});
  Q_INVOKABLE void postSlicingComplete();          ///< 切片完成，含导出/预览按钮
  Q_INVOKABLE void postExportFinished(const QString &filePath);
  Q_INVOKABLE void postExportOngoing(const QString &stage = {});
  Q_INVOKABLE void postPlaterWarning(const QString &message);
  Q_INVOKABLE void postPlaterError(const QString &message);
  Q_INVOKABLE void postValidateError(const QString &message);
  Q_INVOKABLE void postValidateWarning(const QString &message);
  Q_INVOKABLE void postArrangeOngoing(int percent);

  /// 提示通知（对齐上游 HintNotification / DidYouKnowHint）
  Q_INVOKABLE void postHint();
  Q_INVOKABLE void nextHint();
  Q_INVOKABLE void prevHint();
  Q_INVOKABLE int hintCount() const;
  Q_INVOKABLE int currentHintIndex() const;
  Q_INVOKABLE QString currentHintText() const;
  Q_INVOKABLE QString currentHintHypertext() const;
  Q_INVOKABLE QString currentHintFollowText() const;
  /// 打开当前提示的文档链接（对齐上游 HintNotification documentation button）
  Q_INVOKABLE bool openHintDocumentation() const;
  /// 当前提示是否有文档链接
  Q_INVOKABLE bool currentHintHasDocumentationLink() const;

  Q_INVOKABLE void openSettings(); // H3
  /// 请求显示首次配置向导（QML 侧触发）
  Q_INVOKABLE void showConfigWizard();
  /// 请求显示热床形状设置对话框（QML 侧触发）
  Q_INVOKABLE void showBedShapeDialog();
  /// 请求显示 G-code 编辑对话框（QML 侧触发，对齐上游 EditGCodeDialog）
  Q_INVOKABLE void showEditGCodeDialog(const QString &key = {}, const QString &value = {});
  /// 请求显示 AMS 设置对话框（QML 侧触发，对齐上游 AMSMaterialsSetting / AMSSetting）
  Q_INVOKABLE void showAMSSettingsDialog();
  /// 请求显示固件升级对话框（QML 侧触发，对齐上游 UpgradePanel / MachineInfoPanel）
  Q_INVOKABLE void showFirmwareDialog();
  /// 请求显示速度限制对话框（QML 侧触发，对齐上游 AccelerationAndSpeedLimitDialog）
  Q_INVOKABLE void showSpeedLimitDialog();
  /// 请求显示擦料塔设置对话框（QML 侧触发，对齐上游 WipeTowerDialog）
  Q_INVOKABLE void showWipeTowerDialog();
  /// 请求显示打印主机设置对话框（QML 侧触发，对齐上游 PhysicalPrinterDialog）
  Q_INVOKABLE void showPrintHostDialog();
  /// 请求显示插件管理对话框（QML 侧触发，对齐上游 WebDownPluginDlg）
  Q_INVOKABLE void showPluginManagerDialog();
  /// 请求显示精简预览模式对话框（QML 侧触发，对齐上游 EnableLiteModeDialog）
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
  /// 通知中心
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
  void showBedShapeDialogRequested();
  void showEditGCodeDialogRequested(const QString &key, const QString &value);
  void showAMSSettingsDialogRequested();
  void showFirmwareDialogRequested();
  void showSpeedLimitDialogRequested();
  void showWipeTowerDialogRequested();
  void showPrintHostDialogRequested();
  void showPluginManagerDialogRequested();
  void showEnableLiteModeDialogRequested();

private:
  CalibrationServiceMock *calibrationService_ = nullptr;
  SliceService *sliceService_ = nullptr;
  PresetServiceMock *presetService_ = nullptr;
  DeviceServiceMock *deviceService_ = nullptr;
  ProjectServiceMock *projectService_ = nullptr;
  NetworkServiceMock *networkService_ = nullptr;

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

  bool visualCompareMode_ = false;
  int currentPage_ = 1;
  QString lastErrorMessage_;
  QString lastErrorTitle_;
  int lastErrorSeverity_ = -1;

  struct NotificationEntry
  {
    QString message;
    QString title;
    int severity = 0;                 ///< NotificationLevel
    int type = NotiTypeCustom;         ///< NotificationType
    bool persistent = false;           ///< true = 不自动关闭，需用户关闭
    int progressValue = 0;            ///< 进度条当前值 (0-100)
    int progressMin = 0;
    int progressMax = 100;
    bool hasProgress = false;          ///< 是否显示进度条
    bool requiresConfirm = false;      ///< 是否需要用户确认
    int confirmAction = 0;             ///< 自定义确认动作标识
    QDateTime timestamp;               ///< 通知时间戳（用于历史记录）
    /// 切片完成后操作按钮（对齐上游 SlicingProgressNotification）
    bool showExportButton = false;
    bool showPreviewButton = false;
    /// 提示导航（对齐上游 HintNotification next/prev）
    bool hintHasNext = false;
    bool hintHasPrev = false;
  };
  QQueue<NotificationEntry> m_notificationQueue;
  NotificationEntry m_currentNotification; ///< 当前显示的通知（支持进度更新）
  QVector<NotificationEntry> m_notificationHistory; ///< 已关闭的通知历史
  int m_unreadHistoryCount = 0;
  void showNextNotification();
  /// 更新当前通知的进度值（对齐上游 notification_manager 进度通知）
  Q_INVOKABLE void updateNotificationProgress(int value);
  /// 确认当前需要确认的通知（对齐上游 notification_manager confirm）
  Q_INVOKABLE void confirmCurrentNotification();
  /// 取消当前需要确认的通知
  Q_INVOKABLE void cancelCurrentNotification();

  // 外观状态
  QTranslator *m_translator = nullptr;
  double m_uiScale = 1.0;
  QColor m_bgColor{"#0d0f12"};
  QColor m_surfaceColor{"#0f1217"};
  QColor m_sidebarColor{"#0f1218"};
  QColor m_borderColor{"#242a33"};

  // 通知偏好（对齐上游 notification_manager preferences）
  bool m_notificationsEnabled = true;
  bool m_hintsEnabled = true;
  int m_autoDismissSec = 5;           ///< 默认自动消失秒数（非 persistent 通知）
  bool m_showProgressNotifications = true; ///< 显示切片进度通知

  /// 首次配置向导状态（对齐上游 ConfigWizard 首次运行检测）
  bool m_configWizardCompleted = false;

  /// 提示数据库（对齐上游 HintDatabase）
  QVector<HintData> m_hints;
  int m_currentHintIndex = -1;
  QSet<QString> m_displayedHintIds;
  QTimer *m_hintTimer = nullptr;      ///< 定期显示提示（对齐上游 30s 间隔）
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
