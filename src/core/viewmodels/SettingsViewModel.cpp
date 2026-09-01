#include "SettingsViewModel.h"
#include <QCoreApplication>
#include <QSettings>
#include <QUuid>

// Preference setters update the live draft. Persistence is intentionally
// deferred until applyPreferences(), so Cancel can restore a real transaction.
#define SAVE_SETTING(key, val) do { Q_UNUSED(key); Q_UNUSED(val); } while(0)

static QStringList categoryTitles()
{
  return {
      SettingsViewModel::tr("通用"),
      SettingsViewModel::tr("外观"),
      SettingsViewModel::tr("语言"),
      SettingsViewModel::tr("快捷键"),
      SettingsViewModel::tr("打印机"),
      SettingsViewModel::tr("账号与隐私"),
      SettingsViewModel::tr("更新"),
      SettingsViewModel::tr("高级"),
      SettingsViewModel::tr("开发者"),
      SettingsViewModel::tr("AI 助手"),
      SettingsViewModel::tr("关于")};
}

static QStringList languageNames()
{
  return {SettingsViewModel::tr("简体中文"), "English", "日本語", "한국어", "Deutsch", "Français"};
}

SettingsViewModel::SettingsViewModel(QObject *parent) : QObject(parent)
{
  m_presetNames = {tr("0.20mm 标准"), tr("0.20mm 精细"), tr("0.30mm 快速"), tr("0.15mm 超精细")};
  m_currentPreset = m_presetNames.first();
  loadFromSettings();
}

void SettingsViewModel::loadFromSettings()
{
  // Missing keys fall back to the factory defaults (not the current draft
  // members): cancelPreferences() relies on this to restore the last
  // committed state when draft edits were never applied.
  QSettings s;
  m_themeIndex       = s.value("themeIndex", 0).toInt();
  m_fontSize         = s.value("fontSize", 12).toInt();
  m_uiScaleIndex     = s.value("uiScaleIndex", 0).toInt();
  m_languageIndex    = s.value("languageIndex", 0).toInt();
  m_showHomePage     = s.value("showHomePage", true).toBool();
  m_defaultPage      = s.value("defaultPage", 1).toInt();
  m_units            = s.value("units", 0).toInt();
  m_autoSave         = s.value("autoSave", true).toBool();
  m_autoSaveInterval = s.value("autoSaveInterval", 10).toInt();
  m_checkUpdates     = s.value("checkUpdates", true).toBool();
  m_region           = s.value("region", 0).toInt();
  m_autoBackup       = s.value("autoBackup", false).toBool();
  m_undoLimit        = s.value("undoLimit", 100).toInt();
  m_defaultNozzleIndex = s.value("defaultNozzleIndex", 1).toInt();
  m_defaultBedShape  = s.value("defaultBedShape", 0).toInt();
  // v5.12 camera settings
  m_cameraNavStyle   = s.value("cameraNavStyle", 0).toInt();
  m_zoomToMouse      = s.value("zoomToMouse", true).toBool();
  m_freeCamera       = s.value("freeCamera", false).toBool();
  m_reverseZoom      = s.value("reverseZoom", false).toBool();
  m_show3DNavigator  = s.value("show3DNavigator", true).toBool();
  m_autoUpload       = s.value("autoUpload", false).toBool();
  m_updateChannel    = s.value("updateChannel", 0).toInt();
  m_notificationsEnabled = s.value("notificationsEnabled", true).toBool();
  m_hintsEnabled     = s.value("hintsEnabled", true).toBool();
  m_autoDismissSec   = s.value("autoDismissSec", 5).toInt();
  m_showProgressNotifications = s.value("showProgressNotifications", true).toBool();
  m_developerMode    = s.value("developerMode", false).toBool();
  m_showDebugOverlay = s.value("showDebugOverlay", false).toBool();
  m_logLevel         = s.value("logLevel", 2).toInt();
  m_verboseGcode     = s.value("verboseGcode", false).toBool();
  m_glDebugContext   = s.value("glDebugContext", false).toBool();
  m_maxLogSizeMb     = s.value("maxLogSizeMb", 50).toInt();
  // AI 助手（OWzx-only，docs/ai-control.md）
  m_aiEnabled        = s.value("aiEnabled", false).toBool();
  m_aiApiKey         = s.value("aiApiKey", QString()).toString();
  m_aiModel          = s.value("aiModel", QStringLiteral("glm-5.3-flash")).toString();
  m_aiBaseUrl        = s.value("aiBaseUrl", QStringLiteral("https://open.bigmodel.cn/api/anthropic")).toString();
  m_aiPort           = s.value("aiPort", 27417).toInt();
}

