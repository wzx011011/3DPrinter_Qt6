#include "BackendContext.h"

#include <QFileInfo>

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CameraServiceMock.h"
#include "core/services/CloudServiceMock.h"
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
#include <QTimer>
#include <QRandomGenerator>

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
  auto *cameraService = new CameraServiceMock(this);
  auto *cloudService = new CloudServiceMock(this);

  editorViewModel_ = new EditorViewModel(projectService_, sliceService_, this);
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, &BackendContext::displayProjectTitleChanged);
  previewViewModel_ = new PreviewViewModel(sliceService_, this);
  monitorViewModel_ = new MonitorViewModel(deviceService_, networkService_, cameraService, this);
  configViewModel_ = new ConfigViewModel(presetService_, projectService_, this);
  homeViewModel_ = new HomeViewModel(cloudService, this);
  settingsViewModel_ = new SettingsViewModel(this);
  projectViewModel_ = new ProjectViewModel(this);
  calibrationViewModel_ = new CalibrationViewModel(calibrationService_, this);
  calibrationViewModel_->setPresetService(presetService_);
  modelMallViewModel_ = new ModelMallViewModel(this);
  multiMachineViewModel_ = new MultiMachineViewModel(this);

  // 初始化提示数据库（对齐上游 HintDatabase::init）
  initHintDatabase();
  m_hintTimer = new QTimer(this);
  m_hintTimer->setInterval(60000); // 每 60 秒尝试显示提示（对齐上游 30s 间隔，Mock 模式降低频率）
  connect(m_hintTimer, &QTimer::timeout, this, [this]()
          {
    if (m_hintsEnabled && m_notificationsEnabled &&
        lastErrorMessage_.isEmpty() && m_hints.size() > 0)
      postHint();
  });
  m_hintTimer->start();

  connect(projectService_, &ProjectServiceMock::loadFinished, this,
          [this](bool success, const QString &message)
          {
            if (success)
              clearError();
            else
              postError(message.isEmpty() ? tr("导入失败") : message, 2);
          });

  // 切片进度通知（对齐上游 NotificationManager::SlicingProgressNotification）
  if (sliceService_)
  {
    connect(sliceService_, &SliceService::progressUpdated, this,
            [this](int percent, const QString &label)
            { postSlicingProgress(percent, label); });
    connect(sliceService_, &SliceService::sliceFinished, this,
            [this](const QString &estimatedTime)
            {
              Q_UNUSED(estimatedTime)
              postSlicingComplete();
            });
    connect(sliceService_, &SliceService::sliceFailed, this,
            [this](const QString &message)
            {
              postNotification(message, tr("切片失败"), NotiError);
            });
  }

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

  bool loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;

  // In Mock mode (no libslic3r), loadFile() fails for 3MF — try JSON project load
  if (!loaded && projectService_)
  {
    loaded = projectService_->loadProject(localPath);
    if (loaded && editorViewModel_)
    {
      editorViewModel_->refreshAfterLoad();
      setCurrentPage(1);
    }
  }

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

  // 实际保存项目数据到磁盘（对齐上游 Plater::save_project）
  if (projectService_ && !projectService_->saveProject(projectViewModel_->currentProjectPath()))
  {
    postError(projectService_->lastError(), 1);
    pushLatencySample(QStringLiteral("topbar-save-project"), int(m_latencyClock.elapsed() - start), QString());
    return false;
  }

  projectViewModel_->saveProject();
  postError(tr("项目已保存"), 0);
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

  // 实际保存项目数据到磁盘
  if (projectService_ && !projectService_->saveProject(localPath))
  {
    postError(projectService_->lastError(), 1);
    pushLatencySample(QStringLiteral("topbar-save-project-as"), int(m_latencyClock.elapsed() - start), localPath);
    return false;
  }

  projectViewModel_->saveProjectAs(localPath);
  postError(tr("项目已保存到: %1").arg(QFileInfo(localPath).fileName()), 0);
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
  if (!m_notificationsEnabled)
    return;

  // 将旧的当前通知移入历史
  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = message;
  lastErrorSeverity_ = severity;
  lastErrorTitle_.clear();
  m_currentNotification.message = message;
  m_currentNotification.title.clear();
  m_currentNotification.severity = severity;
  m_currentNotification.timestamp = QDateTime::currentDateTime();
  emit errorChanged();
}

