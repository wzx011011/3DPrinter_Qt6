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

/// 閫氱煡绾у埆锛堝榻愪笂娓?NotificationLevel锛?
enum NotificationLevel {
  NotiInfo = 0,           ///< 甯歌淇℃伅
  NotiSuccess = 1,        ///< 鎴愬姛/瀹屾垚
  NotiWarning = 2,        ///< 涓€鑸鍛?
  NotiError = 3,          ///< 閿欒
  NotiSeriousWarning = 4, ///< 涓ラ噸璀﹀憡
  NotiHint = 5,           ///< 鎻愮ず/Did you know
  NotiPrintInfo = 6,      ///< 鎵撳嵃淇℃伅
  NotiPrintInfoShort = 7, ///< 鐭墦鍗颁俊鎭?
  NotiProgress = 8,       ///< 杩涘害閫氱煡
  NotiSlicingProgress = 9 ///< 鍒囩墖杩涘害锛堝惈涓嬩竴姝ユ搷浣滄寜閽級
};

/// 閫氱煡绫诲瀷锛堝榻愪笂娓?NotificationManager::NotificationType锛?
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

/// 鎻愮ず鏁版嵁搴撴潯鐩紙瀵归綈涓婃父 HintData锛?
struct HintData {
  QString id;
  QString text;
  int weight = 1;
  QString hypertext;           ///< 鍙偣鍑荤殑閾炬帴鏂囨湰
  QString followText;          ///< 璺熼殢閾炬帴鍚庣殑鏂囧瓧
  QString documentationLink;   ///< 鏂囨。閾炬帴
  QString callbackType;        ///< link / settings / preferences
  QString callbackTarget;      ///< URL 鎴栬缃?key
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
  Q_PROPERTY(int currentPage READ currentPage NOTIFY currentPageChanged)
  Q_PROPERTY(QString lastErrorMessage READ lastErrorMessage NOTIFY errorChanged)
  Q_PROPERTY(int lastErrorSeverity READ lastErrorSeverity NOTIFY errorChanged)
  Q_PROPERTY(QString lastErrorTitle READ lastErrorTitle NOTIFY errorChanged)
  Q_PROPERTY(int pendingNotificationCount READ pendingNotificationCount NOTIFY errorChanged)
  /// 褰撳墠閫氱煡杩涘害锛堜緵 QML 杩涘害鏉＄粦瀹氾級
  Q_PROPERTY(int currentNotificationProgress READ currentNotificationProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationHasProgress READ currentNotificationHasProgress NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationPersistent READ currentNotificationPersistent NOTIFY errorChanged)
  Q_PROPERTY(int currentNotificationType READ currentNotificationType NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowExport READ currentNotificationShowExport NOTIFY errorChanged)
  Q_PROPERTY(bool currentNotificationShowPreview READ currentNotificationShowPreview NOTIFY errorChanged)
  /// 閫氱煡涓績锛堝榻愪笂娓?notification_manager 婊氬姩閫氱煡鍖哄煙锛?
  Q_PROPERTY(int historyCount READ historyCount NOTIFY historyChanged)
  Q_PROPERTY(int unreadHistoryCount READ unreadHistoryCount NOTIFY historyChanged)
  Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(bool hintsEnabled READ hintsEnabled WRITE setHintsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(int autoDismissSec READ autoDismissSec WRITE setAutoDismissSec NOTIFY settingsChanged)
  Q_PROPERTY(bool showProgressNotifications READ showProgressNotifications WRITE setShowProgressNotifications NOTIFY settingsChanged)
  Q_PROPERTY(QString latencyBrief READ latencyBrief NOTIFY latencyChanged)
  Q_PROPERTY(QString lastLatencyOperation READ lastLatencyOperation NOTIFY latencyChanged)
  Q_PROPERTY(int lastLatencyMs READ lastLatencyMs NOTIFY latencyChanged)
  // 澶栬 / 缂╂斁 / 涓婚棰滆壊
  Q_PROPERTY(double uiScale READ uiScale NOTIFY uiScaleChanged)
  Q_PROPERTY(QColor bgColor READ bgColor NOTIFY themeChanged)
  Q_PROPERTY(QColor surfaceColor READ surfaceColor NOTIFY themeChanged)
  Q_PROPERTY(QColor sidebarColor READ sidebarColor NOTIFY themeChanged)
  Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)
  /// 椤圭洰鏍囬锛堜紭鍏堟樉绀?projectName锛屽惁鍒欎粠 projectPath 涓彁鍙栨枃浠跺悕锛?
  Q_PROPERTY(QString displayProjectTitle READ displayProjectTitle NOTIFY displayProjectTitleChanged)
  /// 棣栨閰嶇疆鍚戝鏄惁宸插畬鎴愶紙鎸佷箙鍖栧埌 QSettings锛?
  Q_PROPERTY(bool configWizardCompleted READ configWizardCompleted WRITE setConfigWizardCompleted NOTIFY configWizardCompletedChanged)
  // 鈹€鈹€ TabPosition 鏋氫妇鍊兼毚闇茬粰 QML 浣滀负鍙 int 灞炴€?
  // 瑙ｅ喅锛歈_ENUM 鍦?Qt 6.10 QML 涓€氳繃 context-property 瀹炰緥璁块棶涓嶇ǔ瀹氾紙"of undefined"锛夛紝
  // 鏀圭敤 Q_PROPERTY(int) 鐩存帴鏆撮湶姣忎釜鏋氫妇鍊笺€俀_ENUM 浠嶄繚鐣欎緵 C++ 渚у厓瀵硅薄浣跨敤銆?
  // 瀵归綈 third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229 TabPosition 鏋氫妇銆?
  Q_PROPERTY(int tpHome READ tpHome CONSTANT)
  Q_PROPERTY(int tp3DEditor READ tp3DEditor CONSTANT)
  Q_PROPERTY(int tpPreview READ tpPreview CONSTANT)
  Q_PROPERTY(int tpDevice READ tpDevice CONSTANT)
  Q_PROPERTY(int tpMultiDevice READ tpMultiDevice CONSTANT)
  Q_PROPERTY(int tpProject READ tpProject CONSTANT)
  Q_PROPERTY(int tpCalibration READ tpCalibration CONSTANT)
  Q_PROPERTY(int tpPlaceholder1 READ tpPlaceholder1 CONSTANT)
  Q_PROPERTY(int tpPlaceholder2 READ tpPlaceholder2 CONSTANT)
  // 鈹€鈹€ Phase 3: ViewMode 鏋氫妇锛圥later 鍐呴儴瑙嗗浘鍒囨崲锛屽榻愪笂娓?view3D/preview/assemble_view 涓?canvas锛?
  // 鍚?TabPosition 妯″紡锛歈_PROPERTY(int) 鏆撮湶姣忎釜鏋氫妇鍊硷紝Q_ENUM 渚?C++ 鍏冨璞′娇鐢ㄣ€?
  Q_PROPERTY(int vmView3D READ vmView3D CONSTANT)
  Q_PROPERTY(int vmPreview READ vmPreview CONSTANT)
  Q_PROPERTY(int vmAssembleView READ vmAssembleView CONSTANT)
  /// 褰撳墠 Plater 瑙嗗浘妯″紡锛堢敱 currentPage 鑱斿姩锛歵p3DEditor鈫扸iew3D锛宼pPreview鈫扨review锛?
  Q_PROPERTY(int currentViewMode READ currentViewMode NOTIFY currentViewModeChanged)

