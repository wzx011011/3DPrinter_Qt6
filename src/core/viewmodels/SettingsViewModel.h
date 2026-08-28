#pragma once
#include <QObject>
#include <QStringList>
#include <QSettings>

class SettingsViewModel : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int prefCategory READ prefCategory WRITE setPrefCategory NOTIFY prefCategoryChanged)
  Q_PROPERTY(QString prefCategoryTitle READ prefCategoryTitle NOTIFY prefCategoryChanged)
  Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
  Q_PROPERTY(int themeIndex READ themeIndex WRITE setThemeIndex NOTIFY themeIndexChanged)
  Q_PROPERTY(int uiScaleIndex READ uiScaleIndex WRITE setUiScaleIndex NOTIFY uiScaleIndexChanged)
  Q_PROPERTY(int languageIndex READ languageIndex WRITE setLanguageIndex NOTIFY languageIndexChanged)
  Q_PROPERTY(QString language READ language NOTIFY languageIndexChanged)
  Q_PROPERTY(bool showHomePage READ showHomePage WRITE setShowHomePage NOTIFY settingsChanged)
  Q_PROPERTY(int defaultPage READ defaultPage WRITE setDefaultPage NOTIFY settingsChanged)
  /// Units: 0=Metric (mm), 1=Imperial (inch) — upstream app_config
  /// "use_inches" (Preferences.cpp:1110). Display-only conversion; storage
  /// stays in mm (Phase 241 PAGE-04).
  Q_PROPERTY(int units READ units WRITE setUnits NOTIFY settingsChanged)
  /// 通用偏好扩展（对齐上游 PreferencesDialog::create_general_page）
  Q_PROPERTY(bool autoSave READ autoSave WRITE setAutoSave NOTIFY settingsChanged)
  Q_PROPERTY(int autoSaveInterval READ autoSaveInterval WRITE setAutoSaveInterval NOTIFY settingsChanged)
  Q_PROPERTY(bool checkUpdates READ checkUpdates WRITE setCheckUpdates NOTIFY settingsChanged)
  /// 区域设置（对齐上游 PreferencesDialog::create_general_page region combo）
  Q_PROPERTY(int region READ region WRITE setRegion NOTIFY settingsChanged)
  /// 自动备份到云端（对齐上游 cloud sync backup）
  Q_PROPERTY(bool autoBackup READ autoBackup WRITE setAutoBackup NOTIFY settingsChanged)
  /// 撤销栈上限（对齐上游 undo/redo 历史限制）
  Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit NOTIFY settingsChanged)
  /// 默认喷嘴直径选择（对齐上游 PreferencesDialog 打印机设置）
  Q_PROPERTY(int defaultNozzleIndex READ defaultNozzleIndex WRITE setDefaultNozzleIndex NOTIFY settingsChanged)
  /// 默认热床形状（对齐上游 bed_shape）
  Q_PROPERTY(int defaultBedShape READ defaultBedShape WRITE setDefaultBedShape NOTIFY settingsChanged)
  /// v5.12 gap-closure: camera settings (对齐上游 Preferences General tab,
  /// Preferences.cpp:1123-1128). Persisted to QSettings.
  Q_PROPERTY(int cameraNavStyle READ cameraNavStyle WRITE setCameraNavStyle NOTIFY settingsChanged)
  Q_PROPERTY(bool zoomToMouse READ zoomToMouse WRITE setZoomToMouse NOTIFY settingsChanged)
  Q_PROPERTY(bool freeCamera READ freeCamera WRITE setFreeCamera NOTIFY settingsChanged)
  Q_PROPERTY(bool reverseZoom READ reverseZoom WRITE setReverseZoom NOTIFY settingsChanged)
  /// v5.16 (NAVIGATOR): show_3d_navigator (upstream app_config default true,
  /// AppConfig.cpp:200; View-menu check item MainFrame.cpp:2630-2638).
  Q_PROPERTY(bool show3DNavigator READ show3DNavigator WRITE setShow3DNavigator NOTIFY settingsChanged)
  /// 切片完成后自动上传（对齐上游 print host upload after slicing）
  Q_PROPERTY(bool autoUpload READ autoUpload WRITE setAutoUpload NOTIFY settingsChanged)
  /// 更新通道（对齐上游 update_channel: 0=Stable, 1=Beta, 2=Dev）
  Q_PROPERTY(int updateChannel READ updateChannel WRITE setUpdateChannel NOTIFY settingsChanged)
  /// 通知偏好（对齐上游 notification_manager preferences）
  Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(bool hintsEnabled READ hintsEnabled WRITE setHintsEnabled NOTIFY settingsChanged)
  Q_PROPERTY(int autoDismissSec READ autoDismissSec WRITE setAutoDismissSec NOTIFY settingsChanged)
  Q_PROPERTY(bool showProgressNotifications READ showProgressNotifications WRITE setShowProgressNotifications NOTIFY settingsChanged)
  /// 开发者模式（对齐上游 PreferencesDialog::create_debug_page）。Gates the
  /// Preferences Developer category visibility (Phase 241 PAGE-04 consumer).
  Q_PROPERTY(bool developerMode READ developerMode WRITE setDeveloperMode NOTIFY settingsChanged)
  /// 调试覆盖层
  Q_PROPERTY(bool showDebugOverlay READ showDebugOverlay WRITE setShowDebugOverlay NOTIFY settingsChanged)
  /// 日志级别（对齐上游 GUI_App log level）: 0=Error, 1=Warning, 2=Info, 3=Debug, 4=Trace
  Q_PROPERTY(int logLevel READ logLevel WRITE setLogLevel NOTIFY settingsChanged)
  /// 详细 G-code（对齐上游 debug verbose gcode）
  Q_PROPERTY(bool verboseGcode READ verboseGcode WRITE setVerboseGcode NOTIFY settingsChanged)
  /// OpenGL 调试上下文
  Q_PROPERTY(bool glDebugContext READ glDebugContext WRITE setGlDebugContext NOTIFY settingsChanged)
  /// 最大日志大小 MB（对齐上游 log rotation size）
  Q_PROPERTY(int maxLogSizeMb READ maxLogSizeMb WRITE setMaxLogSizeMb NOTIFY settingsChanged)
  /// AI 助手（OWzx-only 决策，docs/ai-control.md）：内嵌 Claude Agent SDK
  /// sidecar + GLM（智谱 Anthropic 兼容端点）。默认整体关闭。
  Q_PROPERTY(bool aiEnabled READ aiEnabled WRITE setAiEnabled NOTIFY settingsChanged)
  Q_PROPERTY(QString aiApiKey READ aiApiKey WRITE setAiApiKey NOTIFY settingsChanged)
  Q_PROPERTY(QString aiModel READ aiModel WRITE setAiModel NOTIFY settingsChanged)
  Q_PROPERTY(QString aiBaseUrl READ aiBaseUrl WRITE setAiBaseUrl NOTIFY settingsChanged)
  Q_PROPERTY(int aiPort READ aiPort WRITE setAiPort NOTIFY settingsChanged)
  Q_PROPERTY(QString aiControlToken READ aiControlToken CONSTANT)
  Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetsChanged)
  Q_PROPERTY(QString currentPreset READ currentPreset WRITE setCurrentPreset NOTIFY presetsChanged)
  Q_PROPERTY(double layerHeight READ layerHeight WRITE setLayerHeight NOTIFY configChanged)

