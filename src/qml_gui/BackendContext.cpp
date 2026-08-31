#include "BackendContext.h"

#include <QFileInfo>

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/CameraServiceMock.h"
#include "core/services/CloudServiceMock.h"
#include "core/services/CalibrationServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/PluginService.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/AppSettingsService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "core/viewmodels/HomeViewModel.h"
#include "core/viewmodels/SettingsViewModel.h"
#include "core/viewmodels/ProjectViewModel.h"
#include "core/viewmodels/CalibrationViewModel.h"
#include "core/viewmodels/MultiMachineViewModel.h"
#include "core/viewmodels/AmsMaterialsViewModel.h"
#include "core/viewmodels/AiViewModel.h"
#include "AiChatBridge.h"
#include "core/ai/AiAgentService.h"
#include "core/ai/AppToolRegistry.h"
#include "core/ai/McpHttpServer.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QFont>
#include <iterator>
#include <utility>
#include <QUrl>
#include <algorithm>
#include <QTimer>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QWebChannel>
#include <QJsonObject>
#include <QDesktopServices>
#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QMetaEnum>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGRendererInterface>
#include <QSysInfo>
#include <QDir>

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
  const QByteArray compareMode = qgetenv("OWZX_VISUAL_COMPARE_MODE");
  visualCompareMode_ = (compareMode == "1" || compareMode.compare("true", Qt::CaseInsensitive) == 0);

  calibrationService_ = new CalibrationServiceMock(this);
  presetService_ = new PresetServiceMock(this);
  deviceService_ = new DeviceServiceMock(this);
  projectService_ = new ProjectServiceMock(this);
  sliceService_ = new SliceService(projectService_, this);
  // v2.7 P1：注入 SliceService 到 CalibrationService（校准时触发 calib slice）
  calibrationService_->setSliceService(sliceService_);
  // Phase 197: inject ProjectServiceMock so the tower modes can load a
  // dedicated tower model (qrc:/qml/assets/calib/*.stl/.step) before slicing.
  calibrationService_->setProjectService(projectService_);
  networkService_ = new NetworkServiceMock(this);
  cameraService_ = new CameraServiceMock(this);
  auto *cloudService = new CloudServiceMock(this);

  editorViewModel_ = new EditorViewModel(projectService_, sliceService_, this);
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, &BackendContext::displayProjectTitleChanged);

  // Undo/Redo 框架（对齐上游 UndoRedo）：创建管理器并注入到 EditorViewModel
  auto *undoManager = new UndoRedoManager(this);
  editorViewModel_->setUndoRedoManager(undoManager);
  // Phase 118 (TICK-02/TICK-03): inject projectService_ so PreviewViewModel can
  // write tickMarks_ back into libslic3r's plates_custom_gcodes and re-slice.
  previewViewModel_ = new PreviewViewModel(projectService_, sliceService_, this);
  // Phase 51 SHELL-02/SHELL-03: forward viewmodel stateChanged so shell gates
  // (canImport/canSlice/isSlicing/canExport/canSave/canUndo/canRedo/isBusy) and
  // Prepare/Preview state stay live across page round-trips.
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, [this]() {
    emit stateChanged();
  });
  connect(previewViewModel_, &PreviewViewModel::stateChanged, this, [this]() {
    emit stateChanged();
  });
  monitorViewModel_ = new MonitorViewModel(deviceService_, networkService_, cameraService_, this);
  configViewModel_ = new ConfigViewModel(presetService_, projectService_, this);
  // Phase 52 PREPSB-05 (CRITICAL): a real slicing config change makes any
  // previously-sliced/previewed/exported result stale. UI-only settings state
  // changes (opening a dialog or switching tabs) must not clear results.
  connect(configViewModel_, &ConfigViewModel::sliceAffectingConfigChanged, editorViewModel_,
          [this]() {
            editorViewModel_->invalidateAllSliceResults();
            emit editorViewModel_->stateChanged();
          });

  // Wire ConfigViewModel into EditorViewModel for preset injection at slice time
  // (对齐上游 PresetBundle::full_fff_config → BackgroundSlicingProcess)
  editorViewModel_->setConfigViewModel(configViewModel_);
  // v5.16 (PSET2-06): keep the per-extruder filament preset slot vector sized
  // to the extruder count (upstream update_multi_material_filament_presets).
  // setExtruderCount no-ops when the count is unchanged, so riding
  // stateChanged is cheap.
  configViewModel_->setExtruderCount(editorViewModel_->extruderCount());
  connect(editorViewModel_, &EditorViewModel::stateChanged, this, [this]() {
    configViewModel_->setExtruderCount(editorViewModel_->extruderCount());
  });
  homeViewModel_ = new HomeViewModel(cloudService, this);
  settingsViewModel_ = new SettingsViewModel(this);
  projectViewModel_ = new ProjectViewModel(this);
  // Phase 241 (PAGE-01/PAGE-02): Home mirrors the persisted recent list from
  // ProjectViewModel (single source of truth, upstream app_config
  // "recent_projects"); the recentChanged -> refresh wiring lives INSIDE
  // HomeViewModel::setProjectViewModel. ProjectViewModel's resource tree
  // reads the live project service. Recent-card activation routes through
  // topbarOpenProject (upstream Plater::load_file).
  homeViewModel_->setProjectViewModel(projectViewModel_);
  projectViewModel_->setProjectService(projectService_);
  calibrationViewModel_ = new CalibrationViewModel(calibrationService_, this);
  calibrationViewModel_->setPresetService(presetService_);
  multiMachineViewModel_ = new MultiMachineViewModel(this);
  // Phase 201 (v5.6 AMS Architecture Cleanup): mock data + persistence for
  // AMSSettingsDialog. Data source stays mock; persistence is local QSettings.
  amsMaterialsViewModel_ = new AmsMaterialsViewModel(this);
  // v2.8 W3: application-level persisted settings.
  appSettings_ = new AppSettingsService(this);
  // v2.8 W3: inject settings into SliceService for persisted bed size lookup.
  sliceService_->setAppSettings(appSettings_);
  // Phase 202 (v5.6 Plugin Manager UI Real Backend): plugin registry + mock
  // install/enable state with QSettings persistence under plugins/*.
  pluginService_ = new PluginService(this);

  // AI 助手（OWzx-only，docs/ai-control.md）：工具注册表把整个应用暴露为
  // JSON-Schema 工具；MCP 服务器在 127.0.0.1 上提供 tools/list + tools/call；
  // sidecar harness（Claude Agent SDK → GLM 智谱兼容端点）经 QProcess 接入。
  // BackendContext 本体实现 AppToolUiProvider 的三个 UI 动作。
  aiRegistry_ = new OWzx::AppToolRegistry(projectService_, sliceService_,
                                          configViewModel_, editorViewModel_,
                                          this);
  aiMcp_ = new OWzx::McpHttpServer(aiRegistry_, this);
  aiAgentService_ = new AiAgentService(this);
  aiViewModel_ = new AiViewModel(aiAgentService_, this);
  // Web chat page bridge (QWebChannel) — the WebEngine chat panel talks to
  // the same ViewModel/harness through this object. Registration happens in
  // attachAiChatChannel() once QML hands over its channel element.
  aiChatBridge_ = new AiChatBridge(aiViewModel_, aiAgentService_, this);
  // Preferences drive start/stop; re-apply on every settings change so the
  // sidebar reacts immediately to enable/key/model/port edits.
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          &BackendContext::applyAiSettings);
  applyAiSettings();

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

  // Phase 241 (PAGE-04): periodic project backup (upstream backup_switch,
  // Preferences.cpp:1179 "Backup your project periodically for restoring
  // from the occasional crash"). Interval follows the autoSaveInterval
  // preference in minutes; each tick backs up only when autoSave is on AND
  // the project has unsaved changes (save-on-change semantics — documented
  // delta from the upstream seconds-level crash backup timer).
  m_backupTimer = new QTimer(this);
  m_backupTimer->setInterval(settingsViewModel_->autoSaveInterval() * 60 * 1000);
  connect(m_backupTimer, &QTimer::timeout, this, [this]()
          {
    if (settingsViewModel_ && settingsViewModel_->autoSave()
        && projectViewModel_ && projectViewModel_->isDirty())
      triggerProjectBackup();
  });
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this, [this]()
          {
    if (m_backupTimer && settingsViewModel_)
      m_backupTimer->setInterval(
          qMax(1, settingsViewModel_->autoSaveInterval()) * 60 * 1000);
  });
  m_backupTimer->start();

  connect(projectService_, &ProjectServiceMock::loadFinished, this,
          [this](bool success, const QString &message)
          {
            if (success)
              clearError();
            else
              postError(message.isEmpty() ? tr("导入失败") : message, 2);
            // Phase 236 (DLG-03): after a successful load/drop, re-run the
            // outside-bed detection (upstream shows the outside-bed prompt on
            // import). A non-empty result raises the RecenterDialog.
            if (success && editorViewModel_ && editorViewModel_->checkObjectsOutsideBed() > 0)
              emit recenterPromptRequested();
          });
  // Phase 237 (VIEW-04): forward the editor viewmodel's post-import prompts.
  // Zero-volume removal mirrors upstream Model::removed_objects_with_
  // zero_volume + the Plater.cpp:4231-4233 info dialog (OWzx uses a
  // notification); the unit prompt re-emits so main.qml opens the confirm.
  if (editorViewModel_)
  {
    connect(editorViewModel_, &EditorViewModel::zeroVolumeObjectsRemoved, this,
            [this](int removedCount)
            {
              postNotification(tr("%1 object(s) with zero volume were removed.")
                                   .arg(removedCount),
                               tr("Objects with zero volume removed"));
            });
    connect(editorViewModel_, &EditorViewModel::unitConversionPromptRequested, this,
            &BackendContext::unitConversionPromptRequested);
  }

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
              requestSelectTab(static_cast<int>(TabPosition::tpPreview));
            });
    connect(sliceService_, &SliceService::sliceFailed, this,
            [this](const QString &message)
            {
              postNotification(message, tr("切片失败"), NotiError);
            });
    // Phase 239 (ENGN-03): upstream distinguishes non-fatal Print::validate
    // warnings from errors (Plater.cpp:13742-13759 process_validation_warning
    // shows them as a notification while the slice continues). Errors keep
    // flowing through sliceFailed above.
    connect(sliceService_, &SliceService::validateWarning, this,
            [this](const QString &message)
            { postValidateWarning(message); });

    connect(sliceService_, &SliceService::exportStarted, this,
            [this](const QString &stage)
            { postExportOngoing(stage); });
    connect(sliceService_, &SliceService::exportFinished, this,
            [this](const QString &filePath)
            { postExportFinished(filePath); });
    connect(sliceService_, &SliceService::exportFailed, this,
            [this](const QString &message)
            { postNotification(message, tr("Export failed"), NotiError); });

    // Propagate 3MF embedded config to ConfigViewModel on project load
    connect(projectService_, &ProjectServiceMock::projectConfigLoaded,
            configViewModel_, &ConfigViewModel::applyProjectConfig);
    // Phase 236 (DLG-03): 3MF generator-version notice (upstream
    // Newer3mfVersion / MsgDataIncompatible warning family). projectVersionInfo
    // is parsed on the load worker, so the check is signal-driven; the notice
    // is a non-modal plater warning + log line rather than upstream's modal
    // (no update server — out of scope).
    connect(projectService_, &ProjectServiceMock::projectVersionInfoChanged, this, [this]()
            {
      if (!projectService_)
        return;
      const QString versionInfo = projectService_->projectVersionInfo();
      if (versionInfo.isEmpty() || versionInfo.startsWith(QStringLiteral("OWzx"), Qt::CaseInsensitive))
        return;
      qInfo("[Backend] 3mf generator=%s", versionInfo.toUtf8().constData());
      postPlaterWarning(tr("Project was saved by %1. Some settings may differ from this version.")
                            .arg(versionInfo));
    });
    // v5.16 (PLATE-05): real edits (objects/plates/configs) drive the
    // unsaved-changes indicator. Loads are excluded — ProjectViewModel's
    // openProject/newProject clear the flag after loadFinished anyway.
    // Phase 241 (PAGE-02): the same edits refresh the ProjectPage resource
    // tree so object add/delete/rename show up without a page re-entry.
    connect(projectService_, &ProjectServiceMock::projectChanged, this, [this]()
            {
              if (projectService_ && !projectService_->loading() && projectViewModel_)
              {
                projectViewModel_->markDirty();
                projectViewModel_->refreshFileTree();
              }
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
  // 初始同步：启动时立即加载持久化的语言翻译（默认 zh_CN）。
  // applyLanguage 只连接到 languageIndexChanged 信号，而构造时 m_languageIndex
  // 默认已是 0 不会触发信号，导致程序以英文启动。此处补上初始调用。
  applyLanguage(settingsViewModel_->languageIndex());

  // 同步撤销栈上限设置到 UndoRedoManager（对齐上游 UndoRedo::UndoRedoStackLimit）
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          [this, undoManager]()
          { undoManager->setUndoLimit(settingsViewModel_->undoLimit()); });
  // 初始同步
  undoManager->setUndoLimit(settingsViewModel_->undoLimit());

  // 同步通知偏好设置到 BackendContext（对齐上游 notification_manager preferences）
  connect(settingsViewModel_, &SettingsViewModel::settingsChanged, this,
          [this]()
          {
            m_notificationsEnabled = settingsViewModel_->notificationsEnabled();
            m_hintsEnabled = settingsViewModel_->hintsEnabled();
            m_autoDismissSec = settingsViewModel_->autoDismissSec();
            m_showProgressNotifications = settingsViewModel_->showProgressNotifications();
          });

  // Load config wizard state from QSettings (对齐上游 ConfigWizard 首次运行检测)
  QSettings settings;
  m_configWizardCompleted = settings.value(QStringLiteral("wizard/completed"), false).toBool();

  // Phase 4: Load sidebar dockable state from QSettings
  // 对齐上游 app_config collapsed_sidebar；width/dockArea 为增强（上游无）
  // 命名空间 owzx/sidebar/* 避免与其它 key 冲突
  sidebarCollapsed_ = settings.value(QStringLiteral("owzx/sidebar/collapsed"), false).toBool();
  const QVariant savedSidebarWidthValue = settings.value(QStringLiteral("owzx/sidebar/width"));
  int savedWidth = savedSidebarWidthValue.isValid()
                     ? savedSidebarWidthValue.toInt()
                     : kSidebarDefaultWidth;
  const int sidebarSettingsVersion =
      settings.value(QStringLiteral("owzx/sidebar/settingsVersion"), 1).toInt();
  if (sidebarSettingsVersion < kSidebarSettingsVersion &&
      savedSidebarWidthValue.isValid()) {
    savedWidth = kSidebarDefaultWidth;
    settings.setValue(QStringLiteral("owzx/sidebar/width"), savedWidth);
    settings.setValue(QStringLiteral("owzx/sidebar/settingsVersion"), kSidebarSettingsVersion);
    settings.sync();
  }
  sidebarWidth_ = qBound(kSidebarMinWidth, savedWidth, kSidebarMaxWidth);  // clamp 防御损坏的存储值
  const int savedArea = settings.value(QStringLiteral("owzx/sidebar/dockArea"),
                                       static_cast<int>(SidebarDockArea::Left)).toInt();
  sidebarDockArea_ = (savedArea == static_cast<int>(SidebarDockArea::Right))
                       ? SidebarDockArea::Right
                       : SidebarDockArea::Left;  // 防御：非 Right 一律按 Left
}