QString SettingsViewModel::aiControlToken() const
{
  // Dedicated QSettings group so resetPreferences never wipes it (a rotating
  // token would silently break an already-connected harness/sidecar session).
  QSettings s;
  s.beginGroup(QStringLiteral("aiControl"));
  QString token = s.value(QStringLiteral("token")).toString();
  if (token.isEmpty()) {
    token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    s.setValue(QStringLiteral("token"), token);
  }
  return token;
}

QString SettingsViewModel::prefCategoryTitle() const
{
  const auto titles = categoryTitles();
  if (m_prefCategory >= 0 && m_prefCategory < titles.size())
    return titles.at(m_prefCategory);
  return {};
}

QString SettingsViewModel::language() const
{
  const auto names = languageNames();
  if (m_languageIndex >= 0 && m_languageIndex < names.size())
    return names.at(m_languageIndex);
  return {};
}

void SettingsViewModel::setPrefCategory(int cat)
{
  if (m_prefCategory != cat)
  {
    m_prefCategory = cat;
    emit prefCategoryChanged();
  }
}

void SettingsViewModel::setFontSize(int size)
{
  if (m_fontSize != size)
  {
    m_fontSize = size;
    SAVE_SETTING("fontSize", size);
    emit fontSizeChanged();
  }
}

void SettingsViewModel::setThemeIndex(int idx)
{
  if (m_themeIndex != idx)
  {
    m_themeIndex = idx;
    SAVE_SETTING("themeIndex", idx);
    emit themeIndexChanged();
  }
}

void SettingsViewModel::setUiScaleIndex(int idx)
{
  if (m_uiScaleIndex != idx)
  {
    m_uiScaleIndex = idx;
    SAVE_SETTING("uiScaleIndex", idx);
    emit uiScaleIndexChanged();
  }
}

void SettingsViewModel::setLanguageIndex(int idx)
{
  if (m_languageIndex != idx)
  {
    m_languageIndex = idx;
    SAVE_SETTING("languageIndex", idx);
    emit languageIndexChanged();
  }
}

void SettingsViewModel::setCurrentPreset(const QString &preset)
{
  if (m_currentPreset != preset)
  {
    m_currentPreset = preset;
    emit presetsChanged();
  }
}

void SettingsViewModel::setLayerHeight(double h)
{
  if (qAbs(m_layerHeight - h) > 1e-6)
  {
    m_layerHeight = h;
    emit configChanged();
  }
}

void SettingsViewModel::resetPreferences()
{
  // Restore defaults in the draft only. Apply persists them; Cancel preserves
  // the previous local commit. Credentials and the AI control token remain out
  // of this action so a visual reset cannot silently discard access material.
  setThemeIndex(0);
  setFontSize(12);
  setUiScaleIndex(0);
  setLanguageIndex(0);
  setShowHomePage(true);
  setDefaultPage(1);
  setUnits(0);
  setAutoSave(true);
  setAutoSaveInterval(10);
  setCheckUpdates(true);
  setRegion(0);
  setAutoBackup(false);
  setUndoLimit(100);
  setDefaultNozzleIndex(1);
  setDefaultBedShape(0);
  setCameraNavStyle(0);
  setZoomToMouse(true);
  setFreeCamera(false);
  setReverseZoom(false);
  setShow3DNavigator(true);
  setAutoUpload(false);
  setUpdateChannel(0);
  setNotificationsEnabled(true);
  setHintsEnabled(true);
  setAutoDismissSec(5);
  setShowProgressNotifications(true);
  setDeveloperMode(false);
  setShowDebugOverlay(false);
  setLogLevel(2);
  setVerboseGcode(false);
  setGlDebugContext(false);
  setMaxLogSizeMb(50);
  setAiEnabled(false);
  setAiModel(QStringLiteral("glm-5.3-flash"));
  setAiBaseUrl(QStringLiteral("https://open.bigmodel.cn/api/anthropic"));
  setAiPort(27417);
}