  // 鈹€鈹€ Phase 4: Sidebar Dockable 鐘舵€侊紙瀵归綈涓婃父 collapse_sidebar + 澧炲己 dockArea/width锛夆攢鈹€
  // 鎸佷箙鍖栧埌 QSettings owzx/sidebar/* 锛堝榻愪笂娓?app_config collapsed_sidebar锛?
  Q_PROPERTY(bool sidebarCollapsed READ sidebarCollapsed NOTIFY sidebarCollapsedChanged)
  Q_PROPERTY(int sidebarWidth READ sidebarWidth NOTIFY sidebarWidthChanged)
  Q_PROPERTY(int sidebarDockArea READ sidebarDockArea NOTIFY sidebarDockAreaChanged)
  // 甯搁噺 accessors锛圦ML 鐢紝閬垮厤榄旀硶鏁帮級
  Q_PROPERTY(int sidebarMinWidth READ sidebarMinWidth CONSTANT)
  Q_PROPERTY(int sidebarMaxWidth READ sidebarMaxWidth CONSTANT)
  Q_PROPERTY(int sdaLeft READ sdaLeft CONSTANT)
  Q_PROPERTY(int sdaRight READ sdaRight CONSTANT)

public:
  /// 涓婃父瀵归綈 TabPosition 鏋氫妇锛?:1 鏁板€煎榻?third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.hpp:218-229锛夈€?
  /// OWzx 閲嶅懡鍚嶏細tpMonitor鈫抰pDevice, tpAuxiliary鈫抰pPlaceholder1, toDebugTool鈫抰pPlaceholder2銆?
  /// 鏁板€间笉鍙樹互淇濇寔涓庝笂娓?Notebook.cpp / EVT_SELECT_TAB 瀹屽叏鍏煎銆?
  enum class TabPosition
  {
    tpHome = 0,
    tp3DEditor = 1,
    tpPreview = 2,
    tpDevice = 3,         // upstream: tpMonitor 鈥?OWzx rename (CONTEXT.md D-ARCH-02)
    tpMultiDevice = 4,
    tpProject = 5,
    tpCalibration = 6,
    tpPlaceholder1 = 7,   // upstream: tpAuxiliary 鈥?reserved for future use
    tpPlaceholder2 = 8,   // upstream: toDebugTool 鈥?reserved for future use
  };
  Q_ENUM(TabPosition)