void BackendContext::postNotification(const QString &message, const QString &title, int severity)
{
  if (!m_notificationsEnabled)
    return;

  NotificationEntry entry;
  entry.message = message;
  entry.title = title;
  entry.severity = severity;
  entry.timestamp = QDateTime::currentDateTime();

  if (lastErrorMessage_.isEmpty())
  {
    lastErrorMessage_ = message;
    lastErrorSeverity_ = severity;
    lastErrorTitle_ = title;
    m_currentNotification = entry;
    emit errorChanged();
  }
  else
  {
    m_notificationQueue.enqueue(entry);
    emit errorChanged();
  }
}

void BackendContext::dismissNotification()
{
  // 将当前通知移入历史（对齐上游 notification_manager 历史记录）
  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    // 保留最近 100 条历史
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_.clear();
  lastErrorTitle_.clear();
  lastErrorSeverity_ = -1;
  m_currentNotification = {};
  showNextNotification();
}

void BackendContext::clearError()
{
  dismissNotification();
}

void BackendContext::showNextNotification()
{
  if (!m_notificationQueue.isEmpty())
  {
    const auto entry = m_notificationQueue.dequeue();
    lastErrorMessage_ = entry.message;
    lastErrorSeverity_ = entry.severity;
    lastErrorTitle_ = entry.title;
    m_currentNotification = entry;
  }
  emit errorChanged();
}

int BackendContext::pendingNotificationCount() const
{
  return m_notificationQueue.size();
}

int BackendContext::currentNotificationProgress() const
{
  return m_currentNotification.progressValue;
}

bool BackendContext::currentNotificationHasProgress() const
{
  return m_currentNotification.hasProgress;
}

bool BackendContext::currentNotificationPersistent() const
{
  return m_currentNotification.persistent;
}

void BackendContext::updateNotificationProgress(int value)
{
  m_currentNotification.progressValue = qBound(
    m_currentNotification.progressMin,
    value,
    m_currentNotification.progressMax);
  emit errorChanged();
}

void BackendContext::confirmCurrentNotification()
{
  // 对齐上游 notification_manager::confirm
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  dismissNotification();
}

void BackendContext::cancelCurrentNotification()
{
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  dismissNotification();
}

// ------- 专用通知类型实现（对齐上游 NotificationManager 专用通知） -------

int BackendContext::currentNotificationType() const
{
  return m_currentNotification.type;
}

bool BackendContext::currentNotificationShowExport() const
{
  return m_currentNotification.showExportButton;
}

bool BackendContext::currentNotificationShowPreview() const
{
  return m_currentNotification.showPreviewButton;
}

void BackendContext::postSlicingProgress(int percent, const QString &stage)
{
  if (!m_notificationsEnabled)
    return;

  NotificationEntry entry;
  entry.type = NotiTypeSlicingProgress;
  entry.severity = NotiSlicingProgress;
  entry.persistent = true;
  entry.hasProgress = true;
  entry.progressValue = qBound(0, percent, 100);
  entry.title = tr("切片中");
  entry.message = stage.isEmpty()
      ? tr("正在切片... %1%").arg(percent)
      : tr("%1... %2%").arg(stage, QString::number(percent));
  entry.timestamp = QDateTime::currentDateTime();

  // 如果当前通知也是切片进度，就地更新（不排队）
  if (m_currentNotification.type == NotiTypeSlicingProgress && !lastErrorMessage_.isEmpty())
  {
    m_currentNotification.progressValue = entry.progressValue;
    m_currentNotification.message = entry.message;
    m_currentNotification.title = entry.title;
    emit errorChanged();
    return;
  }

  // 关闭旧通知并存入历史
  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = entry.message;
  lastErrorSeverity_ = entry.severity;
  lastErrorTitle_ = entry.title;
  m_currentNotification = entry;
  emit errorChanged();
}

void BackendContext::postSlicingComplete()
{
  if (!m_notificationsEnabled)
    return;

  NotificationEntry entry;
  entry.type = NotiTypeSlicingProgress;
  entry.severity = NotiSuccess;
  entry.title = tr("切片完成");
  entry.message = tr("切片已完成，可以预览或导出 G-code");
  entry.persistent = true;
  entry.requiresConfirm = false;
  entry.showExportButton = true;
  entry.showPreviewButton = true;
  entry.timestamp = QDateTime::currentDateTime();

  // 替换切片进度通知
  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = entry.message;
  lastErrorSeverity_ = entry.severity;
  lastErrorTitle_ = entry.title;
  m_currentNotification = entry;
  emit errorChanged();
}