QObject *BackendContext::editorViewModel() const { return editorViewModel_; }
QObject *BackendContext::sliceService() const { return sliceService_; }

QObject *BackendContext::aiViewModel() const { return aiViewModel_; }

QObject *BackendContext::aiChatBridge() const { return aiChatBridge_; }

void BackendContext::attachAiChatChannel(QObject *channel) {
  // channel is the QML WebChannel element (QQmlWebChannel) hosting the chat
  // page's transport; QQmlWebChannel derives from QWebChannel, so the public
  // base-API registration applies. The page resolves it as
  // channel.objects.bridge right after its own load.
  auto *webChannel = qobject_cast<QWebChannel *>(channel);
  if (!webChannel) {
    qWarning("[Backend] attachAiChatChannel: not a QWebChannel instance");
    return;
  }
  webChannel->registerObject(QStringLiteral("bridge"), aiChatBridge_);
}

void BackendContext::applyAiSettings()
{
  const bool enabled = settingsViewModel_->aiEnabled()
      && !settingsViewModel_->aiApiKey().isEmpty();

  if (!enabled) {
    aiAgentService_->stop();
    aiMcp_->stop();
    if (aiControlActive_) {
      aiControlActive_ = false;
      emit stateChanged();
    }
    return;
  }

  // Loopback MCP server first (the sidecar connects to it at startup).
  if (!aiMcp_->isActive()) {
    if (!aiMcp_->start(quint16(settingsViewModel_->aiPort()),
                       settingsViewModel_->aiControlToken())) {
      aiAgentService_->stop();
      postError(QStringLiteral("AI MCP 服务器启动失败（端口被占用？）"),
                /*severity=*/2);
      if (aiControlActive_) {
        aiControlActive_ = false;
        emit stateChanged();
      }
      return;
    }
  }

  // Sidecar layout: <appDir>/ai_sidecar/{agent.py, python/python.exe}
  // (assembled by scripts/package_ai_sidecar.ps1; optional component).
  const QString appDir = QCoreApplication::applicationDirPath();
  const QString sidecarDir = QDir(appDir).filePath(QStringLiteral("ai_sidecar"));
  AiAgentService::Config cfg;
  cfg.sidecarScriptPath = QDir(sidecarDir).filePath(QStringLiteral("agent.py"));
  cfg.pythonPath = QDir(sidecarDir).filePath(QStringLiteral("python/python.exe"));
  cfg.mcpUrl = QStringLiteral("http://127.0.0.1:%1/mcp")
                   .arg(aiMcp_->port());
  cfg.mcpToken = settingsViewModel_->aiControlToken();
  cfg.model = settingsViewModel_->aiModel();
  cfg.anthropicBaseUrl = settingsViewModel_->aiBaseUrl();
  cfg.apiKey = settingsViewModel_->aiApiKey();
  aiAgentService_->configure(cfg);
  if (!aiAgentService_->running())
    aiAgentService_->start();

  aiViewModel_->setEnabled(true);
  if (!aiControlActive_) {
    aiControlActive_ = true;
    emit stateChanged();
  }
}

