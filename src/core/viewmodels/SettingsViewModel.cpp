#include "SettingsViewModel.h"
#include <QCoreApplication>
#include <QSettings>

// Helper: save a setting value and sync
#define SAVE_SETTING(key, val) do { QSettings s; s.setValue(key, val); } while(0)

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
  QSettings s;
  m_themeIndex       = s.value("themeIndex", m_themeIndex).toInt();
  m_fontSize         = s.value("fontSize", m_fontSize).toInt();
  m_uiScaleIndex     = s.value("uiScaleIndex", m_uiScaleIndex).toInt();
  m_languageIndex    = s.value("languageIndex", m_languageIndex).toInt();
  m_showHomePage     = s.value("showHomePage", m_showHomePage).toBool();
  m_defaultPage      = s.value("defaultPage", m_defaultPage).toInt();
  m_units            = s.value("units", m_units).toInt();
  m_userRole         = s.value("userRole", m_userRole).toInt();
  m_autoSave         = s.value("autoSave", m_autoSave).toBool();
  m_autoSaveInterval = s.value("autoSaveInterval", m_autoSaveInterval).toInt();
  m_checkUpdates     = s.value("checkUpdates", m_checkUpdates).toBool();
  m_reducedMotion    = s.value("reducedMotion", m_reducedMotion).toBool();
  m_region           = s.value("region", m_region).toInt();
  m_compactMode      = s.value("compactMode", m_compactMode).toBool();
  m_autoBackup       = s.value("autoBackup", m_autoBackup).toBool();
  m_undoLimit        = s.value("undoLimit", m_undoLimit).toInt();
  m_defaultNozzleIndex = s.value("defaultNozzleIndex", m_defaultNozzleIndex).toInt();
  m_defaultBedShape  = s.value("defaultBedShape", m_defaultBedShape).toInt();
  m_autoUpload       = s.value("autoUpload", m_autoUpload).toBool();
  m_updateChannel    = s.value("updateChannel", m_updateChannel).toInt();
  m_notificationsEnabled = s.value("notificationsEnabled", m_notificationsEnabled).toBool();
  m_hintsEnabled     = s.value("hintsEnabled", m_hintsEnabled).toBool();
  m_autoDismissSec   = s.value("autoDismissSec", m_autoDismissSec).toInt();
  m_showProgressNotifications = s.value("showProgressNotifications", m_showProgressNotifications).toBool();
  m_developerMode    = s.value("developerMode", m_developerMode).toBool();
  m_showDebugOverlay = s.value("showDebugOverlay", m_showDebugOverlay).toBool();
  m_logLevel         = s.value("logLevel", m_logLevel).toInt();
  m_verboseGcode     = s.value("verboseGcode", m_verboseGcode).toBool();
  m_glDebugContext   = s.value("glDebugContext", m_glDebugContext).toBool();
  m_maxLogSizeMb     = s.value("maxLogSizeMb", m_maxLogSizeMb).toInt();
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
  QSettings s;
  s.clear();
  setThemeIndex(0);
  setFontSize(12);
  setUiScaleIndex(0);
  setLanguageIndex(0);
  setShowHomePage(true);
  setDefaultPage(1);
  setUnits(0);
  setUserRole(0);
  setAutoSave(true);
  setAutoSaveInterval(10);
  setCheckUpdates(true);
  setReducedMotion(false);
  setRegion(0);
  setCompactMode(false);
  setAutoBackup(false);
  setUndoLimit(100);
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

void SettingsViewModel::setUserRole(int role)
{
  if (m_userRole != role) { m_userRole = role; SAVE_SETTING("userRole", role); emit settingsChanged(); }
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

void SettingsViewModel::setReducedMotion(bool v)
{
  if (m_reducedMotion != v) { m_reducedMotion = v; SAVE_SETTING("reducedMotion", v); emit settingsChanged(); }
}

void SettingsViewModel::setRegion(int r)
{
  if (m_region != r) { m_region = r; SAVE_SETTING("region", r); emit settingsChanged(); }
}

void SettingsViewModel::setCompactMode(bool v)
{
  if (m_compactMode != v) { m_compactMode = v; SAVE_SETTING("compactMode", v); emit settingsChanged(); }
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