void BackendContext::postExportFinished(const QString &filePath)
{
  postNotification(tr("已导出到: %1").arg(QFileInfo(filePath).fileName()),
                   tr("导出完成"), NotiSuccess);
}

void BackendContext::postExportOngoing(const QString &stage)
{
  NotificationEntry entry;
  entry.type = NotiTypeExportOngoing;
  entry.severity = NotiInfo;
  entry.title = tr("导出中");
  entry.message = stage.isEmpty() ? tr("正在导出 G-code...") : stage;
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();

  if (m_currentNotification.type == NotiTypeExportOngoing && !lastErrorMessage_.isEmpty())
  {
    m_currentNotification.message = entry.message;
    emit errorChanged();
    return;
  }

  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = entry.message;
  lastErrorSeverity_ = entry.severity;
  lastErrorTitle_ = entry.title;
  m_currentNotification = entry;
  emit errorChanged();
}

void BackendContext::postPlaterWarning(const QString &message)
{
  // PlaterWarning: 对象超出打印范围但不禁止切片（对齐上游 NormalNotificationLevel）
  postNotification(message, tr("工作区警告"), NotiWarning);
}

void BackendContext::postPlaterError(const QString &message)
{
  // PlaterError: 对象超出打印范围，无法切片（对齐上游 ErrorNotificationLevel）
  postNotification(message, tr("工作区错误"), NotiError);
}

void BackendContext::postValidateError(const QString &message)
{
  // ValidateError: 切片验证错误（对齐上游 NormalNotificationLevel）
  postNotification(message, tr("验证错误"), NotiError);
}

void BackendContext::postValidateWarning(const QString &message)
{
  postNotification(message, tr("验证警告"), NotiWarning);
}

void BackendContext::postArrangeOngoing(int percent)
{
  NotificationEntry entry;
  entry.type = NotiTypeArrangeOngoing;
  entry.severity = NotiInfo;
  entry.title = tr("排列中");
  entry.message = tr("正在自动排列... %1%").arg(qBound(0, percent, 100));
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();

  if (m_currentNotification.type == NotiTypeArrangeOngoing && !lastErrorMessage_.isEmpty())
  {
    m_currentNotification.message = entry.message;
    emit errorChanged();
    return;
  }

  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = entry.message;
  lastErrorSeverity_ = entry.severity;
  lastErrorTitle_ = entry.title;
  m_currentNotification = entry;
  emit errorChanged();
}

// ------- 提示数据库实现（对齐上游 HintDatabase） -------