bool BackendContext::switchPage(int position)
{
  if (position < 0 || position > 8)
    return false;
  requestSelectTab(position);
  return true;
}

bool BackendContext::toggleSidebar()
{
  requestToggleSidebar();
  return true;
}
QObject *BackendContext::previewViewModel() const { return previewViewModel_; }
QObject *BackendContext::monitorViewModel() const { return monitorViewModel_; }
QObject *BackendContext::configViewModel() const { return configViewModel_; }
QObject *BackendContext::homeViewModel() const { return homeViewModel_; }
QObject *BackendContext::settingsViewModel() const { return settingsViewModel_; }
QObject *BackendContext::projectViewModel() const { return projectViewModel_; }
QObject *BackendContext::calibrationViewModel() const { return calibrationViewModel_; }
QObject *BackendContext::multiMachineViewModel() const { return multiMachineViewModel_; }
QObject *BackendContext::amsMaterialsViewModel() const { return amsMaterialsViewModel_; }
QObject *BackendContext::appSettings() const { return appSettings_; }

QObject *BackendContext::pluginService() const { return pluginService_; }

// Phase 199 (WIZ-01): expose the preset data service so the ConfigWizard can
// enumerate vendors / printer models / materials / bed surfaces.
QObject *BackendContext::presetServiceMock() const { return presetService_; }

bool BackendContext::visualCompareMode() const
{
  return visualCompareMode_;
}

int BackendContext::currentPage() const
{
  return currentPage_;
}

// ── Phase 51: shell action gates (forward to EditorViewModel/PreviewViewModel) ──
// These gates aggregate upstream-aligned viewmodel state so the shell binds a
// single property per action (SHELL-03: action states live in C++, not QML-only).

bool BackendContext::isSlicing() const
{
  // Aggregate slice state across both viewmodels (editor prepares, preview renders).
  return (editorViewModel_ && editorViewModel_->isSlicing())
      || (previewViewModel_ && previewViewModel_->slicing());
}

bool BackendContext::isBusy() const
{
  // Any blocking operation in flight: slicing or mid-load.
  return isSlicing() || (editorViewModel_ && editorViewModel_->loading());
}

bool BackendContext::canImport() const
{
  // Import available whenever no blocking op is in flight (mirrors upstream File menu).
  return !isBusy();
}

bool BackendContext::canSlice() const
{
  return editorViewModel_ && editorViewModel_->canRequestSlice() && !isSlicing();
}

bool BackendContext::canExport() const
{
  return editorViewModel_ && editorViewModel_->canExportGCode() && !isSlicing();
}

bool BackendContext::canSave() const
{
  // Mutating the project mid-slice is unsafe (CONTEXT safety decision).
  return !isSlicing() && !isBusy();
}

bool BackendContext::canUndo() const
{
  // Raw undo-stack availability; the page gate (currentPage === tp3DEditor)
  // is applied in QML in Plan 51-02.
  // Phase 90 (ASMROUTE-01): the UndoRedoManager is a single shared stack
  // (90-CONTEXT.md decision 8), so undo/redo operates on the active canvas
  // regardless of view mode — mirroring upstream Plater.cpp:11744 (undo entry
  // routing) and Plater.cpp:11823 (selection passed to undo/redo). When
  // currentViewMode_ == ViewMode::AssembleView (CanvasAssembleView=2) the
  // stack still routes through the same editorViewModel_; no isolated
  // per-canvas stack is created.
  return editorViewModel_ && editorViewModel_->canUndo();
}

bool BackendContext::canRedo() const
{
  // Phase 90 (ASMROUTE-01): see canUndo() — shared single undo stack applies
  // to the AssembleView canvas host the same as Prepare/Preview.
  return editorViewModel_ && editorViewModel_->canRedo();
}

QString BackendContext::exportActionLabel() const
{
  // Stable label; exportActionHint() carries the blocked reason.
  return tr("Export G-code");
}

QString BackendContext::exportActionHint() const
{
  // No hint once a slice result exists; otherwise forward the viewmodel reason.
  if (editorViewModel_ && editorViewModel_->hasSliceResult())
    return {};
  return editorViewModel_ ? editorViewModel_->exportActionHint() : QString{};
}

QString BackendContext::saveActionLabel() const
{
  return tr("Save Project");
}

QString BackendContext::saveActionHint() const
{
  return (isSlicing() || isBusy()) ? tr("busy") : QString{};
}

void BackendContext::setCurrentPage(int page)
{
  if (currentPage_ == page)
    return;
  currentPage_ = page;
  emit currentPageChanged();
}

void BackendContext::requestSelectTab(int position)
{
  // 越界拒绝（对齐 Pitfall A3 — 防止 StackLayout currentIndex 越界破坏绑定）
  // Use the final semantic tab value instead of a numeric bound.
  constexpr int kLastTab = static_cast<int>(TabPosition::tpPreferences);
  if (position < 0 || position > kLastTab) {
    qWarning("[Backend] requestSelectTab: invalid position %d (valid 0..%d)", position, kLastTab);
    return;
  }
  // 先广播信号（对齐上游 wxQueueEvent 语义：消费者在 tab 实际切换前看到事件）
  emit tabSelectRequested(position);
  // 再切换页面（setCurrentPage 已含去重 + emit currentPageChanged）
  setCurrentPage(position);
  // Phase 3: tab 联动 viewMode（对齐上游 Plater 在 Notebook 切 tab 时自动切 view3D/preview）
  // tp3DEditor → View3D；tpPreview → Preview；其他 tab 不改 viewMode（切回 Plater 时维持上次视图）
  if (position == static_cast<int>(TabPosition::tp3DEditor))
    setCurrentViewMode(static_cast<int>(ViewMode::View3D));
  else if (position == static_cast<int>(TabPosition::tpPreview))
    setCurrentViewMode(static_cast<int>(ViewMode::Preview));
}