  /// Plater 瑙嗗浘妯″紡锛堝榻愪笂娓?Plater.cpp view3D / preview / assemble_view 涓?canvas 璁捐锛夈€?
  /// 鍒囨崲鍙敼鍙鎬э紝涓嶉攢姣?閲嶅缓缁勪欢锛圓RCH-07 鐘舵€佷繚鐣欏绾︼級銆?
  /// AssembleView 鍦?v2.0 涓?Out of Scope锛屼粎鐣欏崰浣嶃€?
  enum class ViewMode
  {
    View3D = 0,        ///< Prepare 瑙嗗浘锛堝璞＄紪杈?3D 鍦烘櫙锛?
    Preview = 1,       ///< G-code 棰勮锛堝垏鐗囩粨鏋滐級
    AssembleView = 2,  ///< 澶氭澘瑁呴厤瑙嗗浘锛坴2.0 鍗犱綅锛?
  };
  Q_ENUM(ViewMode)

  /// Sidebar dock 鍖哄煙锛圥hase 4 澧炲己锛屼笂娓稿彧鏈?collapse 鏃?dock 鍒囨崲锛夈€?
  /// Left=0 榛樿锛堝榻愪笂娓?Plater 宸︿晶鍥哄畾锛夛紱Right=1锛堝寮猴細鍙垏鍒板彸渚э級銆?
  /// 娴姩绐楀彛 dock Out of Scope锛堜笂娓镐篃娌℃湁锛夈€?
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
  /// v2.6 CAM-03锛氭毚闇?CameraServiceMock锛堜緵 CameraImageProvider 娉ㄥ唽浣跨敤锛?
  CameraServiceMock *cameraService() const { return cameraService_; }
  bool visualCompareMode() const;

  int currentPage() const;
  /// Phase 3: 璁剧疆褰撳墠 Plater 瑙嗗浘妯″紡锛堝幓閲?+ emit currentViewModeChanged锛夈€備緵 requestSelectTab 鑱斿姩 + requestChangeViewMode 澶嶇敤銆?
  void setCurrentViewMode(int mode);
  double uiScale() const { return m_uiScale; }
  QColor bgColor() const { return m_bgColor; }
  QColor surfaceColor() const { return m_surfaceColor; }
  QColor sidebarColor() const { return m_sidebarColor; }
  QColor borderColor() const { return m_borderColor; }
  QString displayProjectTitle() const;
  bool canLeaveSettingsPage() const;

