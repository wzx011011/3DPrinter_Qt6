#include "SettingsViewModel.h"
#include <QCoreApplication>

static QStringList categoryTitles()
{
  return {
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
}