void BackendContext::setCurrentViewMode(int mode)
{
  // 内联 setter：供 requestSelectTab 联动 + requestChangeViewMode 复用
  // 越界拒绝（对齐 Pitfall A3）
  constexpr int kLastVm = static_cast<int>(ViewMode::AssembleView);
  if (mode < 0 || mode > kLastVm) {
    qWarning("[Backend] setCurrentViewMode: invalid mode %d (valid 0..%d)", mode, kLastVm);
    return;
  }
  if (static_cast<int>(currentViewMode_) == mode)
    return;
  currentViewMode_ = static_cast<ViewMode>(mode);
  emit currentViewModeChanged();
  // Phase 90 (ASMROUTE-01): push the active canvas type to EditorViewModel so
  // selection / gizmo / undo-redo routing branches on the active canvas,
  // mirroring upstream Plater.cpp:7322 (CanvasAssembleView render branch on
  // selection change) and Plater.cpp:11744/11823 (undo/redo routing). The
  // ViewMode value equals the RhiViewport::CanvasType value
  // (View3D=0/CanvasView3D, Preview=1/CanvasPreview, AssembleView=2/
  // CanvasAssembleView). When AssembleView is active the shared UndoRedoManager
  // (single stack) routes to the AssembleView canvas host.
  if (editorViewModel_)
    editorViewModel_->setActiveCanvasType(static_cast<int>(currentViewMode_));
}

void BackendContext::requestChangeViewMode(int mode)
{
  // 越界拒绝
  constexpr int kLastVm = static_cast<int>(ViewMode::AssembleView);
  if (mode < 0 || mode > kLastVm) {
    qWarning("[Backend] requestChangeViewMode: invalid mode %d (valid 0..%d)", mode, kLastVm);
    return;
  }
  // 先广播（对齐 tabSelectRequested 语义）
  emit viewModeChangeRequested(mode);
  setCurrentViewMode(mode);
}

// ── Phase 4: Sidebar Dockable 操作（对齐上游 collapse_sidebar + 持久化增强）──
// 设计：所有 request 方法去重 + emit NOTIFY + 持久化到 QSettings owzx/sidebar/*。
// 对齐上游 Plater.cpp:4452 collapse_sidebar(bool) 的显隐语义。

void BackendContext::requestToggleSidebar()
{
  // 对齐上游 EVT_GLCANVAS_COLLAPSE_SIDEBAR 触发的 toggle 语义
  requestSetSidebarCollapsed(!sidebarCollapsed_);
}

void BackendContext::requestSetSidebarCollapsed(bool c)
{
  if (sidebarCollapsed_ == c)
    return;  // 去重
  sidebarCollapsed_ = c;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/collapsed"), c);  // 持久化（对齐上游 app_config）
  settings.sync();
  emit sidebarCollapsedChanged();
}

void BackendContext::requestSetSidebarWidth(int w)
{
  // clamp 到 [min, max]，防御 QML 传入越界值
  const int clamped = qBound(kSidebarMinWidth, w, kSidebarMaxWidth);
  if (sidebarWidth_ == clamped)
    return;  // 去重（含 clamp 后相等）
  sidebarWidth_ = clamped;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/width"), clamped);  // 持久化
  settings.setValue(QStringLiteral("owzx/sidebar/settingsVersion"), kSidebarSettingsVersion);
  settings.sync();
  emit sidebarWidthChanged();
}

void BackendContext::requestSetSidebarDockArea(int area)
{
  // 防御：非 Right 一律按 Left（避免越界枚举值破坏 QML 绑定）
  const SidebarDockArea newArea = (area == static_cast<int>(SidebarDockArea::Right))
                                    ? SidebarDockArea::Right
                                    : SidebarDockArea::Left;
  if (sidebarDockArea_ == newArea)
    return;  // 去重
  sidebarDockArea_ = newArea;
  QSettings settings;
  settings.setValue(QStringLiteral("owzx/sidebar/dockArea"), static_cast<int>(newArea));  // 持久化
  settings.sync();
  emit sidebarDockAreaChanged();
}
void BackendContext::resetWindowLayout()
{
  requestSetSidebarCollapsed(false);
  requestSetSidebarWidth(kSidebarDefaultWidth);
  requestSetSidebarDockArea(static_cast<int>(SidebarDockArea::Left));
}

void BackendContext::openSettings()
{
  requestSelectTab(static_cast<int>(TabPosition::tpPreferences));
}

// ── ConfigWizard first-run trigger ──

bool BackendContext::configWizardCompleted() const
{
  return m_configWizardCompleted;
}

void BackendContext::setConfigWizardCompleted(bool completed)
{
  if (m_configWizardCompleted == completed)
    return;
  m_configWizardCompleted = completed;

  // Persist to QSettings (对齐上游 app_config 首次启动标记)
  QSettings settings;
  settings.setValue(QStringLiteral("wizard/completed"), completed);

  emit configWizardCompletedChanged();
}

void BackendContext::showConfigWizard()
{
  emit showConfigWizardRequested();
}

void BackendContext::forwardSettingsRequest(const QString &category)
{
  // Phase 56: show/create the correct SettingsDialog for this category.
  // Dialog instances are created once and shown/hidden (not destroyed per open).
  // Step 1: ensure the active preset tier is set before any consumer reads it.
  configViewModel_->setActivePresetTier(category);
  // Step 2: emit signal; QML side connects to create/show the dialog.
  emit settingsRequested(category);
}

void BackendContext::showBedShapeDialog()
{
  emit showBedShapeDialogRequested();
}

void BackendContext::showEditGCodeDialog(const QString &key, const QString &value)
{
  emit showEditGCodeDialogRequested(key, value);
}

void BackendContext::showAMSSettingsDialog()
{
  emit showAMSSettingsDialogRequested();
}

void BackendContext::showFirmwareDialog()
{
  emit showFirmwareDialogRequested();
}

void BackendContext::showSpeedLimitDialog()
{
  emit showSpeedLimitDialogRequested();
}

void BackendContext::showWipeTowerDialog()
{
  emit showWipeTowerDialogRequested();
}

void BackendContext::showPrintHostDialog()
{
  emit showPrintHostDialogRequested();
}

void BackendContext::showPluginManagerDialog()
{
  emit showPluginManagerDialogRequested();
}

void BackendContext::showEnableLiteModeDialog()
{
  emit showEnableLiteModeDialogRequested();
}

void BackendContext::showExportPresetBundleDialog()
{
  // Phase 236 (DLG-01): File > Export Preset Bundle entry point (upstream
  // Mainframe menu opens ExportPresetBundleDialog).
  emit showExportPresetBundleDialogRequested();
}

void BackendContext::showSysInfoDialog()
{
  // Phase 236 (DLG-03): Help menu entry for the system-info dump (upstream
  // Help > About > System Information opens a SysInfoDialog).
  emit showSysInfoDialogRequested();
}