  Q_INVOKABLE void setCurrentPage(int page);
  /// 璇锋眰鍒囨崲鍒版寚瀹?tab锛堝榻愪笂娓?MainFrame::request_select_tab锛孧ainFrame.cpp:3943-3948锛夈€?
  /// 鍏堝箍鎾?tabSelectRequested 淇″彿锛堣鐩戝惉鑰呭湪椤甸潰鍒囨崲鍓嶅搷搴旓級锛屽啀璋冪敤 setCurrentPage銆?
  /// 瓒婄晫浣嶇疆闈欓粯鎷掔粷骞?qWarning锛圥itfall A3 鈥?闃叉 StackLayout currentIndex 瓒婄晫锛夈€?
  Q_INVOKABLE void requestSelectTab(int position);
  /// 璇锋眰鍒囨崲 Plater 瑙嗗浘妯″紡锛堝榻愪笂娓?Plater show_view3D / show_preview锛夈€?
  /// 瓒婄晫/鍚屽€奸潤榛樻嫆缁濄€傚厛骞挎挱 viewModeChangeRequested锛屽啀 setCurrentViewMode銆?
  Q_INVOKABLE void requestChangeViewMode(int mode);
  // Phase 4: Sidebar Dockable 鎿嶄綔锛堝榻愪笂娓?collapse_sidebar + 鎸佷箙鍖栵級
  Q_INVOKABLE void requestToggleSidebar();           ///< 鎶樺彔/灞曞紑鍒囨崲锛堝榻愪笂娓?collapse_sidebar锛?
  Q_INVOKABLE void requestSetSidebarCollapsed(bool c);  ///< 鏄惧紡璁剧疆鎶樺彔鐘舵€?
  Q_INVOKABLE void requestSetSidebarWidth(int w);       ///< 璁剧疆瀹藉害锛坈lamp [min,max]锛屾寔涔呭寲锛?
  Q_INVOKABLE void requestSetSidebarDockArea(int area); ///< 璁剧疆 dock 鍖哄煙锛圠eft/Right锛?
  Q_INVOKABLE void postError(const QString &message, int severity = 0);
  Q_INVOKABLE void postNotification(const QString &message, const QString &title = {}, int severity = 0);
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void dismissNotification();

  /// 涓撶敤閫氱煡绫诲瀷锛堝榻愪笂娓?NotificationManager 涓撶敤閫氱煡锛?
  Q_INVOKABLE void postSlicingProgress(int percent, const QString &stage = {});
  Q_INVOKABLE void postSlicingComplete();          ///< 鍒囩墖瀹屾垚锛屽惈瀵煎嚭/棰勮鎸夐挳
  Q_INVOKABLE void postExportFinished(const QString &filePath);
  Q_INVOKABLE void postExportOngoing(const QString &stage = {});
  Q_INVOKABLE void postPlaterWarning(const QString &message);
  Q_INVOKABLE void postPlaterError(const QString &message);
  Q_INVOKABLE void postValidateError(const QString &message);
  Q_INVOKABLE void postValidateWarning(const QString &message);
  Q_INVOKABLE void postArrangeOngoing(int percent);

  /// 鎻愮ず閫氱煡锛堝榻愪笂娓?HintNotification / DidYouKnowHint锛?
  Q_INVOKABLE void postHint();
  Q_INVOKABLE void nextHint();
  Q_INVOKABLE void prevHint();
  Q_INVOKABLE int hintCount() const;
  Q_INVOKABLE int currentHintIndex() const;
  Q_INVOKABLE QString currentHintText() const;
  Q_INVOKABLE QString currentHintHypertext() const;
  Q_INVOKABLE QString currentHintFollowText() const;
  /// 鎵撳紑褰撳墠鎻愮ず鐨勬枃妗ｉ摼鎺ワ紙瀵归綈涓婃父 HintNotification documentation button锛?
  Q_INVOKABLE bool openHintDocumentation() const;
  /// 褰撳墠鎻愮ず鏄惁鏈夋枃妗ｉ摼鎺?
  Q_INVOKABLE bool currentHintHasDocumentationLink() const;