public:
  explicit SettingsViewModel(QObject *parent = nullptr);

  // Phase 241 (PAGE-04): mm <-> inch display conversion for length readouts
  // (upstream use_inches consumers: object manipulation fields
  // Plater.cpp:14147, GCodeViewer.cpp:4081). STORAGE STAYS IN mm — only the
  // displayed number converts (1 inch = 25.4 mm exactly).
  Q_INVOKABLE double displayLength(double mm) const;
  /// Inverse of displayLength: converts a user-entered display value back to
  /// the mm storage unit before it is written into a transform/size field.
  Q_INVOKABLE double storageLength(double display) const;
  /// Unit suffix for length readouts: "mm" or "in".
  Q_INVOKABLE QString lengthUnitLabel() const;

  int prefCategory() const { return m_prefCategory; }
  QString prefCategoryTitle() const;
  int fontSize() const { return m_fontSize; }
  int themeIndex() const { return m_themeIndex; }
  int uiScaleIndex() const { return m_uiScaleIndex; }
  int languageIndex() const { return m_languageIndex; }
  QString language() const;
  bool showHomePage() const { return m_showHomePage; }
  int defaultPage() const { return m_defaultPage; }
  int units() const { return m_units; }
  bool autoSave() const { return m_autoSave; }
  int autoSaveInterval() const { return m_autoSaveInterval; }
  bool checkUpdates() const { return m_checkUpdates; }
  int region() const { return m_region; }
  bool autoBackup() const { return m_autoBackup; }
  int undoLimit() const { return m_undoLimit; }
  int defaultNozzleIndex() const { return m_defaultNozzleIndex; }
  int defaultBedShape() const { return m_defaultBedShape; }
  // v5.12 camera settings
  int cameraNavStyle() const { return m_cameraNavStyle; }
  bool zoomToMouse() const { return m_zoomToMouse; }
  bool freeCamera() const { return m_freeCamera; }
  bool reverseZoom() const { return m_reverseZoom; }
  bool show3DNavigator() const { return m_show3DNavigator; }
  bool autoUpload() const { return m_autoUpload; }
  int updateChannel() const { return m_updateChannel; }
  bool notificationsEnabled() const { return m_notificationsEnabled; }
  bool hintsEnabled() const { return m_hintsEnabled; }
  int autoDismissSec() const { return m_autoDismissSec; }
  bool showProgressNotifications() const { return m_showProgressNotifications; }
  bool developerMode() const { return m_developerMode; }
  bool showDebugOverlay() const { return m_showDebugOverlay; }
  int logLevel() const { return m_logLevel; }
  bool verboseGcode() const { return m_verboseGcode; }
  bool glDebugContext() const { return m_glDebugContext; }
  int maxLogSizeMb() const { return m_maxLogSizeMb; }
  bool aiEnabled() const { return m_aiEnabled; }
  QString aiApiKey() const { return m_aiApiKey; }
  QString aiModel() const { return m_aiModel; }
  QString aiBaseUrl() const { return m_aiBaseUrl; }
  int aiPort() const { return m_aiPort; }
  /// Loopback MCP auth token. Generated once, persisted in QSettings under a
  /// dedicated group; CONSTANT because regenerating would break a connected
  /// harness session (a future "regenerate" action would restart the server).
  QString aiControlToken() const;
  QStringList presetNames() const { return m_presetNames; }
  QString currentPreset() const { return m_currentPreset; }
  double layerHeight() const { return m_layerHeight; }