QVariantMap BackendContext::systemInfo() const
{
  // Phase 236 (DLG-03): runtime environment dump for SysInfoDialog —
  // compile-time constants, graphics-API/GL strings (when an OpenGL context
  // is current), and the key config paths.
  QVariantMap info;
  info.insert(QStringLiteral("appName"), QStringLiteral("OWzx Slicer"));
  info.insert(QStringLiteral("appVersion"), QStringLiteral("2.4.0-dev (Qt6 QML)"));
  info.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
  info.insert(QStringLiteral("buildDate"), QString::fromLatin1(__DATE__));
  info.insert(QStringLiteral("platform"),
              QSysInfo::prettyProductName() + QStringLiteral(" [") + QSysInfo::buildCpuArchitecture() +
                  QStringLiteral("]"));
  // Graphics API selected by the QML runtime (OpenGL is forced at startup).
  // Qt6: QSGRendererInterface::GraphicsApi is not a Q_ENUM, so QMetaEnum
  // cannot stringify it — map manually (main_qml.cpp forces OpenGL).
  const QSGRendererInterface::GraphicsApi api = QQuickWindow::graphicsApi();
  const QString graphicsApi =
      api == QSGRendererInterface::OpenGL ? QStringLiteral("OpenGL") :
      api == QSGRendererInterface::Vulkan ? QStringLiteral("Vulkan") :
      api == QSGRendererInterface::Direct3D11 ? QStringLiteral("Direct3D11") :
      api == QSGRendererInterface::Direct3D12 ? QStringLiteral("Direct3D12") :
      api == QSGRendererInterface::Metal ? QStringLiteral("Metal") :
      api == QSGRendererInterface::Software ? QStringLiteral("Software") :
      api == QSGRendererInterface::Null ? QStringLiteral("Null") :
      QStringLiteral("Unknown");
  info.insert(QStringLiteral("graphicsApi"), graphicsApi);
  // Surface format actually used by the render loop.
  const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  info.insert(QStringLiteral("surfaceFormat"),
              QStringLiteral("OpenGL %1.%2 %3")
                  .arg(format.majorVersion())
                  .arg(format.minorVersion())
                  .arg(format.profile() == QSurfaceFormat::CoreProfile
                           ? QStringLiteral("Core")
                           : QStringLiteral("Compatibility")));
  // GL vendor/renderer: only readable with a current context; the QML scene
  // thread usually owns one. Missing values degrade to "n/a".
  QString glVendor = QStringLiteral("n/a");
  QString glRenderer = QStringLiteral("n/a");
  QString glVersion = QStringLiteral("n/a");
  if (QOpenGLContext *context = QOpenGLContext::currentContext())
  {
    QOpenGLFunctions functions(context);
    functions.initializeOpenGLFunctions();
    const unsigned char *vendor = functions.glGetString(GL_VENDOR);
    const unsigned char *renderer = functions.glGetString(GL_RENDERER);
    const unsigned char *version = functions.glGetString(GL_VERSION);
    if (vendor)
      glVendor = QString::fromLatin1(reinterpret_cast<const char *>(vendor));
    if (renderer)
      glRenderer = QString::fromLatin1(reinterpret_cast<const char *>(renderer));
    if (version)
      glVersion = QString::fromLatin1(reinterpret_cast<const char *>(version));
  }
  info.insert(QStringLiteral("glVendor"), glVendor);
  info.insert(QStringLiteral("glRenderer"), glRenderer);
  info.insert(QStringLiteral("glVersion"), glVersion);
  // Key configuration paths.
  info.insert(QStringLiteral("appDataLocation"),
              QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
  if (presetService_)
    info.insert(QStringLiteral("userPresetDir"), presetService_->userPresetDir());
  return info;
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

void BackendContext::applyStartupPagePreference()
{
  // Phase 241 (PAGE-04): the persisted startup-page preference now actually
  // drives the initial page. showHomePage=false + defaultPage=0 (Home) still
  // lands on Home — only showHomePage=false + defaultPage=1 (Prepare) starts
  // on the editor, matching the two upstream app_config keys
  // "show_home_page" / "default_page" consumed at MainFrame startup.
  if (!settingsViewModel_)
    return;
  const int target = (settingsViewModel_->showHomePage()
                          || settingsViewModel_->defaultPage() == 0)
                         ? static_cast<int>(TabPosition::tpHome)
                         : static_cast<int>(TabPosition::tp3DEditor);
  setCurrentPage(target);
}

QString BackendContext::triggerProjectBackup()
{
  // Phase 241 (PAGE-04): write one project snapshot into the app-data backup
  // directory. Upstream backup_switch copies the live project on a
  // seconds-level crash timer (Preferences.cpp:1179); the OWzx delta is a
  // periodic save-on-change backup (minute granularity, driven by the
  // autoSave/autoSaveInterval preferences) — the setting label says exactly
  // that. Returns the backup path, or an empty string when there is no
  // project content to back up or the write failed.
  if (!projectService_ || projectService_->modelCount() <= 0)
    return QString();
  const QString backupDir = QStandardPaths::writableLocation(
                                QStandardPaths::AppDataLocation)
                            + QStringLiteral("/backup");
  if (!QDir().mkpath(backupDir))
    return QString();
  const QString stamp = QDateTime::currentDateTime().toString(
      QStringLiteral("yyyyMMdd-HHmmss"));
  const QString base = projectService_->projectName().isEmpty()
                           ? QStringLiteral("project")
                           : projectService_->projectName();
  // ASCII-safe file-stem sanitize (keep CJK letters, drop path separators).
  QString safeBase;
  for (const QChar &c : base)
    safeBase += (c.isLetterOrNumber() || c == QLatin1Char('-')
                 || c == QLatin1Char('_'))
                    ? c
                    : QLatin1Char('_');
  if (safeBase.isEmpty())
    safeBase = QStringLiteral("project");
  const QString backupPath = backupDir + QStringLiteral("/") + safeBase
                             + QStringLiteral("-") + stamp + QStringLiteral(".3mf");
  // writeProjectSnapshot keeps currentProjectPath_ intact (a backup must not
  // hijack the user's current project file).
  if (!projectService_->writeProjectSnapshot(backupPath))
  {
    postError(tr("Project backup failed"), 1);
    return QString();
  }
  postNotification(tr("Project backup saved: %1").arg(backupPath),
                   tr("Auto backup"));
  return backupPath;
}

bool BackendContext::topbarOpenProject(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty())
    return false;

  // Phase 237 (VIEW-05): .3mf/.cxprj project opens route through
  // ProjectServiceMock::loadProject, which loads with LoadModel|LoadConfig
  // and emits projectConfigLoaded so the embedded config/presets apply
  // (upstream Plater::load_project / load_files is_project_file path;
  // the wiring above forwards projectConfigLoaded to
  // ConfigViewModel::applyProjectConfig). Plain model imports keep the
  // loadFile path (LoadModel only).
  const QString suffix = QFileInfo(localPath).suffix().toLower();
  bool loaded = false;
  if ((suffix == QStringLiteral("3mf") || suffix == QStringLiteral("cxprj"))
      && projectService_)
  {
    loaded = projectService_->loadProject(localPath);
    if (loaded && editorViewModel_)
      editorViewModel_->refreshAfterLoad();
  }
  if (!loaded)
    loaded = editorViewModel_ ? editorViewModel_->loadFile(localPath) : false;

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
    // Phase 236 (DLG-03): the 3MF version notice is emitted by the
    // projectVersionInfoChanged connection above (the parse runs on the
    // load worker thread; this call site would race it).
  }
  pushLatencySample(QStringLiteral("topbar-open-project"), int(m_latencyClock.elapsed() - start), localPath);
  return loaded;
}

