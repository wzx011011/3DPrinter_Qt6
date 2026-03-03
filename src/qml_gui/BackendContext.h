#pragma once

#include <QObject>
#include <QColor>
#include <QTranslator>

class SliceServiceMock;
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
  // 外观 / 缩放 / 主题颜色
  Q_PROPERTY(double uiScale READ uiScale NOTIFY uiScaleChanged)
  Q_PROPERTY(QColor bgColor READ bgColor NOTIFY themeChanged)
  Q_PROPERTY(QColor surfaceColor READ surfaceColor NOTIFY themeChanged)
  Q_PROPERTY(QColor sidebarColor READ sidebarColor NOTIFY themeChanged)
  Q_PROPERTY(QColor borderColor READ borderColor NOTIFY themeChanged)

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

  Q_INVOKABLE void setCurrentPage(int page);
  Q_INVOKABLE void postError(const QString &message, int severity = 0);
  Q_INVOKABLE void clearError();
  Q_INVOKABLE void openSettings(); // H3

  QString lastErrorMessage() const;
  int lastErrorSeverity() const;

signals:
  void currentPageChanged();
  void errorChanged();
  void uiScaleChanged();
  void themeChanged();
  void languageChanged();

private:
  CalibrationServiceMock *calibrationService_ = nullptr;
  SliceServiceMock *sliceService_ = nullptr;
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
  int lastErrorSeverity_ = -1;

  // 外观状态
  QTranslator *m_translator = nullptr;
  double m_uiScale = 1.0;
  QColor m_bgColor{"#0d0f12"};
  QColor m_surfaceColor{"#0f1217"};
  QColor m_sidebarColor{"#0f1218"};
  QColor m_borderColor{"#242a33"};

  void applyTheme(int idx);
  void applyLanguage(int idx);
  void applyFontSize(int size);
  void applyUiScale(int idx);
};