void BackendContext::initHintDatabase()
{
  // 对齐上游 HintData，Mock 模式内置一组 "Did you know" 提示
  auto add = [this](const QString &id, const QString &text, int weight,
                    const QString &hypertext, const QString &docLink,
                    const QString &cbType, const QString &cbTarget)
  {
    HintData h;
    h.id = id;
    h.text = text;
    h.weight = weight;
    h.hypertext = hypertext;
    h.documentationLink = docLink;
    h.callbackType = cbType;
    h.callbackTarget = cbTarget;
    m_hints.append(h);
  };

  add(QStringLiteral("hint_layer_height"),
      tr("层高越小打印越精细，但耗时越长。常用范围: 0.1mm - 0.3mm。"),
      10, tr("了解层高"), QStringLiteral("layer_height"),
      QStringLiteral("settings"), QStringLiteral("layer_height"));

  add(QStringLiteral("hint_infill"),
      tr("填充密度影响模型强度和重量。20% 适合大多数场景，100% 为实心。"),
      10, tr("填充设置"), QStringLiteral("infill_density"),
      QStringLiteral("settings"), QStringLiteral("infill_density"));

  add(QStringLiteral("hint_support"),
      tr("悬空角度超过 45° 的部分需要支撑。合理使用支撑可以提升打印质量。"),
      8, tr("支撑设置"), QStringLiteral("support"),
      QStringLiteral("settings"), QStringLiteral("support_type"));

  add(QStringLiteral("hint_speed"),
      tr("打印速度越快效率越高，但可能影响表面质量。建议先慢后快测试。"),
      7, tr("速度设置"), QStringLiteral("speed"),
      QStringLiteral("settings"), QStringLiteral("outer_wall_speed"));

  add(QStringLiteral("hint_brims"),
      tr("Brim（裙边）可以增加模型与热床的附着力，防止翘边。"),
      6, tr("附着力设置"), QStringLiteral("brim"),
      QStringLiteral("settings"), QStringLiteral("brim_type"));

  add(QStringLiteral("hint_temperature"),
      tr("喷嘴温度和热床温度需要根据耗材类型调整。PLA 通常 200°C/60°C。"),
      9, tr("温度设置"), QStringLiteral("temperature"),
      QStringLiteral("settings"), QStringLiteral("nozzle_temperature"));

  add(QStringLiteral("hint_retract"),
      tr("回退设置可以减少空驶时的拉丝现象。调整回退距离和速度可改善表面质量。"),
      5, tr("回退设置"), QStringLiteral("retract"),
      QStringLiteral("settings"), QStringLiteral("retract_length"));

  add(QStringLiteral("hint_perimeters"),
      tr("外壁层数越多模型表面越光滑。通常 2-3 层即可获得良好效果。"),
      6, tr("壁设置"), QStringLiteral("wall_loops"),
      QStringLiteral("settings"), QStringLiteral("wall_loops"));

  add(QStringLiteral("hint_adaptive_layer"),
      tr("自适应层高根据模型表面角度自动调整层高，在平坦区域使用大层高提高速度，在细节区域使用小层高提升质量。"),
      4, tr("了解自适应层高"), QStringLiteral("adaptive_layer_height"),
      QStringLiteral("settings"), QStringLiteral("adaptive_layer_height"));

  add(QStringLiteral("hint_multi_plate"),
      tr("使用多平板功能可以将不同模型安排到不同平板上，方便批量打印。"),
      3, tr("平板管理"), QStringLiteral("plate"),
      QStringLiteral("settings"), QStringLiteral("default_plate"));

  add(QStringLiteral("hint_gcode_preview"),
      tr("预览模式下可以逐层查看 G-code 路径，帮助您在打印前发现潜在问题。"),
      5, tr("预览模式"), QStringLiteral("preview"),
      QStringLiteral("settings"), QStringLiteral("preview"));

  add(QStringLiteral("hint_variable_layer"),
      tr("可变层高功能可以在重要表面区域使用更小的层高，其余区域保持较大层高以节省时间。"),
      3, tr("可变层高"), QStringLiteral("variable_layer_height"),
      QStringLiteral("settings"), QStringLiteral("variable_layer_height"));

  m_currentHintIndex = -1;
}

int BackendContext::selectNextHint(bool random)
{
  if (m_hints.isEmpty())
    return -1;

  // 对齐上游 HintDatabase::get_next_hint_id — 加权随机选择未显示提示
  QVector<int> candidates;
  int totalWeight = 0;
  for (int i = 0; i < m_hints.size(); ++i)
  {
    if (!m_displayedHintIds.contains(m_hints[i].id))
    {
      candidates.append(i);
      totalWeight += m_hints[i].weight;
    }
  }

  // 所有提示都已显示，重置
  if (candidates.isEmpty())
  {
    m_displayedHintIds.clear();
    for (int i = 0; i < m_hints.size(); ++i)
    {
      candidates.append(i);
      totalWeight += m_hints[i].weight;
    }
  }

  if (candidates.isEmpty())
    return 0;

  if (random && totalWeight > 0)
  {
    // 加权随机选择
    int r = QRandomGenerator::global()->bounded(totalWeight);
    int cumulative = 0;
    for (int idx : candidates)
    {
      cumulative += m_hints[idx].weight;
      if (r < cumulative)
        return idx;
    }
    return candidates.last();
  }

  // 顺序选择
  return (m_currentHintIndex + 1) % m_hints.size();
}