// Phase 237 (VIEW-02): Import Configs (upstream MainFrame::load_config_file,
// MainFrame.cpp:3204-3247). The upstream file dialog accepts
// *.json/zip/orca_* files; the OWzx import surface with a real consumer is
// the .json bundle (PresetServiceMock::importBundle), so the QML dialog
// filters .json and this entry routes there. The import result surfaces as
// a notification (upstream shows a summary dialog).
bool BackendContext::topbarImportConfigs(const QString &filePath)
{
  const qint64 start = m_latencyClock.elapsed();
  const QUrl url(filePath);
  const QString localPath = url.isLocalFile() ? url.toLocalFile() : filePath;
  if (localPath.isEmpty() || !presetService_)
    return false;

  const bool imported = presetService_->importBundle(localPath);
  if (imported)
    postNotification(tr("There is 1 config imported."),
                     tr("Import result"));
  else
    postError(tr("Config import failed: %1").arg(QFileInfo(localPath).fileName()), 1);
  pushLatencySample(QStringLiteral("topbar-import-configs"),
                    int(m_latencyClock.elapsed() - start), localPath);
  return imported;
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
    {
      projectViewModel_->importModel(QStringList{localPath});
      // Phase 241 (PAGE-02): keep the ProjectPage resource tree in sync with
      // the freshly imported objects (import marks dirty, tree re-reads the
      // live service state).
      projectViewModel_->refreshFileTree();
    }
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
  // v5.16 (PSET2-06): overlay the preset selection state (tier preset ids +
  // per-extruder filament_presets vector) into the stored project config so
  // a reload restores it (upstream embeds the PresetBundle selections).
  if (projectService_ && configViewModel_)
    projectService_->setProjectConfigOverlay(configViewModel_->projectPresetConfigOverlay());
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
  // v5.16 (PSET2-06): same preset-selection overlay as topbarSaveProject.
  if (projectService_ && configViewModel_)
    projectService_->setProjectConfigOverlay(configViewModel_->projectPresetConfigOverlay());
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

  // Phase 240 (NOTI-01): errors join the stacked surface instead of
  // replacing the current toast (upstream pushes a new PopNotification and
  // keeps the old ones visible, NotificationManager.cpp:2479-2505).
  NotificationEntry entry;
  entry.message = message;
  entry.severity = severity;
  entry.timestamp = QDateTime::currentDateTime();
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/false);
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
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/false);
}

int BackendContext::notificationImportanceRank(const NotificationEntry &e)
{
  // Upstream NotificationLevel (NotificationManager.hpp:158-180) is sorted by
  // importance: ProgressBar(1) < Hint(2) < Regular(3) < PrintInfo(4) <
  // PrintInfoShort(5) < Important(6) < Warning(7) < SeriousWarning(8) <
  // Error(9). Map the Qt NotiLevel enum onto that ladder.
  switch (e.severity)
  {
  case NotiProgress:
  case NotiSlicingProgress:
    return 1; // ProgressBarNotificationLevel
  case NotiHint:
    return 2; // HintNotificationLevel
  case NotiPrintInfo:
    return 4; // PrintInfoNotificationLevel
  case NotiPrintInfoShort:
    return 5; // PrintInfoShortNotificationLevel
  case NotiWarning:
    return 7; // WarningNotificationLevel
  case NotiSeriousWarning:
    return 8; // SeriousWarningNotificationLevel
  case NotiError:
    return 9; // ErrorNotificationLevel
  case NotiSuccess:
  case NotiInfo:
  default:
    // Persistent info (export/arrange ongoing) behaves like upstream
    // ImportantNotificationLevel (no fade-out); everything else is Regular.
    return e.persistent ? 6 : 3;
  }
}

void BackendContext::archiveNotification(const NotificationEntry &entry)
{
  m_notificationHistory.prepend(entry);
  ++m_unreadHistoryCount;
  if (m_notificationHistory.size() > 100)
    m_notificationHistory.removeLast();
  emit historyChanged();
}

void BackendContext::pushNotificationEntry(NotificationEntry entry, bool dedupByTypeOnly)
{
  // Duplicate compression (upstream activate_existing, NotificationManager.
  // cpp:2643-2675): a matching live entry is UPDATED in place instead of a
  // second toast stacking on top. Progress-family types (slicing progress /
  // export ongoing / arrange ongoing / hint) dedup on type alone because
  // their message text changes on every tick; everything else dedups on
  // type + message + title (the upstream multiple-types rule: "multiple of
  // one type allowed, but must have different text").
  for (int i = 0; i < m_activeNotifications.size(); ++i)
  {
    NotificationEntry &live = m_activeNotifications[i];
    const bool sameType = (live.type == entry.type);
    const bool sameText = (live.message == entry.message && live.title == entry.title);
    if (!sameType || (!dedupByTypeOnly && !sameText))
      continue;
    // Escalation counter (upstream UpdatedItemsInfoNotification counter
    // pattern, NotificationManager.hpp:818): repeated identical posts bump
    // the xN badge instead of spamming the stack.
    if (sameText || dedupByTypeOnly)
      live.repeatCount += 1;
    live.message = entry.message;
    live.title = entry.title;
    live.severity = entry.severity;
    live.persistent = entry.persistent;
    live.hasProgress = entry.hasProgress;
    live.progressValue = entry.progressValue;
    live.progressMin = entry.progressMin;
    live.progressMax = entry.progressMax;
    live.requiresConfirm = entry.requiresConfirm;
    live.showExportButton = entry.showExportButton;
    live.showPreviewButton = entry.showPreviewButton;
    live.timestamp = entry.timestamp;
    syncCurrentNotificationFromStack();
    emit errorChanged();
    return;
  }

  entry.id = m_nextNotificationId++;
  if (m_activeNotifications.size() < kMaxVisibleNotifications)
    m_activeNotifications.append(entry);
  else
    m_notificationQueue.enqueue(entry);

  // Importance ordering (upstream sort_notifications, NotificationManager.
  // cpp:2633-2639): stable sort so equal-importance entries keep insertion
  // order. The list is MOST important first so the QML column renders errors
  // at the top (upstream renders bottom-up with Error at the "Top most
  // position").
  std::stable_sort(m_activeNotifications.begin(), m_activeNotifications.end(),
                   [](const NotificationEntry &a, const NotificationEntry &b)
                   { return notificationImportanceRank(a) > notificationImportanceRank(b); });

  syncCurrentNotificationFromStack();
  emit errorChanged();
}

void BackendContext::syncCurrentNotificationFromStack()
{
  // Legacy single-toast view: the NEWEST entry (last in insertion order)
  // keeps driving lastError*/currentNotification* so all pre-Phase-240
  // callers and tests see "last post wins" semantics.
  if (!m_activeNotifications.isEmpty())
  {
    // Find the newest by id (ids are monotonic; the sort reordered the list).
    int newestIdx = 0;
    for (int i = 1; i < m_activeNotifications.size(); ++i)
      if (m_activeNotifications[i].id > m_activeNotifications[newestIdx].id)
        newestIdx = i;
    m_currentNotification = m_activeNotifications[newestIdx];
    lastErrorMessage_ = m_currentNotification.message;
    lastErrorSeverity_ = m_currentNotification.severity;
    lastErrorTitle_ = m_currentNotification.title;
  }
  else
  {
    m_currentNotification = {};
    lastErrorMessage_.clear();
    lastErrorTitle_.clear();
    lastErrorSeverity_ = -1;
  }
}

void BackendContext::dismissNotification()
{
  // Dismiss the NEWEST entry (legacy single-toast behavior: the visible
  // toast being closed was the current one).
  if (!m_activeNotifications.isEmpty())
  {
    int newestIdx = 0;
    for (int i = 1; i < m_activeNotifications.size(); ++i)
      if (m_activeNotifications[i].id > m_activeNotifications[newestIdx].id)
        newestIdx = i;
    const NotificationEntry dismissed = m_activeNotifications.takeAt(newestIdx);
    archiveNotification(dismissed);
  }
  showNextNotification();
}

void BackendContext::dismissNotificationById(int id)
{
  for (int i = 0; i < m_activeNotifications.size(); ++i)
  {
    if (m_activeNotifications[i].id != id)
      continue;
    const NotificationEntry dismissed = m_activeNotifications.takeAt(i);
    archiveNotification(dismissed);
    showNextNotification();
    return;
  }
}

void BackendContext::confirmNotificationById(int id)
{
  for (const NotificationEntry &entry : std::as_const(m_activeNotifications))
  {
    if (entry.id == id && entry.requiresConfirm)
    {
      dismissNotificationById(id);
      return;
    }
  }
}

void BackendContext::cancelNotificationById(int id)
{
  for (const NotificationEntry &entry : std::as_const(m_activeNotifications))
  {
    if (entry.id == id && entry.requiresConfirm)
    {
      dismissNotificationById(id);
      return;
    }
  }
}

void BackendContext::clearError()
{
  dismissNotification();
}