void SettingsViewModel::applyPreferences()
{
  saveToSettings();
}

void SettingsViewModel::cancelPreferences()
{
  loadFromSettings();
  notifyPreferencesRestored();
}

void SettingsViewModel::saveToSettings() const
{
  QSettings s;
  s.setValue(QStringLiteral("themeIndex"), m_themeIndex);
  s.setValue(QStringLiteral("fontSize"), m_fontSize);
  s.setValue(QStringLiteral("uiScaleIndex"), m_uiScaleIndex);
  s.setValue(QStringLiteral("languageIndex"), m_languageIndex);
  s.setValue(QStringLiteral("showHomePage"), m_showHomePage);
  s.setValue(QStringLiteral("defaultPage"), m_defaultPage);
  s.setValue(QStringLiteral("units"), m_units);
  s.setValue(QStringLiteral("autoSave"), m_autoSave);
  s.setValue(QStringLiteral("autoSaveInterval"), m_autoSaveInterval);
  s.setValue(QStringLiteral("checkUpdates"), m_checkUpdates);
  s.setValue(QStringLiteral("region"), m_region);
  s.setValue(QStringLiteral("autoBackup"), m_autoBackup);
  s.setValue(QStringLiteral("undoLimit"), m_undoLimit);
  s.setValue(QStringLiteral("defaultNozzleIndex"), m_defaultNozzleIndex);
  s.setValue(QStringLiteral("defaultBedShape"), m_defaultBedShape);
  s.setValue(QStringLiteral("cameraNavStyle"), m_cameraNavStyle);
  s.setValue(QStringLiteral("zoomToMouse"), m_zoomToMouse);
  s.setValue(QStringLiteral("freeCamera"), m_freeCamera);
  s.setValue(QStringLiteral("reverseZoom"), m_reverseZoom);
  s.setValue(QStringLiteral("show3DNavigator"), m_show3DNavigator);
  s.setValue(QStringLiteral("autoUpload"), m_autoUpload);
  s.setValue(QStringLiteral("updateChannel"), m_updateChannel);
  s.setValue(QStringLiteral("notificationsEnabled"), m_notificationsEnabled);
  s.setValue(QStringLiteral("hintsEnabled"), m_hintsEnabled);
  s.setValue(QStringLiteral("autoDismissSec"), m_autoDismissSec);
  s.setValue(QStringLiteral("showProgressNotifications"), m_showProgressNotifications);
  s.setValue(QStringLiteral("developerMode"), m_developerMode);
  s.setValue(QStringLiteral("showDebugOverlay"), m_showDebugOverlay);
  s.setValue(QStringLiteral("logLevel"), m_logLevel);
  s.setValue(QStringLiteral("verboseGcode"), m_verboseGcode);
  s.setValue(QStringLiteral("glDebugContext"), m_glDebugContext);
  s.setValue(QStringLiteral("maxLogSizeMb"), m_maxLogSizeMb);
  s.setValue(QStringLiteral("aiEnabled"), m_aiEnabled);
  s.setValue(QStringLiteral("aiApiKey"), m_aiApiKey);
  s.setValue(QStringLiteral("aiModel"), m_aiModel);
  s.setValue(QStringLiteral("aiBaseUrl"), m_aiBaseUrl);
  s.setValue(QStringLiteral("aiPort"), m_aiPort);
  s.sync();
}

void SettingsViewModel::notifyPreferencesRestored()
{
  emit themeIndexChanged();
  emit fontSizeChanged();
  emit uiScaleIndexChanged();
  emit languageIndexChanged();
  emit settingsChanged();
}