  Q_INVOKABLE void openSettings(); // H3
  /// 璇锋眰鏄剧ず棣栨閰嶇疆鍚戝锛圦ML 渚цЕ鍙戯級
  Q_INVOKABLE void showConfigWizard();
  /// 璇锋眰鏄剧ず鐑簥褰㈢姸璁剧疆瀵硅瘽妗嗭紙QML 渚цЕ鍙戯級
  Q_INVOKABLE void showBedShapeDialog();
  /// 璇锋眰鏄剧ず G-code 缂栬緫瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 EditGCodeDialog锛?
  Q_INVOKABLE void showEditGCodeDialog(const QString &key = {}, const QString &value = {});
  /// 璇锋眰鏄剧ず AMS 璁剧疆瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 AMSMaterialsSetting / AMSSetting锛?
  Q_INVOKABLE void showAMSSettingsDialog();
  /// 璇锋眰鏄剧ず鍥轰欢鍗囩骇瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 UpgradePanel / MachineInfoPanel锛?
  Q_INVOKABLE void showFirmwareDialog();
  /// 璇锋眰鏄剧ず閫熷害闄愬埗瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 AccelerationAndSpeedLimitDialog锛?
  Q_INVOKABLE void showSpeedLimitDialog();
  /// 璇锋眰鏄剧ず鎿︽枡濉旇缃璇濇锛圦ML 渚цЕ鍙戯紝瀵归綈涓婃父 WipeTowerDialog锛?
  Q_INVOKABLE void showWipeTowerDialog();
  /// 璇锋眰鏄剧ず鎵撳嵃涓绘満璁剧疆瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 PhysicalPrinterDialog锛?
  Q_INVOKABLE void showPrintHostDialog();
  /// 璇锋眰鏄剧ず鎻掍欢绠＄悊瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 WebDownPluginDlg锛?
  Q_INVOKABLE void showPluginManagerDialog();
  /// 璇锋眰鏄剧ず绮剧畝棰勮妯″紡瀵硅瘽妗嗭紙QML 渚цЕ鍙戯紝瀵归綈涓婃父 EnableLiteModeDialog锛?
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
  /// 閫氱煡涓績
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
  /// 骞挎挱 tab 鍒囨崲璇锋眰锛堝榻愪笂娓?EVT_SELECT_TAB锛夈€?
  /// 鍦?currentPage 鏀瑰彉涔嬪墠鍙戝嚭锛屼究浜庣洃鍚€呭厛浜庨〉闈㈠垏鎹㈠仛鍑哄搷搴斻€?
  void tabSelectRequested(int position);
  /// 骞挎挱 Plater 瑙嗗浘妯″紡鍒囨崲璇锋眰锛堝榻愪笂娓?view3D/preview 鍙鎬у垏鎹級銆?
  void viewModeChangeRequested(int mode);
  /// 褰撳墠瑙嗗浘妯″紡宸叉敼鍙橈紙currentViewMode Q_PROPERTY NOTIFY锛夈€?
  void currentViewModeChanged();
  // Phase 4: Sidebar Dockable 鐘舵€佸彉鏇翠俊鍙?
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
  /// v2.6 CAM-03锛氭憚鍍忓ご鏈嶅姟锛圧TSP 瑙ｇ爜浠ｇ悊锛?
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
  /// Phase 3: 褰撳墠 Plater 瑙嗗浘妯″紡锛堥粯璁?View3D锛屽榻愪笂娓?Plater 榛樿鏄剧ず view3D锛?
  ViewMode currentViewMode_ = ViewMode::View3D;
  // Phase 4: Sidebar Dockable 鐘舵€侊紙鏋勯€犳椂浠?QSettings load锛宻etter 鏃?save锛?
  static constexpr int kSidebarMinWidth = 240;   ///< 鏈€灏忓搴︼紙瀵归綈涓婃父 sidebar 涓嶅彲绐勪簬姝わ級
  static constexpr int kSidebarMaxWidth = 480;   ///< 鏈€澶у搴︼紙閬垮厤鐙崰杩囧 3D 鍖猴級
  static constexpr int kSidebarDefaultWidth = 280; ///< 榛樿瀹藉害锛堝榻愪笂娓?LeftSidebar 280px锛?
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
    bool persistent = false;           ///< true = 涓嶈嚜鍔ㄥ叧闂紝闇€鐢ㄦ埛鍏抽棴
    int progressValue = 0;            ///< 杩涘害鏉″綋鍓嶅€?(0-100)
    int progressMin = 0;
    int progressMax = 100;
    bool hasProgress = false;          ///< 鏄惁鏄剧ず杩涘害鏉?
    bool requiresConfirm = false;      ///< 鏄惁闇€瑕佺敤鎴风‘璁?
    int confirmAction = 0;             ///< 鑷畾涔夌‘璁ゅ姩浣滄爣璇?
    QDateTime timestamp;               ///< 閫氱煡鏃堕棿鎴筹紙鐢ㄤ簬鍘嗗彶璁板綍锛?
    /// 鍒囩墖瀹屾垚鍚庢搷浣滄寜閽紙瀵归綈涓婃父 SlicingProgressNotification锛?
    bool showExportButton = false;
    bool showPreviewButton = false;
    /// 鎻愮ず瀵艰埅锛堝榻愪笂娓?HintNotification next/prev锛?
    bool hintHasNext = false;
    bool hintHasPrev = false;
  };
  QQueue<NotificationEntry> m_notificationQueue;
  NotificationEntry m_currentNotification; ///< 褰撳墠鏄剧ず鐨勯€氱煡锛堟敮鎸佽繘搴︽洿鏂帮級
  QVector<NotificationEntry> m_notificationHistory; ///< 宸插叧闂殑閫氱煡鍘嗗彶
  int m_unreadHistoryCount = 0;
  void showNextNotification();
  /// 鏇存柊褰撳墠閫氱煡鐨勮繘搴﹀€硷紙瀵归綈涓婃父 notification_manager 杩涘害閫氱煡锛?
  Q_INVOKABLE void updateNotificationProgress(int value);
  /// 纭褰撳墠闇€瑕佺‘璁ょ殑閫氱煡锛堝榻愪笂娓?notification_manager confirm锛?
  Q_INVOKABLE void confirmCurrentNotification();
  /// 鍙栨秷褰撳墠闇€瑕佺‘璁ょ殑閫氱煡
  Q_INVOKABLE void cancelCurrentNotification();
  bool requestConfigPageExitIfNeeded();
  void handleConfigPendingActionApplied(const QString &action, const QString &target);
  void clearDeferredConfigExit();
  bool executeDeferredConfigExit();

  // 澶栬鐘舵€?
  QTranslator *m_translator = nullptr;
  double m_uiScale = 1.0;
  QColor m_bgColor{"#0d0f12"};
  QColor m_surfaceColor{"#0f1217"};
  QColor m_sidebarColor{"#0f1218"};
  QColor m_borderColor{"#242a33"};

  // 閫氱煡鍋忓ソ锛堝榻愪笂娓?notification_manager preferences锛?
  bool m_notificationsEnabled = true;
  bool m_hintsEnabled = true;
  int m_autoDismissSec = 5;           ///< 榛樿鑷姩娑堝け绉掓暟锛堥潪 persistent 閫氱煡锛?
  bool m_showProgressNotifications = true; ///< 鏄剧ず鍒囩墖杩涘害閫氱煡

  /// 棣栨閰嶇疆鍚戝鐘舵€侊紙瀵归綈涓婃父 ConfigWizard 棣栨杩愯妫€娴嬶級
  bool m_configWizardCompleted = false;

  /// 鎻愮ず鏁版嵁搴擄紙瀵归綈涓婃父 HintDatabase锛?
  QVector<HintData> m_hints;
  int m_currentHintIndex = -1;
  QSet<QString> m_displayedHintIds;
  QTimer *m_hintTimer = nullptr;      ///< 瀹氭湡鏄剧ず鎻愮ず锛堝榻愪笂娓?30s 闂撮殧锛?
  void initHintDatabase();
  void initFallbackHintDatabase();
  int selectNextHint(bool random = true);

  int deferredConfigExitPage_ = -1;
  enum class DeferredConfigExitKind
  {
    None = 0,
    PageChange,
    NewProject,
    OpenProject,
    ImportModel,
  };
  DeferredConfigExitKind deferredConfigExitKind_ = DeferredConfigExitKind::None;
  QString deferredConfigExitPath_;

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
