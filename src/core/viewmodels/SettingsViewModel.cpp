#include "SettingsViewModel.h"
#include <QCoreApplication>

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
    emit fontSizeChanged();
  }
}

void SettingsViewModel::setThemeIndex(int idx)
{
  if (m_themeIndex != idx)
  {
    m_themeIndex = idx;
    emit themeIndexChanged();
  }
}

void SettingsViewModel::setUiScaleIndex(int idx)
{
  if (m_uiScaleIndex != idx)
  {
    m_uiScaleIndex = idx;
    emit uiScaleIndexChanged();
  }
}

void SettingsViewModel::setLanguageIndex(int idx)
{
  if (m_languageIndex != idx)
  {
    m_languageIndex = idx;
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
}

void SettingsViewModel::setShowHomePage(bool v)
{
  if (m_showHomePage != v) { m_showHomePage = v; emit settingsChanged(); }
}

void SettingsViewModel::setDefaultPage(int page)
{
  if (m_defaultPage != page) { m_defaultPage = page; emit settingsChanged(); }
}

void SettingsViewModel::setUnits(int u)
{
  if (m_units != u) { m_units = u; emit settingsChanged(); }
}

void SettingsViewModel::setUserRole(int role)
{
  if (m_userRole != role) { m_userRole = role; emit settingsChanged(); }
}

void SettingsViewModel::setAutoSave(bool v)
{
  if (m_autoSave != v) { m_autoSave = v; emit settingsChanged(); }
}

void SettingsViewModel::setAutoSaveInterval(int minutes)
{
  if (m_autoSaveInterval != minutes) { m_autoSaveInterval = minutes; emit settingsChanged(); }
}

void SettingsViewModel::setCheckUpdates(bool v)
{
  if (m_checkUpdates != v) { m_checkUpdates = v; emit settingsChanged(); }
}

void SettingsViewModel::setReducedMotion(bool v)
{
  if (m_reducedMotion != v) { m_reducedMotion = v; emit settingsChanged(); }
}

void SettingsViewModel::setRegion(int r)
{
  if (m_region != r) { m_region = r; emit settingsChanged(); }
}

void SettingsViewModel::setCompactMode(bool v)
{
  if (m_compactMode != v) { m_compactMode = v; emit settingsChanged(); }
}

void SettingsViewModel::setAutoBackup(bool v)
{
  if (m_autoBackup != v) { m_autoBackup = v; emit settingsChanged(); }
}

void SettingsViewModel::setUndoLimit(int limit)
{
  if (m_undoLimit != limit) { m_undoLimit = limit; emit settingsChanged(); }
}
