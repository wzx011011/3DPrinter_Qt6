#include "BackendContext.h"

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CalibrationServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceServiceMock.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "core/viewmodels/HomeViewModel.h"
#include "core/viewmodels/SettingsViewModel.h"
#include "core/viewmodels/ProjectViewModel.h"
#include "core/viewmodels/CalibrationViewModel.h"
#include "core/viewmodels/ModelMallViewModel.h"
#include "core/viewmodels/MultiMachineViewModel.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QFont>
#include <iterator>

// 主题颜色预设
struct ThemeColors
{
  QColor bg, surface, sidebar, border;
};
static const ThemeColors kThemes[] = {
    {"#0d0f12", "#0f1217", "#0f1218", "#242a33"}, // 0 深色（默认）
    {"#060d18", "#081020", "#061428", "#112244"}, // 1 深蓝
    {"#050507", "#080809", "#070708", "#1a1a1e"}, // 2 极暗
};
// uiScale 预设
static const double kScales[] = {1.0, 1.25, 1.5, 1.75, 2.0};
// 语言代码映射
static const char *kLangCodes[] = {"zh_CN", "en", "ja", "ko", "de", "fr"};

BackendContext::BackendContext(QObject *parent)
    : QObject(parent)
{
  const QByteArray compareMode = qgetenv("CREALITY_VISUAL_COMPARE_MODE");
  visualCompareMode_ = (compareMode == "1" || compareMode.compare("true", Qt::CaseInsensitive) == 0);

  calibrationService_ = new CalibrationServiceMock(this);
  sliceService_ = new SliceServiceMock(this);
  presetService_ = new PresetServiceMock(this);
  deviceService_ = new DeviceServiceMock(this);
  projectService_ = new ProjectServiceMock(this);
  networkService_ = new NetworkServiceMock(this);

  editorViewModel_ = new EditorViewModel(projectService_, sliceService_, this);
  previewViewModel_ = new PreviewViewModel(sliceService_, this);
  monitorViewModel_ = new MonitorViewModel(deviceService_, networkService_, this);
  configViewModel_ = new ConfigViewModel(presetService_, this);
  homeViewModel_ = new HomeViewModel(this);
  settingsViewModel_ = new SettingsViewModel(this);
  projectViewModel_ = new ProjectViewModel(this);
  calibrationViewModel_ = new CalibrationViewModel(calibrationService_, this);
  modelMallViewModel_ = new ModelMallViewModel(this);
  multiMachineViewModel_ = new MultiMachineViewModel(this);

  // 实时监听偏好设置变化
  connect(settingsViewModel_, &SettingsViewModel::themeIndexChanged, this,
          [this]()
          { applyTheme(settingsViewModel_->themeIndex()); });
  connect(settingsViewModel_, &SettingsViewModel::uiScaleIndexChanged, this,
          [this]()
          { applyUiScale(settingsViewModel_->uiScaleIndex()); });
  connect(settingsViewModel_, &SettingsViewModel::fontSizeChanged, this,
          [this]()
          { applyFontSize(settingsViewModel_->fontSize()); });
  connect(settingsViewModel_, &SettingsViewModel::languageIndexChanged, this,
          [this]()
          { applyLanguage(settingsViewModel_->languageIndex()); });
}

QObject *BackendContext::editorViewModel() const { return editorViewModel_; }
QObject *BackendContext::previewViewModel() const { return previewViewModel_; }
QObject *BackendContext::monitorViewModel() const { return monitorViewModel_; }
QObject *BackendContext::configViewModel() const { return configViewModel_; }
QObject *BackendContext::homeViewModel() const { return homeViewModel_; }
QObject *BackendContext::settingsViewModel() const { return settingsViewModel_; }
QObject *BackendContext::projectViewModel() const { return projectViewModel_; }
QObject *BackendContext::calibrationViewModel() const { return calibrationViewModel_; }
QObject *BackendContext::modelMallViewModel() const { return modelMallViewModel_; }
QObject *BackendContext::multiMachineViewModel() const { return multiMachineViewModel_; }

bool BackendContext::visualCompareMode() const
{
  return visualCompareMode_;
}

int BackendContext::currentPage() const
{
  return currentPage_;
}

void BackendContext::setCurrentPage(int page)
{
  if (currentPage_ == page)
    return;
  currentPage_ = page;
  emit currentPageChanged();
}
void BackendContext::openSettings()
{
  setCurrentPage(11);
}
void BackendContext::postError(const QString &message, int severity)
{
  lastErrorMessage_ = message;
  lastErrorSeverity_ = severity;
  emit errorChanged();
}

void BackendContext::clearError()
{
  lastErrorMessage_.clear();
  lastErrorSeverity_ = -1;
  emit errorChanged();
}

QString BackendContext::lastErrorMessage() const { return lastErrorMessage_; }
int BackendContext::lastErrorSeverity() const { return lastErrorSeverity_; }

// ------- 外观实现 -------

void BackendContext::applyTheme(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(std::size(kThemes)))
    return;
  m_bgColor = kThemes[idx].bg;
  m_surfaceColor = kThemes[idx].surface;
  m_sidebarColor = kThemes[idx].sidebar;
  m_borderColor = kThemes[idx].border;
  emit themeChanged();
}

void BackendContext::applyLanguage(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(std::size(kLangCodes)))
    return;

  if (m_translator)
  {
    QCoreApplication::removeTranslator(m_translator);
    delete m_translator;
    m_translator = nullptr;
  }

  m_translator = new QTranslator(this);
  const QString qmFile = QString(":/i18n/%1.qm").arg(kLangCodes[idx]);
  if (m_translator->load(qmFile))
    QCoreApplication::installTranslator(m_translator);
  else
  {
    delete m_translator;
    m_translator = nullptr;
  }
  emit languageChanged();
}

void BackendContext::applyFontSize(int size)
{
  if (QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
  {
    QFont f = app->font();
    f.setPixelSize(size);
    app->setFont(f);
  }
}

void BackendContext::applyUiScale(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(std::size(kScales)))
    return;
  m_uiScale = kScales[idx];
  emit uiScaleChanged();
}