signals:
  void prefCategoryChanged();
  void fontSizeChanged();
  void themeIndexChanged();
  void uiScaleIndexChanged();
  void languageIndexChanged();
  void settingsChanged();
  void presetsChanged();
  void configChanged();

public slots:
  void setPrefCategory(int cat);
  void setFontSize(int size);
  void setThemeIndex(int idx);
  void setUiScaleIndex(int idx);
  void setLanguageIndex(int idx);
  void setShowHomePage(bool v);
  void setDefaultPage(int page);
  void setUnits(int u);
  void setAutoSave(bool v);
  void setAutoSaveInterval(int minutes);
  void setCheckUpdates(bool v);
  void setRegion(int r);
  void setAutoBackup(bool v);
  void setUndoLimit(int limit);
  void setDefaultNozzleIndex(int idx);
  void setDefaultBedShape(int shape);
  // v5.12 camera settings
  void setCameraNavStyle(int style);
  void setZoomToMouse(bool on);
  void setFreeCamera(bool on);
  void setReverseZoom(bool on);
  void setShow3DNavigator(bool on);  // v5.16 (NAVIGATOR)
  void setAutoUpload(bool v);
  void setUpdateChannel(int channel);
  void setNotificationsEnabled(bool v);
  void setHintsEnabled(bool v);
  void setAutoDismissSec(int sec);
  void setShowProgressNotifications(bool v);
  void setDeveloperMode(bool v);
  void setShowDebugOverlay(bool v);
  void setLogLevel(int v);
  void setVerboseGcode(bool v);
  void setGlDebugContext(bool v);
  void setMaxLogSizeMb(int v);
  void setAiEnabled(bool v);
  void setAiApiKey(const QString &v);
  void setAiModel(const QString &v);
  void setAiBaseUrl(const QString &v);
  void setAiPort(int v);
  void setCurrentPreset(const QString &preset);
  void setLayerHeight(double h);
  Q_INVOKABLE void resetPreferences();

private:
  void loadFromSettings();

  int m_prefCategory = 0;
  int m_fontSize = 12;
  int m_themeIndex = 0;
  int m_uiScaleIndex = 0;
  int m_languageIndex = 0;
  QStringList m_presetNames;
  QString m_currentPreset;
  double m_layerHeight = 0.2;
  bool m_showHomePage = true;
  int m_defaultPage = 1;   // 0=Home, 1=Prepare (对齐上游 default_page radio)
  int m_units = 0;          // 0=Metric, 1=Imperial
  // Phase 241 (PAGE-04): userRole / reducedMotion / compactMode removed —
  // zero upstream Preferences mapping and zero consumers (dead UI).
  bool m_autoSave = true;
  int m_autoSaveInterval = 10;  // minutes
  bool m_checkUpdates = true;
  int m_region = 0;           // 0=System, 1=China, 2=US, 3=EU, 4=Japan
  bool m_autoBackup = false;  // cloud sync backup
  int m_undoLimit = 100;      // undo stack limit
  int m_defaultNozzleIndex = 1; // 0=0.2mm, 1=0.4mm, 2=0.6mm, 3=0.8mm
  int m_defaultBedShape = 0;    // 0=Rectangular, 1=Round
  // v5.12 camera settings (persisted)
  int m_cameraNavStyle = 0;     // 0=Default, 1=Touchpad
  bool m_zoomToMouse = true;
  bool m_freeCamera = false;
  bool m_reverseZoom = false;
  bool m_show3DNavigator = true;  // upstream show_3d_navigator default
  bool m_autoUpload = false;    // auto upload after slicing
  int m_updateChannel = 0;      // 0=Stable, 1=Beta, 2=Dev
  bool m_notificationsEnabled = true;
  bool m_hintsEnabled = true;
  int m_autoDismissSec = 5;
  bool m_showProgressNotifications = true;
  bool m_developerMode = false;
  bool m_showDebugOverlay = false;
  int m_logLevel = 2;            // Info
  bool m_verboseGcode = false;
  bool m_glDebugContext = false;
  int m_maxLogSizeMb = 50;
  // AI 助手（OWzx-only，docs/ai-control.md）
  bool m_aiEnabled = false;
  QString m_aiApiKey;
  QString m_aiModel = QStringLiteral("glm-5.3-flash");
  QString m_aiBaseUrl = QStringLiteral("https://open.bigmodel.cn/api/anthropic");
  int m_aiPort = 27417;
};
