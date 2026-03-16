#pragma once
#include <QObject>
#include <QStringList>

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
  Q_PROPERTY(int units READ units WRITE setUnits NOTIFY settingsChanged)
  Q_PROPERTY(int userRole READ userRole WRITE setUserRole NOTIFY settingsChanged)
  /// 通用偏好扩展（对齐上游 PreferencesDialog::create_general_page）
  Q_PROPERTY(bool autoSave READ autoSave WRITE setAutoSave NOTIFY settingsChanged)
  Q_PROPERTY(int autoSaveInterval READ autoSaveInterval WRITE setAutoSaveInterval NOTIFY settingsChanged)
  Q_PROPERTY(bool checkUpdates READ checkUpdates WRITE setCheckUpdates NOTIFY settingsChanged)
  Q_PROPERTY(bool reducedMotion READ reducedMotion WRITE setReducedMotion NOTIFY settingsChanged)
  /// 区域设置（对齐上游 PreferencesDialog::create_general_page region combo）
  Q_PROPERTY(int region READ region WRITE setRegion NOTIFY settingsChanged)
  /// 3D 视图低细节模式（对齐上游 enable_reduce_detail / LOD 开关）
  Q_PROPERTY(bool compactMode READ compactMode WRITE setCompactMode NOTIFY settingsChanged)
  /// 自动备份到云端（对齐上游 cloud sync backup）
  Q_PROPERTY(bool autoBackup READ autoBackup WRITE setAutoBackup NOTIFY settingsChanged)
  /// 撤销栈上限（对齐上游 undo/redo 历史限制）
  Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit NOTIFY settingsChanged)
  /// 默认喷嘴直径选择（对齐上游 PreferencesDialog 打印机设置）
  Q_PROPERTY(int defaultNozzleIndex READ defaultNozzleIndex WRITE setDefaultNozzleIndex NOTIFY settingsChanged)
  /// 默认热床形状（对齐上游 bed_shape）
  Q_PROPERTY(int defaultBedShape READ defaultBedShape WRITE setDefaultBedShape NOTIFY settingsChanged)
  /// 切片完成后自动上传（对齐上游 print host upload after slicing）
  Q_PROPERTY(bool autoUpload READ autoUpload WRITE setAutoUpload NOTIFY settingsChanged)
  /// 更新通道（对齐上游 update_channel: 0=Stable, 1=Beta, 2=Dev）
  Q_PROPERTY(int updateChannel READ updateChannel WRITE setUpdateChannel NOTIFY settingsChanged)
  /// 开发者模式（对齐上游 PreferencesDialog::create_debug_page）
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
  Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetsChanged)
  Q_PROPERTY(QString currentPreset READ currentPreset WRITE setCurrentPreset NOTIFY presetsChanged)
  Q_PROPERTY(double layerHeight READ layerHeight WRITE setLayerHeight NOTIFY configChanged)

public:
  explicit SettingsViewModel(QObject *parent = nullptr);

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
  int userRole() const { return m_userRole; }
  bool autoSave() const { return m_autoSave; }
  int autoSaveInterval() const { return m_autoSaveInterval; }
  bool checkUpdates() const { return m_checkUpdates; }
  bool reducedMotion() const { return m_reducedMotion; }
  int region() const { return m_region; }
  bool compactMode() const { return m_compactMode; }
  bool autoBackup() const { return m_autoBackup; }
  int undoLimit() const { return m_undoLimit; }
  int defaultNozzleIndex() const { return m_defaultNozzleIndex; }
  int defaultBedShape() const { return m_defaultBedShape; }
  bool autoUpload() const { return m_autoUpload; }
  int updateChannel() const { return m_updateChannel; }
  bool developerMode() const { return m_developerMode; }
  bool showDebugOverlay() const { return m_showDebugOverlay; }
  int logLevel() const { return m_logLevel; }
  bool verboseGcode() const { return m_verboseGcode; }
  bool glDebugContext() const { return m_glDebugContext; }
  int maxLogSizeMb() const { return m_maxLogSizeMb; }
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
  void setUserRole(int role);
  void setAutoSave(bool v);
  void setAutoSaveInterval(int minutes);
  void setCheckUpdates(bool v);
  void setReducedMotion(bool v);
  void setRegion(int r);
  void setCompactMode(bool v);
  void setAutoBackup(bool v);
  void setUndoLimit(int limit);
  void setDefaultNozzleIndex(int idx);
  void setDefaultBedShape(int shape);
  void setAutoUpload(bool v);
  void setUpdateChannel(int channel);
  void setDeveloperMode(bool v);
  void setShowDebugOverlay(bool v);
  void setLogLevel(int v);
  void setVerboseGcode(bool v);
  void setGlDebugContext(bool v);
  void setMaxLogSizeMb(int v);
  void setCurrentPreset(const QString &preset);
  void setLayerHeight(double h);
  Q_INVOKABLE void resetPreferences();

private:
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
  int m_userRole = 0;       // 0=Basic, 1=Professional
  bool m_autoSave = true;
  int m_autoSaveInterval = 10;  // minutes
  bool m_checkUpdates = true;
  bool m_reducedMotion = false;
  int m_region = 0;           // 0=System, 1=China, 2=US, 3=EU, 4=Japan
  bool m_compactMode = false; // 3D view LOD / compact mode
  bool m_autoBackup = false;  // cloud sync backup
  int m_undoLimit = 100;      // undo stack limit
  int m_defaultNozzleIndex = 1; // 0=0.2mm, 1=0.4mm, 2=0.6mm, 3=0.8mm
  int m_defaultBedShape = 0;    // 0=Rectangular, 1=Round
  bool m_autoUpload = false;    // auto upload after slicing
  int m_updateChannel = 0;      // 0=Stable, 1=Beta, 2=Dev
  bool m_developerMode = false;
  bool m_showDebugOverlay = false;
  int m_logLevel = 2;            // Info
  bool m_verboseGcode = false;
  bool m_glDebugContext = false;
  int m_maxLogSizeMb = 50;
};