void SettingsViewModel::setShowHomePage(bool v)
{
  if (m_showHomePage != v) { m_showHomePage = v; SAVE_SETTING("showHomePage", v); emit settingsChanged(); }
}

void SettingsViewModel::setDefaultPage(int page)
{
  if (m_defaultPage != page) { m_defaultPage = page; SAVE_SETTING("defaultPage", page); emit settingsChanged(); }
}

void SettingsViewModel::setUnits(int u)
{
  if (m_units != u) { m_units = u; SAVE_SETTING("units", u); emit settingsChanged(); }
}

void SettingsViewModel::setAutoSave(bool v)
{
  if (m_autoSave != v) { m_autoSave = v; SAVE_SETTING("autoSave", v); emit settingsChanged(); }
}

void SettingsViewModel::setAutoSaveInterval(int minutes)
{
  if (m_autoSaveInterval != minutes) { m_autoSaveInterval = minutes; SAVE_SETTING("autoSaveInterval", minutes); emit settingsChanged(); }
}

void SettingsViewModel::setCheckUpdates(bool v)
{
  if (m_checkUpdates != v) { m_checkUpdates = v; SAVE_SETTING("checkUpdates", v); emit settingsChanged(); }
}

void SettingsViewModel::setRegion(int r)
{
  if (m_region != r) { m_region = r; SAVE_SETTING("region", r); emit settingsChanged(); }
}

void SettingsViewModel::setAutoBackup(bool v)
{
  if (m_autoBackup != v) { m_autoBackup = v; SAVE_SETTING("autoBackup", v); emit settingsChanged(); }
}

void SettingsViewModel::setUndoLimit(int limit)
{
  if (m_undoLimit != limit) { m_undoLimit = limit; SAVE_SETTING("undoLimit", limit); emit settingsChanged(); }
}

void SettingsViewModel::setDefaultNozzleIndex(int idx)
{
  if (m_defaultNozzleIndex != idx) { m_defaultNozzleIndex = idx; SAVE_SETTING("defaultNozzleIndex", idx); emit settingsChanged(); }
}

void SettingsViewModel::setDefaultBedShape(int shape)
{
  if (m_defaultBedShape != shape) { m_defaultBedShape = shape; SAVE_SETTING("defaultBedShape", shape); emit settingsChanged(); }
}
// v5.12 camera settings
void SettingsViewModel::setCameraNavStyle(int style)
{
  if (m_cameraNavStyle != style) { m_cameraNavStyle = style; SAVE_SETTING("cameraNavStyle", style); emit settingsChanged(); }
}
void SettingsViewModel::setZoomToMouse(bool on)
{
  if (m_zoomToMouse != on) { m_zoomToMouse = on; SAVE_SETTING("zoomToMouse", on); emit settingsChanged(); }
}
void SettingsViewModel::setFreeCamera(bool on)
{
  if (m_freeCamera != on) { m_freeCamera = on; SAVE_SETTING("freeCamera", on); emit settingsChanged(); }
}
void SettingsViewModel::setReverseZoom(bool on)
{
  if (m_reverseZoom != on) { m_reverseZoom = on; SAVE_SETTING("reverseZoom", on); emit settingsChanged(); }
}
void SettingsViewModel::setShow3DNavigator(bool on)
{
  if (m_show3DNavigator != on) { m_show3DNavigator = on; SAVE_SETTING("show3DNavigator", on); emit settingsChanged(); }
}

void SettingsViewModel::setAutoUpload(bool v)
{
  if (m_autoUpload != v) { m_autoUpload = v; SAVE_SETTING("autoUpload", v); emit settingsChanged(); }
}

void SettingsViewModel::setUpdateChannel(int channel)
{
  if (m_updateChannel != channel) { m_updateChannel = channel; SAVE_SETTING("updateChannel", channel); emit settingsChanged(); }
}

void SettingsViewModel::setNotificationsEnabled(bool v)
{
  if (m_notificationsEnabled != v) { m_notificationsEnabled = v; SAVE_SETTING("notificationsEnabled", v); emit settingsChanged(); }
}

