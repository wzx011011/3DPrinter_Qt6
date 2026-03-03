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
  QStringList presetNames() const { return m_presetNames; }
  QString currentPreset() const { return m_currentPreset; }
  double layerHeight() const { return m_layerHeight; }

signals:
  void prefCategoryChanged();
  void fontSizeChanged();
  void themeIndexChanged();
  void uiScaleIndexChanged();
  void languageIndexChanged();
  void presetsChanged();
  void configChanged();

public slots:
  void setPrefCategory(int cat);
  void setFontSize(int size);
  void setThemeIndex(int idx);
  void setUiScaleIndex(int idx);
  void setLanguageIndex(int idx);
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
};