void BackendContext::showNextNotification()
{
  // Pull one queued entry into the freed visible slot (upstream pops from
  // m_pop_notifications only on finish; the overflow queue is the Qt6
  // screen-space bound).
  if (m_activeNotifications.size() < kMaxVisibleNotifications && !m_notificationQueue.isEmpty())
  {
    const auto entry = m_notificationQueue.dequeue();
    m_activeNotifications.append(entry);
    std::stable_sort(m_activeNotifications.begin(), m_activeNotifications.end(),
                     [](const NotificationEntry &a, const NotificationEntry &b)
                     { return notificationImportanceRank(a) > notificationImportanceRank(b); });
  }
  syncCurrentNotificationFromStack();
  emit errorChanged();
}

QVariantList BackendContext::notificationStack() const
{
  QVariantList list;
  list.reserve(m_activeNotifications.size());
  for (const NotificationEntry &e : m_activeNotifications)
  {
    QVariantMap m;
    m.insert(QStringLiteral("id"), e.id);
    m.insert(QStringLiteral("message"), e.message);
    m.insert(QStringLiteral("title"), e.title);
    m.insert(QStringLiteral("severity"), e.severity);
    m.insert(QStringLiteral("type"), e.type);
    m.insert(QStringLiteral("persistent"), e.persistent);
    m.insert(QStringLiteral("hasProgress"), e.hasProgress);
    m.insert(QStringLiteral("progressValue"), e.progressValue);
    m.insert(QStringLiteral("repeatCount"), e.repeatCount);
    m.insert(QStringLiteral("requiresConfirm"), e.requiresConfirm);
    m.insert(QStringLiteral("showExportButton"), e.showExportButton);
    m.insert(QStringLiteral("showPreviewButton"), e.showPreviewButton);
    list.append(m);
  }
  return list;
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
  // Update the progress on the newest entry AND its stacked twin so the
  // visible toast and the legacy getter stay in lockstep.
  m_currentNotification.progressValue = qBound(
    m_currentNotification.progressMin,
    value,
    m_currentNotification.progressMax);
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.progressValue = m_currentNotification.progressValue;
      break;
    }
  }
  emit errorChanged();
}

void BackendContext::confirmCurrentNotification()
{
  // 对齐上游 notification_manager::confirm
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.persistent = false;
      e.requiresConfirm = false;
      break;
    }
  }
  dismissNotification();
}

void BackendContext::cancelCurrentNotification()
{
  m_currentNotification.persistent = false;
  m_currentNotification.requiresConfirm = false;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id)
    {
      e.persistent = false;
      e.requiresConfirm = false;
      break;
    }
  }
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
  if (!m_notificationsEnabled || !m_showProgressNotifications)
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

  // Phase 240 (NOTI-01): progress notifications update the LIVE slicing
  // entry in place (upstream SlicingProgressNotification updates the same
  // PopNotification, NotificationManager.cpp:2248) via the type-only dedup
  // in pushNotificationEntry.
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
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

  // Replaces the slicing progress entry (same type-only dedup).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
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

  // Phase 240 (NOTI-01): type-only dedup updates the live export-ongoing
  // entry in place (upstream ExportOngoing updates the same notification).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
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
  if (!m_notificationsEnabled)
    return;
  NotificationEntry entry;
  entry.type = NotiTypeArrangeOngoing;
  entry.severity = NotiInfo;
  entry.title = tr("排列中");
  entry.message = tr("正在自动排列... %1%").arg(qBound(0, percent, 100));
  entry.persistent = true;
  entry.timestamp = QDateTime::currentDateTime();

  // Phase 240 (NOTI-01): type-only dedup updates the live arrange entry.
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
}

// ------- 提示数据库实现（对齐上游 HintDatabase） -------

void BackendContext::initHintDatabase()
{
  // 对齐上游 HintDatabase::load_hints_from_file
  // 从 Qt 资源中的 hints.json 加载提示数据（替代上游 hints.ini + boost::property_tree）
  QFile file(QStringLiteral(":/qml/data/hints.json"));
  if (!file.open(QIODevice::ReadOnly))
  {
    qWarning("[Backend] hints.json not found, using fallback hints");
    initFallbackHintDatabase();
    return;
  }

  const QByteArray data = file.readAll();
  QJsonParseError parseError;
  const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
  if (doc.isNull() || !doc.isArray())
  {
    qWarning("[Backend] hints.json parse error: %s", qPrintable(parseError.errorString()));
    initFallbackHintDatabase();
    return;
  }

  const QJsonArray arr = doc.array();
  m_hints.reserve(arr.size());
  for (const auto &val : arr)
  {
    if (!val.isObject())
      continue;
    const QJsonObject obj = val.toObject();
    HintData h;
    h.id = obj.value(QLatin1String("id")).toString();
    h.text = obj.value(QLatin1String("text")).toString();
    h.weight = obj.value(QLatin1String("weight")).toInt(1);
    h.documentationLink = obj.value(QLatin1String("documentation_link")).toString();
    m_hints.append(h);
  }

  m_currentHintIndex = -1;
  qInfo("[Backend] Loaded %d hints from hints.json", m_hints.size());
}

void BackendContext::initFallbackHintDatabase()
{
  // 内置最小提示集（当 JSON 加载失败时的兜底）
  auto add = [this](const QString &id, const QString &text, int weight, const QString &docLink)
  {
    HintData h;
    h.id = id;
    h.text = text;
    h.weight = weight;
    h.documentationLink = docLink;
    m_hints.append(h);
  };

  add(QStringLiteral("hint_layer_height"),
      tr("层高越小打印越精细，但耗时越长。常用范围: 0.1mm - 0.3mm。"),
      10, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Layer-height"));
  add(QStringLiteral("hint_infill"),
      tr("填充密度影响模型强度和重量。20% 适合大多数场景，100% 为实心。"),
      10, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Infill"));
  add(QStringLiteral("hint_support"),
      tr("悬空角度超过 45° 的部分需要支撑。合理使用支撑可以提升打印质量。"),
      8, QStringLiteral("https://github.com/SoftFever/OrcaSlicer/wiki/Supports"));
  add(QStringLiteral("hint_speed"),
      tr("打印速度越快效率越高，但可能影响表面质量。建议先慢后快测试。"),
      7, {});
  add(QStringLiteral("hint_brims"),
      tr("Brim（裙边）可以增加模型与热床的附着力，防止翘边。"),
      6, {});

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

  // Phase 240 (NOTI-01): a new hint replaces the live hint entry (upstream
  // keeps at most one HintNotification). Non-hint notifications stay
  // visible (stacked surface).
  pushNotificationEntry(entry, /*dedupByTypeOnly=*/true);
  emit dailyTipChanged();
}

void BackendContext::showDailyTip()
{
  // Phase 241 (PAGE-01): advance the hint cursor for the HomePage Daily Tips
  // card WITHOUT posting a notification (postHint is the notification-path
  // twin). Mirrors upstream DailyTips rotation over the hints database
  // (hints.ini upstream, hints.json here); weighted random selection.
  if (m_hints.isEmpty())
    return;
  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;
  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);
  emit dailyTipChanged();
}

void BackendContext::nextHint()
{
  const int idx = selectNextHint(true);
  if (idx < 0 || idx >= m_hints.size())
    return;

  m_currentHintIndex = idx;
  m_displayedHintIds.insert(m_hints[idx].id);
  // Phase 240 (NOTI-01): keep the stacked hint entry + the legacy getter in
  // lockstep when navigating hints.
  const QString text = m_hints[idx].text;
  m_currentNotification.message = text;
  lastErrorMessage_ = text;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id && e.type == NotiTypeDidYouKnowHint)
    {
      e.message = text;
      break;
    }
  }
  emit errorChanged();
  emit dailyTipChanged();
}