void BackendContext::postHint()
{
  if (!m_hintsEnabled || m_hints.isEmpty())
    return;

  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;

  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);

  NotificationEntry entry;
  entry.type = NotiTypeDidYouKnowHint;
  entry.severity = NotiHint;
  entry.title = tr("你知道吗");
  entry.message = m_hints[idx].text;
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();
  entry.hintHasPrev = (m_currentHintIndex > 0 || m_displayedHintIds.size() > 1);
  entry.hintHasNext = true;

  if (!lastErrorMessage_.isEmpty() && m_currentNotification.timestamp.isValid())
  {
    m_notificationHistory.prepend(m_currentNotification);
    ++m_unreadHistoryCount;
    if (m_notificationHistory.size() > 100)
      m_notificationHistory.removeLast();
    emit historyChanged();
  }

  lastErrorMessage_ = entry.message;
  lastErrorSeverity_ = entry.severity;
  lastErrorTitle_ = entry.title;
  m_currentNotification = entry;
  emit errorChanged();
}

void BackendContext::nextHint()
{
  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;

  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);
  m_currentNotification.message = m_hints[idx].text;
  lastErrorMessage_ = m_hints[idx].text;
  emit errorChanged();
}

void BackendContext::prevHint()
{
  if (m_hints.isEmpty() || m_currentHintIndex < 0)
    return;
  m_currentHintIndex = (m_currentHintIndex - 1 + m_hints.size()) % m_hints.size();
  m_currentNotification.message = m_hints[m_currentHintIndex].text;
  lastErrorMessage_ = m_hints[m_currentHintIndex].text;
  emit errorChanged();
}

int BackendContext::hintCount() const
{
  return m_hints.size();
}

int BackendContext::currentHintIndex() const
{
  return m_currentHintIndex;
}

QString BackendContext::currentHintText() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].text;
  return {};
}

QString BackendContext::currentHintHypertext() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].hypertext;
  return {};
}

QString BackendContext::currentHintFollowText() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return m_hints[m_currentHintIndex].followText;
  return {};
}

QString BackendContext::lastErrorMessage() const { return lastErrorMessage_; }
int BackendContext::lastErrorSeverity() const { return lastErrorSeverity_; }
QString BackendContext::lastErrorTitle() const { return lastErrorTitle_; }

// ------- 通知中心实现（对齐上游 notification_manager） -------

int BackendContext::historyCount() const
{
  return m_notificationHistory.size();
}

int BackendContext::unreadHistoryCount() const
{
  return m_unreadHistoryCount;
}

QString BackendContext::historyMessage(int index) const
{
  if (index < 0 || index >= m_notificationHistory.size())
    return {};
  return m_notificationHistory[index].message;
}

QString BackendContext::historyTitle(int index) const
{
  if (index < 0 || index >= m_notificationHistory.size())
    return {};
  return m_notificationHistory[index].title;
}

int BackendContext::historySeverity(int index) const
{
  if (index < 0 || index >= m_notificationHistory.size())
    return 0;
  return m_notificationHistory[index].severity;
}

QString BackendContext::historyTime(int index) const
{
  if (index < 0 || index >= m_notificationHistory.size())
    return {};
  return m_notificationHistory[index].timestamp.toString(QStringLiteral("HH:mm:ss"));
}

void BackendContext::clearHistory()
{
  m_notificationHistory.clear();
  m_unreadHistoryCount = 0;
  emit historyChanged();
}

void BackendContext::markHistoryRead()
{
  m_unreadHistoryCount = 0;
  emit historyChanged();
}

void BackendContext::setNotificationsEnabled(bool v)
{
  if (m_notificationsEnabled == v)
    return;
  m_notificationsEnabled = v;
  emit settingsChanged();
}

void BackendContext::setHintsEnabled(bool v)
{
  if (m_hintsEnabled == v)
    return;
  m_hintsEnabled = v;
  emit settingsChanged();
}

void BackendContext::setAutoDismissSec(int sec)
{
  sec = qBound(2, sec, 30);
  if (m_autoDismissSec == sec)
    return;
  m_autoDismissSec = sec;
  emit settingsChanged();
}

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

QString BackendContext::displayProjectTitle() const
{
  // 优先使用 EditorViewModel 的 projectName
  if (editorViewModel_) {
    const QString name = editorViewModel_->property("projectName").toString();
    if (!name.isEmpty())
      return name;
  }
  // 回退：从 projectViewModel 的 currentProjectPath 提取文件名
  if (projectViewModel_) {
    const QString path = projectViewModel_->property("currentProjectPath").toString();
    if (!path.isEmpty()) {
      const QFileInfo fi(path);
      return fi.fileName();
    }
  }
  return tr("未命名");
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