void SettingsViewModel::setHintsEnabled(bool v)
{
  if (m_hintsEnabled != v) { m_hintsEnabled = v; SAVE_SETTING("hintsEnabled", v); emit settingsChanged(); }
}

void SettingsViewModel::setAutoDismissSec(int sec)
{
  if (m_autoDismissSec != sec) { m_autoDismissSec = sec; SAVE_SETTING("autoDismissSec", sec); emit settingsChanged(); }
}

void SettingsViewModel::setShowProgressNotifications(bool v)
{
  if (m_showProgressNotifications != v) { m_showProgressNotifications = v; SAVE_SETTING("showProgressNotifications", v); emit settingsChanged(); }
}

void SettingsViewModel::setDeveloperMode(bool v)
{
  if (m_developerMode != v) { m_developerMode = v; SAVE_SETTING("developerMode", v); emit settingsChanged(); }
}

void SettingsViewModel::setShowDebugOverlay(bool v)
{
  if (m_showDebugOverlay != v) { m_showDebugOverlay = v; SAVE_SETTING("showDebugOverlay", v); emit settingsChanged(); }
}

void SettingsViewModel::setLogLevel(int v)
{
  if (m_logLevel != v) { m_logLevel = v; SAVE_SETTING("logLevel", v); emit settingsChanged(); }
}

void SettingsViewModel::setVerboseGcode(bool v)
{
  if (m_verboseGcode != v) { m_verboseGcode = v; SAVE_SETTING("verboseGcode", v); emit settingsChanged(); }
}

void SettingsViewModel::setGlDebugContext(bool v)
{
  if (m_glDebugContext != v) { m_glDebugContext = v; SAVE_SETTING("glDebugContext", v); emit settingsChanged(); }
}

void SettingsViewModel::setMaxLogSizeMb(int v)
{
  if (m_maxLogSizeMb != v) { m_maxLogSizeMb = v; SAVE_SETTING("maxLogSizeMb", v); emit settingsChanged(); }
}

// AI 助手（OWzx-only，docs/ai-control.md）
void SettingsViewModel::setAiEnabled(bool v)
{
  if (m_aiEnabled != v) { m_aiEnabled = v; SAVE_SETTING("aiEnabled", v); emit settingsChanged(); }
}

void SettingsViewModel::setAiApiKey(const QString &v)
{
  if (m_aiApiKey != v) { m_aiApiKey = v; SAVE_SETTING("aiApiKey", v); emit settingsChanged(); }
}

void SettingsViewModel::setAiModel(const QString &v)
{
  const QString trimmed = v.trimmed();
  if (!trimmed.isEmpty() && m_aiModel != trimmed) { m_aiModel = trimmed; SAVE_SETTING("aiModel", trimmed); emit settingsChanged(); }
}

void SettingsViewModel::setAiBaseUrl(const QString &v)
{
  const QString trimmed = v.trimmed();
  if (!trimmed.isEmpty() && m_aiBaseUrl != trimmed) { m_aiBaseUrl = trimmed; SAVE_SETTING("aiBaseUrl", trimmed); emit settingsChanged(); }
}

void SettingsViewModel::setAiPort(int v)
{
  const int clamped = qBound(1024, v, 65535);
  if (m_aiPort != clamped) { m_aiPort = clamped; SAVE_SETTING("aiPort", clamped); emit settingsChanged(); }
}

// Phase 241 (PAGE-04): mm <-> inch display conversion (upstream
// use_inches, Preferences.cpp:1109-1110). Display-only — every stored
// transform / size value stays in mm.
double SettingsViewModel::displayLength(double mm) const
{
  return m_units == 1 ? mm / 25.4 : mm;
}

double SettingsViewModel::storageLength(double display) const
{
  return m_units == 1 ? display * 25.4 : display;
}

QString SettingsViewModel::lengthUnitLabel() const
{
  return m_units == 1 ? QStringLiteral("in") : QStringLiteral("mm");
}
