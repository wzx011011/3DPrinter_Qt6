#include "BackendContext.h"

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CalibrationServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
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
#include <QUrl>
#include <algorithm>

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
  m_latencyClock.start();
  const QByteArray compareMode = qgetenv("CREALITY_VISUAL_COMPARE_MODE");
  visualCompareMode_ = (compareMode == "1" || compareMode.compare("true", Qt::CaseInsensitive) == 0);

  calibrationService_ = new CalibrationServiceMock(this);
  presetService_ = new PresetServiceMock(this);
  deviceService_ = new DeviceServiceMock(this);
  projectService_ = new ProjectServiceMock(this);
  sliceService_ = new SliceService(projectService_, this);
  networkService_ = new NetworkServiceMock(this);

  editorViewModel_ = new EditorViewModel(projectService_, sliceService_, this);
  previewViewModel_ = new PreviewViewModel(sliceService_, this);
  monitorViewModel_ = new MonitorViewModel(deviceService_, networkService_, this);
  configViewModel_ = new ConfigViewModel(presetService_, projectService_, this);
  homeViewModel_ = new HomeViewModel(this);
  settingsViewModel_ = new SettingsViewModel(this);
  projectViewModel_ = new ProjectViewModel(this);
  calibrationViewModel_ = new CalibrationViewModel(calibrationService_, this);
  modelMallViewModel_ = new ModelMallViewModel(this);
  multiMachineViewModel_ = new MultiMachineViewModel(this);

  connect(projectService_, &ProjectServiceMock::loadFinished, this,
          [this](bool success, const QString &message)
          {
            if (success)
              clearError();
            else
              postError(message.isEmpty() ? tr("导入失败") : message, 2);
          });

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

void BackendContext::topbarNewProject()
{
  const qint64 start = m_latencyClock.elapsed();
  if (projectViewModel_)
    projectViewModel_->newProject();
  if (editorViewModel_)
    editorViewModel_->clearWorkspace();
  setCurrentPage(1);
  pushLatencySample(QStringLiteral("topbar-new-project"), int(m_latencyClock.elapsed() - start), QString());
}

bool BackendContext::topbarOpenProject(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  const bool loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;
  if (loaded)
  {
    if (projectViewModel_)
      projectViewModel_->openProject(localPath);
    setCurrentPage(1);
  }
  pushLatencySample(QStringLiteral("topbar-open-project"), int(m_latencyClock.elapsed() - start), localPath);
  return loaded;
}

bool BackendContext::topbarImportModel(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  const bool loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;
  if (loaded)
  {
    if (projectViewModel_)
      projectViewModel_->importModel(QStringList{localPath});
    setCurrentPage(1);
  }
  pushLatencySample(QStringLiteral("topbar-import-model"), int(m_latencyClock.elapsed() - start), localPath);
  return loaded;
}

bool BackendContext::topbarSaveProject()
{
  const qint64 start = m_latencyClock.elapsed();
  if (!projectViewModel_)
    return false;

  if (projectViewModel_->currentProjectPath().isEmpty())
    return false;

  projectViewModel_->saveProject();
  pushLatencySample(QStringLiteral("topbar-save-project"), int(m_latencyClock.elapsed() - start), QString());
  return true;
}

bool BackendContext::topbarSaveProjectAs(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  if (!projectViewModel_)
    return false;

  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  projectViewModel_->saveProjectAs(localPath);
  pushLatencySample(QStringLiteral("topbar-save-project-as"), int(m_latencyClock.elapsed() - start), localPath);
  return true;
}

int BackendContext::beginLatency(const QString &operation, const QString &detail)
{
  if (operation.isEmpty())
    return -1;
  const int token = m_latencyNextToken++;
  m_pendingLatencies.insert(token, PendingLatency{operation, detail, m_latencyClock.elapsed()});
  return token;
}

void BackendContext::endLatency(int token)
{
  const auto it = m_pendingLatencies.find(token);
  if (it == m_pendingLatencies.end())
    return;
  const PendingLatency pending = it.value();
  m_pendingLatencies.erase(it);
  const int elapsedMs = int(m_latencyClock.elapsed() - pending.startMs);
  pushLatencySample(pending.operation, elapsedMs, pending.detail);
}

void BackendContext::recordLatency(const QString &operation, int elapsedMs, const QString &detail)
{
  pushLatencySample(operation, elapsedMs, detail);
}

void BackendContext::resetLatency()
{
  m_pendingLatencies.clear();
  m_latencyStats.clear();
  m_lastLatencyOperation.clear();
  m_lastLatencyMs = 0;
  emit latencyChanged();
}

QString BackendContext::latencyBrief() const
{
  const auto tabIt = m_latencyStats.find(QStringLiteral("tab-switch"));
  if (tabIt == m_latencyStats.end() || tabIt->count == 0)
    return QStringLiteral("Latency: --");

  const auto &s = tabIt.value();
  const int avgMs = s.totalMs / s.count;
  const int p95Ms = percentile95(s.samples);
  return QStringLiteral("Tab %1ms | Avg %2ms | P95 %3ms")
      .arg(s.lastMs)
      .arg(avgMs)
      .arg(p95Ms);
}

QString BackendContext::lastLatencyOperation() const
{
  return m_lastLatencyOperation;
}

int BackendContext::lastLatencyMs() const
{
  return m_lastLatencyMs;
}

void BackendContext::pushLatencySample(const QString &operation, int elapsedMs, const QString &detail)
{
  if (operation.isEmpty() || elapsedMs < 0)
    return;

  auto &stats = m_latencyStats[operation];
  stats.count += 1;
  stats.totalMs += elapsedMs;
  stats.lastMs = elapsedMs;
  stats.maxMs = std::max(stats.maxMs, elapsedMs);
  stats.samples.push_back(elapsedMs);
  constexpr int kMaxSamples = 120;
  if (stats.samples.size() > kMaxSamples)
    stats.samples.remove(0, stats.samples.size() - kMaxSamples);

  m_lastLatencyOperation = detail.isEmpty() ? operation : QStringLiteral("%1 (%2)").arg(operation, detail);
  m_lastLatencyMs = elapsedMs;

  if (elapsedMs > 120)
    qWarning() << "[Latency] slow" << operation << elapsedMs << "ms" << detail;
  else
    qInfo() << "[Latency]" << operation << elapsedMs << "ms" << detail;

  emit latencyChanged();
}

int BackendContext::percentile95(const QVector<int> &samples)
{
  if (samples.isEmpty())
    return 0;
  QVector<int> sorted = samples;
  std::sort(sorted.begin(), sorted.end());
  const int sampleCount = int(sorted.size());
  const int idx = std::min(sampleCount - 1, int((sampleCount - 1) * 0.95));
  return sorted[idx];
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