void BackendContext::prevHint()
{
  if (m_hints.isEmpty() || m_currentHintIndex < 0)
    return;
  m_currentHintIndex = (m_currentHintIndex - 1 + m_hints.size()) % m_hints.size();
  const QString text = m_hints[m_currentHintIndex].text;
  m_currentNotification.message = text;
  lastErrorMessage_ = text;
  for (NotificationEntry &e : m_activeNotifications)
  {
    if (e.id == m_currentNotification.id && e.type == NotiTypeDidYouKnowHint)
    {
      e.message = text;
      break;
    }
  }
  emit errorChanged();
  emit dailyTipChanged();
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

bool BackendContext::currentHintHasDocumentationLink() const
{
  if (m_currentHintIndex >= 0 && m_currentHintIndex < m_hints.size())
    return !m_hints[m_currentHintIndex].documentationLink.isEmpty();
  return false;
}

bool BackendContext::openHintDocumentation() const
{
  // 对齐上游 HintNotification hypertext_type=documentation 链接点击
  if (m_currentHintIndex < 0 || m_currentHintIndex >= m_hints.size())
    return false;
  const QString link = m_hints[m_currentHintIndex].documentationLink;
  if (link.isEmpty())
    return false;
  return QDesktopServices::openUrl(QUrl(link));
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

void BackendContext::setShowProgressNotifications(bool v)
{
  if (m_showProgressNotifications == v)
    return;
  m_showProgressNotifications = v;
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
  // WR-06: tr("未命名") 占位符在 displayProjectTitle 内惰性求值，需要显式 emit
  // displayProjectTitleChanged 让 QML 绑定重新求值并刷新本地化占位符。
  emit displayProjectTitleChanged();
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

// ── Dead-control elimination: real behaviors for the former placeholder
// Help-menu items / dialog buttons (upstream MainFrame help menu,
// NetworkTestDialog, PrintHostDialog). ──────────────────────────────────────

namespace {
// Mirrors the OWZX_VERSION compat constant generated into
// build/libslic3r_generated/buildinfo.h (2.4.0-dev line). Kept local so the
// QML layer does not depend on the generated include path.
constexpr const char *kOwzxUpdateVersion = "2.4.0";
constexpr const char *kOwzxUpdateRepoApi =
    "https://api.github.com/repos/wzx011011/3DPrinter_Qt6/releases/latest";

QVector<int> splitVersionParts(const QString &version)
{
  QVector<int> parts;
  QString current;
  for (const QChar ch : version) {
    if (ch.isDigit()) {
      current += ch;
    } else if (!current.isEmpty()) {
      parts.append(current.toInt());
      current.clear();
      if (parts.size() == 3)
        break;
    }
  }
  if (!current.isEmpty() && parts.size() < 3)
    parts.append(current.toInt());
  while (parts.size() < 3)
    parts.append(0);
  return parts;
}

bool remoteVersionNewer(const QString &remoteTag)
{
  // Strip a leading 'v'/'V' and any -suffix; compare the first three numeric
  // components component-wise.
  QString remote = remoteTag.trimmed();
  if (remote.startsWith(QLatin1Char('v')) || remote.startsWith(QLatin1Char('V')))
    remote.remove(0, 1);
  const int dash = remote.indexOf(QLatin1Char('-'));
  if (dash >= 0)
    remote.truncate(dash);
  const QVector<int> r = splitVersionParts(remote);
  const QVector<int> l = splitVersionParts(QString::fromLatin1(kOwzxUpdateVersion));
  for (int i = 0; i < 3; ++i) {
    if (r[i] != l[i])
      return r[i] > l[i];
  }
  return false;
}
}  // namespace

QNetworkAccessManager *BackendContext::probeNam()
{
  if (!probeNam_)
    probeNam_ = new QNetworkAccessManager(this);
  return probeNam_;
}

void BackendContext::setUpdateCheckRunning(bool running)
{
  if (updateCheckRunning_ == running)
    return;
  updateCheckRunning_ = running;
  emit updateCheckRunningChanged();
}

void BackendContext::checkForUpdates()
{
  // Upstream Help > Check for Update (MainFrame.cpp:2160): async release
  // check with a user-visible result either way.
  if (updateCheckRunning_)
    return;
  setUpdateCheckRunning(true);
  QNetworkRequest request(QUrl(QString::fromLatin1(kOwzxUpdateRepoApi)));
  request.setRawHeader("User-Agent", "OWzxSlicer");
  request.setTransferTimeout(10000);
  QNetworkReply *reply = probeNam()->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    setUpdateCheckRunning(false);
    if (reply->error() != QNetworkReply::NoError) {
      const QString message = tr("更新检查失败：%1").arg(reply->errorString());
      qWarning("[Backend] update check failed: %s", reply->errorString().toUtf8().constData());
      postNotification(message, tr("检查更新"), 1);
      emit updateCheckFinished(false, false, message);
      return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QString tag = doc.object().value(QStringLiteral("tag_name")).toString();
    if (tag.isEmpty()) {
      const QString message = tr("更新服务器返回了无法解析的数据");
      postNotification(message, tr("检查更新"), 1);
      emit updateCheckFinished(false, false, message);
      return;
    }
    const bool available = remoteVersionNewer(tag);
    const QString message = available
        ? tr("发现新版本 %1，请到项目发布页下载。").arg(tag)
        : tr("当前已是最新版本（%1）。").arg(QString::fromLatin1(kOwzxUpdateVersion));
    postNotification(message, tr("检查更新"), 0);
    emit updateCheckFinished(true, available, message);
  });
}

bool BackendContext::openConfigFolder()
{
  // Upstream Help > Show Configuration Folder (MainFrame.cpp:2146,
  // desktop_open_datadir_folder).
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (!QDir(dir).exists())
    QDir().mkpath(dir);
  return QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void BackendContext::runNetworkTest()
{
  // Upstream NetworkTestDialog connectivity probe: DNS resolve followed by an
  // HTTPS GET whose round-trip time is reported as the latency.
  QElapsedTimer dnsTimer;
  dnsTimer.start();
  QHostInfo::lookupHost(QStringLiteral("api.github.com"), this,
                        [this](const QHostInfo &dns) {
    const bool dnsOk = dns.error() == QHostInfo::NoError && !dns.addresses().isEmpty();
    if (!dnsOk) {
      emit networkTestFinished(false, false, -1, tr("DNS 解析失败：%1").arg(dns.errorString()));
      return;
    }
    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com")));
    request.setRawHeader("User-Agent", "OWzxSlicer");
    request.setTransferTimeout(8000);
    QElapsedTimer latencyTimer;
    latencyTimer.start();
    QNetworkReply *reply = probeNam()->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, latencyTimer] {
      reply->deleteLater();
      const bool online = reply->error() == QNetworkReply::NoError
          || reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid();
      const int latencyMs = static_cast<int>(latencyTimer.elapsed());
      const QString detail = online
          ? tr("HTTPS 连接正常")
          : tr("HTTPS 连接失败：%1").arg(reply->errorString());
      emit networkTestFinished(true, online, latencyMs, detail);
    });
  });
  Q_UNUSED(dnsTimer);
}

void BackendContext::testPrintHost(const QString &hostUrl)
{
  // Upstream PrintHostDialog "test connection": reachability GET against the
  // configured host. Any HTTP response counts as reachable.
  QString url = hostUrl.trimmed();
  if (url.isEmpty()) {
    emit printHostTestFinished(false, tr("主机地址为空"));
    return;
  }
  if (!url.startsWith(QStringLiteral("http://")) && !url.startsWith(QStringLiteral("https://")))
    url.prepend(QStringLiteral("http://"));
  const QUrl targetUrl(url);
  QNetworkRequest hostProbe(targetUrl);
  hostProbe.setRawHeader("User-Agent", "OWzxSlicer");
  hostProbe.setTransferTimeout(8000);
  QNetworkReply *reply = probeNam()->get(hostProbe);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const bool ok = reply->error() == QNetworkReply::NoError || status.isValid();
    const QString detail = ok
        ? tr("连接成功（HTTP %1）").arg(status.isValid() ? status.toString() : QStringLiteral("OK"))
        : tr("连接失败：%1").arg(reply->errorString());
    emit printHostTestFinished(ok, detail);
  });
}
